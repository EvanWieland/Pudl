#include "CodeGen.h"

std::unique_ptr<llvm::LLVMContext> Pudl::CodeGen::TheContext;
std::unique_ptr<llvm::Module> Pudl::CodeGen::TheModule;
std::unique_ptr<llvm::IRBuilder<>> Pudl::CodeGen::Builder;
std::map<std::string, llvm::AllocaInst *> Pudl::CodeGen::NamedValues;
std::unique_ptr<llvm::legacy::FunctionPassManager> Pudl::CodeGen::TheFPM;
std::unique_ptr<Pudl::PudlJIT> Pudl::CodeGen::TheJIT;
std::map<std::string, std::unique_ptr<Pudl::AST::PrototypeAST>> Pudl::CodeGen::FunctionProtos;
llvm::ExitOnError Pudl::CodeGen::ExitOnErr;

llvm::Value *Pudl::CodeGen::LogErrorV(const char *Str) {
    Pudl::Parser::LogError(Str);
    return nullptr;
}

llvm::Function *Pudl::CodeGen::getFunction(std::string Name) {
    // First, see if the function has already been added to the current module.
    if (auto *F = TheModule->getFunction(Name))
        return F;

    // If not, check whether we can codegen the declaration from some existing
    // prototype.
    auto FI = FunctionProtos.find(Name);
    if (FI != FunctionProtos.end())
        return FI->second->codegen();

    // If no existing prototype exists, return null.
    return nullptr;
}

/// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of
/// the function.  This is used for mutable variables etc.
llvm::AllocaInst *Pudl::CodeGen::CreateEntryBlockAlloca(llvm::Function *TheFunction, llvm::StringRef VarName) {
    llvm::IRBuilder<> TmpB(&TheFunction->getEntryBlock(),
                           TheFunction->getEntryBlock().begin());
    return TmpB.CreateAlloca(llvm::Type::getDoubleTy(*TheContext), nullptr, VarName);
}

llvm::Value *Pudl::AST::NumberExprAST::codegen() {
    return ConstantFP::get(*CodeGen::TheContext, APFloat(Val));
}

llvm::Value *Pudl::AST::VariableExprAST::codegen() {
    // Look this variable up in the function.
    AllocaInst *A = CodeGen::NamedValues[Name];
    if (!A)
        return CodeGen::LogErrorV("Unknown variable name");

    // Load the value.
    return CodeGen::Builder->CreateLoad(A->getAllocatedType(), A, Name.c_str());
}

llvm::Value *Pudl::AST::UnaryExprAST::codegen() {
    Value *OperandV = Operand->codegen();
    if (!OperandV)
        return nullptr;

    Function *F = CodeGen::getFunction(std::string("unary") + Opcode);
    if (!F)
        return CodeGen::LogErrorV("Unknown unary operator");

    return CodeGen::Builder->CreateCall(F, OperandV, "unop");
}

llvm::Value *Pudl::AST::BinaryExprAST::codegen() {
    // Special case '=' because we don't want to emit the LHS as an expression.
    if (Op == '=') {
        // Assignment requires the LHS to be an identifier.
        // This assume we're building without RTTI because LLVM builds that way by
        // default.  If you build LLVM with RTTI this can be changed to a
        // dynamic_cast for automatic error checking.
        VariableExprAST *LHSE = static_cast<VariableExprAST *>(LHS.get());
        if (!LHSE)
            return CodeGen::LogErrorV("destination of '=' must be a variable");
        // Codegen the RHS.
        Value *Val = RHS->codegen();
        if (!Val)
            return nullptr;

        // Look up the name.
        Value *Variable = CodeGen::NamedValues[LHSE->getName()];
        if (!Variable)
            return CodeGen::LogErrorV("Unknown variable name");

        CodeGen::Builder->CreateStore(Val, Variable);
        return Val;
    }

    Value *L = LHS->codegen();
    Value *R = RHS->codegen();
    if (!L || !R)
        return nullptr;

    switch (Op) {
        case '+':
            return CodeGen::Builder->CreateFAdd(L, R, "addtmp");
        case '-':
            return CodeGen::Builder->CreateFSub(L, R, "subtmp");
        case '*':
            return CodeGen::Builder->CreateFMul(L, R, "multmp");
        case '<':
            L = CodeGen::Builder->CreateFCmpULT(L, R, "cmptmp");
            // Convert bool 0/1 to double 0.0 or 1.0
            return CodeGen::Builder->CreateUIToFP(L, Type::getDoubleTy(*CodeGen::TheContext), "booltmp");
        default:
            break;
    }

    // If it wasn't a builtin binary operator, it must be a user defined one. Emit
    // a call to it.
    Function *F = CodeGen::getFunction(std::string("binary") + Op);
    assert(F && "binary operator not found!");

    Value *Ops[] = {L, R};
    return CodeGen::Builder->CreateCall(F, Ops, "binop");
}

llvm::Value *Pudl::AST::CallExprAST::codegen() {
    // Look up the name in the global module table.
    Function *CalleeF = CodeGen::getFunction(Callee);
    if (!CalleeF)
        return CodeGen::LogErrorV("Unknown function referenced");

    // If argument mismatch error.
    if (CalleeF->arg_size() != Args.size())
        return CodeGen::LogErrorV("Incorrect # arguments passed");

    std::vector<Value *> ArgsV;
    for (unsigned i = 0, e = Args.size(); i != e; ++i) {
        ArgsV.push_back(Args[i]->codegen());
        if (!ArgsV.back())
            return nullptr;
    }

    return CodeGen::Builder->CreateCall(CalleeF, ArgsV, "calltmp");
}

llvm::Value *Pudl::AST::IfExprAST::codegen() {
    Value *CondV = Cond->codegen();
    if (!CondV)
        return nullptr;

    // Convert condition to a bool by comparing non-equal to 0.0.
    CondV = CodeGen::Builder->CreateFCmpONE(
            CondV, ConstantFP::get(*CodeGen::TheContext, APFloat(0.0)), "ifcond");

    Function *TheFunction = CodeGen::Builder->GetInsertBlock()->getParent();

    // Create blocks for the then and else cases.  Insert the 'then' block at the
    // end of the function.
    BasicBlock *ThenBB = BasicBlock::Create(*CodeGen::TheContext, "then", TheFunction);
    BasicBlock *ElseBB = BasicBlock::Create(*CodeGen::TheContext, "else");
    BasicBlock *MergeBB = BasicBlock::Create(*CodeGen::TheContext, "ifcont");

    CodeGen::Builder->CreateCondBr(CondV, ThenBB, ElseBB);

    // Emit then value.
    CodeGen::Builder->SetInsertPoint(ThenBB);

    Value *ThenV = Then->codegen();
    if (!ThenV)
        return nullptr;

    CodeGen::Builder->CreateBr(MergeBB);
    // Codegen of 'Then' can change the current block, update ThenBB for the PHI.
    ThenBB = CodeGen::Builder->GetInsertBlock();

    // Emit else block.
    TheFunction->getBasicBlockList().push_back(ElseBB);
    CodeGen::Builder->SetInsertPoint(ElseBB);

    Value *ElseV = Else->codegen();
    if (!ElseV)
        return nullptr;

    CodeGen::Builder->CreateBr(MergeBB);
    // Codegen of 'Else' can change the current block, update ElseBB for the PHI.
    ElseBB = CodeGen::Builder->GetInsertBlock();

    // Emit merge block.
    TheFunction->getBasicBlockList().push_back(MergeBB);
    CodeGen::Builder->SetInsertPoint(MergeBB);
    PHINode *PN = CodeGen::Builder->CreatePHI(Type::getDoubleTy(*CodeGen::TheContext), 2, "iftmp");

    PN->addIncoming(ThenV, ThenBB);
    PN->addIncoming(ElseV, ElseBB);
    return PN;
}

// Output for-loop as:
//   var = alloca double
//   ...
//   start = startexpr
//   store start -> var
//   goto loop
// loop:
//   ...
//   bodyexpr
//   ...
// loopend:
//   step = stepexpr
//   endcond = endexpr
//
//   curvar = load var
//   nextvar = curvar + step
//   store nextvar -> var
//   br endcond, loop, endloop
// outloop:
llvm::Value *Pudl::AST::ForExprAST::codegen() {
    Function *TheFunction = CodeGen::Builder->GetInsertBlock()->getParent();

    // Create an alloca for the variable in the entry block.
    AllocaInst *Alloca = CodeGen::CreateEntryBlockAlloca(TheFunction, VarName);

    // Emit the start code first, without 'variable' in scope.
    Value *StartVal = Start->codegen();
    if (!StartVal)
        return nullptr;

    // Store the value into the alloca.
    CodeGen::Builder->CreateStore(StartVal, Alloca);

    // Make the new basic block for the loop header, inserting after current
    // block.
    BasicBlock *LoopBB = BasicBlock::Create(*CodeGen::TheContext, "loop", TheFunction);

    // Insert an explicit fall through from the current block to the LoopBB.
    CodeGen::Builder->CreateBr(LoopBB);

    // Start insertion in LoopBB.
    CodeGen::Builder->SetInsertPoint(LoopBB);

    // Within the loop, the variable is defined equal to the PHI node.  If it
    // shadows an existing variable, we have to restore it, so save it now.
    AllocaInst *OldVal = CodeGen::NamedValues[VarName];
    CodeGen::NamedValues[VarName] = Alloca;

    // Emit the body of the loop.  This, like any other expr, can change the
    // current BB.  Note that we ignore the value computed by the body, but don't
    // allow an error.
    if (!Body->codegen())
        return nullptr;

    // Emit the step value.
    Value *StepVal = nullptr;
    if (Step) {
        StepVal = Step->codegen();
        if (!StepVal)
            return nullptr;
    } else {
        // If not specified, use 1.0.
        StepVal = ConstantFP::get(*CodeGen::TheContext, APFloat(1.0));
    }

    // Compute the end condition.
    Value *EndCond = End->codegen();
    if (!EndCond)
        return nullptr;

    // Reload, increment, and restore the alloca.  This handles the case where
    // the body of the loop mutates the variable.
    Value *CurVar = CodeGen::Builder->CreateLoad(Alloca->getAllocatedType(), Alloca, VarName.c_str());
    Value *NextVar = CodeGen::Builder->CreateFAdd(CurVar, StepVal, "nextvar");
    CodeGen::Builder->CreateStore(NextVar, Alloca);

    // Convert condition to a bool by comparing non-equal to 0.0.
    EndCond = CodeGen::Builder->CreateFCmpONE(
            EndCond, ConstantFP::get(*CodeGen::TheContext, APFloat(0.0)), "loopcond");

    // Create the "after loop" block and insert it.
    BasicBlock *AfterBB =
            BasicBlock::Create(*CodeGen::TheContext, "afterloop", TheFunction);

    // Insert the conditional branch into the end of LoopEndBB.
    CodeGen::Builder->CreateCondBr(EndCond, LoopBB, AfterBB);

    // Any new code will be inserted in AfterBB.
    CodeGen::Builder->SetInsertPoint(AfterBB);

    // Restore the unshadowed variable.
    if (OldVal)
        CodeGen::NamedValues[VarName] = OldVal;
    else
        CodeGen::NamedValues.erase(VarName);

    // for expr always returns 0.0.
    return Constant::getNullValue(Type::getDoubleTy(*CodeGen::TheContext));
}

llvm::Value *Pudl::AST::VarExprAST::codegen() {
    std::vector<AllocaInst *> OldBindings;

    Function *TheFunction = CodeGen::Builder->GetInsertBlock()->getParent();

    // Register all variables and emit their initializer.
    for (unsigned i = 0, e = VarNames.size(); i != e; ++i) {
        const std::string &VarName = VarNames[i].first;
        ExprAST *Init = VarNames[i].second.get();

        // Emit the initializer before adding the variable to scope, this prevents
        // the initializer from referencing the variable itself, and permits stuff
        // like this:
        //  var a = 1 in
        //    var a = a in ...   # refers to outer 'a'.
        Value *InitVal;
        if (Init) {
            InitVal = Init->codegen();
            if (!InitVal)
                return nullptr;
        } else { // If not specified, use 0.0.
            InitVal = ConstantFP::get(*CodeGen::TheContext, APFloat(0.0));
        }

        AllocaInst *Alloca = CodeGen::CreateEntryBlockAlloca(TheFunction, VarName);
        CodeGen::Builder->CreateStore(InitVal, Alloca);

        // Remember the old variable binding so that we can restore the binding when
        // we unrecurse.
        OldBindings.push_back(CodeGen::NamedValues[VarName]);

        // Remember this binding.
        CodeGen::NamedValues[VarName] = Alloca;
    }

    // Codegen the body, now that all vars are in scope.
    Value *BodyVal = Body->codegen();
    if (!BodyVal)
        return nullptr;

    // Pop all our variables from scope.
    for (unsigned i = 0, e = VarNames.size(); i != e; ++i)
        CodeGen::NamedValues[VarNames[i].first] = OldBindings[i];

    // Return the body computation.
    return BodyVal;
}

llvm::Function *Pudl::AST::PrototypeAST::codegen() {
    // Make the function type:  double(double,double) etc.
    std::vector<Type *> Doubles(Args.size(), Type::getDoubleTy(*CodeGen::TheContext));
    FunctionType *FT =
            FunctionType::get(Type::getDoubleTy(*CodeGen::TheContext), Doubles, false);

    Function *F =
            Function::Create(FT, Function::ExternalLinkage, Name, CodeGen::TheModule.get());

    // Set names for all arguments.
    unsigned Idx = 0;
    for (auto &Arg: F->args())
        Arg.setName(Args[Idx++]);

    return F;
}

llvm::Function *Pudl::AST::FunctionAST::codegen() {
    // Transfer ownership of the prototype to the FunctionProtos map, but keep a
    // reference to it for use below.
    auto &P = *Proto;
    CodeGen::FunctionProtos[Proto->getName()] = std::move(Proto);
    Function *TheFunction = CodeGen::getFunction(P.getName());
    if (!TheFunction)
        return nullptr;

    // If this is an operator, install it.
    if (P.isBinaryOp())
        Parser::BinopPrecedence[P.getOperatorName()] = P.getBinaryPrecedence();

    // Create a new basic block to start insertion into.
    BasicBlock *BB = BasicBlock::Create(*CodeGen::TheContext, "entry", TheFunction);
    CodeGen::Builder->SetInsertPoint(BB);

    // Record the function arguments in the NamedValues map.
    CodeGen::NamedValues.clear();
    for (auto &Arg: TheFunction->args()) {
        // Create an alloca for this variable.
        AllocaInst *Alloca = CodeGen::CreateEntryBlockAlloca(TheFunction, Arg.getName());

        // Store the initial value into the alloca.
        CodeGen::Builder->CreateStore(&Arg, Alloca);

        // Add arguments to variable symbol table.
        CodeGen::NamedValues[std::string(Arg.getName())] = Alloca;
    }

    if (Value * RetVal = Body->codegen()) {
        // Finish off the function.
        CodeGen::Builder->CreateRet(RetVal);

        // Validate the generated code, checking for consistency.
        llvm::verifyFunction(*TheFunction);

        // Run the optimizer on the function.
        CodeGen::TheFPM->run(*TheFunction);

        return TheFunction;
    }

    // Error reading body, remove function.
    TheFunction->eraseFromParent();

    if (P.isBinaryOp())
        Parser::BinopPrecedence.erase(P.getOperatorName());
    return nullptr;
}
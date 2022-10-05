#pragma once

#include <iostream>
#include <stack>
#include <typeinfo>
#include <map>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Utils.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Host.h>
#include <llvm/MC/TargetRegistry.h>
#include "llvm/Support/TargetSelect.h"
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>

#include "AST/ASTVisitor.h"

#include "Compiler/Linker/Linker.h"

using namespace llvm;

class Codegen : public ASTVisitor {
private:
    bool isSuccess;
    bool generateIR;
    bool isDebugMode;

    Module *module;
    IRBuilder<> builder;

    GlobalVariable *formati;
    GlobalVariable *formatf;
    Function *print;

    std::stack<Value *> operands;
    Value *retValue;

    std::map<std::string, Value *> scope;
    std::map<std::string, Value *> argScope;

    std::map<std::string, Function *> funcs;
    std::map<std::string, FunctionDefNode *> astFuncs;
    FunctionDefNode *currentFunc;

    llvm::legacy::FunctionPassManager *passManager;

    LLVMContext &getGlobalContext() {
        static LLVMContext GlobalContext;
        return GlobalContext;
    }

    Type *toLLVMType(TType aType) {
        switch (aType) {
            case TType::BOOL:
                return builder.getInt1Ty();
            case TType::INTEGER:
                return builder.getInt32Ty();
            case TType::FLOAT:
                return builder.getFloatTy();
        }
        return builder.getInt32Ty();
    }

    void error(std::string aMessage) {
        std::cout << "ERROR@IR: " << aMessage << std::endl;
        generateIR = false;
        isSuccess = false;
    }

    void infoln(std::string aMsg = "") {
        if (isDebugMode) {
            std::cout << aMsg << std::endl;
        }
    }

    /**
    Generates cast instruction
    \param aValue Value to cast
    \param aFrom Type of Value
    \param aTo New type of Value
    \return Cast instruction or Value (if types are equal)
    */
    Value *cast(Value *aValue, TType aFrom, TType aTo) {
        return builder.CreateCast(
                /*OpCode=*/ CastInst::getCastOpcode(
                        /*Value=*/ aValue,
                        /*SrcIsSigned=*/ true,
                        /*DestType=*/ toLLVMType(aTo),
                        /*DestIsSigned=*/ true),
                /*Value=*/ aValue,
                /*DestType=*/ toLLVMType(aTo)
        );

        infoln("created cast");
    }

    Value *pop() {
        Value *val = operands.top();
        operands.pop();
        return val;
    }

public:
    Codegen(bool debug = false) : builder(getGlobalContext()) {
        isSuccess = true;
        generateIR = true;
        isDebugMode = debug;

        module = new Module("pudl compiler", getGlobalContext());

        formati = new GlobalVariable(
                /*Module=*/ *module,
                /*Type=*/ ArrayType::get(IntegerType::get(module->getContext(), 8), 4),
                /*isConst=*/ true,
                /*Linkage=*/ GlobalVariable::PrivateLinkage,
                /*Initializer=*/ ConstantDataArray::getString(module->getContext(), "%d\n", true),
                /*Name=*/ ".formati"
        );

        formatf = new GlobalVariable(
                /*Module=*/ *module,
                /*Type=*/ ArrayType::get(IntegerType::get(module->getContext(), 8), 4),
                /*isConst=*/ true,
                /*Linkage=*/ GlobalVariable::PrivateLinkage,
                /*Initializer=*/ ConstantDataArray::getString(module->getContext(), "%f\n", true),
                /*Name=*/ ".formatf"
        );

        std::vector<Type *> args;
        args.push_back(builder.getInt8Ty()->getPointerTo());
        ArrayRef < Type * > argsRef(args);
        FunctionType *type = FunctionType::get(builder.getInt32Ty(), argsRef, true);

        auto func = module->getOrInsertFunction("printf", type);

        print = llvm::cast<llvm::Function>(func.getCallee());

        passManager = new llvm::legacy::FunctionPassManager(module);

        currentFunc = nullptr;
    }

    Module *getModule() {
        return module;
    }

    // Promote allocas to registers.
    void opt_promote_to_reg() {
        passManager->add(createPromoteMemoryToRegisterPass());
        passManager->doInitialization();
    }

    // Do simple "peephole" optimizations and bit-twiddling optzns.
    void opt_instcombine() {
        passManager->add(createInstructionCombiningPass());
        passManager->doInitialization();
    }

    // Reassociate expressions.
    void opt_reassociate() {
        passManager->add(createReassociatePass());
        passManager->doInitialization();
    }

    // Eliminate dead code & expressions.
    void opt_dce() {
        passManager->add(createDeadCodeEliminationPass());
        passManager->doInitialization();
    }

    // Eliminate Common SubExpressions.
    void opt_gvn() {
        passManager->add(createGVNPass());
        passManager->doInitialization();
    }

    // Simplify the control flow graph (deleting unreachable blocks, etc).
    void opt_simplifyCFG() {
        passManager->add(createCFGSimplificationPass());
        passManager->doInitialization();
    }

    void runSource() {
        std::string irOutput = "./out.ll";

        std::error_code EC;
        raw_fd_ostream dest(irOutput, EC, sys::fs::OF_None);

        if (EC) {
            errs() << "Could not open file: " << EC.message();
            return;
        }

        module->print(dest, nullptr);

        const std::string lliInsertionPoint = R"(
        define i32 @main() {
            start:
            %0 = call i32 @mast()
            ret i32 %0
        }
        )";

        std::ofstream file;
        file.open(irOutput, std::ios::app);
        file << std::endl << lliInsertionPoint << std::endl;
        file.close();

        std::cout << "Executing -----------------------" << std::endl << std::endl;

        std::string cmd = "lli " + irOutput;

        // Run lli output file
        std::system(cmd.c_str());

        // remove irOutput file and check for errors
        if (remove(irOutput.c_str()) != 0) {
            std::cerr << "Error deleting temp IR file " << irOutput << std::endl;
        }
    }

    void runObject(const char *cInPath, const char *linker) {
        std::string oOutPath = "out";

        linkObject(cInPath, oOutPath.c_str(), linker);

        std::cout << "Executing -----------------------" << std::endl << std::endl;

        std::string cmd = "./" + oOutPath;
        std::system(cmd.c_str());

        // Remove oOutPath file and check for errors
        if (remove(oOutPath.c_str()) != 0) {
            std::cerr << "Error deleting temp executable file " << oOutPath << std::endl;
        }
    }

    /**
     * Compiles and links source code to executable object file
     * @param oOutPath Path to output executable object file
     * @param linker Linker command
     */
    void linkSource(const char *oOutPath, const char *linker) {
        const char *cOutPath = "temp.o";

        // Compile source to object file
        if (compile(cOutPath) != 0) {
            std::cerr << "ERROR@COMPILE: Compilation failed" << std::endl;
            return;
        }

        // Link source object file with linker
        if (Linker::Link(cOutPath, oOutPath, linker) != 0) {
            std::cerr << "ERROR@LINK: Linking failed" << std::endl;
            isSuccess = false;
        }

        // Delete temp compile file after linking
        if (std::remove(cOutPath) != 0) {
            std::cerr << "ERROR@LINK: Deletion of temp compile file failed" << std::endl;
        }
    }

    /**
     * Links compiled object file with to executable object file
     * @param cInPath Path to compiled object file
     * @param oOutPath Path to output executable object file
     * @param linker Linker command
     */
    void linkObject(const char *cInPath, const char *oOutPath, const char *linker) {
        //  Link object file with compiled object cInPath
        if (Linker::Link(cInPath, oOutPath, linker) != 0) {
            std::cerr << "ERROR@LINK: Linking failed" << std::endl;
            isSuccess = false;
        }
    }

    /**
     * Compiles source code to object file
     * @param oOutPath Path to output object file
     * @return 0 if successful, != 0 if failed
     */
    int compile(const char *cOutPath) {
        InitializeAllTargetInfos();
        InitializeAllTargets();
        InitializeAllTargetMCs();
        InitializeAllAsmParsers();
        InitializeAllAsmPrinters();

        auto TargetTriple = sys::getDefaultTargetTriple();
        module->setTargetTriple(TargetTriple);

        std::string Error;
        auto Target = TargetRegistry::lookupTarget(TargetTriple, Error);

        // Print an error and exit if we couldn't find the requested target.
        // This generally occurs if we've forgotten to initialise the
        // TargetRegistry or we have a bogus target triple.
        if (!Target) {
            errs() << Error;
            return 1;
        }

        auto CPU = "generic";
        auto Features = "";

        TargetOptions opt;
        auto RM = Optional<Reloc::Model>();
        auto TheTargetMachine =
                Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);

        module->setDataLayout(TheTargetMachine->createDataLayout());

        std::error_code EC;
        raw_fd_ostream dest(cOutPath, EC, sys::fs::OF_None);

        if (EC) {
            errs() << "Could not open file: " << EC.message();
            return 1;
        }

        legacy::PassManager pass;
        auto FileType = llvm::CGFT_ObjectFile;

        if (TheTargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
            errs() << "TheTargetMachine can't emit a file of this type";
            return 1;
        }

        pass.run(*module);

        dest.flush();

        outs() << "Wrote " << cOutPath << "\n";

        return 0;
    }

    /**
    Saves IR to output file
    \param aOutPath File path
    */
    void save(const char *aOutPath) {
        std::error_code error_code;

        llvm::raw_fd_ostream out(aOutPath, error_code, llvm::sys::fs::OpenFlags::OF_None);
        module->print(
                out,
                NULL
        );
    }

    void dump() {
        module->dump();
    }

    /// Generates IR for all nodes in vector
    void visit(VectorNode aNode) {
        for (Node *node: aNode.getNodes()) {
            node->accept((*this));
        }
    }

    //// Do nothing
    void visit(DummyNode aNode) {}

    /**
    Generates IR for variable
    \return Returns via stack LOAD instruction if variable is presented in symbol table,
      else generates ERROR
    */
    void visit(VarNode aNode) {
        Value *val = argScope[aNode.getName()];
        Type *type = toLLVMType(aNode.getType());

        bool isArg = false;
        if (val == NULL) {
            val = scope[aNode.getName()];
        } else {
            isArg = true;
        }

        if (val == NULL) {
            error("Can't find variable " + aNode.getName());
            return;
        }

        if (isArg) {
            operands.push(val);
        } else {
            Value *loadInstr = builder.CreateLoad(type, val, aNode.getName());
            operands.push(loadInstr);
        }
    }

    void visit(BooleanNode aNode) {
        Value *val = ConstantInt::get(
                /*IntegerType=*/ builder.getInt1Ty(),
                /*value=*/ aNode.getValue(),
                /*isSigned*/ false
        );
        operands.push(val);
    }

    /**
    Generates IR for integer
    \return Returns via stack I32 constant
    */
    void visit(IntegerNode aNode) {
        Value *val = ConstantInt::get(
                /*IntegerType=*/ builder.getInt32Ty(),
                /*value=*/ aNode.getValue(),
                /*isSigned*/ false
        );
        operands.push(val);
    }

    /**
    Generates IR for float
    \return Returns via stack FP constant
    */
    void visit(FloatNode aNode) {
        infoln(std::to_string(aNode.getValue()));

        Value *val = ConstantFP::get(
                /*Type=*/ builder.getFloatTy(),
                /*value=*/ aNode.getValue()
        );
        operands.push(val);
    }

    /**
    Generates IR for assignment and declaration:
      ALLOCA instruction if variable is not presented in symbol table
      RHS IR (and type casting if needed)
      STORE
      IR: Name = alloca LLVMType, LLVMType Value
    */
    void visit(AssignmentNode aNode) {
        VarNode *variable = aNode.getLHS();
        Value *alloca = scope[variable->getName()];

        infoln("Assignment: " + variable->getName());

        aNode.getRHS()->accept((*this));
        if (!isSuccess) { return; }

        if (alloca == NULL) {
            alloca = builder.CreateAlloca(
                    toLLVMType(aNode.getType()),
                    NULL,
                    variable->getName()
            );

            Value *rhs = operands.top();
            if (variable->getType() != aNode.getRHS()->getType()) {
                rhs = cast(rhs, aNode.getRHS()->getType(), variable->getType());
            }
            builder.CreateStore(rhs, alloca);
            operands.pop();

            scope[variable->getName()] = alloca;

            infoln("gen?: Declared variable " + variable->getName());
        } else {
            Value *rhs = operands.top();
            if (variable->getType() != aNode.getRHS()->getType()) {
                rhs = cast(rhs, aNode.getRHS()->getType(), variable->getType());
            }

            builder.CreateStore(rhs, alloca);
            operands.pop();

            infoln("gen?: Assigned variable " + variable->getName());
        }
    }

    /**
    Generates IR for binary arithmetic operation:
      LHS IR
      RHS IR
      [Cast LHS to FP]
      [Cast RHS to FP]
      OpCode LHS RHS
    \return Result of operation
    */
    Value *biarithmetic(BinaryNode aNode) {
        TType lhsTy = aNode.getLHS()->getType();
        TType rhsTy = aNode.getRHS()->getType();

        aNode.getLHS()->accept((*this));
        aNode.getRHS()->accept((*this));

        Value *rhs = operands.top();
        operands.pop();
        Value *lhs = operands.top();
        operands.pop();

        std::string op = aNode.getOp();
        if (lhsTy == TType::FLOAT || rhsTy == TType::FLOAT) {
            lhs = cast(lhs, lhsTy, TType::FLOAT);
            rhs = cast(rhs, rhsTy, TType::FLOAT);

            if (op == "+") {
                return builder.CreateFAdd(lhs, rhs);
            } else if (op == "-") {
                return builder.CreateFSub(lhs, rhs);
            } else if (op == "*") {
                return builder.CreateFMul(lhs, rhs);
            } else if (op == "/") {
                return builder.CreateFDiv(lhs, rhs);
            }
        } else {
            if (op == "+") {
                return builder.CreateAdd(lhs, rhs);
            } else if (op == "-") {
                return builder.CreateSub(lhs, rhs);
            } else if (op == "*") {
                return builder.CreateMul(lhs, rhs);
            } else if (op == "/") {
                return builder.CreateUDiv(lhs, rhs);
            }
        }
        return NULL;
    }

    Value *birel(BinaryNode aNode) {
        TType lhsTy = aNode.getLHS()->getType();
        TType rhsTy = aNode.getRHS()->getType();

        aNode.getLHS()->accept((*this));
        aNode.getRHS()->accept((*this));

        Value *rhs = operands.top();
        operands.pop();
        Value *lhs = operands.top();
        operands.pop();

        std::string op = aNode.getOp();

        if (lhsTy == TType::FLOAT || rhsTy == TType::FLOAT) {
            lhs = cast(lhs, lhsTy, TType::FLOAT);
            rhs = cast(rhs, rhsTy, TType::FLOAT);

            if (op == "==") {
                return builder.CreateFCmp(CmpInst::FCMP_OEQ, lhs, rhs);
            } else if (op == "!=") {
                return builder.CreateFCmp(CmpInst::FCMP_ONE, lhs, rhs);
            } else if (op == ">") {
                return builder.CreateFCmp(CmpInst::FCMP_OGT, lhs, rhs);
            } else if (op == "<") {
                return builder.CreateFCmp(CmpInst::FCMP_OLT, lhs, rhs);
            } else if (op == ">=") {
                return builder.CreateFCmp(CmpInst::FCMP_OGE, lhs, rhs);
            } else if (op == "<=") {
                return builder.CreateFCmp(CmpInst::FCMP_OLE, lhs, rhs);
            }
        } else {
            if (op == "==") {
                return builder.CreateICmp(CmpInst::ICMP_EQ, lhs, rhs);
            } else if (op == "!=") {
                return builder.CreateICmp(CmpInst::ICMP_NE, lhs, rhs);
            } else if (op == ">") {
                return builder.CreateICmp(CmpInst::ICMP_SGT, lhs, rhs);
            } else if (op == "<") {
                return builder.CreateICmp(CmpInst::ICMP_SLT, lhs, rhs);
            } else if (op == ">=") {
                return builder.CreateICmp(CmpInst::ICMP_SGE, lhs, rhs);
            } else if (op == "<=") {
                return builder.CreateICmp(CmpInst::ICMP_SLE, lhs, rhs);
            }
        }
        return NULL;
    }

    Value *bilog(BinaryNode aNode) {
        TType lhsTy = aNode.getLHS()->getType();
        TType rhsTy = aNode.getRHS()->getType();

        aNode.getLHS()->accept((*this));
        aNode.getRHS()->accept((*this));

        Value *rhs = operands.top();
        operands.pop();
        Value *lhs = operands.top();
        operands.pop();

        std::string op = aNode.getOp();
        if (op == "&&") {
            return builder.CreateAnd(lhs, rhs);
        } else if (op == "||") {
            return builder.CreateOr(lhs, rhs);
        } else if (op == "^") {
            return builder.CreateXor(lhs, rhs);
        }
        return NULL;
    }

    /**
    Generates IR for binary operation
    \return Returns via stack result of operation
    */
    void visit(BinaryNode aNode) {
        std::string op = aNode.getOp();

        if (op == "+" || op == "-" || op == "*" || op == "/") {
            Value *res = biarithmetic(aNode);
            if (res == NULL) {
                error("unknown error");
                return;
            }
            operands.push(res);
        } else if (
                op == "==" || op == "!="
                || op == ">" || op == "<"
                || op == ">=" || op == "<="
                ) {
            Value *res = birel(aNode);
            if (res == NULL) {
                error("unknown error");
                return;
            }
            operands.push(res);
        } else if (op == "&&" || op == "||" || op == "^") {
            Value *res = bilog(aNode);
            if (res == NULL) {
                error("unknown error");
                return;
            }
            operands.push(res);
        }
    }

    Value *unarithmetic(UnaryNode aNode) {
        TType ty = aNode.getType();
        aNode.getSubexpr()->accept((*this));
        Value *val = pop();
        if (ty == TType::FLOAT) {
            return builder.CreateFNeg(val);
        } else if (ty == TType::INTEGER) {
            return builder.CreateNeg(val);
        }
        return nullptr;
    }

    Value *unlog(UnaryNode aNode) {
        aNode.getSubexpr()->accept((*this));
        Value *val = pop();
        return builder.CreateNot(val);
    }

    // (<operator> <subexpr>)
    void visit(UnaryNode aNode) {
        std::string op = aNode.getOp();

        if (op == "-") {
            Value *res = unarithmetic(aNode);
            if (res == NULL) {
                error("unknown error");
                return;
            }
            operands.push(res);
        } else if (op == "!") {
            Value *res = unlog(aNode);
            if (res == NULL) {
                error("unknown error");
                return;
            }
            operands.push(res);
        }
    }

    /**
    Generates IR for function call
    \return Returns via stack call instruction
    */
    void visit(FuncallNode aNode) {
        Function *func = funcs[aNode.getName()];
        FunctionDefNode *ast = astFuncs[aNode.getName()];

        if (func == NULL) {
            std::cout << "undefined function " << aNode.getName() << std::endl;
            return;
        }

        std::vector<ExpressionNode *> args = aNode.getArgs();
        std::vector<VarNode *> astArgs = ast->getArgs();
        std::vector<Value *> argsVal;

        if (args.size() != astArgs.size()) {
            std::cout << "excepted " << astArgs.size() << " arguments but given "
                      << args.size() << std::endl;
            return;
        }

        int idx(0);
        for (ExpressionNode *arg: args) {
            arg->accept((*this));

            Value *res = operands.top();
            operands.pop();

            res = cast(res, arg->getType(), astArgs[idx]->getType());
            argsVal.push_back(res);

            idx++;
        }

        Value *callInstr = builder.CreateCall(func, argsVal);
        operands.push(callInstr);
    }

    /**
    Generates IR for function
    */
    void visit(FunctionDefNode aNode) {
        infoln("gen?: generating function definition " + aNode.getName());

        scope.clear();
        argScope.clear();

        std::vector<Type *> argsTy;
        std::vector<VarNode *> args = aNode.getArgs();
        for (auto it = args.begin(); it != args.end(); ++it) {
            argsTy.push_back(toLLVMType((*it)->getType()));
        }

        FunctionType *funcType = FunctionType::get(
                toLLVMType(aNode.getType()), argsTy, false
        );
        Function *func = Function::Create(
                funcType, Function::ExternalLinkage, aNode.getName(), module
        );

        infoln("DEF " + aNode.getName());

        funcs[aNode.getName()] = func;

        astFuncs[aNode.getName()] = new FunctionDefNode(
                aNode.getName(), aNode.getArgs(), aNode.getBody(), aNode.getType()
        );
        currentFunc = &aNode;

        int idx(0);
        for (auto arg = func->arg_begin(); idx != argsTy.size(); ++arg, ++idx) {
            std::string argName = args[idx]->getName();
            arg->setName(argName);
            argScope[argName] = arg;
        }

        BasicBlock *bb = BasicBlock::Create(
                getGlobalContext(), "entry", func
        );
        builder.SetInsertPoint(bb);

        aNode.getBody()->accept((*this));

        passManager->run(*func);
    }

    void visit(BlockStatementNode aNode) {
        for (StatementNode *node: aNode.getStatements()) {
            node->accept((*this));
        }
    }

    // (If <expression> <statement> (Else <statement>)?
    void visit(IfStatementNode aNode) {
        Function *func = funcs[currentFunc->getName()];
        aNode.getCond()->accept((*this));
        Value *cond = pop();

        BasicBlock *thenBb = BasicBlock::Create(getGlobalContext(), "Then", func);
        BasicBlock *elseBb = BasicBlock::Create(getGlobalContext(), "Else");
        BasicBlock *mergeBb = BasicBlock::Create(getGlobalContext(), "Merge");

        builder.CreateCondBr(cond, thenBb, elseBb);
        builder.SetInsertPoint(thenBb);

        aNode.getTrueBranch()->accept((*this));
        builder.CreateBr(mergeBb);
        thenBb = builder.GetInsertBlock();

        func->getBasicBlockList().push_back(elseBb);
        builder.SetInsertPoint(elseBb);

        if (aNode.getFalseBranch()) {
            aNode.getFalseBranch()->accept((*this));
        }

        builder.CreateBr(mergeBb);
        elseBb = builder.GetInsertBlock();

        func->getBasicBlockList().push_back(mergeBb);
        builder.SetInsertPoint(mergeBb);
    }

    void visit(WhileStatementNode aNode) {
        Function *func = funcs[currentFunc->getName()];

        BasicBlock *loopBb = BasicBlock::Create(getGlobalContext(), "Loop", func);
        BasicBlock *thenBb = BasicBlock::Create(getGlobalContext(), "Then");
        BasicBlock *afterBb = BasicBlock::Create(getGlobalContext(), "After");

        builder.CreateBr(loopBb);

        builder.SetInsertPoint(loopBb);
        aNode.getCond()->accept((*this));
        Value *cond = pop();

        builder.CreateCondBr(cond, thenBb, afterBb);
        builder.SetInsertPoint(thenBb);
        aNode.getBody()->accept((*this));
        builder.CreateBr(loopBb);

        func->getBasicBlockList().push_back(thenBb);
        thenBb = builder.GetInsertBlock();

        func->getBasicBlockList().push_back(afterBb);
        builder.SetInsertPoint(afterBb);
    }

    void visit(DoWhileStatementNode aNode) {
        infoln("gen?: generating do-while statement");
        Function *func = funcs[currentFunc->getName()];

        BasicBlock *loopBb = BasicBlock::Create(getGlobalContext(), "Loop", func);
        BasicBlock *afterBb = BasicBlock::Create(getGlobalContext(), "After");

        builder.CreateBr(loopBb);

        builder.SetInsertPoint(loopBb);
        aNode.getBody()->accept((*this));
        aNode.getCond()->accept((*this));
        Value *cond = pop();
        builder.CreateCondBr(cond, loopBb, afterBb);

        func->getBasicBlockList().push_back(afterBb);
        builder.SetInsertPoint(afterBb);
    }

    // <expression>
    void visit(ExpressionWrapperNode aNode) {
        // aNode.getExpr()->accept((*this));
    }

    /**
    Generates IR for print statement (intrinsic)
    */
    void visit(IoPrintNode aNode) {
        aNode.getSubexpr()->accept((*this));

        std::vector<Value *> args;
        std::vector<Value *> idx;
        idx.push_back(ConstantInt::get(builder.getInt32Ty(), 0, false));
        idx.push_back(ConstantInt::get(builder.getInt32Ty(), 0, false));
        ArrayRef < Value * > idxRef(idx);


        if (aNode.getSubexpr()->getType() == TType::FLOAT) {
            args.push_back(builder.CreateInBoundsGEP(formatf->getValueType(), formatf, idxRef, formatf->getName()));
            args.push_back(builder.CreateFPExt(operands.top(), builder.getDoubleTy()));
        } else {
            args.push_back(builder.CreateInBoundsGEP(formati->getValueType(), formati, idxRef, formati->getName()));
            args.push_back(operands.top());
        }
        operands.pop();

        ArrayRef < Value * > argsRef(args);
        builder.CreateCall(print, argsRef);
    }

    /**
    Generates IR for return statement
    */
    void visit(ReturnNode aNode) {
        aNode.getSubexpr()->accept((*this));

        Value *res = operands.top();
        operands.pop();

        res = cast(res, aNode.getSubexpr()->getType(), currentFunc->getType());
        builder.CreateRet(res);
    }
};
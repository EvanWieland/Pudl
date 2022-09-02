#include "Driver.h"

void Pudl::Driver::InitializeModuleAndPassManager() {
    // Open a new module.
    CodeGen::TheContext = std::make_unique<llvm::LLVMContext>();
    CodeGen::TheModule = std::make_unique<llvm::Module>("PudlJIT", *CodeGen::TheContext);
    CodeGen::TheModule->setDataLayout(CodeGen::TheJIT->getDataLayout());

    // Create a new builder for the module.
    CodeGen::Builder = std::make_unique<llvm::IRBuilder<>>(*CodeGen::TheContext);

    // Create a new pass manager attached to it.
    CodeGen::TheFPM = std::make_unique<llvm::legacy::FunctionPassManager>(CodeGen::TheModule.get());

    // Promote allocas to registers.
    CodeGen::TheFPM->add(llvm::createPromoteMemoryToRegisterPass());
    // Do simple "peephole" optimizations and bit-twiddling optzns.
    CodeGen::TheFPM->add(llvm::createInstructionCombiningPass());
    // Reassociate expressions.
    CodeGen::TheFPM->add(llvm::createReassociatePass());
    // Eliminate Common SubExpressions.
    CodeGen::TheFPM->add(llvm::createGVNPass());
    // Simplify the control flow graph (deleting unreachable blocks, etc).
    CodeGen::TheFPM->add(llvm::createCFGSimplificationPass());

    CodeGen::TheFPM->doInitialization();
}

void Pudl::Driver::HandleDefinition() {
    if (auto FnAST = Parser::ParseDefinition()) {
        if (auto *FnIR = FnAST->codegen()) {
            if (FileStreamer::InREPL()) {
                fprintf(stderr, "Read function definition:");
                FnIR->print(llvm::errs());
                fprintf(stderr, "\n");
            }
            CodeGen::ExitOnErr(CodeGen::TheJIT->addModule(
                    ThreadSafeModule(std::move(CodeGen::TheModule),
                                     std::move(CodeGen::TheContext))));
            InitializeModuleAndPassManager();
        }
    } else {
        // Skip token for error recovery.
        Parser::getNextToken();
    }
}

void Pudl::Driver::HandleExtern() {
    if (auto ProtoAST = Parser::ParseExtern()) {
        if (auto *FnIR = ProtoAST->codegen()) {
            if (FileStreamer::InREPL()) {
                fprintf(stderr, "Read extern: ");
                FnIR->print(llvm::errs());
                fprintf(stderr, "\n");
            }
            CodeGen::FunctionProtos[ProtoAST->getName()] = std::move(ProtoAST);
        }
    } else {
        // Skip token for error recovery.
        Parser::getNextToken();
    }
}

void Pudl::Driver::HandleTopLevelExpression() {
    // Evaluate a top-level expression into an anonymous function.
    if (auto FnAST = Parser::ParseTopLevelExpr()) {
        if (FnAST->codegen()) {
            // Create a ResourceTracker to track JIT'd memory allocated to our
            // anonymous expression -- that way we can free it after executing.
            auto RT = CodeGen::TheJIT->getMainJITDylib().createResourceTracker();

            auto TSM = ThreadSafeModule(std::move(CodeGen::TheModule),
                                        std::move(CodeGen::TheContext));
            CodeGen::ExitOnErr(CodeGen::TheJIT->addModule(std::move(TSM), RT));
            InitializeModuleAndPassManager();

            // Search the JIT for the __anon_expr symbol.
            auto ExprSymbol = CodeGen::ExitOnErr(CodeGen::TheJIT->lookup("__anon_expr"));

            // Get the symbol's address and cast it to the right type (takes no
            // arguments, returns a double) so we can call it as a native function.
            auto (*FP)() = (double (*)()) (intptr_t) ExprSymbol.getAddress();

            double result = FP();

            if (FileStreamer::InREPL())
                fprintf(stderr, "Evaluated to %f\n", result);


            // Delete the anonymous expression module from the JIT.
            CodeGen::ExitOnErr(RT->remove());
        }
    } else {
        // Skip token for error recovery.
        Parser::getNextToken();
    }
}

void Pudl::Driver::MainLoop() {
    while (true) {
        if (FileStreamer::InREPL()) {
            fprintf(stderr, "ready> ");
        }

        switch (Parser::CurTok) {
            case Lexer::tok_eof:
                return;
            case ';': // ignore top-level semicolons.
                Parser::getNextToken();
                break;
            case Lexer::tok_def:
                HandleDefinition();
                break;
            case Lexer::tok_extern:
                HandleExtern();
                break;
            default:
                HandleTopLevelExpression();
                break;
        }
    }
}
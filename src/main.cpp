//#include "llvm/ADT/APFloat.h"
//#include "llvm/Support/TargetSelect.h"
//#include <cstdio>
//#include <iostream>
//
//#include "grammar/Parser.h"
//#include "lib/IO.h"
//#include "logic/PudlJIT.h"
//#include "logic/CLIManager.h"
//#include "logic/ObjectEmitter.h"
//#include "logic/CodeGen.h"
//#include "logic/Driver.h"

//
//
//int run() {
//    // Prime the first token.
//    Pudl::Parser::getNextToken();
//
//    Pudl::CodeGen::TheJIT = Pudl::CodeGen::ExitOnErr(Pudl::PudlJIT::Create());
//
//    Pudl::CodeGen::TheJIT->registerSymbols(
//            [&](llvm::orc::MangleAndInterner interner) {
//                llvm::orc::SymbolMap symbolMap;
//                // Add symbols here
//                symbolMap[interner("printd")] = llvm::JITEvaluatedSymbol::fromPointer(Pudl::Lib::IO::printd);
//                return symbolMap;
//            });
//
//    Pudl::Driver::InitializeModuleAndPassManager();
//
//    // Run the main "interpreter loop" now.
//    Pudl::Driver::MainLoop();
//
//    return 0;
//}
//
//int main(int argc, char **argv) {
//    Pudl::CLIManager cli(argc, argv);
//
//    if (cli.hasOption("--help") || cli.hasOption("-h")) {
//        std::cerr << "Usage: compiler <file> [--help,-h] [-p --print-ir]" << std::endl;
//        return 1;
//    }
//
//    if (cli.hasOption("-p") || cli.hasOption("--print-ir")) {
//        Pudl::CodeGen::TheModule->print(llvm::outs(), nullptr);
//        std::cout << std::endl;
//    }
//
//    if (argc > 1)
//        Pudl::Driver::OpenFileStream("test.pudl");
//        // Pudl::Driver::OpenFileStream(argv[1]);
//
//    InitializeNativeTarget();
//    InitializeNativeTargetAsmPrinter();
//    InitializeNativeTargetAsmParser();
//
//    // Install standard binary operators. 1 is the lowest precedence.
//    Pudl::Parser::BinopPrecedence['='] = 2;
//    Pudl::Parser::BinopPrecedence['<'] = 10;
//    Pudl::Parser::BinopPrecedence['+'] = 20;
//    Pudl::Parser::BinopPrecedence['-'] = 20;
//    Pudl::Parser::BinopPrecedence['*'] = 40; // highest.
//
//    if (Pudl::Driver::InREPL())
//        fprintf(stderr, "ready> ");
//
//    run();
//
//    // Exit stream
//    if (!Pudl::Driver::InREPL())
//        Pudl::Driver::CloseFileStream();
//
//    return 0;
//}


#include "llvm/Support/TargetSelect.h"
#include <cstdio>

#include "CLIManager.h"
#include "FileStreamer.h"
#include "PudlJIT.h"
#include "grammar/Parser.h"
#include "grammar/CodeGen.h"
#include "Driver.h"
#include "lib/IO.h"

using namespace llvm;
using namespace llvm::orc;
using namespace Pudl;

//===----------------------------------------------------------------------===//
// Main driver code.
//===----------------------------------------------------------------------===//

int main(int argc, char **argv) {
    CLIManager cli(argc, argv);

    if (cli.hasOption("--help") || cli.hasOption("-h")) {
        std::cerr << "Usage: compiler <file> [--help,-h] [-p --print-ir]" << std::endl;
        return 1;
    }

    if (cli.hasOption("-p") || cli.hasOption("--print-ir")) {
        Pudl::CodeGen::TheModule->print(llvm::outs(), nullptr);
        std::cout << std::endl;
    }

    if (argc > 1)
        FileStreamer::OpenFileStream(argv[1]);

    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();

    // Install standard binary operators.
    // 1 is lowest precedence.
    Parser::BinopPrecedence['='] = 2;
    Parser::BinopPrecedence['<'] = 10;
    Parser::BinopPrecedence['+'] = 20;
    Parser::BinopPrecedence['-'] = 20;
    Parser::BinopPrecedence['*'] = 40; // highest.

    if (FileStreamer::InREPL())
        fprintf(stderr, "ready> ");

    // Prime the first token.
    Parser::getNextToken();

    CodeGen::TheJIT = CodeGen::ExitOnErr(PudlJIT::Create());

    CodeGen::TheJIT->registerSymbols(
            [&](llvm::orc::MangleAndInterner interner) {
                llvm::orc::SymbolMap symbolMap;
                symbolMap[interner("printd")] = llvm::JITEvaluatedSymbol::fromPointer(Lib::IO::printd);
                symbolMap[interner("putchard")] = llvm::JITEvaluatedSymbol::fromPointer(Lib::IO::putchard);
                return symbolMap;
            }
    );

    Driver::InitializeModuleAndPassManager();

    // Run the main "interpreter loop" now.
    Driver::MainLoop();

    if (!FileStreamer::InREPL())
        FileStreamer::CloseFileStream();

    return 0;
}
#include "llvm/Support/TargetSelect.h"
#include <cstdio>

#include "CLIManager.h"
#include "FileStreamer.h"
#include "PudlJIT.h"
#include "grammar/Parser.h"
#include "grammar/CodeGen.h"
#include "Driver.h"
#include "lib/IO.h"

//===----------------------------------------------------------------------===//
// Main driver code.
//===----------------------------------------------------------------------===//

int main(int argc, char **argv) {
    Pudl::CLIManager cli(argc, argv);

    if (cli.hasOption("--help") || cli.hasOption("-h")) {
        std::cerr << "Usage: compiler <file> [--help,-h] [-p --print-ir]" << std::endl;
        return 1;
    }

    if (cli.hasOption("-p") || cli.hasOption("--print-ir")) {
        Pudl::CodeGen::TheModule->print(llvm::outs(), nullptr);
        std::cout << std::endl;
    }

    if (argc > 1)
        Pudl::FileStreamer::OpenFileStream(argv[1]);

    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();

    // Install standard binary operators.
    // 1 is lowest precedence.
    Pudl::Parser::BinopPrecedence['='] = 2;
    Pudl::Parser::BinopPrecedence['<'] = 10;
    Pudl::Parser::BinopPrecedence['+'] = 20;
    Pudl::Parser::BinopPrecedence['-'] = 20;
    Pudl::Parser::BinopPrecedence['*'] = 40; // highest.

    if (Pudl::FileStreamer::InREPL())
        fprintf(stderr, "ready> ");

    // Prime the first token.
    Pudl::Parser::getNextToken();

    Pudl::CodeGen::TheJIT = Pudl::CodeGen::ExitOnErr(Pudl::PudlJIT::Create());

    Pudl::CodeGen::TheJIT->registerSymbols(
            [&](llvm::orc::MangleAndInterner interner) {
                llvm::orc::SymbolMap symbolMap;
                symbolMap[interner("printd")] = llvm::JITEvaluatedSymbol::fromPointer(Pudl::Lib::IO::printd);
                symbolMap[interner("putchard")] = llvm::JITEvaluatedSymbol::fromPointer(Pudl::Lib::IO::putchard);
                return symbolMap;
            }
    );

    Pudl::Driver::InitializeModuleAndPassManager();

    // Run the main "interpreter loop" now.
    Pudl::Driver::MainLoop();

    if (!Pudl::FileStreamer::InREPL())
        Pudl::FileStreamer::CloseFileStream();

    return 0;
}

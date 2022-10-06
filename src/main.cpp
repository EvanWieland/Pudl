#include <iostream>
#include <cstdio>

#include "llvm/Support/Host.h"

#include "Parser/Printer.h"
#include "Parser/Codegen.h"
#include "Parser/Parser.h"
#include "Compiler/CLIManager.h"

int main(int argc, char *argv[]) {


    std::cout << ".--------------------------------." << std::endl
              << "| Pudl Language Compiler v.0.0.1 |" << std::endl
              << "'--------------------------------'" << std::endl;

    bool isSourceFile, compile, link, printIR, debug = false;

    CLIManager cli(argc, argv);

    if (argc < 2 || cli.hasOption("--help") || cli.hasOption("-h")) {
        std::string help = R"(
        ./pudl.sh <file> [--help,-h]  [-p,--print-ir <out file>] [-c,--compile <out file>] [-o, --output <out file>] [-O<N>] [-l,--linker <linker>]

        Options:
        <file>                The file to run.
                                          - Source file
        - Compiled object file

        -c, --compile         Compile the source file to an object file
        -o, --output          Create an executable output file
        -l, --linker          Linker command to call when linking object files.
                                                                         - Default: clang++-13
                                                                                    -h, --help            Get usage and available options
        -p  --print-ir        Print generated LLVM IR to stdout or to a file
        -d, --debug           Print debug information

        -O<N>                 Optimization level
        - Default: OAll
        - None: No optimizations
        - 0: No optimizations
        - 1: Promote allocas to registers
        - 2: Reassociate expressions
        - 3: Dead code elimination
        - 4: Global value numbering
        - 5: Simplify control flow graph (CFG)
        - All: All optimizations
        )";

        std::cerr << help << std::endl;

        return 1;
    }

    // Check if file is binary or ASCII
    FILE *file = fopen(argv[1], "r");
    if (file) {
        int c;
        while ((c = getc(file)) != EOF && c <= 127);
        if (c == EOF) {
            // File is ASCII, ostensibly a Pudl source file
            isSourceFile = true;
        } else {
            // File is binary, ostensibly a compiled Pudl object file
            isSourceFile = false;
        }
        rewind(file);
    } else {
        std::cout << "Can't open file " << argv[1] << "\n";
        return 1;
    }

    std::cout << "Loading " << (isSourceFile ? "source" : "object") << " file " << argv[1] << std::endl;

    if (cli.hasOption("-d") || cli.hasOption("--debug")) {
        std::cout << "In debug mode" << std::endl;
        debug = true;
    }

    std::string pOut = cli.getOptionValue("-p", cli.getOptionValue("--print-ir"));
    if (cli.hasOption("-p") || cli.hasOption("--print-ir")) {
        std::cout << "Printing LLVM IR: " << (pOut.empty() ? " stderr" : pOut) << std::endl;
        printIR = true;
    }

    std::string cOut = cli.getOptionValue("-c", cli.getOptionValue("--compile"));
    if (cli.hasOption("-c") || cli.hasOption("--compile")) {
        if (cOut.empty()) {
            // If no compile output file is specified, use the input file name
            cOut = argv[1];
            cOut = cOut.substr(cOut.find_last_of("/\\") + 1);
            cOut = cOut.substr(0, cOut.find_last_of('.'));
            cOut += ".o";

            std::cout << "No compile output was specified for compile step." << std::endl;
        }
        std::cout << "Compiling to object file: " << cOut << std::endl;
        compile = true;
    }

    std::string linker = cli.getOptionValue("-l", cli.getOptionValue("--linker"));
    std::string oOut = cli.getOptionValue("-o", cli.getOptionValue("--output"));
    if (cli.hasOption("-o") || cli.hasOption("--output")) {
        if (linker.empty()) {
            std::cout << "No linker was specified for linker step." << std::endl;
            linker = LinkerCmd;
        }

        std::cout << "Using linker: " << linker << std::endl;

        if (argv[1] == nullptr) {
            std::cerr << "No input file specified for link step!" << std::endl;
            return 1;
        }

        if (argv[1] == oOut) {
            std::cerr << "Input file and output file are the same for linker step!" << std::endl;
            return 1;
        }

        if (oOut.empty()) {
            // If no output file is specified, use the input file name
            oOut = argv[1];
            oOut = oOut.substr(oOut.find_last_of("/\\") + 1);
            oOut = oOut.substr(0, oOut.find_last_of('.'));

            std::cout << "No output file was specified for linker step." << std::endl;
        }

        std::cout << "Emitting executable: " << oOut << std::endl;

        link = true;
    }

    if (compile && link) {
        std::cout << "Can't compile and link at the same time" << std::endl;
        return 1;
    }

    auto parser = Parser(debug);

    Node *root = parser.parse(file);

    Printer printer = Printer();
    Codegen codegen = Codegen(debug);

    bool optimizeLevelSpecified = false;

    if (cli.hasOption("-O0") || cli.hasOption("-ONone")) {
        optimizeLevelSpecified = true;
        std::cout << "Optimization: none" << std::endl;
    }

    if (cli.hasOption("-O1")) {
        optimizeLevelSpecified = true;
        std::cout << "Optimization: promote allocas to registers" << std::endl;
        codegen.opt_promote_to_reg();
    }

    if (cli.hasOption("-O2")) {
        optimizeLevelSpecified = true;
        std::cout << "Optimization: instruction combining" << std::endl;
        codegen.opt_instcombine();
    }

    if (cli.hasOption("-O3")) {
        optimizeLevelSpecified = true;
        std::cout << "Optimization: reassociate" << std::endl;
        codegen.opt_reassociate();
    }

    if (cli.hasOption("-O4")) {
        optimizeLevelSpecified = true;
        std::cout << "Optimization: dead code elimination" << std::endl;
        codegen.opt_dce();
    }

    if (cli.hasOption("-O5")) {
        optimizeLevelSpecified = true;
        std::cout << "Optimization: global value numbering" << std::endl;
        codegen.opt_gvn();
    }

    if (cli.hasOption("-O6")) {
        optimizeLevelSpecified = true;
        std::cout << "Optimization: simplify CFG" << std::endl;
        codegen.opt_simplifyCFG();
    }

    if (cli.hasOption("-Oall") || !optimizeLevelSpecified) {

        if (!cli.hasOption("-Oall") && !optimizeLevelSpecified) {
            std::cout << "No optimization level specified, using level -Oall" << std::endl;
        }

        std::cout << "Optimization: instruction combining, " << std::endl
                  << "\t promote allocas to registers, " << std::endl
                  << "\t reassociate, " << std::endl
                  << "\t dead code elimination, " << std::endl
                  << "\t global value numbering, " << std::endl
                  << "\t simplify CFG" << std::endl;

        codegen.opt_promote_to_reg();
        codegen.opt_instcombine();
        codegen.opt_reassociate();
        codegen.opt_dce();
        codegen.opt_gvn();
        codegen.opt_simplifyCFG();
    }

    if (root != nullptr) {
        if (debug) {
            root->accept(printer);
        }

        if (!parser.isFailed()) {
            std::cout << std::endl;
            root->accept(codegen);

            if (compile) {
                // Equivalence: gcc foo.pudl -c foo.o >> foo.o
                codegen.compile(cOut.c_str());
            }

            if (link) {
                if (isSourceFile) {
                    // Equivalence: gcc foo.pudl -o foo >> foo
                    codegen.linkSource(oOut.c_str(), linker.c_str());
                } else {
                    // Equivalence: gcc foo.o -o foo >> foo
                    codegen.linkObject(argv[1], oOut.c_str(), linker.c_str());
                }
            }

            // Run input file if no compile or link steps are requested
            if (!link && !compile) {
                if (isSourceFile) {
                    codegen.runSource();
                } else {
                    if (linker.empty()) {
                        std::cout << "No linker was specified for linker step." << std::endl;
                        linker = LinkerCmd;
                    }

                    std::cout << "Using linker: " << linker << std::endl;

                    codegen.runObject(argv[1], linker.c_str());
                }
            }

            // Print out LLVM IR
            if (printIR) {
                if (pOut.empty()) {
                    // Print to stderr if no output file is specified
                    codegen.dump();
                } else {
                    // Save to file if output file is specified
                    codegen.save(pOut.c_str());
                }
            }
        }
    } else {
        std::cout << "EXIT" << std::endl;
    }

    std::cout << std::endl;
}

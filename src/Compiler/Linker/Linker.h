#pragma once

#include <cstdlib>
#include <fstream>
#include <string>
#include <iostream>
#include <vector>

#include "../Process.h"

#define LinkerProgram "TempLinker.cpp"

class Linker {
private:
    inline static const std::string program = R"(
    #include <iostream>

    extern "C" {
        int mast();
    }

    int main() {
        std::cout << mast() << std::endl;
    }
    )";

public:
    /**
     * Best-effort detection of an available clang++-family compiler/linker
     * driver on PATH. This used to be a hardcoded "clang++-13", which broke
     * the moment the installed LLVM's version number didn't match.
     */
    static std::string DetectDefault() {
        return Process::Detect({
                "clang++", "clang++-18", "clang++-19", "clang++-20",
                "clang++-17", "c++", "g++"
        });
    }

    static int Link(const char *inPath, const char *outPath, const char *linker) {
        // save raw program to file for linking
        std::ofstream out(LinkerProgram);
        out << program;
        out.close();

        int result = Process::Run({linker, LinkerProgram, inPath, "-o", outPath});

        if (result == 0) {
            std::cout << "Linking successful" << std::endl;
        } else {
            std::cout << "Linking failed" << std::endl;
            return 1;
        }

        if (remove(LinkerProgram) != 0) {
            std::cout << "Failed to remove temp linker file" << std::endl;
            return 1;
        }

        return 0;
    }

};
#pragma once

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <string>
#include <iostream>
#include <vector>

#include "../Process.h"

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
     * Best-effort detection of an available C++ compiler/linker driver on
     * PATH. This used to be a hardcoded "clang++-13", which broke the
     * moment the installed LLVM's version number didn't match.
     *
     * On Windows, MSVC's own cl.exe is preferred over clang++: the
     * clang++ that ships alongside a given LLVM release is frequently
     * *older* than the installed MSVC STL headers will accept (current
     * MSVC headers hard-error with "STL1000: Unexpected compiler version,
     * expected Clang 20 or newer"), so a clang++ found on PATH there
     * cannot reliably compile the #include <iostream> wrapper below.
     */
    static std::string DetectDefault() {
#ifdef _WIN32
        return Process::Detect({
                "cl", "clang++", "clang++-20", "clang++-19", "clang++-18"
        }, "/?");
#else
        return Process::Detect({
                "clang++", "clang++-18", "clang++-19", "clang++-20",
                "clang++-17", "c++", "g++"
        });
#endif
    }

    /// True if `linker` is MSVC's cl.exe, which takes /Fe: rather than -o
    /// for its output path (and doesn't understand -o at all).
    static bool isMsvcCl(const std::string &linker) {
        std::string lower;
        for (char c: linker) {
            lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return lower == "cl" || lower == "cl.exe";
    }

    static int Link(const char *inPath, const char *outPath, const char *linker) {
        // Unique per call so two concurrent `pudl` invocations in the same
        // directory don't clobber each other's throwaway linker source.
        std::string linkerProgram = Process::UniqueTempPath("TempLinker", ".cpp");

        // save raw program to file for linking
        std::ofstream out(linkerProgram);
        out << program;
        out.close();

        std::vector<std::string> args;
        if (isMsvcCl(linker)) {
            std::string outArg = std::string("/Fe:") + outPath;
            args = {linker, linkerProgram, inPath, "/EHsc", "/nologo", outArg};
        } else {
            args = {linker, linkerProgram, inPath, "-o", outPath};
        }

        int result = Process::Run(args);

        if (result == 0) {
            std::cout << "Linking successful" << std::endl;
        } else {
            std::cout << "Linking failed" << std::endl;
            return 1;
        }

        if (remove(linkerProgram.c_str()) != 0) {
            std::cout << "Failed to remove temp linker file" << std::endl;
            return 1;
        }

        return 0;
    }

};
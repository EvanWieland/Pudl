#pragma once

#include <cstdlib>
#include <fstream>
#include <string>
#include <iostream>

#define LinkerCmd "clang++-9"
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
    static int Link(const char *inPath, const char *outPath, const char *linker) {
        // save raw program to file for linking
        std::ofstream out(LinkerProgram);
        out << program;
        out.close();

        std::string cmd = std::string(linker) + " " + LinkerProgram + " " + inPath + " -o " + outPath;

        if (std::system(cmd.c_str()) == 0) {
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
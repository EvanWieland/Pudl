//===----------------------------------------------------------------------===//
// File stream logic for the Pudl compiler.
//===----------------------------------------------------------------------===//

#pragma once

#include <fstream>

namespace Pudl {
    class FileStreamer {
    private:
        static std::fstream *FileStream;
    public:
        static void OpenFileStream(const char *);

        static void CloseFileStream();

        static bool InREPL();

        static int GetChar();
    };
} // namespace Pudl
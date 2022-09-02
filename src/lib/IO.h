//===----------------------------------------------------------------------===//
// IO library functions that can be "extern'd" from user code.
//===----------------------------------------------------------------------===//

#pragma once

#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

#include <iostream>

namespace Pudl::Lib::IO {
    extern "C" {

    /// putchard - putchar that takes a double and returns 0.
    DLLEXPORT double putchard(double X) {
        fputc((char) X, stderr);
        return 0;
    }

    /// printd - printf that takes a double prints it as "%f\n", returning 0.
    DLLEXPORT double printd(double X) {
        fprintf(stderr, "%f\n", X);
        return 0;
    }

    } // extern "C"
} // namespace Pudl::Lib::IO
//===----------------------------------------------------------------------===//
// Top-Level parsing and JIT Driver
//===----------------------------------------------------------------------===//

#pragma once

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Utils.h"

#include <memory>
#include <iostream>

#include "grammar/Lexer.h"
#include "grammar/CodeGen.h"
#include "FileStreamer.h"

namespace Pudl {
    class Driver {
    private:

        static void HandleDefinition();

        static void HandleExtern();

        static void HandleTopLevelExpression();

    public:
        /// top ::= definition | external | expression | ';'
        static void MainLoop();

        static void InitializeModuleAndPassManager();
    };
} // namespace Pudl
//===----------------------------------------------------------------------===//
// Code Generation
//===----------------------------------------------------------------------===//

#pragma once

#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <llvm-14/llvm/IR/Type.h>
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Verifier.h"

#include "PudlJIT.h"
#include "grammar/AST.h"
#include "grammar/Lexer.h"
#include "grammar/Parser.h"

namespace Pudl {
    class CodeGen {
    public:
        // Log a value error
        static llvm::Value *LogErrorV(const char *);
        // Log a type error
        static llvm::Type *LogErrorT(const char *);

        static llvm::Function *getFunction(std::string);

        /// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of
        /// the function.  This is used for mutable variables etc.
        static llvm::AllocaInst *CreateEntryBlockAlloca(llvm::Function *, llvm::StringRef);

        static std::unique_ptr<llvm::LLVMContext> TheContext;
        static std::unique_ptr<llvm::Module> TheModule;
        static std::unique_ptr<Pudl::PudlJIT> TheJIT;
        static std::unique_ptr<llvm::IRBuilder<>> Builder;
        static std::unique_ptr<llvm::legacy::FunctionPassManager> TheFPM;
        static llvm::ExitOnError ExitOnErr;
        static std::map<std::string, std::unique_ptr<Pudl::AST::PrototypeAST>> FunctionProtos;
        static std::map<std::string, llvm::AllocaInst *> NamedValues;

        static Type *getType(const std::string &type);
    };
} // namespace Pudl
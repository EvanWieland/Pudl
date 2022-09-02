// https://llvm.org/docs/tutorial/BuildingAJIT2.html

#pragma once

#include "llvm/ADT/StringRef.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/ExecutionEngine/Orc/ExecutorProcessControl.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/IRTransformLayer.h"
#include "llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include <memory>

using namespace llvm;
using namespace llvm::orc;

namespace Pudl {
    class PudlJIT {
    private:
        std::unique_ptr<ExecutionSession> ES;

        DataLayout DL;
        MangleAndInterner Mangle;

        RTDyldObjectLinkingLayer ObjectLayer;
        IRCompileLayer CompileLayer;
        IRTransformLayer OptimizeLayer;

        JITDylib &MainJD;

    public:
        PudlJIT(std::unique_ptr<ExecutionSession> ES,
                JITTargetMachineBuilder JTMB, DataLayout DL)
                : ES(std::move(ES)), DL(std::move(DL)), Mangle(*this->ES, this->DL),
                  ObjectLayer(*this->ES,
                              []() { return std::make_unique<SectionMemoryManager>(); }),
                  CompileLayer(*this->ES, ObjectLayer,
                               std::make_unique<ConcurrentIRCompiler>(std::move(JTMB))),
                  OptimizeLayer(*this->ES, CompileLayer, optimizeModule),
                  MainJD(this->ES->createBareJITDylib("<main>")) {
            MainJD.addGenerator(
                    cantFail(DynamicLibrarySearchGenerator::GetForCurrentProcess(
                            DL.getGlobalPrefix())));
        }

        ~PudlJIT() {
            if (auto Err = ES->endSession())
                ES->reportError(std::move(Err));
        }

        static Expected <std::unique_ptr<PudlJIT>> Create() {
            auto EPC = SelfExecutorProcessControl::Create();
            if (!EPC)
                return EPC.takeError();

            auto ES = std::make_unique<ExecutionSession>(std::move(*EPC));

            JITTargetMachineBuilder JTMB(
                    ES->getExecutorProcessControl().getTargetTriple());

            auto DL = JTMB.getDefaultDataLayoutForTarget();
            if (!DL)
                return DL.takeError();

            return std::make_unique<PudlJIT>(std::move(ES), std::move(JTMB),
                                             std::move(*DL));
        }

        const DataLayout &getDataLayout() const { return DL; }

        JITDylib &getMainJITDylib() { return MainJD; }

        Error addModule(ThreadSafeModule TSM, ResourceTrackerSP RT = nullptr) {
            if (!RT)
                RT = MainJD.getDefaultResourceTracker();

            return OptimizeLayer.add(RT, std::move(TSM));
        }

        Expected <JITEvaluatedSymbol> lookup(StringRef Name) {
            return ES->lookup({&MainJD}, Mangle(Name.str()));
        }

        void registerSymbols(
                llvm::function_ref<llvm::orc::SymbolMap(llvm::orc::MangleAndInterner)> symbolMap) {
            llvm::cantFail(getMainJITDylib().define(
                    absoluteSymbols(symbolMap(llvm::orc::MangleAndInterner(
                            getMainJITDylib().getExecutionSession(), getDataLayout())))));
        }

    private:
        static Expected <ThreadSafeModule>
        optimizeModule(ThreadSafeModule TSM, const MaterializationResponsibility &R) {
            TSM.withModuleDo([](Module &M) {
                // Create a function pass manager.
                auto FPM = std::make_unique<legacy::FunctionPassManager>(&M);

                // Add some optimizations.
                FPM->add(createInstructionCombiningPass());
                FPM->add(createReassociatePass());
                FPM->add(createGVNPass());
                FPM->add(createCFGSimplificationPass());
                FPM->doInitialization();

                // Run the optimizations over all functions in the module being added to
                // the JIT.
                for (auto &F: M)
                    FPM->run(F);
            });

            return std::move(TSM);
        }
    };

} // end namespace Pudl
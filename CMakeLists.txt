cmake_minimum_required(VERSION 3.18)
project(pudl)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC src)

set(TARGET_WebAssembly WebAssemblyCodeGen WebAssemblyAsmParser WebAssemblyDesc WebAssemblyInfo)
set(TARGET_XCore XCoreCodeGen XCoreDesc XCoreInfo)
set(TARGET_SystemZ SystemZCodeGen SystemZAsmParser SystemZDesc SystemZInfo)
set(TARGET_Sparc SparcCodeGen SparcAsmParser SparcDesc SparcInfo)
set(TARGET_RISCV RISCVCodeGen RISCVAsmParser RISCVDesc RISCVInfo)
set(TARGET_PowerPC PowerPCCodeGen PowerPCAsmParser PowerPCDesc PowerPCInfo)
set(TARGET_NVPTX NVPTXCodeGen NVPTXDesc NVPTXInfo)
set(TARGET_MSP430 MSP430CodeGen MSP430AsmParser MSP430Desc MSP430Info)
set(TARGET_Mips MipsCodeGen MipsAsmParser MipsDesc MipsInfo)
set(TARGET_Lanai LanaiCodeGen LanaiAsmParser LanaiDesc LanaiInfo)
set(TARGET_Hexagon HexagonCodeGen HexagonAsmParser HexagonDesc HexagonInfo)
set(TARGET_BPF BPFCodeGen BPFAsmParser BPFDesc BPFInfo)
set(TARGET_ARM ARMCodeGen ARMAsmParser ARMDesc ARMUtils ARMInfo)
set(TARGET_AMDGPU AMDGPUCodeGen AMDGPUAsmParser AMDGPUDesc AMDGPUUtils AMDGPUInfo)
set(TARGET_X86 X86CodeGen X86AsmParser X86Desc X86Info)
set(TARGET_AArch64 AArch64CodeGen AArch64AsmParser AArch64Desc AArch64Utils AArch64Info)
set(TARGET_AVR AVRCodeGen AVRAsmParser AVRDesc AVRInfo)
set(TARGET_VE VECodeGen VEAsmParser VEDesc VEInfo)
set(TARGET_M68k M68kCodeGen M68kAsmParser M68kDesc M68kInfo)
set(TARGET_LoongArch LoongArchCodeGen LoongArchAsmParser LoongArchDesc LoongArchInfo)
set(TARGET_Xtensa XtensaCodeGen XtensaAsmParser XtensaDesc XtensaInfo)

# NOTE: Codegen.h's compile() and main.cpp both call the InitializeAllTargets*()
# family of functions, which reference every target LLVM knows about (per
# llvm/Config/Targets.def) regardless of which ones Pudl actually needs. This
# list must therefore include every target LLVM ships, or linking fails with
# "undefined reference to LLVMInitialize<Target>*" for whichever target is
# missing. Newer LLVM releases add targets here periodically (LoongArch and
# Xtensa were added after this list was first written) -- if a future LLVM
# upgrade adds another one, either add it here or switch away from
# InitializeAllTargets*() to InitializeNativeTarget*() (Pudl only ever
# targets the host machine; see the project hardening plan, Phase 3).
set(TARGETS_TO_BUILD "WebAssembly" "XCore" "SystemZ" "Sparc" "RISCV" "PowerPC" "NVPTX" "MSP430" "Mips" "Lanai" "Hexagon" "BPF" "ARM" "AMDGPU" "X86" "AArch64" "AVR" "VE" "M68k" "LoongArch" "Xtensa")

set(LLVM_TARGETS)
foreach (target IN ITEMS ${TARGETS_TO_BUILD})
    list(APPEND LLVM_TARGETS "${TARGET_${target}}")
endforeach (target)

llvm_map_components_to_libnames(
        llvm_libs
        TextAPI
        OrcJIT
        JITLink
        ObjectYAML
        WindowsManifest
        Coverage
        TableGen
        LTO
        Passes
        ObjCARCOpts
        Coroutines
        LibDriver
        XRay
        ${LLVM_TARGETS}
        MIRParser
        ipo
        Instrumentation
        Vectorize
        Linker
        IRReader
        AsmParser
        Symbolize
        DebugInfoPDB
        FuzzMutate
        LineEditor
        MCA
        DebugInfoGSYM
        GlobalISel
        SelectionDAG
        AsmPrinter
        DebugInfoDWARF
        MCJIT
        Interpreter
        ExecutionEngine
        RuntimeDyld
        CodeGen
        Target
        ScalarOpts
        InstCombine
        AggressiveInstCombine
        TransformUtils
        BitWriter
        Analysis
        ProfileData
        DlltoolDriver
        Option
        Object
        MCParser
        MC
        DebugInfoCodeView
        DebugInfoMSF
        BitReader
        Core
        Remarks
        BinaryFormat
        BitstreamReader
        Support
        Demangle
)

target_link_libraries(${PROJECT_NAME} ${llvm_libs})

enable_testing()

if (WIN32)
    add_test(
            NAME golden_tests
            COMMAND powershell -NoProfile -ExecutionPolicy Bypass
                    -File "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_golden_tests.ps1"
                    -Bin "$<TARGET_FILE:${PROJECT_NAME}>"
    )
    add_test(
            NAME golden_ir_tests
            COMMAND powershell -NoProfile -ExecutionPolicy Bypass
                    -File "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_golden_tests.ps1"
                    -Bin "$<TARGET_FILE:${PROJECT_NAME}>" -Ir
    )
else ()
    add_test(
            NAME golden_tests
            COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_golden_tests.sh"
                    "$<TARGET_FILE:${PROJECT_NAME}>"
    )
    add_test(
            NAME golden_ir_tests
            COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_golden_tests.sh"
                    "$<TARGET_FILE:${PROJECT_NAME}>" --ir
    )
endif ()
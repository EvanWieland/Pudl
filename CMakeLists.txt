cmake_minimum_required(VERSION 3.24)
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

set(TARGETS_TO_BUILD "WebAssembly" "XCore" "SystemZ" "Sparc" "RISCV" "PowerPC" "NVPTX" "MSP430" "Mips" "Lanai" "Hexagon" "BPF" "ARM" "AMDGPU" "X86" "AArch64" "AVR")

set(LLVM_TARGETS)
foreach (target IN ITEMS ${TARGETS_TO_BUILD})
    list(APPEND LLVM_TARGETS "${TARGET_${target}}")
endforeach(target)

llvm_map_components_to_libnames(llvm_libs
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
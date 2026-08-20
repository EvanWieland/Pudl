cmake_minimum_required(VERSION 3.18)
project(pudl)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Everything except main.cpp (the lexer, parser, AST, codegen, and
# compiler/linker machinery) lives in pudl_core so it can be linked
# independently of the CLI -- e.g. by unit tests, or a future tool that
# wants to embed the compiler without pudl.exe's argv/--help handling
# (which stays in main.cpp/Compiler/CLIManager.h, a header with no .cpp of
# its own, so it's naturally excluded here already).
file(GLOB_RECURSE CORE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
list(REMOVE_ITEM CORE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

add_library(pudl_core STATIC ${CORE_SOURCES})
target_include_directories(pudl_core PUBLIC src)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PUDL_VERSION LIMIT_COUNT 1)
configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Version.h.in"
        "${CMAKE_CURRENT_BINARY_DIR}/generated/Version.h"
        @ONLY
)

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE pudl_core)
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")

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

# Pudl only ever emits code for the host machine (Codegen.h's compile() uses
# InitializeNativeTarget*(), not InitializeAllTargets*() -- there is no
# cross-compilation flag anywhere in the CLI/AST), so only the target
# families a Pudl developer might actually be building/running on need to be
# linked. Deliberately NOT "all of them": different LLVM distributions ship
# different subsets of the long tail of (particularly experimental) backend
# targets -- e.g. the official LLVM Windows release omits M68k, which some
# Linux distro packages include -- so linking every target LLVM's headers
# declare is fragile across platforms for no benefit here.
set(TARGETS_TO_BUILD "X86" "AArch64")

set(LLVM_TARGETS)
foreach (target IN ITEMS ${TARGETS_TO_BUILD})
    list(APPEND LLVM_TARGETS "${TARGET_${target}}")
endforeach (target)

# Curated by build-and-iterate against every #include <llvm/...> actually
# used in src/ (grep for them if extending this): the original list here
# was copy-pasted broad rather than curated, and pulled in several
# libraries with real downstream cost -- WindowsManifest and DebugInfoPDB
# in particular were the direct cause of a Windows-CI-only link failure
# needing the MS DIA SDK, for functionality (Windows manifest embedding,
# PDB debug-info parsing) Pudl never touches. llvm_map_components_to_libnames
# resolves each of these components' own transitive dependencies
# automatically, so only directly-used components need listing here.
llvm_map_components_to_libnames(
        llvm_libs
        Core
        Support
        TargetParser
        MC
        Target
        ${LLVM_TARGETS}
        CodeGen
        AsmPrinter
        Analysis
        TransformUtils
        InstCombine
        ScalarOpts
        Passes
)

target_link_libraries(pudl_core PUBLIC ${llvm_libs})

# Off by default: needs clang (libFuzzer isn't available for GCC/MSVC).
# Compiles its own copy of CORE_SOURCES directly into the fuzz executable
# rather than linking pudl_core, and puts every -fsanitize=fuzzer,... flag
# on that one target via target_compile_options/target_link_options rather
# than CMAKE_CXX_FLAGS -- global flags would also reach CMake's own
# internal "does this compiler work" test program, which then fails to
# link (libFuzzer's runtime provides its own main(), colliding with that
# test program's). Scoping to the target avoids that entirely, and means
# a plain build of `pudl` never needs to know this option exists.
option(PUDL_ENABLE_FUZZING "Build the libFuzzer harness in fuzz/ (needs clang)" OFF)
if (PUDL_ENABLE_FUZZING)
    add_executable(pudl_fuzz_pipeline fuzz/fuzz_pipeline.cpp ${CORE_SOURCES})
    target_include_directories(pudl_fuzz_pipeline PRIVATE src)
    target_link_libraries(pudl_fuzz_pipeline PRIVATE ${llvm_libs})
    target_compile_options(pudl_fuzz_pipeline PRIVATE
            -fsanitize=fuzzer,address,undefined -fno-sanitize-recover=all -g)
    target_link_options(pudl_fuzz_pipeline PRIVATE
            -fsanitize=fuzzer,address,undefined)
endif ()

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
    add_test(
            NAME no_shell_injection_test
            COMMAND powershell -NoProfile -ExecutionPolicy Bypass
                    -File "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_no_shell_injection.ps1"
                    -Bin "$<TARGET_FILE:${PROJECT_NAME}>"
    )
    add_test(
            NAME parser_error_recovery_test
            COMMAND powershell -NoProfile -ExecutionPolicy Bypass
                    -File "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_parser_error_recovery.ps1"
                    -Bin "$<TARGET_FILE:${PROJECT_NAME}>"
    )
    add_test(
            NAME compile_and_link_test
            COMMAND powershell -NoProfile -ExecutionPolicy Bypass
                    -File "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_compile_and_link.ps1"
                    -Bin "$<TARGET_FILE:${PROJECT_NAME}>"
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
    add_test(
            NAME no_shell_injection_test
            COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_no_shell_injection.sh"
                    "$<TARGET_FILE:${PROJECT_NAME}>"
    )
    add_test(
            NAME parser_error_recovery_test
            COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_parser_error_recovery.sh"
                    "$<TARGET_FILE:${PROJECT_NAME}>"
    )
    add_test(
            NAME compile_and_link_test
            COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_compile_and_link.sh"
                    "$<TARGET_FILE:${PROJECT_NAME}>"
    )
endif ()
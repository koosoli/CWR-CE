set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_TRY_COMPILE_CONFIGURATION Release)

# Every TU is compiled with ASan + SanitizerCoverage, but only the fuzz exe links
# the libFuzzer/sancov/ASan runtime (-fsanitize=fuzzer, per target). CMake's own
# compiler-ABI probe would otherwise link a tiny instrumented exe with no runtime
# and fail on undefined __asan_*/__sanitizer_cov_* symbols. Build the probes as
# static libraries so there is no link step to satisfy.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Find LLVM compilers and the sanitizer import-lib directory dynamically
include(${CMAKE_CURRENT_LIST_DIR}/../FindLLVMSanitizer.cmake)
find_llvm_compilers("x64")
find_asan_import_lib(LLVM_LIB_DIR "x64")

if(NOT LLVM_LIB_DIR)
    message(FATAL_ERROR "LLVM sanitizer libraries not found for x64")
endif()

# Coverage-guided fuzzing build: AddressSanitizer + SanitizerCoverage on every
# translation unit (fuzzer-no-link instruments without pulling libFuzzer's main).
# ASan is fatal so a memory-safety hit aborts straight into a libFuzzer crash
# artifact. UBSan is intentionally omitted on this lane to keep the signal clean
# (OOB read/write, heap overflow, use-after-free) without benign-UB aborts; a
# UBSan lane can be added separately.
# _DISABLE_*_ANNOTATION turns off the MSVC-STL ASan container annotations for
# std::string / std::vector. Without this, our instrumented objects emit a
# /failifmismatch 'annotate_string=1' record that refuses to link against the
# prebuilt, non-instrumented vcpkg deps (spdlog, etc.). The network decoder uses
# Poseidon's own AutoArray/RString, not std containers, so disabling these annotations
# costs nothing — heap-buffer redzones on malloc'd buffers (where the decoder bugs
# live) are core ASan and unaffected.
set(_fuzz_flags "-m64 -fsanitize=address,fuzzer-no-link -fno-sanitize-recover=address -g -gline-tables-only -O1 -D_DISABLE_STRING_ANNOTATION=1 -D_DISABLE_VECTOR_ANNOTATION=1")
set(CMAKE_C_FLAGS_INIT   "${_fuzz_flags}")
set(CMAKE_CXX_FLAGS_INIT "${_fuzz_flags}")

# The fuzz executable links the full libFuzzer + ASan runtime via -fsanitize=fuzzer
# at the target level (apps/fuzzers/Fuzzer); the toolchain only needs the import-lib path.
set(CMAKE_EXE_LINKER_FLAGS_INIT    "/machine:x64 /LIBPATH:\"${LLVM_LIB_DIR}\" /DEBUG")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "/machine:x64")
set(CMAKE_STATIC_LINKER_FLAGS_INIT "/machine:x64")

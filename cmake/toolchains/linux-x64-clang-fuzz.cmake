set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

# Native build - prevent CMake from treating this as cross-compilation.
set(CMAKE_CROSSCOMPILING FALSE)

# Every TU is compiled with ASan + SanitizerCoverage (fuzzer-no-link instruments
# without pulling libFuzzer's main); only the fuzz exe links the full driver via
# -fsanitize=fuzzer,address (apps/fuzzers/Fuzzer). CMake's own compiler-ABI probe would
# otherwise link a tiny instrumented exe with no driver and fail on undefined
# __sanitizer_cov_* symbols. Build the probes as static libraries so there is no
# link step to satisfy.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Coverage-guided fuzzing: AddressSanitizer + UndefinedBehaviorSanitizer +
# SanitizerCoverage on every translation unit. Both are fatal (-fno-sanitize-
# recover) so a memory-safety or UB hit aborts straight into a libFuzzer crash
# artifact — UBSan turns every campaign into an integer-overflow / bad-shift /
# enum-range hunter over the same parse/load paths the harnesses already drive.
# The known-benign UB sites (FSM function-type casts etc.) already carry
# no_sanitize annotations from the ASan/UBSan sweep, so the signal stays clean.
# Frame pointers + line tables give fast, well-symbolized stacks. NDEBUG (from
# the Release build type) strips PoseidonAssert / AutoArray bounds checks so OOB
# reaches the ASan redzone instead of an assert — the shipping server.
set(_fuzz_flags "-m64 -fsanitize=address,undefined,fuzzer-no-link -fno-sanitize-recover=address,undefined -fno-omit-frame-pointer -g -gline-tables-only")
set(CMAKE_C_FLAGS_INIT   "${_fuzz_flags}")
set(CMAKE_CXX_FLAGS_INIT "${_fuzz_flags}")

# --as-needed: drop libraries pulled in transitively (e.g. mimalloc's -latomic)
# with no referenced symbols. Without this the ASan runtime rejects libatomic.so
# via AsanCheckIncompatibleRT() before main() runs. CACHE ... FORCE so this
# overrides a stale cached value on reconfigure.
set(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed" CACHE STRING "" FORCE)

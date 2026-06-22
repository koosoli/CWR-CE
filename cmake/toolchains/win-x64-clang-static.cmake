set(CMAKE_SYSTEM_NAME Windows)

# Find LLVM compilers dynamically
include(${CMAKE_CURRENT_LIST_DIR}/../FindLLVMSanitizer.cmake)
find_llvm_compilers("x64")

# 64 bit
set(CMAKE_C_FLAGS_INIT   "-m64")
set(CMAKE_CXX_FLAGS_INIT "-m64")
set(CMAKE_EXE_LINKER_FLAGS_INIT    "/machine:x64")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "/machine:x64")
set(CMAKE_STATIC_LINKER_FLAGS_INIT "/machine:x64")

# Force the static CRT (/MT, /MTd) for dependency builds. clang-cl leaves
# unsanitized code at /MD by default, which trips /failifmismatch RuntimeLibrary
# against clang's /MT libFuzzer runtime when the fuzz harness links them. Forcing
# CMP0091 NEW makes CMAKE_MSVC_RUNTIME_LIBRARY effective even for ports whose
# CMakeLists predate the policy.
set(CMAKE_POLICY_DEFAULT_CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Belt-and-suspenders for ports that ignore CMP0091 (cmake_minimum_required <
# 3.15, e.g. openal-soft 3.13 / enkiTS 3.5): rewrite the /MD baked into their
# per-config flags to /MT after the platform defaults are computed.
set(CMAKE_USER_MAKE_RULES_OVERRIDE
    "${CMAKE_CURRENT_LIST_DIR}/static-crt-flags-override.cmake"
)

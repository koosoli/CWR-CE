set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_TRY_COMPILE_CONFIGURATION Release)

# Setting CMAKE_SYSTEM_NAME puts CMake in cross-compile mode, where the Ninja
# generator no longer auto-discovers ninja from PATH on a fresh configure. vcpkg
# passes CMAKE_MAKE_PROGRAM for dep builds; supply it for the main project too.
if(NOT CMAKE_MAKE_PROGRAM)
    # NO_CMAKE_FIND_ROOT_PATH: in cross-compile mode find_program searches the
    # target root by default; ninja is a host tool, so search the host PATH.
    find_program(NINJA_EXECUTABLE ninja NO_CMAKE_FIND_ROOT_PATH)
    if(NINJA_EXECUTABLE)
        set(CMAKE_MAKE_PROGRAM "${NINJA_EXECUTABLE}" CACHE FILEPATH "ninja" FORCE)
    endif()
endif()

# Find LLVM compilers and lib directory dynamically
include(${CMAKE_CURRENT_LIST_DIR}/../FindLLVMSanitizer.cmake)
find_llvm_compilers("x64")
find_asan_import_lib(LLVM_LIB_DIR "x64")

if(NOT LLVM_LIB_DIR)
    message(FATAL_ERROR "LLVM sanitizer libraries not found for x64")
endif()

# 64 bit with AddressSanitizer + UBSan and debug symbols for symbolized output.
# _DISABLE_{STRING,VECTOR}_ANNOTATIONS: MSVC STL annotates string/vector buffers
# for ASan and embeds a /failifmismatch guard, so a value of 1 here can't link
# against a non-ASan dep (value 0). Disabling them keeps the link compatible with
# non-instrumented deps (the engine-only ASan build) and only forgoes
# container-spare-capacity overflow checks — raw heap UAF/overflow detection,
# which is what matters here, is unaffected.
set(SAN_C_CXX_FLAGS "-m64 -fsanitize=address,undefined -fsanitize-recover=address,undefined -g -gline-tables-only -D_DISABLE_STRING_ANNOTATION -D_DISABLE_VECTOR_ANNOTATION")
set(CMAKE_C_FLAGS_INIT   "${SAN_C_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${SAN_C_CXX_FLAGS}")

# Both EXEs and DLLs built with -fsanitize=address must link the ASan runtime:
# clang ships no separate dll_thunk, so dynamic ASan uses the same runtime libs
# for both. vcpkg deps build as DLLs (VCPKG_LIBRARY_LINKAGE dynamic) — without
# these flags on the SHARED link, an instrumented dep DLL fails to resolve
# __asan_*/__ubsan_* (e.g. cjson: undefined __asan_init).
set(SAN_RUNTIME_LINK_FLAGS "/wholearchive:clang_rt.asan_dynamic_runtime_thunk-x86_64.lib /wholearchive:clang_rt.asan_dynamic-x86_64.lib /LIBPATH:\"${LLVM_LIB_DIR}\" /DEBUG")
set(CMAKE_EXE_LINKER_FLAGS_INIT    "/machine:x64 ${SAN_RUNTIME_LINK_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "/machine:x64 ${SAN_RUNTIME_LINK_FLAGS}")
set(CMAKE_STATIC_LINKER_FLAGS_INIT "/machine:x64")

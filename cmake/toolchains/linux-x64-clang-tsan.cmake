set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

# Native build - prevent CMake from treating this as cross-compilation
set(CMAKE_CROSSCOMPILING FALSE)

set(CMAKE_C_FLAGS_INIT "-m64")
set(CMAKE_CXX_FLAGS_INIT "-m64")

set(SANITIZER_FLAGS "-fsanitize=thread -fno-omit-frame-pointer -g")

set(CMAKE_C_FLAGS_DEBUG_INIT "${SANITIZER_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG_INIT "${SANITIZER_FLAGS}")

set(CMAKE_EXE_LINKER_FLAGS_INIT "${SANITIZER_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${SANITIZER_FLAGS}")

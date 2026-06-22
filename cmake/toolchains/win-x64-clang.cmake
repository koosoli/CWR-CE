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

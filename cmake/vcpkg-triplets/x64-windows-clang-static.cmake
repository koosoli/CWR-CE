set(VCPKG_TARGET_ARCHITECTURE x64)
# Static CRT (/MT) so the dependency objects match clang's libFuzzer runtime
# (clang_rt.fuzzer-x86_64.lib is built /MT); a /MD dep would trip
# /failifmismatch RuntimeLibrary against it. Used only by the win-x64-clang-fuzz
# preset; the normal presets keep the dynamic-CRT x64-windows-clang triplet.
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)
# Everything static here, including openal-soft. The normal triplet keeps openal
# dynamic for LGPL replaceability; this triplet only builds the local, never-
# distributed fuzz harness, and a dynamic DLL with a static CRT is an unsupported
# vcpkg combo, so openal links statically like the rest.

# Static-CRT clang toolchain (forces /MT via CMP0091) so dep objects match the
# /MT libFuzzer runtime.
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE
    "${CMAKE_CURRENT_LIST_DIR}/../toolchains/win-x64-clang-static.cmake"
)

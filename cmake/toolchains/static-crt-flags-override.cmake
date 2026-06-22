# Included via CMAKE_USER_MAKE_RULES_OVERRIDE after the platform module computes
# the default CMAKE_<LANG>_FLAGS_<CONFIG>_INIT, so it can rewrite them.
#
# Dependencies whose CMakeLists declare cmake_minimum_required below 3.15 select
# the CMP0091 OLD codepath, which bakes /MD straight into the per-config flags
# regardless of CMAKE_MSVC_RUNTIME_LIBRARY (openal-soft, enkiTS). A /MD object
# then trips lld-link's /failifmismatch:RuntimeLibrary against clang's /MT
# libFuzzer runtime. Rewrite /MD(d) -> /MT(d) here, after the defaults are set,
# so every dependency object links against the same static CRT.
foreach(lang C CXX)
    foreach(cfg "" _DEBUG _RELEASE _MINSIZEREL _RELWITHDEBINFO)
        set(_var CMAKE_${lang}_FLAGS${cfg}_INIT)
        if(DEFINED ${_var})
            string(REGEX REPLACE "/MDd" "/MTd" ${_var} "${${_var}}")
            string(REGEX REPLACE "/MD" "/MT" ${_var} "${${_var}}")
        endif()
    endforeach()
endforeach()

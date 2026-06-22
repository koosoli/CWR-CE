include(Catch)

set(
    _CATCH_DISCOVER_TESTS_SCRIPT
    "${CMAKE_SOURCE_DIR}/cmake/CatchAddWindowsSafeTests.cmake"
    CACHE INTERNAL "OFPR Catch2 discovery with Windows-safe CTest names" FORCE
)

function(ofpr_catch_discover_tests TARGET)
    catch_discover_tests(${TARGET}
        TEST_PREFIX "${TARGET} - "
        DISCOVERY_MODE PRE_TEST
        ${ARGN}
    )
endfunction()

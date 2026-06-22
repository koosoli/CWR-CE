if(NOT DEFINED TRIDENT_TEST_PATH)
    message(FATAL_ERROR "TRIDENT_TEST_PATH is required")
endif()
if(NOT DEFINED TRIDENT_GAME_DIR)
    message(FATAL_ERROR "TRIDENT_GAME_DIR is required")
endif()
if(NOT DEFINED TRIDENT_DATA_DIR)
    message(FATAL_ERROR "TRIDENT_DATA_DIR is required")
endif()
if(NOT DEFINED TRIDENT_EXECUTABLES)
    message(FATAL_ERROR "TRIDENT_EXECUTABLES is required")
endif()

set(_tri "")
foreach(_candidate IN LISTS TRIDENT_EXECUTABLES)
    if(EXISTS "${_candidate}")
        set(_tri "${_candidate}")
        break()
    endif()
endforeach()

if(NOT _tri)
    string(REPLACE ";" "\n  " _candidates "${TRIDENT_EXECUTABLES}")
    message(FATAL_ERROR "tri executable not found. Checked:\n  ${_candidates}")
endif()

execute_process(
    COMMAND
        "${_tri}" test "${TRIDENT_TEST_PATH}"
        -j3
        --game-dir "${TRIDENT_GAME_DIR}"
        --data-dir "${TRIDENT_DATA_DIR}"
    RESULT_VARIABLE _result
)

if(NOT _result EQUAL 0)
    message(FATAL_ERROR "Trident integration CTest failed: ${TRIDENT_TEST_PATH}")
endif()

set(EVALUATOR_SOURCES
    Cli/main.cpp
)

if(WIN32)
    list(APPEND EVALUATOR_SOURCES ${CMAKE_SOURCE_DIR}/resources/icon-tool.rc)
endif()

add_executable(PoseidonEvaluator ${EVALUATOR_SOURCES})

find_package(CLI11 CONFIG REQUIRED)

target_link_libraries(PoseidonEvaluator PRIVATE
    Poseidon
    CLI11::CLI11
)

target_include_directories(PoseidonEvaluator PRIVATE
    ${CMAKE_SOURCE_DIR}/engine
)

set_target_properties(PoseidonEvaluator PROPERTIES
    OUTPUT_NAME "PoseidonEvaluator"
)

# GameBase - boot code shared by the CWA game binaries (client + dedicated server)

add_library(GameBase STATIC
    GameBase.cpp
)

target_include_directories(GameBase PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}            # GameBase.hpp for consumers (Game/Server)
    ${CMAKE_SOURCE_DIR}/engine
)

target_compile_definitions(GameBase PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
)

target_link_libraries(GameBase PUBLIC Poseidon)

if(MSVC OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
    target_compile_options(GameBase PRIVATE
        /W3
        /GR
        $<$<CONFIG:Debug>:/Od>
        $<$<CONFIG:Debug>:/MDd>
    )
endif()

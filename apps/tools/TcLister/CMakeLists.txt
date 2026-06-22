cmake_minimum_required(VERSION 3.25)

project(TcLister
    VERSION 1.0.0
    DESCRIPTION "Total Commander OFP lister plugin"
    LANGUAGES CXX
)

set(TCLISTER_SOURCES tc_lister.cpp listplug.h)
if(WIN32)
    list(APPEND TCLISTER_SOURCES tc_lister.def)
endif()

add_library(TcLister MODULE ${TCLISTER_SOURCES})

if(WIN32)
    target_link_libraries(TcLister PRIVATE Poseidon winmm)
else()
    target_link_libraries(TcLister PRIVATE Poseidon dl)
endif()

target_include_directories(TcLister PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(TcLister PRIVATE cxx_std_20)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(WLX_SUFFIX ".wlx64")
else()
    set(WLX_SUFFIX ".wlx")
endif()

set_target_properties(TcLister PROPERTIES
    OUTPUT_NAME "poseidon"
    SUFFIX "${WLX_SUFFIX}"
    PREFIX ""
)

include(${CMAKE_SOURCE_DIR}/cmake/DistCopy.cmake)
dist_copy(TcLister RENAME poseidon${WLX_SUFFIX})
add_custom_command(TARGET TcLister POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/pluginst.inf ${DIST_DIR}/poseidon_pluginst.inf
    VERBATIM
)

cmake_minimum_required(VERSION 3.25)

project(TcPbo
    VERSION 1.0.0
    DESCRIPTION "Total Commander PBO packer plugin"
    LANGUAGES CXX
)

set(TCPBO_SOURCES tc_pbo.cpp wcxhead.h)
if(WIN32)
    list(APPEND TCPBO_SOURCES tc_pbo.def)
endif()

add_library(TcPbo MODULE ${TCPBO_SOURCES})

if(WIN32)
    target_link_libraries(TcPbo PRIVATE Poseidon winmm)
else()
    target_link_libraries(TcPbo PRIVATE Poseidon)
endif()

target_include_directories(TcPbo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(TcPbo PRIVATE cxx_std_20)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(WCX_SUFFIX ".wcx64")
else()
    set(WCX_SUFFIX ".wcx")
endif()

set_target_properties(TcPbo PROPERTIES
    OUTPUT_NAME "pbo"
    SUFFIX "${WCX_SUFFIX}"
    PREFIX ""
)

include(${CMAKE_SOURCE_DIR}/cmake/DistCopy.cmake)
dist_copy(TcPbo RENAME pbo${WCX_SUFFIX} EXTRA pluginst.inf)

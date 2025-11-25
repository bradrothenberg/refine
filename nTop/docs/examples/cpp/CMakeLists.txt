cmake_minimum_required(VERSION 3.24)
 
project(cppexamples)

include(FetchContent)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "$<0:>${CMAKE_BINARY_DIR}/bin") # $<0:> prevents the appending of buildtype to the path

SET(NTOP_CORE_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/ntop_core" CACHE STRING "Path to the nTop Core Module directory.")

FetchContent_Declare(ntop_core
    SOURCE_DIR ${NTOP_CORE_LIB_DIR}
    OVERRIDE_FIND_PACKAGE
)

FetchContent_Declare(matplotplusplus
    GIT_REPOSITORY https://github.com/alandefreitas/matplotplusplus
    GIT_TAG origin/master
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/matplotplusplus
)

FetchContent_GetProperties(matplotplusplus)
if(NOT matplotplusplus_POPULATED)
    FetchContent_Populate(matplotplusplus)
    add_subdirectory(${matplotplusplus_SOURCE_DIR} ${matplotplusplus_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

add_subdirectory(boundingboxApp)
add_subdirectory(sliceApp)
add_subdirectory(sliceLegacyApp)
add_subdirectory(volumeApp)
add_subdirectory(closestpointApp)

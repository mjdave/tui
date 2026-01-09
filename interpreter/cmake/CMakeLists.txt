# Specify the minimum version for CMake

cmake_minimum_required(VERSION 3.5)

# Project's name

project(tui VERSION 0.1)

set(CMAKE_BUILD_TYPE RelWithDebInfo)
set (CMAKE_CXX_STANDARD 17)

set(SOURCES 
    main.cpp

    ../Interpreter.cpp

    ../../source/TuiFileUtils.cpp
    ../../source/TuiFunction.cpp
    ../../source/TuiRef.cpp
    ../../source/TuiSha1.cpp
    ../../source/TuiBuiltInFunctions.cpp
)

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

if ( CMAKE_COMPILER_IS_GNUCC )
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wno-deprecated")
endif ( CMAKE_COMPILER_IS_GNUCC )

include_directories(
    ../
    ../../source
    ../../thirdParty/glm/glm
)

add_executable(tui ${SOURCES})

set (CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl")

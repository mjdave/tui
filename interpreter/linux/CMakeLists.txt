# Specify the minimum version for CMake

cmake_minimum_required(VERSION 3.5)

# Project's name

project(tui VERSION 0.1)

set(CMAKE_BUILD_TYPE RelWithDebInfo)

add_definitions(-DDEV_RESOURCES_DIR=\"./GameResources\")

set(GCC_COVERAGE_LINK_FLAGS    "-no-pie")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS} -static-libgcc -static-libstdc++")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--disable-new-dtags,-rpath='$ORIGIN'")
set(CMAKE_INSTALL_RPATH "$ORIGIN")

set(SOURCES 
    main.cpp

    ../Interpreter.cpp

    ../../source/TuiFileUtils.cpp
    ../../source/TuiFunction.cpp
    ../../source/TuiRef.cpp
    ../../source/TuiSha1.cpp
    ../../source/TuiStringUtils.cpp
)

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set (CMAKE_CXX_STANDARD 11)

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
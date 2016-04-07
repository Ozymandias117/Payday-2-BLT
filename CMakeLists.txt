cmake_minimum_required(VERSION 2.8.0)
project(Payday-2-BLT)

add_subdirectory(src/subhook)

add_definitions(-D_GNU_SOURCE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++11 -fpermissive")

include_directories(src src/util src/subhook)

set(SOURCE_FILES
    src/linux_preload.cpp
    src/console/console.cpp
    src/http/http.cpp
    src/signatures/signatures_linux.cpp
    src/subhook/subhook.c
    src/threading/threadqueue.cpp
    src/util/logging.cpp
    src/util/files_linux.cpp
    src/util/compression.cpp
    src/InitiateState.cpp
    )

add_library(blt_hook SHARED ${SOURCE_FILES})

target_link_libraries(blt_hook subhook dl ssl curl z)

cmake_minimum_required(VERSION 3.20)
project(illumina)
set(CMAKE_CXX_STANDARD 17)

set(ILLUMINA_LIBRARY_SOURCE_DIR ${CMAKE_SOURCE_DIR}/illumina)
set(ILLUMINA_CLI_SOURCE_DIR ${CMAKE_SOURCE_DIR}/cli)
set(ILLUMINA_TESTS_DIR ${CMAKE_SOURCE_DIR}/tests)
set(ILLUMINA_EXT_DIR ${CMAKE_SOURCE_DIR}/ext)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # GCC requires some flags in order for threads and <filesystem> to work.
    set(CMAKE_CXX_FLAGS "-mavx2 -mbmi2 -pthread -static-libstdc++ -static-libgcc --static")
endif()

include_directories(ext/include)

enable_testing()

add_subdirectory(ext)

add_subdirectory(illumina)
add_subdirectory(cli)
add_subdirectory(tests)
add_subdirectory(datagen)


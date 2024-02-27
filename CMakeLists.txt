cmake_minimum_required(VERSION 3.24)
project(boa)
set(CMAKE_CXX_STANDARD 17)

set(BOA_LIBRARY_SOURCE_DIR ${CMAKE_SOURCE_DIR}/boa)
set(BOA_CLI_SOURCE_DIR ${CMAKE_SOURCE_DIR}/cli)
set(BOA_TESTS_DIR ${CMAKE_SOURCE_DIR}/tests)

add_subdirectory(boa)
add_subdirectory(cli)


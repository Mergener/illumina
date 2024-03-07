cmake_minimum_required(VERSION 3.24)
project(ilumina)
set(CMAKE_CXX_STANDARD 17)

set(ILUMINA_LIBRARY_SOURCE_DIR ${CMAKE_SOURCE_DIR}/ilumina)
set(ILUMINA_CLI_SOURCE_DIR ${CMAKE_SOURCE_DIR}/cli)
set(ILUMINA_TESTS_DIR ${CMAKE_SOURCE_DIR}/tests)

include_directories(ext/include)

enable_testing()

add_subdirectory(ilumina)
add_subdirectory(cli)
add_subdirectory(tests)


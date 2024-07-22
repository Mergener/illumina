cmake_minimum_required(VERSION 3.20)
project(illumina)
set(CMAKE_CXX_STANDARD 17)

# Setup important paths.
set(ILLUMINA_LIBRARY_SOURCE_DIR ${CMAKE_SOURCE_DIR}/illumina)
set(ILLUMINA_CLI_SOURCE_DIR ${CMAKE_SOURCE_DIR}/cli)
set(ILLUMINA_TESTS_DIR ${CMAKE_SOURCE_DIR}/tests)
set(ILLUMINA_EXT_DIR ${CMAKE_SOURCE_DIR}/ext)
set(NNUE_PATH ${CMAKE_SOURCE_DIR}/illumina/net/net.json)

# For GCC-derived compilers, we need to specify some flags in order
# to have <thread> and <filesystem> properly working.
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # GCC requires some flags in order for threads and <filesystem> to work.
    set(CMAKE_CXX_FLAGS "-pthread -static-libstdc++ -static-libgcc --static")
endif()

# Define our supported architectures here.
set(ARCHS bmi2 avx2 base)

# General setup.
enable_testing()
include_directories(${CMAKE_SOURCE_DIR}/ext/include)

# Define architecture specific options here.
function(apply_arch_options TARGET ARCH)
    if (${ARCH} STREQUAL bmi2)
        if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            target_compile_options(${TARGET} PRIVATE -mbmi2)
            target_compile_definitions(${TARGET} PRIVATE USE_PEXT)
        endif()
        apply_arch_options(${TARGET} avx2)
    elseif(${ARCH} STREQUAL avx2)
        if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            target_compile_options(${TARGET} PRIVATE -mavx2)
        endif()
    elseif(${ARCH} STREQUAL base)
        # Do nothing
    else()
        message("Unsupported architecture ${ARCH}")
    endif()
endfunction()

# Add external dependencies.
add_subdirectory(ext)

# Add Illumina's modules.
add_subdirectory(illumina)
add_subdirectory(cli)
add_subdirectory(tests)


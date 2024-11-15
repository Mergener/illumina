cmake_minimum_required(VERSION 3.20)
project(illumina)
set(CMAKE_CXX_STANDARD 17)

# Set illumina version.
set(ILLUMINA_VERSION_MAJOR 1)
set(ILLUMINA_VERSION_MINOR 1)

# Setup important paths.
set(ILLUMINA_LIBRARY_SOURCE_DIR ${CMAKE_SOURCE_DIR}/illumina)
set(ILLUMINA_CLI_SOURCE_DIR ${CMAKE_SOURCE_DIR}/cli)
set(ILLUMINA_TESTS_DIR ${CMAKE_SOURCE_DIR}/tests)
set(ILLUMINA_EXT_DIR ${CMAKE_SOURCE_DIR}/ext)

# We might want to use a custom net.
if(NOT DEFINED NNUE_PATH)
    set(NNUE_PATH ${CMAKE_SOURCE_DIR}/illumina/net/net.json)
endif()
message(STATUS "NNUE_PATH is set to: ${NNUE_PATH}")

# Add some compilation options.
option(TUNING "Tunable values become runtime variables that can be changed via UCI options." OFF)
if (TUNING)
    add_definitions(-DTUNING_BUILD)
endif()

option(DEVELOPMENT "Signals that this is a development build." ON)
if (DEVELOPMENT)
    add_definitions(-DDEVELOPMENT_BUILD)

    # Try to get the Git commit hash, but set a default value as a fallback.
    find_package(Git QUIET)
    if (Git_FOUND)
        execute_process(
                COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                OUTPUT_VARIABLE GIT_COMMIT_HASH
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_VARIABLE GIT_ERROR
                RESULT_VARIABLE GIT_RESULT
        )
        if (GIT_RESULT EQUAL 0)
            set(COMMIT_HASH "${GIT_COMMIT_HASH}")
        else()
            message(WARNING "Could not get commit hash: ${GIT_ERROR}.")
            set(COMMIT_HASH null_commit)
        endif()
    else()
        message(WARNING "Git not found.")
        set(COMMIT_HASH null_commit)
    endif()

endif()

option(INCLUDE_TRACING_MODULE
            "Signals that this will include the tracing module within CLI.
             Note that this includes the heavy SQLITE3 dependency.
             Not supposed to be used in regular Illumina builds."
        OFF)
if (INCLUDE_TRACING_MODULE)
    message("Tracing module included.")
    add_definitions(-DTRACING_ENABLED)
endif()

option(OPENBENCH_COMPLIANCE_MODE
        "Makes this build openbench compliant, which implies applying any necessary changes
         for this compliance to work -- including a simplified version of the bench command."
       OFF)
if (OPENBENCH_COMPLIANCE_MODE)
    message("OpenBench compliance mode enabled.")
    add_definitions(-DOPENBENCH_COMPLIANCE)
endif()

# For GCC-derived compilers, we need to specify some flags in order
# to have <thread> and <filesystem> properly working.
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # GCC requires some flags in order for threads and <filesystem> to work.
    set(CMAKE_CXX_FLAGS "-pthread -static-libstdc++ -static-libgcc --static")
# We don't want Clang to dynamically link the standard library.
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_EXE_LINKER_FLAGS "-static-libstdc++ -static-libgcc ${CMAKE_EXE_LINKER_FLAGS}")
endif()

# Define our supported architectures here.
set(ARCHS bmi2 avx2 base)

# General setup.
enable_testing()
include_directories(${CMAKE_SOURCE_DIR}/ext/include)

# Define architecture specific options here.
function(apply_arch_options TARGET ARCH)
    if (${ARCH} STREQUAL bmi2)
        if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            target_compile_options(${TARGET} PRIVATE -mbmi2)
            target_compile_definitions(${TARGET} PRIVATE HAS_PEXT)
        endif()
        apply_arch_options(${TARGET} avx2)
    elseif(${ARCH} STREQUAL avx2)
        if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            target_compile_options(${TARGET} PRIVATE -mavx2)
            target_compile_definitions(${TARGET} PRIVATE HAS_AVX2)
        endif()
    elseif(${ARCH} STREQUAL base)
        # Do nothing
    else()
        message("Unsupported architecture ${ARCH}")
    endif()
endfunction()

# Define Illumina's version numbers/names.

if (DEVELOPMENT)
    set(ILLUMINA_VERSION_NAME dev-${COMMIT_HASH})
else()
    set(ILLUMINA_VERSION_NAME ${ILLUMINA_VERSION_MAJOR}.${ILLUMINA_VERSION_MINOR})
endif()

message("Generating build files for version ${ILLUMINA_VERSION_NAME} on configuration ${CMAKE_BUILD_TYPE}.")

add_definitions(-DILLUMINA_VERSION_MAJOR=${ILLUMINA_VERSION_MAJOR})
add_definitions(-DILLUMINA_VERSION_MINOR=${ILLUMINA_VERSION_MINOR})
add_definitions(-DILLUMINA_VERSION_NAME=\"${ILLUMINA_VERSION_NAME}\")

# Check for available modules
set(HAS_LITETEST false)
set(HAS_SQLITE   false)

# Add external dependencies.
add_subdirectory(ext)

# Add Illumina's modules.
add_subdirectory(illumina)
add_subdirectory(cli)

if (HAS_LITETEST)
    add_subdirectory(tests)
else()
    message("Litetest submodule not found; tests won't be compiled")
endif()

if (INCLUDE_TRACING_MODULE AND NOT HAS_SQLITE)
    message(FATAL_ERROR "SQLiteCpp module was not found, but tracing module was requested. Make sure to clone submodules or re-clone Illumina using git clone <illumina-repo> --recurse-submodules")
endif()
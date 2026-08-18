set(lib_src types.h types.h debug.h illumina.h types.cpp illumina.cpp board.cpp board.h zobrist.cpp parsehelper.cpp parsehelper.h utils.h attacks.cpp attacks.h movegen.h perft.cpp perft.h clock.h search.h transpositiontable.cpp transpositiontable.h searchdefs.h movepicker.h search.cpp evaluation.cpp evaluation.h timemanager.cpp timemanager.h boardutils.h boardutils.cpp movegen_impl.h utils.cpp movehistory.h
        pext_attacks.cpp
        magic_attacks.cpp
        tunablevalues.cpp endgame.cpp endgame.h nnue.cpp nnue.h
        bench.cpp
        bench.h
        tracing.h
        simd.h)

set_property(SOURCE nnue.cpp APPEND PROPERTY OBJECT_DEPENDS "${NNUE_PATH}")

function(add_illumina_library TARGET ARCH OUTPUT_NAME)
    add_library(${TARGET} STATIC ${lib_src})
    apply_arch_options(${TARGET} ${ARCH})
    set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME ${OUTPUT_NAME})

    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # GCC requires some flags in order for threads and <filesystem> to work.
        target_link_libraries(${TARGET} PUBLIC stdc++fs)
    endif()

    target_include_directories(${TARGET} PUBLIC ${CMAKE_SOURCE_DIR}/ext)

    target_compile_definitions(${TARGET} PUBLIC NNUE_PATH=\"${NNUE_PATH}\")
endfunction()

foreach(ARCH ${ARCHS})
    set(TARGET illumina_lib_${ARCH})
    add_illumina_library(${TARGET} ${ARCH} illumina_${ARCH})

    if (OPTIMIZE_SHALLOW_SEARCH)
        target_compile_definitions(${TARGET} PRIVATE OPTIMIZE_SHALLOW_SEARCH)
    endif()

    set(DATAGEN_TARGET illumina_datagen_lib_${ARCH})
    add_illumina_library(${DATAGEN_TARGET} ${ARCH} illumina_datagen_${ARCH})
    target_compile_definitions(${DATAGEN_TARGET} PRIVATE OPTIMIZE_SHALLOW_SEARCH)
endforeach()

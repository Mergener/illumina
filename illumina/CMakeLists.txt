add_library(illumina_lib STATIC types.h types.h debug.h illumina.h types.cpp illumina.cpp board.cpp board.h zobrist.cpp parsehelper.cpp parsehelper.h utils.h attacks.cpp attacks.h movegen.h)
set_target_properties(illumina_lib PROPERTIES OUTPUT_NAME illumina)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # GCC requires some flags in order for threads and <filesystem> to work.
    target_link_libraries(illumina_lib PUBLIC stdc++fs)
endif()

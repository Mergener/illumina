add_library(illumina_lib STATIC types.h types.h debug.h illumina.h types.cpp illumina.cpp board.cpp board.h)
set_target_properties(illumina_lib PROPERTIES OUTPUT_NAME illumina)
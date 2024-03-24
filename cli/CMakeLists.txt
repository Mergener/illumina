add_executable(illumina_cli main.cpp uciserver.cpp uciserver.h)
set_target_properties(illumina_cli PROPERTIES OUTPUT_NAME illumina)
target_link_libraries(illumina_cli PRIVATE illumina_lib)
target_include_directories(illumina_cli PRIVATE ${ILLUMINA_LIBRARY_SOURCE_DIR})
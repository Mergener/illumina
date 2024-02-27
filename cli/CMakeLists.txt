add_executable(boa_cli main.cpp)
set_target_properties(boa_cli PROPERTIES OUTPUT_NAME boa)
target_link_libraries(boa_cli PRIVATE boa_lib)
target_include_directories(boa_cli PRIVATE ${BOA_LIBRARY_SOURCE_DIR})
add_executable(datagen main.cpp)
set_target_properties(datagen PROPERTIES OUTPUT_NAME datagen)
target_link_libraries(datagen PRIVATE illumina_lib)
target_include_directories(datagen PRIVATE ${ILLUMINA_LIBRARY_SOURCE_DIR})
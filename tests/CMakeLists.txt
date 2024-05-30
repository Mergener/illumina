
add_executable(illumina_tests main.cpp suites/types.cpp suites/board.cpp suites/parsehelper.cpp suites/utils.cpp suites/attacks.cpp suites/perft.cpp suites/staticlist.cpp suites/boardutils.cpp suites/movepicker.cpp)
set_target_properties(illumina_tests PROPERTIES OUTPUT_NAME illumina-tests)
target_link_libraries(illumina_tests PRIVATE illumina_lib)
target_link_libraries(illumina_tests PRIVATE litetest)
target_include_directories(illumina_tests PRIVATE ${ILLUMINA_LIBRARY_SOURCE_DIR})
target_include_directories(illumina_tests PRIVATE ${ILLUMINA_EXT_DIR}/litetest/litetest)

add_test(NAME tests
         COMMAND illumina_tests)
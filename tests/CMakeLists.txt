set(tests_src main.cpp suites/types.cpp suites/board.cpp suites/parsehelper.cpp suites/utils.cpp suites/attacks.cpp suites/perft.cpp suites/staticlist.cpp suites/boardutils.cpp suites/movepicker.cpp)

foreach(ARCH ${ARCHS})
    set(TARGET illumina_tests_${ARCH})
    add_executable(${TARGET} ${tests_src})
    apply_arch_options(${TARGET} ${ARCH})
    set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME illumina-tests_${ARCH})
    target_link_libraries(${TARGET} PRIVATE illumina_lib_${ARCH})
    target_link_libraries(${TARGET} PRIVATE litetest)
    target_include_directories(${TARGET} PRIVATE ${ILLUMINA_LIBRARY_SOURCE_DIR})
    target_include_directories(${TARGET} PRIVATE ${ILLUMINA_EXT_DIR}/litetest/litetest)

    add_test(NAME tests_${ARCH}
             COMMAND ${TARGET})
endforeach()
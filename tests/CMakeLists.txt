set(tests_src main.cpp suites/types.cpp suites/board.cpp suites/parsehelper.cpp suites/utils.cpp suites/attacks.cpp suites/perft.cpp suites/staticlist.cpp suites/boardutils.cpp suites/movepicker.cpp)

include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)

foreach(ARCH ${ARCHS})
    set(TARGET illumina_tests_${ARCH})
    add_executable(${TARGET} ${tests_src})
    apply_arch_options(${TARGET} ${ARCH})
    set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME illumina-tests_${ARCH})
    target_link_libraries(${TARGET} PRIVATE illumina_lib_${ARCH})
    target_link_libraries(${TARGET} PRIVATE doctest::doctest)
    target_include_directories(${TARGET} PRIVATE ${ILLUMINA_LIBRARY_SOURCE_DIR})

    doctest_discover_tests(${TARGET} TEST_PREFIX "${ARCH}::")
endforeach()

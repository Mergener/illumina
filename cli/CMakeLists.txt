set(cli_src main.cpp cliapplication.cpp cliapplication.h commands.h commands.cpp state.cpp state.h ucioption.h ucioption.cpp)

if (INCLUDE_TRACING_MODULE)
    set(cli_src ${cli_src} searchtracer.cpp searchtracer.h)
endif()

foreach (ARCH ${ARCHS})
    set(TARGET illumina_cli_${ARCH})
    add_executable(${TARGET} ${cli_src})
    apply_arch_options(${TARGET} ${ARCH})

    math(EXPR SYS_BITS "${CMAKE_SIZEOF_VOID_P} * 8")
    string(TOLOWER ${CMAKE_SYSTEM_NAME} SYS_NAME)

    if (DEVELOPMENT)
        set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME illumina_${ARCH})
    else()
        set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME illumina_${ILLUMINA_VERSION_NAME}_${ARCH}_${SYS_NAME}_${SYS_BITS}-bit)
    endif()

    if (INCLUDE_TRACING_MODULE)
        target_link_libraries(${TARGET} PRIVATE SQLiteCpp)
        target_include_directories(${TARGET} PRIVATE ${CMAKE_SOURCE_DIR}/ext/SQLiteCpp/include)
    endif()

    target_link_libraries(${TARGET} PRIVATE illumina_lib_${ARCH})
    target_include_directories(${TARGET} PRIVATE ${ILLUMINA_LIBRARY_SOURCE_DIR})
endforeach ()
set(datagen_src main.cpp)

foreach (ARCH ${ARCHS})
    set(TARGET illumina_datagen_${ARCH})
    add_executable(${TARGET} ${datagen_src})
    apply_arch_options(${TARGET} ${ARCH})

    target_link_libraries(${TARGET} PRIVATE illumina_lib_${ARCH})
    target_include_directories(${TARGET} PRIVATE ${ILLUMINA_LIBRARY_SOURCE_DIR})
endforeach ()
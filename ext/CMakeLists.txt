if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/litetest/CMakeLists.txt")
    add_subdirectory(litetest)
    set(HAS_LITETEST true PARENT_SCOPE)
endif()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/litetest/CMakeLists.txt")
    add_subdirectory(SQLiteCpp)
    set(HAS_SQLITE true PARENT_SCOPE)
endif()
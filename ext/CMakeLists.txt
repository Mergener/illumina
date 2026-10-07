if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/SQLiteCpp/CMakeLists.txt")
    add_subdirectory(SQLiteCpp)
    set(HAS_SQLITE true PARENT_SCOPE)
endif()

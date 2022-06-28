
add_executable(vdf_reparse reparse.c)
target_link_libraries(vdf_reparse vdf)

add_test(NAME vdf_reparse_run
    COMMAND
        vdf_reparse ${CMAKE_CURRENT_SOURCE_DIR}/files/registry.vdf reparsed_registry.vdf
    )

add_test(NAME vdf_reparse_compare
    COMMAND
        ${CMAKE_COMMAND} -E compare_files --ignore-eol ${CMAKE_CURRENT_SOURCE_DIR}/files/registry.vdf reparsed_registry.vdf
    )

add_executable(vdf_index index.c)
target_link_libraries(vdf_index vdf)

add_test(NAME vdf_index_test
    COMMAND
        vdf_index ${CMAKE_CURRENT_SOURCE_DIR}/files/registry.vdf HKCU Software Valve Steam steamglobal language
    )


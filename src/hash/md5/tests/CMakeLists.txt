
add_executable(md5cmp md5cmp.c)
target_link_libraries(md5cmp md5)

add_test(NAME md5_hash_test
    COMMAND md5cmp ABCDEF 8827a41122a5028b9808c7bf84b9fcf6
    )
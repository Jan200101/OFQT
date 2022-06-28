
list(APPEND
	MD5_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/global.h
    ${CMAKE_CURRENT_SOURCE_DIR}/md5.h
    ${CMAKE_CURRENT_SOURCE_DIR}/md5c.c
)

add_library(md5 STATIC ${MD5_SOURCES})
target_compile_options(md5 PUBLIC ${CFLAGS})

target_include_directories(md5 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if (ENABLE_TESTS)
    add_subdirectory(tests)
endif()

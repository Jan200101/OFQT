
list(APPEND
    VDF_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/vdf.h
    ${CMAKE_CURRENT_SOURCE_DIR}/vdf.c
)

add_library(vdf STATIC ${VDF_SOURCES})
target_compile_options(vdf PUBLIC ${CFLAGS})

target_include_directories(vdf PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if (ENABLE_TESTS)
    add_subdirectory(tests)
endif()

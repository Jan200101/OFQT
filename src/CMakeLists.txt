
find_package(Libcurl REQUIRED)
find_package(JsonC REQUIRED)
add_subdirectory(hash)

set(CFLAGS
    -Wall -Wextra -pedantic
    -Wconversion -Wshadow -Wstrict-aliasing
    -Winit-self -Wcast-align -Wpointer-arith
    -Wmissing-declarations -Wmissing-include-dirs
    -Wno-unused-parameter -Wuninitialized
    ${LIBCURL_CFLAGS}
    ${JSONC_CFLAGS}
)

list(APPEND
    CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/fs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/fs.h
    ${CMAKE_CURRENT_SOURCE_DIR}/net.c
    ${CMAKE_CURRENT_SOURCE_DIR}/net.h
    ${CMAKE_CURRENT_SOURCE_DIR}/steam.c
    ${CMAKE_CURRENT_SOURCE_DIR}/steam.h
    ${CMAKE_CURRENT_SOURCE_DIR}/toast.c
    ${CMAKE_CURRENT_SOURCE_DIR}/toast.h
)

add_library(libofqt OBJECT ${CORE_SOURCES})
target_compile_options(libofqt PUBLIC ${CFLAGS})

target_include_directories(libofqt PUBLIC ${LIBCURL_INCLUDE_DIRS})
target_include_directories(libofqt PUBLIC ${JSONC_INCLUDE_DIRS})
target_include_directories(libofqt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(libofqt LINK_PUBLIC ${LIBCURL_LIBRARIES})
target_link_libraries(libofqt LINK_PUBLIC ${JSONC_LIBRARIES})
target_link_libraries(libofqt LINK_PUBLIC md5)

if(BUILD_CLI)
    add_subdirectory(cli)
endif()

if(BUILD_QT)
    add_subdirectory(qt)
endif()

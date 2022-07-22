
find_package(Libcurl REQUIRED)
find_package(JsonC REQUIRED)
add_subdirectory(hash)
add_subdirectory(vdf)

set(CFLAGS
    -Wall -Wextra -pedantic
    -Wfloat-equal -Wundef
    -Wstrict-overflow=5 -Wunreachable-code
    -Wcast-qual -Wswitch-default
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

add_library(tvn STATIC ${CORE_SOURCES})
target_compile_options(tvn PUBLIC ${CFLAGS})

target_include_directories(tvn PUBLIC ${LIBCURL_INCLUDE_DIRS})
target_include_directories(tvn PUBLIC ${JSONC_INCLUDE_DIRS})
target_include_directories(tvn PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(tvn LINK_PUBLIC ${LIBCURL_LIBRARIES})
target_link_libraries(tvn LINK_PUBLIC ${JSONC_LIBRARIES})
target_link_libraries(tvn LINK_PUBLIC md5)
target_link_libraries(tvn LINK_PUBLIC vdf)
if (WIN32)
    target_link_libraries(tvn LINK_PUBLIC shlwapi)
endif()

# frontends are included last so they will be
# put in the root of the build directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if(BUILD_CLI)
    add_subdirectory(cli)
endif()

if(BUILD_QT)
    add_subdirectory(qt)
endif()

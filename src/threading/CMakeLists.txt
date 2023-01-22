set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

list(APPEND
    THREADING_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/cpu.c
    ${CMAKE_CURRENT_SOURCE_DIR}/cpu.h
    ${CMAKE_CURRENT_SOURCE_DIR}/pool.c
    ${CMAKE_CURRENT_SOURCE_DIR}/pool.h
)


add_library(threading STATIC ${THREADING_SOURCES})
target_compile_options(threading PUBLIC ${CFLAGS})
target_link_libraries(threading PRIVATE Threads::Threads)

target_include_directories(threading PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# get all modules first

list(APPEND
    HOOK_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/arch.c
    ${CMAKE_CURRENT_SOURCE_DIR}/arch.h
    ${CMAKE_CURRENT_SOURCE_DIR}/hook.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hook.h
    ${CMAKE_CURRENT_SOURCE_DIR}/inject.c
    ${CMAKE_CURRENT_SOURCE_DIR}/inject.h
    ${CMAKE_CURRENT_SOURCE_DIR}/memory.c
    ${CMAKE_CURRENT_SOURCE_DIR}/memory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/module.c
    ${CMAKE_CURRENT_SOURCE_DIR}/module.h
)
add_subdirectory(payloads)

add_library(hook STATIC ${HOOK_SOURCES})
target_compile_options(hook PUBLIC ${CFLAGS})
target_include_directories(hook PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(hook PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

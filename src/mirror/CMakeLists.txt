SET(FRONTEND_NAME "OFmirror")

if (NOT UNIX)
    message(FATAL_ERROR "Mirroring is not supported on Windows")
endif()

SET(MIRROR_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
)

add_executable(${FRONTEND_NAME} ${MIRROR_SOURCES})
target_include_directories(${FRONTEND_NAME} PUBLIC ${JSONC_INCLUDE_DIRS})
target_compile_options(${FRONTEND_NAME} PUBLIC ${CFLAGS})
target_link_libraries(${FRONTEND_NAME} PRIVATE tvn)
target_link_libraries(${FRONTEND_NAME} PRIVATE threading)
target_link_libraries(${FRONTEND_NAME} LINK_PUBLIC ${JSONC_LIBRARIES})
install(TARGETS ${FRONTEND_NAME})

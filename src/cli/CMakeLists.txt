SET(FRONTEND_NAME "OFCL")

SET(CLI_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/commands.c
    ${CMAKE_CURRENT_SOURCE_DIR}/commands.h
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/updater.c
    ${CMAKE_CURRENT_SOURCE_DIR}/updater.h
)

add_executable(${FRONTEND_NAME} ${CLI_SOURCES})
target_compile_options(${FRONTEND_NAME} PUBLIC ${CFLAGS})
target_link_libraries(${FRONTEND_NAME} PRIVATE tvn)
target_link_libraries(${FRONTEND_NAME} PRIVATE threading)
install(TARGETS ${FRONTEND_NAME})

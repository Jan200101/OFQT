
list(APPEND
    modules
    SysLoadLibrary
)

foreach(module ${modules})
    set(module_name "${module}")
    add_library(${module} SHARED "${module}.c")
    # OF is 32 bit, so we need to be too
    set_target_properties(${module} PROPERTIES COMPILE_OPTIONS "-m32" LINK_FLAGS "-m32")


    list(APPEND
        module_embed_output
        ${CMAKE_CURRENT_BINARY_DIR}/lib${module_name}_so.c
        ${CMAKE_CURRENT_BINARY_DIR}/lib${module_name}_so.h
    )
    # embed
    add_custom_command(
        OUTPUT ${module_embed_output}
        COMMAND ${CMAKE_COMMAND}
        "-Dbin_in=$<TARGET_FILE:${module}>"
        -P ${CMAKE_SOURCE_DIR}/cmake/FileEmbed.cmake
        DEPENDS ${module}
    )
    add_library(${module_name}_embed OBJECT ${module_embed_output})
    list(APPEND
        HOOK_SOURCES
        $<TARGET_OBJECTS:${module_name}_embed>
    )

    set(module_embed_output "")
endforeach()

set(HOOK_SOURCES ${HOOK_SOURCES} PARENT_SCOPE)

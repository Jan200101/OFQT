cmake_minimum_required(VERSION 3.12)

if(CMAKE_POLICY_DEFAULT_CMP0017 OR CMAKE_POLICY_DEFAULT_CMP0020)
    # touch these to remove warnings
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING
        "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

project(OFQT VERSION 0.1.1 LANGUAGES C)

option(BUILD_CLI "Build the CLI client" OFF)
option(BUILD_QT "Build the Qt GUI client" OFF)

set(TOAST_DEFAULT_REMOTE "http://toast.openfortress.fun/toast" CACHE STRING
    "Default Mirror to get OpenFortress files from")

add_compile_definitions(NAME="${CMAKE_PROJECT_NAME}")
add_compile_definitions(VERSION="${CMAKE_PROJECT_VERSION}")
add_compile_definitions(TOAST_DEFAULT_REMOTE="${TOAST_DEFAULT_REMOTE}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_subdirectory(src)

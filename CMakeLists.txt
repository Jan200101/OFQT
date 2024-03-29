cmake_minimum_required(VERSION 3.12)

if(CMAKE_POLICY_DEFAULT_CMP0017 OR CMAKE_POLICY_DEFAULT_CMP0020)
    # touch these to remove warnings
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING
        "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

project(OFQT VERSION 0.1.4 LANGUAGES C)

option(BUILD_CLI "Build the CLI client" OFF)
option(BUILD_QT "Build the Qt GUI client" OFF)
option(BUILD_MIRROR "Build the Mirror client" OFF)
option(ENABLE_TESTS "Enable tests of the various components" OFF)
option(DIRECT_LAUNCH "Launch OpenFortress directly instead of via Steam" ON)
option(NAIVE_LAUNCH "Launch through the Steam Browser Protocol [Does not support Launch Options]" OFF)

if (ENABLE_TESTS)
    enable_testing()
endif()


set(TOAST_DEFAULT_REMOTE "https://toast.openfortress.fun/toast" CACHE STRING
    "Default Mirror to get OpenFortress files from")

add_compile_definitions(NAME="${CMAKE_PROJECT_NAME}")
add_compile_definitions(VERSION="${CMAKE_PROJECT_VERSION}")
if(DIRECT_LAUNCH)
    message(STATUS "Launch type: Direct")
    add_compile_definitions("STEAM_DIRECT_LAUNCH")
elseif(NAIVE_LAUNCH)
    message(STATUS "Launch type: Steam Naive")
    add_compile_definitions("STEAM_NAIVE_LAUNCH")
else()
    message(STATUS "Launch type: Steam")
endif()

add_compile_definitions(TOAST_DEFAULT_REMOTE="${TOAST_DEFAULT_REMOTE}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_subdirectory(src)

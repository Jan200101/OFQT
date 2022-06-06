SET(FRONTEND_NAME "OFQT")
enable_language(CXX)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets Gui REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets Gui REQUIRED)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

list(APPEND
    QT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mainwindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mainwindow.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/settings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/settings.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/workers.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/workers.hpp

    ${CMAKE_CURRENT_SOURCE_DIR}/mainwindow.ui
    ${CMAKE_CURRENT_SOURCE_DIR}/assets.qrc
)

if(WIN32)
    string(REPLACE "." "," CMAKE_PROJECT_COMMAVERSION ${CMAKE_PROJECT_VERSION})
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/assets/version.rc.in ${CMAKE_CURRENT_BINARY_DIR}/version.rc @ONLY)
    list(APPEND QT_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/version.rc)
endif()

add_executable(${FRONTEND_NAME} WIN32 ${QT_SOURCES})
target_link_libraries(${FRONTEND_NAME} PRIVATE libofqt)
target_link_libraries(${FRONTEND_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
set_property(TARGET ${FRONTEND_NAME} PROPERTY CXX_STANDARD 11)

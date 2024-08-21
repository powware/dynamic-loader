cmake_minimum_required(VERSION 3.10)

project(portforwarder VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(portforwarder ${PROJECT_SOURCE_DIR}/src/main.cpp
${PROJECT_SOURCE_DIR}/src/mainwindow.cpp
${PROJECT_SOURCE_DIR}/src/mainwindow.ui)

target_link_libraries(portforwarder PRIVATE Qt6::Widgets)

set_target_properties(portforwarder PROPERTIES
    WIN32_EXECUTABLE ON
)
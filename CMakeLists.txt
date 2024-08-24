cmake_minimum_required(VERSION 3.10)

project(
  portforwarder
  VERSION 1.0
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(TARGET_BITNESS
      "64"
      CACHE STRING "")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
  set(TARGET_BITNESS
      "32"
      CACHE STRING "")
else()
  message(FATAL_ERROR "Unknown architecture or pointer size.")
endif()

message(STATUS "Target: x${TARGET_BITNESS}")

find_package(pfw REQUIRED)

if(TARGET_BITNESS STREQUAL "64")
  find_package(Qt6 REQUIRED COMPONENTS Widgets)

  qt_standard_project_setup()

  qt_add_executable(
    portforwarder
    ${PROJECT_SOURCE_DIR}/src/portforwarder/portforwarder.cpp
    ${PROJECT_SOURCE_DIR}/src/portforwarder/mainwindow.cpp
    ${PROJECT_SOURCE_DIR}/src/portforwarder/processselector.cpp
    ${PROJECT_SOURCE_DIR}/src/portforwarder/mainwindow.ui)

  target_include_directories(portforwarder PRIVATE ${PROJECT_SOURCE_DIR}/src)

  target_link_libraries(portforwarder PRIVATE Qt6::Widgets pfw::pfw)

  set_target_properties(portforwarder PROPERTIES WIN32_EXECUTABLE ON)
elseif()
  message(STATUS "Only building injector.")
endif()

add_executable(portinjector
               ${PROJECT_SOURCE_DIR}/src/portinjector/portinjector.cpp)

set(INJECTOR_NAME "portinjector${TARGET_BITNESS}")

set_target_properties(portinjector PROPERTIES OUTPUT_NAME ${INJECTOR_NAME})

find_package(pfw REQUIRED)

target_link_libraries(portinjector PRIVATE pfw::pfw)

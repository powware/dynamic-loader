cmake_minimum_required(VERSION 3.10)

project(
  dynmaic-loader
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
    forwarder
    ${PROJECT_SOURCE_DIR}/src/forwarder/forwarder.cpp
    ${PROJECT_SOURCE_DIR}/src/forwarder/mainwindow.cpp
    ${PROJECT_SOURCE_DIR}/src/forwarder/processselector.cpp
    ${PROJECT_SOURCE_DIR}/src/forwarder/coms.cpp
    ${PROJECT_SOURCE_DIR}/src/forwarder/mainwindow.ui
    ${PROJECT_SOURCE_DIR}/src/pipe.cpp)

  target_include_directories(forwarder PRIVATE ${PROJECT_SOURCE_DIR}/src)

  target_link_libraries(forwarder PRIVATE Qt6::Widgets pfw::pfw Rpcrt4)

  set_target_properties(forwarder PROPERTIES WIN32_EXECUTABLE ON)
elseif()
  message(STATUS "Only building loader.")
endif()

add_executable(loader ${PROJECT_SOURCE_DIR}/src/loader/loader.cpp
                      ${PROJECT_SOURCE_DIR}/src/pipe.cpp)

set(LOADER_NAME "loader${TARGET_BITNESS}")

set_target_properties(loader PROPERTIES OUTPUT_NAME ${LOADER_NAME})

target_include_directories(loader PRIVATE ${PROJECT_SOURCE_DIR}/src)

find_package(pfw REQUIRED)

target_link_libraries(loader PRIVATE pfw::pfw)

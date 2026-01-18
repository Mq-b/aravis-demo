cmake_minimum_required(VERSION 3.16)

project(aravis-demo VERSION 1.0)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}/bin)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_PREFIX_PATH $ENV{VCPKG_LIB} ${CMAKE_PREFIX_PATH})

find_package(PkgConfig REQUIRED)
pkg_check_modules(ARAVIS REQUIRED IMPORTED_TARGET aravis-0.10)
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Concurrent Multimedia MultimediaWidgets)

set(SOURCES
    src/main.cpp
    src/CameraController.cpp
    src/MainWindow.cpp
    src/VideoWidget.cpp
)

set(HEADERS
    include/CameraController.h
    include/MainWindow.h
    include/VideoWidget.h
)

# 启用Qt MOC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE 
    include
    ${ARAVIS_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    PkgConfig::ARAVIS
    Qt5::Core
    Qt5::Widgets
    Qt5::Concurrent
    Qt5::Multimedia
    Qt5::MultimediaWidgets
)

add_custom_command(
  TARGET ${PROJECT_NAME}
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/res" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/res"
)
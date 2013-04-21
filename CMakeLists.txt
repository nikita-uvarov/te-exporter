project(te-exporter)
cmake_minimum_required(VERSION 2.6)
find_package(Qt4 REQUIRED)
enable_testing()

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})

set(te-exporter-sources te-exporter.cpp main.cpp)
qt4_automoc(${te-exporter-sources})
add_executable(te-exporter ${te-exporter-sources})
target_link_libraries(te-exporter ${QT_QTCORE_LIBRARY})

add_subdirectory(tests)

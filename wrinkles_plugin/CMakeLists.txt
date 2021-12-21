cmake_minimum_required(VERSION 3.17)
project(wrinkles_plugin)

set(CMAKE_CXX_STANDARD 14)

add_library(wrinkles_plugin SHARED "wrinkles.cpp" "wrinkles.h")

set(OpenCV_DIR $ENV{OpenCV_DIR}/lib)
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS}) # Not needed for CMake >= 2.8.11
target_link_libraries(wrinkles_plugin ${OpenCV_LIBS})
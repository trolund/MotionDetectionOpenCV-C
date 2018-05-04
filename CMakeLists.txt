cmake_minimum_required(VERSION 3.9)
project(opencv C)
find_package(OpenCV REQUIRED)

set(CMAKE_C_STANDARD 11)

add_executable(opencv main.c log.h)

INCLUDE_DIRECTORIES(${OpenCV_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${OpenCV_LIBS} )
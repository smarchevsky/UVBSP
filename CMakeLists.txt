cmake_minimum_required(VERSION 3.5)

project(ImageBSPSegmentation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

INCLUDE_DIRECTORIES(src/)
#SOURCE_DIR
aux_source_directory(src/ SRC_LIST)
add_executable(${PROJECT_NAME} ${SRC_LIST} "main.cpp")

target_link_libraries(ImageBSPSegmentation sfml-graphics)

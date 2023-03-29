cmake_minimum_required(VERSION 3.5)

project(UVBSP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DPROJECT_DIR="${PROJECT_SOURCE_DIR}")

include_directories(src/)
include_directories(.)
aux_source_directory(src/ SRC_LIST)
aux_source_directory(imgui/ SRC_LIST)
add_executable(${PROJECT_NAME} ${SRC_LIST} "main.cpp" "shaders/BSPshader.glsl")

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else()
#  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

target_link_libraries(${PROJECT_NAME} sfml-system sfml-window sfml-graphics GL)


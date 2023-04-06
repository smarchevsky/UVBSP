cmake_minimum_required(VERSION 3.5)

project(UVBSP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DPROJECT_DIR="${PROJECT_SOURCE_DIR}")
add_definitions(-DSHADER_DIR="${PROJECT_SOURCE_DIR}/resources/shaders")
add_definitions(-DFONT_DIR="${PROJECT_SOURCE_DIR}/resources/fonts")

add_definitions(-DDOCUMENTS_DIR="${PROJECT_SOURCE_DIR}")

#aux_source_directory(src/common/ SRC_LIST)

include_directories(src/common/)
include_directories(src/)
include_directories(third_party/)

FILE(GLOB_RECURSE ALL_HEADERS ${CMAKE_SOURCE_DIR}/*.h)
FILE(GLOB_RECURSE ALL_CPP "src/*.cpp" "third_party/*.cpp")

FILE(GLOB_RECURSE ALL_SHADERS "resources/shaders/*.frag") # I want to see all shader files in CMake project

add_executable(${PROJECT_NAME} ${ALL_CPP} ${ALL_SHADERS} ${ALL_HEADERS} "src/main.cpp")

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else()
#  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

target_link_libraries(${PROJECT_NAME} sfml-system sfml-window sfml-graphics GL)

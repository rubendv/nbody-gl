cmake_minimum_required(VERSION 3.3)
project(nbody_gl)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp thirdparty/glad/src/glad.c)

include_directories(thirdparty/glad/include)
include_directories(thirdparty/glfw/include)
include_directories(thirdparty/glm)

add_subdirectory(thirdparty/glfw EXCLUDE_FROM_ALL)

add_executable(nbody_gl ${SOURCE_FILES})

target_link_libraries(nbody_gl glfw ${GLFW_LIBRARIES})
# Minimum CMake version
cmake_minimum_required(VERSION 3.10)

# Project name
project(main CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/header)

# Find GLM package
find_package(glm CONFIG REQUIRED)

# Find OpenGL and GLFW
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

# Source files
file(GLOB_RECURSE SOURCES ${CMAKE_SOURCE_DIR}/src/*.cpp)

# Add executable target
add_executable(main ${SOURCES})

# Link libraries
target_link_libraries(main PRIVATE glfw OpenGL::GL glm::glm)
cmake_minimum_required(VERSION 3.14)  # FetchContent is fully supported from CMake 3.14 onwards
# Project name
project(main LANGUAGES C CXX)

#add_compile_options(-fsanitize=address)

#set(CMAKE_C_COMPILER gcc) 
#set(CMAKE_CXX_COMPILER g++)  # Set g++ as the C++ compiler

set(CMAKE_C_STANDARD 11)
# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_PARALLEL_LEVEL 8)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/external/include)

add_library(glad external/src/glad.c)
add_library(tinyxml2 external/src/tinyxml2.cpp)

include(FetchContent)

# Fetch GLFW
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
)
FetchContent_MakeAvailable(glfw)

# Fetch GLM
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
)
FetchContent_MakeAvailable(glm)

#FetchContent_Declare(
#  assimp
#  GIT_REPOSITORY https://github.com/assimp/assimp.git
#)
## Set CMake options to reduce Assimp to only GLTF
#set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)  # Disable all importers by default
#set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "" FORCE)              # Enable only GLTF importer
#set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)                        # Disable exporters (optional, if not needed)
#set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)              # Disable tools like assimp viewer
#set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)                     # Disable unit tests to reduce size
#set(ASSIMP_BUILD_FBX_IMPORTER OFF CACHE BOOL "" FORCE)                     # Disable unit tests to reduce size
#set(ASSIMP_BUILD_OBJ_IMPORTER OFF CACHE BOOL "" FORCE)                     # Disable unit tests to reduce size
#set(ASSIMP_BUILD_TYPE "Release" CACHE STRING "Build type for Assimp" FORCE)

#FetchContent_MakeAvailable(assimp)

#FetchContent_Declare(
  #lua
  #URL https://www.lua.org/ftp/lua-5.4.7.tar.gz
  #URL_HASH SHA256=9fbf5e28ef86c69858f6d3d34eccc32e911c1a28b4120ff3e84aaa70cfbf1e30
 # DOWNLOAD_EXTRACT_TIMESTAMP TRUE
#)
#FetchContent_MakeAvailable(lua)

# Locate all .c files in the Lua source directory
#file(GLOB LUA_SOURCES ${lua_SOURCE_DIR}/src/*.c)

# Create the library target
#add_library(lua STATIC ${LUA_SOURCES})

# Include the Lua headers
#target_include_directories(lua PUBLIC ${lua_SOURCE_DIR}/src)

# Suppress all warnings for GLFW
target_compile_options(glfw PRIVATE
    -w  # Suppress all warnings
)

# Suppress all warnings for GLM
target_compile_options(glm PRIVATE
    -w  # Suppress all warnings
)

# Suppress all warnings for GLM
#target_compile_options(assimp PRIVATE
#    -w  # Suppress all warnings
#)

#set_target_properties(assimp PROPERTIES MAP_IMPORTED_CONFIG_DEBUG Release)

# Declare FreeType as a dependency
FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://gitlab.freedesktop.org/freetype/freetype.git
)
FetchContent_MakeAvailable(freetype)

find_package(OpenGL REQUIRED)

# Source files
file(GLOB_RECURSE SOURCES
  ${CMAKE_SOURCE_DIR}/src/*.cpp
  ${CMAKE_SOURCE_DIR}/src/*.c
  ${CMAKE_SOURCE_DIR}/external/src/*.cpp
  ${CMAKE_SOURCE_DIR}/external/src/*.c
)

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g")
add_compile_options(-Wall -Wextra)

# Add executable target
add_executable(main ${SOURCES})
# Link libraries
target_link_libraries(main PRIVATE glfw OpenGL::GL glm::glm glad freetype tinyxml2)

target_compile_options(main PRIVATE -Wall)
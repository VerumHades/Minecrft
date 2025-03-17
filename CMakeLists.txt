cmake_minimum_required(VERSION 3.14)  # FetchContent is fully supported from CMake 3.14 onwards
# Project name
project(majnkraft LANGUAGES C CXX)

#add_compile_options(-fsanitize=address)

#if (WIN32)
#  set(CMAKE_OBJECT_PATH_MAX 260)
#endif ()

##set(CMAKE_C_COMPILER gcc) 
##set(CMAKE_CXX_COMPILER g++)  # Set g++ as the C++ compiler

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Disable shared libraries" FORCE)

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
  GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(glfw)

# Fetch GLM
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(glm)

# Fetch Assimp (uncommented and enabled with configuration)
FetchContent_Declare(
  assimp
  GIT_REPOSITORY https://github.com/assimp/assimp.git
  GIT_PROGRESS TRUE
)
# Set CMake options to reduce Assimp to only GLTF
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)  # Disable all importers by default
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "" FORCE)              # Enable only GLTF importer
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)                        # Disable exporters (optional, if not needed)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)              # Disable tools like assimp viewer
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)                     # Disable unit tests to reduce size
set(ASSIMP_BUILD_FBX_IMPORTER OFF CACHE BOOL "" FORCE)              # Disable FBX importer
set(ASSIMP_BUILD_OBJ_IMPORTER OFF CACHE BOOL "" FORCE)              # Disable OBJ importer
FetchContent_MakeAvailable(assimp)

FetchContent_Declare(
  lua
  URL https://www.lua.org/ftp/lua-5.4.7.tar.gz
  GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(lua)

# Locate all .c files in the Lua source directory
file(GLOB LUA_SOURCES ${lua_SOURCE_DIR}/src/*.c)

# Create the library target
add_library(lua STATIC ${LUA_SOURCES})

# Include the Lua headers
target_include_directories(lua PUBLIC ${lua_SOURCE_DIR}/src)

# Suppress all warnings for GLFW
target_compile_options(glfw PRIVATE
    -w  # Suppress all warnings
)

# Suppress all warnings for GLM
target_compile_options(glm PRIVATE
    -w  # Suppress all warnings
)

# Suppress all warnings for Assimp
target_compile_options(assimp PRIVATE
    -w  # Suppress all warnings
)

include(FetchContent)
FetchContent_Declare(
  cpptrace
  GIT_REPOSITORY https://github.com/jeremy-rifkin/cpptrace.git
  GIT_TAG        v0.8.2 # <HASH or TAG>
)
FetchContent_MakeAvailable(cpptrace)
#set_target_properties(assimp PROPERTIES MAP_IMPORTED_CONFIG_DEBUG Release)

# Declare FreeType as a dependency
FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://github.com/freetype/freetype
    GIT_PROGRESS TRUE
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

# Add executable target
add_executable(majnkraft ${SOURCES})

# Link libraries
target_link_libraries(majnkraft PRIVATE glfw OpenGL::GL glm::glm glad freetype tinyxml2 lua assimp cpptrace::cpptrace)
target_compile_options(majnkraft PRIVATE -Wall)

# Needed for shared library builds on windows:  copy cpptrace.dll to the same directory as the
# executable for your_target
if(WIN32)
  add_custom_command(
    TARGET majnkraft POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:cpptrace::cpptrace>
    $<TARGET_FILE_DIR:majnkraft>
  )
endif()

install(
  FILES $<TARGET_RUNTIME_DLLS:majnkraft>
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Package metadata
set(CPACK_PACKAGE_NAME "Majnkraft")
set(CPACK_PACKAGE_VERSION "0.0.1")
set(CPACK_PACKAGE_DESCRIPTION "Majkraft")
set(CPACK_PACKAGE_VENDOR "ChICorp")
set(CPACK_PACKAGE_CONTACT "")

set(CPACK_GENERATOR "NSIS")  # Windows installe

set(CPACK_NSIS_PACKAGE_NAME "Majnkraft")
set(CPACK_NSIS_EXECUTABLES_DIRECTORY bin)
set(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_NAME} ${CPACK_PACKAGE_VERSION}")
set(CPACK_NSIS_INSTALL_DIRECTORY "C:/Program Files/${CPACK_PACKAGE_NAME}")
set(CPACK_NSIS_CREATE_ICONS_EXTRA
    "CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Majnkraft.lnk' '$INSTDIR\\\\majnkraft.exe'"
)
set(CPACK_NSIS_DELETE_ICONS_EXTRA
    "Delete '$SMPROGRAMS\\\\$START_MENU\\\\Majnkraft.lnk'"
)
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)

# Installation directories
install(TARGETS majnkraft RUNTIME DESTINATION .)
#install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ DESTINATION include)
#install(DIRECTORY ${CMAKE_SOURCE_DIR}/external/include/ DESTINATION include)

# Include library files (if needed)
#install(TARGETS glad tinyxml2 lua assimp freetype
#        ARCHIVE DESTINATION lib
#        LIBRARY DESTINATION lib)

# Add readme/license if you have them
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/README.md)
set(CPACK_RESOURCE_FILE_README ${CMAKE_SOURCE_DIR}/LICENSE)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/resources/ DESTINATION resources)
#install(DIRECTORY C:/msys64/mingw64/bin/
#        DESTINATION .  # Install in the root of the installation directory
#        FILES_MATCHING PATTERN "*.dll"
#)

set(RESOURCE_FILES
    C:/msys64/mingw64/bin/libpcre2-8-0.dll 
    C:/msys64/mingw64/bin/zlib1.dll 
    C:/msys64/mingw64/bin/libgraphite2.dll 
    C:/msys64/mingw64/bin/libglib-2.0-0.dll 
    C:/msys64/mingw64/bin/libpng16-16.dll 
    C:/msys64/mingw64/bin/libfreetype-6.dll 
    C:/msys64/mingw64/bin/libharfbuzz-0.dll 
    C:/msys64/mingw64/bin/libbrotlicommon.dll 
    C:/msys64/mingw64/bin/libgcc_s_seh-1.dll 
    C:/msys64/mingw64/bin/libiconv-2.dll 
    C:/msys64/mingw64/bin/libbz2-1.dll 
    C:/msys64/mingw64/bin/libintl-8.dll 
    C:/msys64/mingw64/bin/libwinpthread-1.dll 
    C:/msys64/mingw64/bin/libbrotlidec.dll
    C:/msys64/mingw64/bin/libstdc++-6.dll
)

install(FILES ${RESOURCE_FILES} DESTINATION .)

include(CPack)
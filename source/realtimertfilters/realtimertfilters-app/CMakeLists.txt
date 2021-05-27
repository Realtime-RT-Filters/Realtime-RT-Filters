# CMakeList.txt : CMake project for realtimertfilters-app, include source and define
# DISCLAIMER: This was heavily inspired by Sascha Willems cmakelists file for his examples
cmake_minimum_required (VERSION 3.8)

# FILE Macros for all sources, main source and main headers
file (GLOB ALL_SOURCES "*.cpp" "sources/*.cpp" "*.inl" "sources/*.inl" "*.h" "*.hpp" "headers/*.h" "headers/*.hpp")
set(MAIN_SOURCE realtimertfliters-app.cpp)
set(MAIN_HEADER realtimertfilters-app.h)

# find_package(OpenMP) # Multithreading API - used in some of Sascha Willems examples

#include IMGUI
file(GLOB ADD_SOURCE "../external/imgui/*.cpp")
SET(ALL_SOURCES ${ALL_SOURCES} ${ADD_SOURCE})

# wayland requires additional source files
IF(USE_WAYLAND_WSI)
	SET(ALL_SOURCES ${ALL_SOURCES} ${CMAKE_BINARY_DIR}/xdg-shell-client-protocol.h ${CMAKE_BINARY_DIR}/xdg-shell-protocol.c)
ENDIF()

set(SHADER_DIR "shaders/")
file(GLOB SHADERS "${SHADER_DIR_GLSL}/*.vert" "${SHADER_DIR_GLSL}/*.frag" "${SHADER_DIR_GLSL}/*.comp" "${SHADER_DIR_GLSL}/*.geom" "${SHADER_DIR_GLSL}/*.tesc" "${SHADER_DIR_GLSL}/*.tese" "${SHADER_DIR_GLSL}/*.mesh" "${SHADER_DIR_GLSL}/*.task" "${SHADER_DIR_GLSL}/*.rgen" "${SHADER_DIR_GLSL}/*.rchit" "${SHADER_DIR_GLSL}/*.rmiss")
source_group("Shaders" FILES ${SHADERS})

# generate your personal project config
configure_file("${CMAKE_SOURCE_DIR}/realtimertfilters-app/project_defines.hpp.in" "${CMAKE_SOURCE_DIR}/realtimertfilters-app/project_defines.hpp")


#execute_process(WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/realtimertfilters-app/data/shaders/glsl" COMMAND "${CMAKE_SOURCE_DIR}/realtimertfilters-app/data/shaders/glsl/compile_all_shaders.bat")

if(WIN32)
	add_executable(realtimertfilters-app WIN32 ${MAIN_CPP} ${ALL_SOURCES} ${MAIN_HEADER} ${SHADERS})
	target_link_libraries(realtimertfilters-app base ${Vulkan_LIBRARY} ${WINLIBS})
        add_custom_command(TARGET realtimertfilters-app POST_BUILD WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/realtimertfilters-app/data/shaders/glsl" COMMAND "${CMAKE_SOURCE_DIR}/realtimertfilters-app/data/shaders/glsl/compile_all_shaders.bat")
else(WIN32)
	add_executable(realtimertfilters-app ${MAIN_CPP} ${ALL_SOURCES} ${MAIN_HEADER} ${SHADERS})
	target_link_libraries(realtimertfilters-app base )
        add_custom_command(TARGET realtimertfilters-app POST_BUILD WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/realtimertfilters-app/data/shaders/glsl" COMMAND "${CMAKE_SOURCE_DIR}/realtimertfilters-app/data/shaders/glsl/compile_all_shaders.sh")
endif(WIN32)


cmake_minimum_required(VERSION 3.16...3.28)
project(platformer2d LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(extensions)

# Compiler.
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_definitions(LK_COMPILER_MSVC)
	add_compile_options(/utf-8)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_definitions(LK_COMPILER_CLANG)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_definitions(LK_COMPILER_GNU)
endif()

add_executable(${PROJECT_NAME})

add_compile_definitions(
	LK_ENABLE_ASSERT=1
	LK_ENABLE_VERIFY=1
)

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(external)
add_subdirectory(modules)
target_sources_ifndef(TESTS_ENABLED ${PROJECT_NAME} PRIVATE src/main.cpp)

# Project interface.
add_library(platformer2d_interface INTERFACE)
target_include_directories(platformer2d_interface INTERFACE
	${CMAKE_SOURCE_DIR}/src
	${CMAKE_SOURCE_DIR}/external
	${CMAKE_SOURCE_DIR}/external/glfw/include
	${CMAKE_SOURCE_DIR}/external/imgui
	${CMAKE_SOURCE_DIR}/external/spdlog/include
	${CMAKE_SOURCE_DIR}/modules/glad/include
)
target_link_libraries(${PROJECT_NAME} PRIVATE platformer2d_interface)

target_link_libraries(${PROJECT_NAME} PRIVATE
	core
	renderer

	# External.
	glad
	glfw
	imgui
	spdlog
	stb_image
)

cmake_minimum_required(VERSION 3.16...3.28)
project(platformer2d LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(cmake/project.cmake)
include(cmake/extensions.cmake)

option(LK_ENABLE_ASSERT "Enable assert" OFF)
if (NOT DEFINED LK_ENABLE_ASSERT AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(LK_ENABLE_ASSERT ON CACHE BOOL "Enable assert" FORCE)
endif()
if (LK_ENABLE_ASSERT)
	project_add_compile_definitions(LK_ENABLE_ASSERT)
endif()

option(LK_ENABLE_VERIFY "Enable verify" OFF)
if (LK_ENABLE_VERIFY)
	project_add_compile_definitions(LK_ENABLE_VERIFY)
endif()

# Project interface.
add_library(${PROJECT_INTERFACE} INTERFACE)
add_executable(${PROJECT_NAME})

# Compiler.
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	project_add_compile_definitions(LK_COMPILER_MSVC)
	add_compile_options(/utf-8)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	project_add_compile_definitions(LK_COMPILER_CLANG)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	project_add_compile_definitions(LK_COMPILER_GNU)
endif()

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(external)
add_subdirectory(modules)
target_include_directories(${PROJECT_INTERFACE} INTERFACE
	${CMAKE_SOURCE_DIR}/src
	${CMAKE_SOURCE_DIR}/external
	${CMAKE_SOURCE_DIR}/external/glfw/include
	${CMAKE_SOURCE_DIR}/external/glm
	${CMAKE_SOURCE_DIR}/external/imgui
	${CMAKE_SOURCE_DIR}/external/spdlog/include
	${CMAKE_SOURCE_DIR}/modules/glad/include
)

# Link external dependencies to the project interface.
target_link_libraries(${PROJECT_INTERFACE} INTERFACE
	glad
	glfw
	glm
	imgui
	spdlog
	stb_image
)

# Get all compile definitions and apply on project executable and all 
# project libraries.
get_property(PROJECT_COMPILE_DEFINITIONS_PROPERTY GLOBAL PROPERTY PROJECT_COMPILE_DEFINITIONS)
message(STATUS "PROJECT_COMPILE_DEFINITIONS: ${PROJECT_COMPILE_DEFINITIONS_PROPERTY}")
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_INTERFACE})
target_compile_definitions(${PROJECT_NAME} PRIVATE ${PROJECT_COMPILE_DEFINITIONS_PROPERTY})

# Iterate all project libraries and setup linkage and definitions.
get_property(PROJECT_LIBS_PROPERTY GLOBAL PROPERTY PROJECT_LIBS)
foreach(library ${PROJECT_LIBS_PROPERTY})
	message(STATUS "${PROJECT_NAME} --> ${library}")
	target_link_libraries(${PROJECT_NAME} PRIVATE ${library})
	target_compile_definitions(${library} PRIVATE ${PROJECT_COMPILE_DEFINITIONS_PROPERTY})
endforeach()

target_sources_ifndef(TESTS_ENABLED ${PROJECT_NAME} PRIVATE src/main.cpp)

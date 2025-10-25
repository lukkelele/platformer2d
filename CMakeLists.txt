cmake_minimum_required(VERSION 3.16...3.28)
project(platformer2d LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(cmake/project.cmake)
include(cmake/extensions.cmake)

option(LK_BUILD_TESTS "Build tests" ON)
option(LK_ENABLE_ASSERT "Enable assert" OFF)
if (NOT DEFINED LK_ENABLE_ASSERT AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(LK_ENABLE_ASSERT ON)
endif()
if (LK_ENABLE_ASSERT)
	project_add_compile_definitions(LK_ENABLE_ASSERT)
endif()

option(LK_ENABLE_VERIFY "Enable verify" OFF)
if (LK_ENABLE_VERIFY)
	project_add_compile_definitions(LK_ENABLE_VERIFY)
endif()

set(LK_SCREEN_WIDTH "1920" CACHE STRING "Screen width")
set(LK_SCREEN_HEIGHT "1080" CACHE STRING "Screen height")
option(LK_LOG_FORCE_INLINE "Force inline log templates" OFF)

set(PROJECT_EXECUTABLE ${PROJECT_NAME})
# Project interface.
add_library(${PROJECT_INTERFACE} INTERFACE)

# Compiler.
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
if (MSVC)
	project_add_compile_definitions(
		LK_COMPILER_MSVC
		__LK_VA_OPT # /Zc:preprocessor
	)
	add_compile_options(
		/Zc:preprocessor
		/utf-8
	)

	# Disable warnings.
	project_add_compile_options(
		/wd4068   # Unknown #pragma mark
		/wd4267   # Possible loss of data
		/wd4244   # Possible loss of data
		/wd4312   # Conversion of greater size
		/wd28020  # Static analysis warning
		/wd33011  # Static analysis warning
		/wd4307
		/wd26451
		/wd26813  # 'Use bitwise and'
		/e0253    # Incorrect warning 'Expected a ;' for [[deprecated]]
	)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	project_add_compile_definitions(LK_COMPILER_GCC)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	project_add_compile_definitions(LK_COMPILER_CLANG)
else()
	message(FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

project_add_compile_definitions(
	LK_OPENGL_MAJOR=4
	LK_OPENGL_MINOR=6
	LK_LOG_FORCE_INLINE=${LK_LOG_FORCE_INLINE}
)

message("Build type: ${CMAKE_BUILD_TYPE}")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	project_add_compile_definitions(LK_BUILD_DEBUG)
endif()

set(LK_ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets")
set(LK_LOGS_DIR "${CMAKE_SOURCE_DIR}/logs")
make_directory(${CMAKE_BINARY_DIR}/intermediate)
configure_file(
    ${CMAKE_SOURCE_DIR}/src/config.h.in
    ${CMAKE_BINARY_DIR}/intermediate/lk_config.h
)
target_include_directories(${PROJECT_INTERFACE} INTERFACE ${CMAKE_BINARY_DIR}/intermediate)

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(external)
add_subdirectory(modules)
target_include_directories(${PROJECT_INTERFACE} INTERFACE
	${CMAKE_SOURCE_DIR}/src
	${CMAKE_SOURCE_DIR}/external
	${CMAKE_SOURCE_DIR}/external/catch2/src
	${CMAKE_SOURCE_DIR}/external/box2d
	${CMAKE_SOURCE_DIR}/external/glfw/include
	${CMAKE_SOURCE_DIR}/external/glm
	${CMAKE_SOURCE_DIR}/external/imgui
	${CMAKE_SOURCE_DIR}/external/spdlog/include
	${CMAKE_SOURCE_DIR}/modules/glad/include
)

# Link external dependencies to the project interface.
target_link_libraries(${PROJECT_INTERFACE} INTERFACE
	box2d
	glad
	glfw
	glm
	imgui
	spdlog
	stb_image
)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE src/main.cpp)
set_target_properties(${PROJECT_NAME} PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${PROJECT_NAME}"
)
target_compile_definitions(${PROJECT_NAME} PRIVATE
	SCREEN_WIDTH=${LK_SCREEN_WIDTH}
	SCREEN_HEIGHT=${LK_SCREEN_HEIGHT}
)

get_property(PROJECT_COMPILE_DEFINITIONS_PROPERTY GLOBAL PROPERTY PROJECT_COMPILE_DEFINITIONS)
get_property(PROJECT_COMPILE_OPTIONS_PROPERTY GLOBAL PROPERTY PROJECT_COMPILE_OPTIONS)

target_compile_options(${PROJECT_EXECUTABLE} PRIVATE ${PROJECT_COMPILE_OPTIONS_PROPERTY})
target_link_libraries(${PROJECT_EXECUTABLE} PRIVATE ${PROJECT_INTERFACE})
target_compile_definitions(${PROJECT_EXECUTABLE} PRIVATE ${PROJECT_COMPILE_DEFINITIONS_PROPERTY})

get_property(PROJECT_LIBS_PROPERTY GLOBAL PROPERTY PROJECT_LIBS)
foreach(library ${PROJECT_LIBS_PROPERTY})
	target_compile_definitions(${library} PRIVATE ${PROJECT_COMPILE_DEFINITIONS_PROPERTY})
	target_compile_options(${library} PRIVATE ${PROJECT_COMPILE_OPTIONS_PROPERTY})
	message(STATUS "Link: ${PROJECT_EXECUTABLE} -> ${library}")
	target_link_libraries(${PROJECT_EXECUTABLE} PRIVATE ${library})
endforeach()

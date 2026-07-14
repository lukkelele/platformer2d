cmake_minimum_required(VERSION 3.20...3.28)
project(platformer2d LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/project.cmake)
include(cmake/extensions.cmake)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

#-------------------------------------------------------------------
# Configuration
#-------------------------------------------------------------------
option(LK_BUILD_TESTS "Build tests" OFF)
option(LK_ENABLE_ASSERT "Enable assert" OFF)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(LK_ENABLE_ASSERT ON)
endif()
if (LK_ENABLE_ASSERT)
	project_add_compile_definitions(LK_ENABLE_ASSERT)
endif()

option(LK_ENABLE_VERIFY "Enable verify" ON)
if (LK_ENABLE_VERIFY)
	project_add_compile_definitions(LK_ENABLE_VERIFY)
endif()

set(LK_SCREEN_WIDTH "1920" CACHE STRING "Screen width")
set(LK_SCREEN_HEIGHT "1080" CACHE STRING "Screen height")
option(LK_LOG_FORCE_INLINE "Force inline log templates" OFF)
option(LK_PROFILING_SCOPED_TIMERS "Timers for scoped profiling" OFF)

set(PROJECT_EXECUTABLE ${PROJECT_NAME})
# Project interface.
add_library(${PROJECT_INTERFACE} INTERFACE)

project_add_compile_definitions(LKLOG_ENABLED)

if (MSVC)
  project_add_compile_options(
    $<$<OR:$<CONFIG:Debug>,$<BOOL:${LK_BUILD_TESTS}>>:/Od>
    $<$<OR:$<CONFIG:Debug>,$<BOOL:${LK_BUILD_TESTS}>>:/Zi>
  )
elseif (CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang)$")
  project_add_compile_options(
    $<$<OR:$<CONFIG:Debug>,$<BOOL:${LK_BUILD_TESTS}>>:-g3>
    $<$<OR:$<CONFIG:Debug>,$<BOOL:${LK_BUILD_TESTS}>>:-Og>
    $<$<OR:$<CONFIG:Debug>,$<BOOL:${LK_BUILD_TESTS}>>:-fno-omit-frame-pointer>
  )
endif()

# Compiler.
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} [${CMAKE_CXX_COMPILER_VERSION}] (${CMAKE_CXX_COMPILER})")
if (MSVC)
	set(LK_COMPILER_MSVC ON)
	project_add_compile_definitions(
		LK_COMPILER_MSVC
		__LK_VA_OPT # /Zc:preprocessor
		_CRT_SECURE_NO_WARNINGS
	)
	add_compile_options(
		/Zc:__cplusplus
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
	)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	set(LK_COMPILER_GCC ON)
	project_add_compile_definitions(
		LK_COMPILER_GCC
		__LK_VA_OPT
	)
	project_add_compile_options(
		-Wno-format-security
		-Wno-format
	)

	find_package(X11 REQUIRED)
	target_link_libraries(${PROJECT_INTERFACE} INTERFACE
		X11
	)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	set(LK_COMPILER_CLANG ON)
	project_add_compile_definitions(
	  LK_COMPILER_CLANG
	  __LK_VA_OPT
	)
else()
	message(FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

project_add_compile_definitions(
	LK_OPENGL_MAJOR=4
	LK_OPENGL_MINOR=6
	LK_LOG_FORCE_INLINE=${LK_LOG_FORCE_INLINE}
)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  if (LK_PROFILING_SCOPED_TIMERS)
    set(LK_PROFILING_SCOPED_TIMERS 1)
  else()
    set(LK_PROFILING_SCOPED_TIMERS 0)
  endif()

	project_add_compile_definitions(
		LK_BUILD_DEBUG
		LK_PROFILING
		LK_PROFILING_SCOPED_TIMERS=${LK_PROFILING_SCOPED_TIMERS}
		TRACY_ENABLE
		TRACY_STATIC
		TRACY_ON_DEMAND
		TRACY_CALLSTACK=15
	)
endif()

set(LK_PROJECT_DIR "${CMAKE_SOURCE_DIR}")
set(LK_BINARY_DIR "${CMAKE_BINARY_DIR}")
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
	${CMAKE_SOURCE_DIR}/external/box2d
	${CMAKE_SOURCE_DIR}/external/catch2/src
	${CMAKE_SOURCE_DIR}/external/glfw/include
	${CMAKE_SOURCE_DIR}/external/glm
	${CMAKE_SOURCE_DIR}/external/lklog/include
	${CMAKE_SOURCE_DIR}/external/imgui
	${CMAKE_SOURCE_DIR}/external/ImGuizmo
	${CMAKE_SOURCE_DIR}/external/nativefiledialog-extended/src/include
	${CMAKE_SOURCE_DIR}/external/tracy/public
	${CMAKE_SOURCE_DIR}/external/yaml-cpp/include
	${CMAKE_SOURCE_DIR}/external/json/include
	${CMAKE_SOURCE_DIR}/modules/glad/include
)

# Link external dependencies to the project interface.
target_link_libraries(${PROJECT_INTERFACE} INTERFACE
	box2d
	glad
	glfw
	glm
	lklog
	imgui
	ImGuizmo
	nfd
	stb_image
	Tracy::TracyClient
	yaml-cpp
	nlohmann_json
)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE src/main.cpp)
set_target_properties(${PROJECT_NAME} PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${PROJECT_NAME}"
)
project_add_compile_definitions(
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

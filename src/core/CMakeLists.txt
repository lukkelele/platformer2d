add_library(core STATIC 
	assert.h
	window.cpp
)

target_include_directories(core PUBLIC
	${CMAKE_SOURCE_DIR}/src
	${CMAKE_SOURCE_DIR}/external
	${CMAKE_SOURCE_DIR}/external/glfw/include
	${CMAKE_SOURCE_DIR}/external/imgui
	${CMAKE_SOURCE_DIR}/external/spdlog/include
	${CMAKE_SOURCE_DIR}/modules/glad/include
)

# TODO: make options and configurable from the project CMakeLists.txt
target_compile_definitions(core PUBLIC
	LK_ENABLE_ASSERT=1
	LK_ENABLE_VERIFY=1
)

# Compiler.
message(WARNING "COMPILER: ${CMAKE_CXX_COMPILER_ID}")
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_definitions(core PUBLIC LK_COMPILER_MSVC)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_definitions(core PUBLIC LK_COMPILER_CLANG)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_definitions(core PUBLIC LK_COMPILER_GCC)
endif()

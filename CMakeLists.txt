cmake_minimum_required(VERSION 3.16...3.28)
project(platformer2d)

add_executable(${PROJECT_NAME} src/main.cpp)
add_subdirectory(src)
add_subdirectory(external)
add_subdirectory(modules)

target_include_directories(${PROJECT_NAME} PRIVATE
	${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PRIVATE
	core
	glad
	glfw
)

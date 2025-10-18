project_library(renderer)
project_library_sources(
	camera.h
	camera.cpp
	imguilayer.h
	imguilayer.cpp
	opengl.h
	opengl.cpp
	shader.h 
	shader.cpp
	texture.h
	texture.cpp
	renderer.h
	renderer.cpp
	rendercommandqueue.h
	rendercommandqueue.cpp
	uniformbuffer.h
	uniformbuffer.cpp
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	project_library_sources(
		opengl_debug.h
		opengl_debug.cpp
	)
endif()

target_link_libraries(renderer PUBLIC 
	core
)

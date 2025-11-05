project_library(renderer)
project_library_sources(
	debugrenderer.h
	debugrenderer.cpp
	camera.h
	camera.cpp
	color.h
	font.h
	font.cpp
	fontawesome.h
	imgui.h
	imguilayer.h
	imguilayer.cpp
	opengl.h
	opengl.cpp
	shader.h
	shader.cpp
	sprite.h
	sprite.cpp
	texture.h
	texture.cpp
	texturearray.h
	texturearray.cpp
	renderer.h
	renderer.cpp
	rendercommandqueue.h
	rendercommandqueue.cpp
	uniformbuffer.h
	uniformbuffer.cpp
)

add_subdirectory(ui)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	project_library_sources(
		opengl_debug.h
		opengl_debug.cpp
	)
endif()

target_link_libraries(renderer PUBLIC 
	core
	physics
)

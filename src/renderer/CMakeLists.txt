project_library(renderer)
project_library_sources(
	opengl.h
	shader.h 
	shader.cpp
	texture.h
	texture.cpp
)

target_link_libraries(renderer PUBLIC core)

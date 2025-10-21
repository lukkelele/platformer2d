project_library(game)
project_library_sources(
	player.h
	player.cpp
)

target_link_libraries(game PUBLIC 
	core
	scene
	physics
)

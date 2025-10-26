project_library(game)
project_library_sources(
	gameinstance.h
	gameinstance.cpp
	player.h
	player.cpp
)

add_subdirectory(level)

target_link_libraries(game PUBLIC 
	core
	scene
	physics
)

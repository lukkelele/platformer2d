project_library(game)
project_library_sources(
	actor.h
	actor.cpp
	player.h
	player.cpp
)

target_link_libraries(game PUBLIC 
	core
	input
)

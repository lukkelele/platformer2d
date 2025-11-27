project_library(game)
project_library_sources(
	instance.h
	instance.cpp
	interaction.h
	player.h
	player.cpp
	spawner.h
	spawner.cpp
	spawnpoint.h
	spawnpoint.cpp
)

add_subdirectory(level)

target_link_libraries(game PUBLIC 
	core
	scene
	physics
)

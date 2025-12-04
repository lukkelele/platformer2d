project_library(game)
project_library_sources(
	editor.h
	editor.cpp
	instance.h
	instance.cpp
	player.h
	player.cpp
	spawner.h
	spawner.cpp
	spawnpoint.h
	spawnpoint.cpp
)

target_link_libraries(game PUBLIC 
	core
	scene
	physics
)

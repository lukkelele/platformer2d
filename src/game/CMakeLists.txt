project_library(game)
project_library_sources(
	editor.h
	editor.cpp
	instance.h
	instance.cpp
	gameplaysystem.h
	gameplaysystem.cpp
	player.h
	player.cpp
	spawner.h
	spawner.cpp
)

target_link_libraries(game PUBLIC 
	core
	scene
	physics
)

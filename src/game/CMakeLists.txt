project_library(game)
project_library_sources(
	editor.h
	editor.cpp
	instance.h
	instance.cpp
	inventory.h
	inventory.cpp
	item.h
	gameplaysystem.h
	gameplaysystem.cpp
	rifle.h
	rifle.cpp
	player.h
	player.cpp
	projectile.h
	projectile.cpp
	spawner.h
	spawner.cpp
	weapon.h
)

target_link_libraries(game PUBLIC 
	core
	scene
	physics
)

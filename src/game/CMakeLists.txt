project_library(game)
project_library_sources(
	controller.h
	editor.h
	editor.cpp
	enemy.h
	enemy.cpp
	instance.h
	instance.cpp
	inventory.h
	inventory.cpp
	item.h
	itemtype.h
	gameplaysystem.h
	gameplaysystem.cpp
	runtimelayer.h
	runtimelayer.cpp
	rifle.h
	rifle.cpp
	player.h
	player.cpp
	projectile.h
	projectile.cpp
	spawner.h
	spawner.cpp
	spritereader.h
	spritereader.cpp
	weapon.h
	weapontype.h
)

add_subdirectory(controller)

target_link_libraries(game PUBLIC 
	core
	scene
	physics
)

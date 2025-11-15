project_library(scene)
project_library_sources(
	actor.h
	actor.cpp
	actorspecification.h
	components.h
	effectmanager.h
	effectmanager.cpp
	scene.h
	scene.cpp
)

target_link_libraries(scene PUBLIC 
	core
	renderer
	physics
)

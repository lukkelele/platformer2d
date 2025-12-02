project_library(scene)
project_library_sources(
	actor.h
	actor.cpp
	actor_impl.h
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

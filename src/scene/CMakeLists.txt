project_library(scene)
project_library_sources(
	actor.h
	actor.cpp
	actorspecification.h
	components.h
)

target_link_libraries(scene PUBLIC 
	core
	renderer
	physics
)

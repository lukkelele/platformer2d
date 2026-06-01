project_library(physics)
project_library_sources(
	body.h
	body.cpp
	bodytype.h
	collisionfilter.h
	events.h
	ray.h
	ray.cpp
	physicsworld.h
	physicsworld.cpp
)

target_link_libraries(physics PUBLIC 
	core
)

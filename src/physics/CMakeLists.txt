project_library(physics)
project_library_sources(
	body.h
	body.cpp
	bodytype.h
	contactlistener.h
	contactlistener.cpp
	ray.h
	ray.cpp
	physicsworld.h
	physicsworld.cpp
)

target_link_libraries(physics PUBLIC 
	core
)

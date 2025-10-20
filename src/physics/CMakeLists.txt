project_library(physics)
project_library_sources(
	body.h
	body.cpp
	contactlistener.h
	contactlistener.cpp
	physicsworld.h
	physicsworld.cpp
)

target_link_libraries(physics PUBLIC 
	core
)

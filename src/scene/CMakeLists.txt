project_library(scene)
project_library_sources(
	actor.h
	actor.cpp
	transformcomponent.h
)

target_link_libraries(scene PUBLIC 
	core
	renderer
)

project_library(physics)
project_library_sources(
	contactlistener.h
	contactlistener.cpp
)

target_link_libraries(physics PUBLIC 
	core
)

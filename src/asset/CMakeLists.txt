project_library(asset)
project_library_sources(
	assetmanager.h
	assetmanager.cpp
)

target_link_libraries(asset PUBLIC 
	core
	renderer
)

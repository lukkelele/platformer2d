project_library(serialization)
project_library_sources(
	serializable.h
	serialization.h
	yaml.h
)

target_link_libraries(serialization PUBLIC 
	core
)

project_library(serialization)
project_library_sources(
	serializable.h
	yamlserialization.h
)

target_link_libraries(serialization PUBLIC 
	core
)

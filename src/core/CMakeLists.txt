project_library(core)
project_library_sources(
	assert.h
	buffer.h
	core.h
	delegate.h
	log.h
	log.cpp
	log_formatters.h
	template.h
	window.h
	window.cpp
)

add_subdirectory(input)

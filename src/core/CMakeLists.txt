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

# TODO: window should be moved from core and placed in renderer (?)
# Because of the dependency to EKeyState.
target_link_libraries(core PRIVATE input)

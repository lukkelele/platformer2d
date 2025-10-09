project_library(input)
project_library_sources(
	keyboard.h
	keyboard.cpp
	keycodes.h
)

target_link_libraries(input PUBLIC core)

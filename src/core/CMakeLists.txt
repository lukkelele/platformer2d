add_library(core STATIC 
	window.cpp
)

target_link_libraries(core PUBLIC platformer2d_interface)
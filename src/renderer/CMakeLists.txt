add_library(renderer STATIC 
	texture.cpp
)

target_link_libraries(renderer PUBLIC platformer2d_interface)

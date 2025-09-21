add_library(core STATIC 
	window.cpp
)

target_include_directories(core PRIVATE
	${CMAKE_SOURCE_DIR}/external/glfw/include
	${CMAKE_SOURCE_DIR}/modules/glad/include
)

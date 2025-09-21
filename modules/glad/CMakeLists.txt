add_library(glad STATIC
	src/glad.c
)

target_include_directories(glad PUBLIC include)
# TODO: target based on platform (add for linux)
target_link_libraries(glad PRIVATE opengl32)

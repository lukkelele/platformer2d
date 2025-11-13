add_library(glad STATIC
	src/glad.c
)

target_include_directories(glad PUBLIC include)
if (MSVC)
	target_link_libraries(glad PRIVATE opengl32)
endif()

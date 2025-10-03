######################################################
# External dependencies.
######################################################

##############################
# Library: Catch2
##############################
add_subdirectory(Catch2)

##############################
# Library: glad
##############################
# Exists in 'modules/glad'.

##############################
# Library: glfw
##############################
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_LIBRARY_TYPE STATIC CACHE STRING "" FORCE)
add_subdirectory(glfw)

##############################
# Library: ImGui
##############################
add_library(imgui STATIC
	imgui/imgui.cpp
	imgui/imgui_demo.cpp
	imgui/imgui_draw.cpp
	imgui/imgui_tables.cpp
	imgui/imgui_tables.cpp
	imgui/imgui_widgets.cpp
	imgui/backends/imgui_impl_glfw.cpp
	imgui/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC imgui imgui/backends)
target_include_directories(imgui PRIVATE glfw/include)

##############################
# Library: spdlog
##############################
set(SPDLOG_USE_STD_FORMAT ON)
add_subdirectory(spdlog)

##############################
# Library: stb_image
##############################
add_library(stb_image STATIC
	stb/stb_image.cpp
)

##############################
# Library: glm
##############################
add_subdirectory(glm)

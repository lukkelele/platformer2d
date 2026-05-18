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
# Library: lklog
##############################
set(LKLOG_BUILD_TESTS OFF)
add_subdirectory(lklog)

##############################
# Library: ImGui
##############################
add_library(imgui STATIC
	imgui/imgui_internal.h
	imgui/imgui.h
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
if (LK_COMPILER_GCC)
	find_package(X11 REQUIRED)
	target_link_libraries(imgui PRIVATE X11)
endif()

##############################
# Library: ImGuizmo
##############################
add_library(ImGuizmo STATIC
	ImGuizmo/ImGuizmo.h
	ImGuizmo/ImGuizmo.cpp
	ImGuizmo/ImGradient.h
	ImGuizmo/ImGradient.cpp
	ImGuizmo/ImCurveEdit.h
	ImGuizmo/ImCurveEdit.cpp
	ImGuizmo/ImSequencer.h
	ImGuizmo/ImSequencer.cpp
	ImGuizmo/GraphEditor.h
	ImGuizmo/GraphEditor.cpp
)
target_include_directories(ImGuizmo PRIVATE imgui)

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
target_compile_definitions(glm PUBLIC
	GLM_ENABLE_EXPERIMENTAL
)

##############################
# Library: box2d
##############################
set(BOX2D_SAMPLES OFF)
set(BOX2D_VALIDATE ON)
set(BOX2D_UNIT_TESTS OFF)
add_subdirectory(box2d)

##############################
# Library: Tracy
##############################
add_subdirectory(tracy)

##############################
# Library: yaml-cpp
##############################
add_subdirectory(yaml-cpp)

##############################
# Library: nlohmann_json
##############################
add_subdirectory(json)

##############################
# Library: nfd-extended
##############################
add_subdirectory(nativefiledialog-extended)

##############################
# Library: msdf-atlas-gen
##############################
#set(MSDFGEN_DISABLE_PNG ON) # TODO: libpng be added as dependency (?)
# TODO: Need to fix freetype dependency. Do not want to use vcpkg toolchain :)
#add_subdirectory(msdf-atlas-gen)


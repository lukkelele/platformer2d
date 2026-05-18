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
# Library: zlib
##############################
set(ZLIB_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(ZLIB_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_MINIZIP OFF CACHE BOOL "" FORCE)
set(ZLIB_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(zlib)
# zlib only creates ZLIB::ZLIB when shared lib is built.
# Alias the static target so libpng finds it.
if(NOT TARGET ZLIB::ZLIB)
	add_library(ZLIB::ZLIB ALIAS zlibstatic)
endif()

##############################
# Library: libpng
##############################
set(PNG_SHARED OFF CACHE BOOL "" FORCE)
set(PNG_STATIC ON CACHE BOOL "" FORCE)
set(PNG_FRAMEWORK OFF CACHE BOOL "" FORCE)
set(PNG_TESTS OFF CACHE BOOL "" FORCE)
set(PNG_TOOLS OFF CACHE BOOL "" FORCE)
set(PNG_EXECUTABLES OFF CACHE BOOL "" FORCE)
set(SKIP_INSTALL_ALL ON CACHE BOOL "" FORCE)
add_subdirectory(libpng)
# msdfgen expects PNG::PNG, libpng's CMake doesn't create that alias.
if(NOT TARGET PNG::PNG)
	add_library(PNG::PNG ALIAS png_static)
endif()

##############################
# Library: FreeType
##############################
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
add_subdirectory(freetype)
if(NOT TARGET Freetype::Freetype)
	add_library(Freetype::Freetype ALIAS freetype)
endif()

##############################
# Library: msdf-atlas-gen
##############################
set(MSDF_ATLAS_USE_VCPKG OFF CACHE BOOL "" FORCE)
set(MSDF_ATLAS_USE_SKIA OFF CACHE BOOL "" FORCE)
set(MSDF_ATLAS_BUILD_STANDALONE ON CACHE BOOL "" FORCE)
set(MSDF_ATLAS_NO_ARTERY_FONT OFF CACHE BOOL "" FORCE)
set(MSDF_ATLAS_INSTALL OFF CACHE BOOL "" FORCE)
# TODO: I'd like to set DYNAMIC_RUNTIME to OFF
set(MSDF_ATLAS_DYNAMIC_RUNTIME ON CACHE BOOL "" FORCE)
set(MSDFGEN_USE_VCPKG OFF CACHE BOOL "" FORCE)
set(MSDFGEN_USE_SKIA OFF CACHE BOOL "" FORCE)
set(MSDFGEN_USE_OPENMP OFF CACHE BOOL "" FORCE)
set(MSDFGEN_DISABLE_SVG ON CACHE BOOL "" FORCE)
set(MSDFGEN_DISABLE_PNG OFF CACHE BOOL "" FORCE)
set(MSDFGEN_BUILD_STANDALONE OFF CACHE BOOL "" FORCE)
set(MSDFGEN_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(msdf-atlas-gen)


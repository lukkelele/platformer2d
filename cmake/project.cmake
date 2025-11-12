###################################################################
# Project CMake
###################################################################
set(PROJECT_INTERFACE platformer2d_interface)

define_property(GLOBAL PROPERTY PROJECT_LIBS
	BRIEF_DOCS "Project libraries"
	FULL_DOCS "Project libraries"
)
set_property(GLOBAL PROPERTY PROJECT_LIBS "")

# Compile definitions.
define_property(GLOBAL PROPERTY PROJECT_COMPILE_DEFINITIONS
	BRIEF_DOCS "Project compile definitions"
	FULL_DOCS "Project compile definitions"
)
set_property(GLOBAL PROPERTY PROJECT_COMPILE_DEFINITIONS "")

# Compile options.
define_property(GLOBAL PROPERTY PROJECT_COMPILE_OPTIONS
	BRIEF_DOCS "Project compile options"
	FULL_DOCS "Project compile options"
)
set_property(GLOBAL PROPERTY PROJECT_COMPILE_OPTIONS "")

# Add definitions to define global property.
# Used to set definitions on the project target and all libraries.
function(project_add_compile_definitions)
	message(DEBUG "definitions: ${ARGN}")
	set_property(GLOBAL APPEND PROPERTY PROJECT_COMPILE_DEFINITIONS ${ARGN})
endfunction()

function(project_add_compile_options)
	message(DEBUG "options: ${ARGN}")
	set_property(GLOBAL APPEND PROPERTY PROJECT_COMPILE_OPTIONS ${ARGN})
endfunction()

function(project_link_interface interface)
	target_link_libraries(${interface} INTERFACE ${PROJECT_INTERFACE})
endfunction()

function(project_compile_definitions)
    target_compile_definitions(${PROJECT_INTERFACE} INTERFACE ${ARGV})
endfunction()

# Add a library to the global library property.
function(project_append_cmake_library library)
	message(DEBUG "append: ${library}")
	set_property(GLOBAL APPEND PROPERTY PROJECT_LIBS ${library})
endfunction()

# Create project library.
# Macro because add_library() has to be executed within the scope of the caller.
macro(project_library name)
	set(PROJECT_CURRENT_LIBRARY ${name})
	if (${ARGC} GREATER 1) 
		set(_args ${ARGN})
		list(GET _args 0 _lib_type)
	else()
		set(_lib_type STATIC)
	endif()
	message(DEBUG "library: ${library}")
	add_library(${name} ${_lib_type} "")
	target_link_libraries(${name} PUBLIC ${PROJECT_INTERFACE})
	project_append_cmake_library(${PROJECT_CURRENT_LIBRARY})
endmacro()

function(project_library_sources source)
	target_sources(${PROJECT_CURRENT_LIBRARY} PRIVATE ${source} ${ARGN})
endfunction()

function(project_library_include_directories)
	target_include_directories(${PROJECT_CURRENT_LIBRARY} PRIVATE ${ARGN})
endfunction()

function(project_library_link_libraries item)
	target_link_libraries(${PROJECT_CURRENT_LIBRARY} PUBLIC ${item} ${ARGN})
endfunction()

function(project_library_add_dependencies)
	add_dependencies(${PROJECT_CURRENT_LIBRARY} ${ARGN})
endfunction()

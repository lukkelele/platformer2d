######################################################################
# Tests.
######################################################################
message(STATUS "LK_BUILD_TESTS: ${LK_BUILD_TESTS}")
set(TESTS_ENABLED ${LK_BUILD_TESTS})

# Declare a test option and append it to TEST_OPTIONS.
# Options are assigned OFF by default.
#
# A test option has the prefix LK_TEST and the rest of the name 
# is the directory where the test source is in.
#
# Usage:
#   test_option(option_name, default_value)
#
# Examples:
#   test_option(TEST_1)
#   test_option(TEST_2 ON)
#   test_option(TEST_3 OFF)
#
macro(test_option name)
	# Set default value to OFF.
	if (${ARGC} GREATER 1) 
		set(_args ${ARGN})
		list(GET _args 0 _default_value)
	else()
		set(_default_value OFF)
	endif()

	string(REGEX REPLACE "LK_TEST_" "" test_option_cleaned "${name}")
	string(TOLOWER "${test_option_cleaned}" test_option_lower)
	message(DEBUG "Option: ${test_option_lower} (${name})")
	option(${name} "Test: ${test_option_lower}" ${_default_value})
	list(APPEND TEST_OPTIONS ${name})
endmacro()

test_option(LK_TEST_DRAW_TRIANGLE)
test_option(LK_TEST_DRAW_TRIANGLE_SHADER)
test_option(LK_TEST_DRAW_TRIANGLE_SHADER_CONFIGURABLE)
test_option(LK_TEST_DRAW_TRIANGLE_SHADER_UNIFORM)
test_option(LK_TEST_DRAW_TRIANGLE_SHADER_VAO)
test_option(LK_TEST_DRAW_TRIANGLE_SHADER_VAO_BUFFERLAYOUT)
test_option(LK_TEST_DRAW_TRIANGLE_SHADER_VAO_INSTANCED)

set(LK_TEST_SCREEN_WIDTH "1600" CACHE STRING "Screen width")
set(LK_TEST_SCREEN_HEIGHT "1050" CACHE STRING "Screen height")

# Scan subdirectories recursively and match them with the test options.
function(add_tests_recursive dir)
    file(GLOB subdirs LIST_DIRECTORIES true "${dir}/*")
    foreach(subdir ${subdirs})
        if (NOT IS_DIRECTORY "${subdir}")
            continue()
        endif()

		get_filename_component(TEST_NAME "${subdir}" NAME)
		string(TOUPPER "${TEST_NAME}" TEST_NAME_UPPER)
		set(OPT_NAME "LK_TEST_${TEST_NAME_UPPER}")

		# Check if this subdir corresponds to an option in TEST_OPTIONS.
		list(FIND TEST_OPTIONS ${OPT_NAME} IDX)
		if (NOT IDX EQUAL -1)
			if (LK_BUILD_TESTS OR ${OPT_NAME})
				string(REGEX REPLACE ".*${PROJECT_NAME}/" "" subdir_short_path "${subdir}")
				message(STATUS "[*] ${OPT_NAME} (${subdir_short_path})")
				set(TESTS_ENABLED ON)

				if (NOT EXISTS "${subdir}/CMakeLists.txt")
					message(FATAL_ERROR "${subdir_short_path}: Missing CMakeLists.txt")
				endif()

				add_executable(${TEST_NAME})
				target_sources(${TEST_NAME} PRIVATE 
					test_base.cpp
					${subdir}/main.cpp
				)
				target_include_directories(${TEST_NAME} PRIVATE
					${CMAKE_CURRENT_SOURCE_DIR}
					${subdir}
				)
				target_compile_definitions(${TEST_NAME} PRIVATE
					LK_TEST_SUITE=${TEST_NAME}
					LK_TEST_NAME="${TEST_NAME}"
					SCREEN_WIDTH=${LK_TEST_SCREEN_WIDTH}
					SCREEN_HEIGHT=${LK_TEST_SCREEN_HEIGHT}
				)
				target_link_libraries(${TEST_NAME} PRIVATE
					${PROJECT_INTERFACE}
					Catch2::Catch2
				)

				# Add compile definitions and options that is added to libraries.
				get_property(PROJECT_COMPILE_DEFINITIONS_PROPERTY GLOBAL PROPERTY PROJECT_COMPILE_DEFINITIONS)
				target_compile_definitions(${TEST_NAME} PRIVATE ${PROJECT_COMPILE_DEFINITIONS_PROPERTY})
				get_property(PROJECT_COMPILE_OPTIONS_PROPERTY GLOBAL PROPERTY PROJECT_COMPILE_OPTIONS)
				target_compile_options(${TEST_NAME} PRIVATE ${PROJECT_COMPILE_OPTIONS_PROPERTY})

				# Place test binary in its own directory.
				set_target_properties(${TEST_NAME} PROPERTIES 
					RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${TEST_NAME}"
				)

				add_subdirectory(${subdir} ${OPT_NAME})
			else()
				message(STATUS "[ ] ${OPT_NAME}")
			endif()
		endif()

		# Recurse further.
		add_tests_recursive("${subdir}")
    endforeach()
endfunction()

message(STATUS "========================================================")
add_tests_recursive("${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "========================================================")

if (TESTS_ENABLED)
	# Always enable LK_ASSERT and LK_VERIFY for tests.
    message(STATUS "Enabling assert and verify")
	set(LK_ENABLE_ASSERT ON)
	set(LK_ENABLE_VERIFY ON)
endif()

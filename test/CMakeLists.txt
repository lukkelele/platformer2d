######################################################################
# Tests.
######################################################################
set(TESTS_ENABLED OFF PARENT_SCOPE)

option(LK_TEST_DRAW_TRIANGLE "Test: draw_triangle" OFF)
option(LK_TEST_DRAW_TRIANGLE_SHADER "Test: draw_triangle_shader" OFF)
option(LK_TEST_DRAW_TRIANGLE_SHADER_CONFIGURABLE "Test: draw_triangle_shader_configurable" OFF)
set(TEST_OPTIONS
    LK_TEST_DRAW_TRIANGLE
    LK_TEST_DRAW_TRIANGLE_SHADER
    LK_TEST_DRAW_TRIANGLE_SHADER_CONFIGURABLE
)

function(extract_test_name input output)
    # Remove TEST_ prefix and convert to lowercase.
    string(REGEX REPLACE "^LK_TEST_" "" stripped "${input}")
    string(TOLOWER "${stripped}" lowered)
    message(DEBUG "${CMAKE_CURRENT_FUNCTION}: ${input} -> ${lowered}")
    set(${output} "${lowered}" PARENT_SCOPE)
endfunction()

message(STATUS "============================================")
foreach(opt IN LISTS TEST_OPTIONS)
    option(${opt} "Test: ${opt}" OFF)
    if (${opt})
	    message(STATUS "[*] ${opt}")
        set(TESTS_ENABLED ON PARENT_SCOPE)
        set(TEST_SUITE "${opt}")

        # Set the test directory as a variable and use it with add_subdirectory.
        set(TEST_DIR "")
        extract_test_name(${opt} TEST_DIR)
        if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_DIR}" OR NOT IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_DIR}")
            message(FATAL_ERROR "Test directory \"${TEST_DIR}\" does not exist")
        endif()

		set(TEST_EXECUTABLE "${TEST_DIR}")
		add_executable(${TEST_EXECUTABLE})
		target_sources(${TEST_EXECUTABLE} PRIVATE test.cpp)
		target_include_directories(${TEST_EXECUTABLE} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
		target_compile_definitions(${TEST_EXECUTABLE} PRIVATE LK_TEST_SUITE=${TEST_SUITE})
        set_target_properties(${TEST_EXECUTABLE} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${TEST_DIR}")

		add_subdirectory(${TEST_DIR} ${opt})
		target_link_libraries(${TEST_EXECUTABLE} PRIVATE ${PROJECT_INTERFACE})

        get_property(PROJECT_COMPILE_DEFINITIONS_PROPERTY GLOBAL PROPERTY PROJECT_COMPILE_DEFINITIONS)
	    target_compile_definitions(${TEST_EXECUTABLE} PRIVATE ${PROJECT_COMPILE_DEFINITIONS_PROPERTY})
    else()
	    message(STATUS "[ ] ${opt}")
    endif()
endforeach()
message(STATUS "============================================")

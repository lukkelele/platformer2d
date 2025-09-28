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
        # TODO: Add support for multiple tests building at a time.

	    message(STATUS "[*] ${opt}")
        set(TESTS_ENABLED ON PARENT_SCOPE)
        set(TEST_SUITE "${opt}")

        # Set the test directory as a variable and use it with add_subdirectory.
        set(TEST_DIR "")
        extract_test_name(${opt} TEST_DIR)
        message(DEBUG "TEST_DIR: ${TEST_DIR}")
    else()
	    message(STATUS "[ ] ${opt}")
    endif()
endforeach()
message(STATUS "============================================")

if (TESTS_ENABLED)
    # TODO: Validate the existance of the directory
    #set(TEST_EXECUTABLE "${TEST_DIR}" PARENT_SCOPE) # TODO: Remove PARENT_SCOPE
    set(TEST_EXECUTABLE "${TEST_DIR}")
    # Override the project executable to resolve linkage to the interface.
    set(PROJECT_EXECUTABLE "${TEST_EXECUTABLE}" PARENT_SCOPE)

    # Always enable LK_ASSERT and LK_VERIFY for tests.
    set(LK_ENABLE_ASSERT ON CACHE BOOL "Enable assert" FORCE)
    set(LK_ENABLE_VERIFY ON CACHE BOOL "Enable verify" FORCE)

    add_executable(${TEST_EXECUTABLE})
	target_sources(${TEST_EXECUTABLE} PRIVATE test.cpp)
	target_include_directories(${TEST_EXECUTABLE} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_definitions(${TEST_EXECUTABLE} PRIVATE LK_TEST_SUITE=${TEST_SUITE})

    add_subdirectory(${TEST_DIR})
endif()

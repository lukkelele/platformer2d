######################################################################
# Tests.
######################################################################
set(TESTS_ENABLED OFF CACHE BOOL "Enable tests")

option(TEST_DRAW_TRIANGLE             "Test: draw_triangle" OFF)
option(TEST_DRAW_TRIANGLE_WITH_SHADER "Test: draw_triangle_with_shader" OFF)
set(TEST_OPTIONS
    TEST_DRAW_TRIANGLE
    TEST_DRAW_TRIANGLE_WITH_SHADER
)

message(STATUS "============================================")
foreach(opt IN LISTS TEST_OPTIONS)
    option(${opt} "Test: ${opt}" OFF)
    if (${opt})
	    message(STATUS "[*] ${opt}")
        set(TESTS_ENABLED ON CACHE BOOL "Enable tests" FORCE)
        target_compile_definitions(${PROJECT_NAME} PRIVATE LK_TEST_SUITE="${opt}")
    else()
	    message(STATUS "[ ] ${opt}")
    endif()
endforeach()
message(STATUS "============================================")

target_sources_ifdef(TESTS_ENABLED ${PROJECT_NAME} PRIVATE test.cpp)
target_include_directories_ifdef(TESTS_ENABLED ${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

######################################################################
# Tests Suites.
######################################################################
add_subdirectory_ifdef(TEST_DRAW_TRIANGLE draw_triangle)
add_subdirectory_ifdef(TEST_DRAW_TRIANGLE_WITH_SHADER draw_triangle_with_shader)

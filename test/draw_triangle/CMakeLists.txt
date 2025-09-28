message(STATUS "Building: ${TEST_SUITE} (${TEST_DIR})")
target_sources(${TEST_EXECUTABLE} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

target_link_libraries(${TEST_EXECUTABLE} PRIVATE 
    renderer
)

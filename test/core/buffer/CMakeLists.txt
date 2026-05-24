target_sources(${TEST_NAME} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/test.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/unit_tests.cpp
)

target_link_libraries(${TEST_NAME} PRIVATE
	core

	# FIXME: Should not be needed, the testbase should be able to run core tests
	game
	renderer
	physics
)


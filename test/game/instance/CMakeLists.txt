target_sources(${TEST_NAME} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/test.cpp
)

target_link_libraries(${TEST_NAME} PRIVATE 
	core
	game
	renderer
	physics
)

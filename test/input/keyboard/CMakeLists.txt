target_sources(${TEST_NAME} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/test.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

target_link_libraries(${TEST_NAME} PRIVATE 
	input
)

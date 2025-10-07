target_sources(${TEST_NAME} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/test.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

target_link_libraries(${TEST_NAME} PRIVATE 
    renderer
)

list(APPEND CMAKE_CONFIGURE_DEPENDS 
	"${CMAKE_CURRENT_SOURCE_DIR}/*.shader"
	"${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.shader"
)
file(GLOB_RECURSE shader_files "${CMAKE_CURRENT_SOURCE_DIR}/*.shader")

if (shader_files)
	add_custom_target(${TEST_NAME}_copy_shaders
		COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${TEST_NAME}>
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
				${shader_files}
				$<TARGET_FILE_DIR:${TEST_NAME}>
		COMMENT "Copying shaders to $<TARGET_FILE_DIR:${TEST_NAME}>"
	)

	add_dependencies(${TEST_NAME} ${TEST_NAME}_copy_shaders)
else()
	message(FATAL_ERROR "Found no shaders")
endif()

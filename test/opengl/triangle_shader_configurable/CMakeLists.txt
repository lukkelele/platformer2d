target_sources(${TEST_NAME} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

target_link_libraries(${TEST_NAME} PRIVATE 
    renderer
)

add_custom_target(${TEST_NAME}_copy_shaders
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${TEST_NAME}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/vertex.shader
            $<TARGET_FILE_DIR:${TEST_NAME}>/vertex.shader
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/frag.shader
            $<TARGET_FILE_DIR:${TEST_NAME}>/frag.shader
    COMMENT "Copying shaders to $<TARGET_FILE_DIR:${TEST_NAME}>"
)

add_dependencies(${TEST_NAME} ${TEST_NAME}_copy_shaders)

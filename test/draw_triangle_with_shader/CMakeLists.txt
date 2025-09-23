target_sources(${PROJECT_NAME} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

add_custom_target(copy_shaders
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:platformer2d>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_LIST_DIR}/vertex.shader
            $<TARGET_FILE_DIR:platformer2d>/vertex.shader
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_LIST_DIR}/frag.shader
            $<TARGET_FILE_DIR:platformer2d>/frag.shader
    COMMENT "Copying shaders to build directory"
)

add_dependencies(platformer2d copy_shaders)

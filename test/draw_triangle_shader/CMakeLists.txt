message(STATUS "Building: ${TEST_SUITE} (${TEST_DIR})")
target_sources(${TEST_EXECUTABLE} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

target_link_libraries(${TEST_EXECUTABLE} PRIVATE 
    renderer
)

add_custom_target(copy_shaders
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${TEST_EXECUTABLE}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_LIST_DIR}/vertex.shader
            $<TARGET_FILE_DIR:${TEST_EXECUTABLE}>/vertex.shader
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_LIST_DIR}/frag.shader
            $<TARGET_FILE_DIR:${TEST_EXECUTABLE}>/frag.shader
    COMMENT "Copying shaders to build directory"
)

add_dependencies(${TEST_EXECUTABLE} copy_shaders)

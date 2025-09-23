###################################################################
# CMake extensions
###################################################################

function(add_subdirectory_ifdef feature_toggle source_dir) 
    if (${${feature_toggle}})
	    add_subdirectory(${source_dir} ${ARGN})
    endif()
endfunction()

function(add_subdirectory_ifndef feature_toggle source_dir)
    if (NOT ${feature_toggle})
        add_subdirectory(${source_dir} ${ARGN})
    endif()
endfunction()

function(target_sources_ifdef feature_toggle target scope item) 
    if (${${feature_toggle}})
        target_sources(${target} ${scope} ${item} ${ARGN})
    endif()
endfunction()

function(target_sources_ifndef feature_toggle target scope item)
    if (NOT ${feature_toggle})
        target_sources(${target} ${scope} ${item} ${ARGN})
    endif()
endfunction()

function(target_compile_definitions_ifdef feature_toggle target scope item) 
    if (${${feature_toggle}})
        target_compile_definitions(${target} ${scope} ${item} ${ARGN})
    endif()
endfunction()

function(target_include_directories_ifdef feature_toggle target scope item) 
    if (${${feature_toggle}})
        target_include_directories(${target} ${scope} ${item} ${ARGN})
    endif()
endfunction()

function(target_link_libraries_ifdef feature_toggle target item) 
    if(${${feature_toggle}})
        target_link_libraries(${target} ${item} ${ARGN})
    endif()
endfunction()

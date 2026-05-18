if(NOT TARGET ZLIB::ZLIB)
	message(FATAL_ERROR
		"FindZLIB shim: target ZLIB::ZLIB is not defined. "
		"Ensure external/zlib has been added via add_subdirectory and that "
		"its alias to zlibstatic has been created before any consumer calls "
		"find_package(ZLIB)")
endif()

set(ZLIB_FOUND TRUE)
set(ZLIB_INCLUDE_DIRS
	"${CMAKE_SOURCE_DIR}/external/zlib"
	"${CMAKE_BINARY_DIR}/external/zlib"
)
set(ZLIB_INCLUDE_DIR "${ZLIB_INCLUDE_DIRS}")
set(ZLIB_LIBRARIES ZLIB::ZLIB)
set(ZLIB_LIBRARY ZLIB::ZLIB)
set(ZLIB_VERSION_STRING "1.3.2")
set(ZLIB_VERSION "1.3.2")

# Try adding fmt as a git submodule first
if (EXISTS "${CMAKE_SOURCE_DIR}/vendor/fmt/CMakeLists.txt")
	set(FMT_TEST OFF CACHE BOOL "" FORCE)
	set(FMT_DOC OFF CACHE BOOL "" FORCE)
	set(FMT_INSTALL OFF CACHE BOOL "" FORCE)

	add_subdirectory(
		"${CMAKE_SOURCE_DIR}/vendor/fmt"
		"${CMAKE_BINARY_DIR}/vendor/fmt"
		EXCLUDE_FROM_ALL
	)
# Otherwise use FetchContent
else()
	message(STATUS "vendor/fmt not found - fetching via FetchContent")
	include (FetchContent)

	FetchContent_Declare(
		fmt
		GIT_REPOSITORY https://github.com/fmtlib/fmt
		GIT_TAG        e69e5f977d458f2650bb346dadf2ad30c5320281
	) # 10.2.1

	FetchContent_MakeAvailable(fmt)

endif()

add_library(imp_core STATIC
	src/core.cpp
	include/core/log/log.cpp
	include/core/fs/vfs.cpp
	include/core/platform/exe_path.cpp
)

target_include_directories(imp_core
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(imp_core
	PUBLIC
		imperium_compile_options
		fmt::fmt
)

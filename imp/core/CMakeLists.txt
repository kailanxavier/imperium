# Try adding fmt as a git submodule first
if (EXISTS "${CMAKE_SOURCE_DIR}/vendor/fmt/CMakeLists.txt")
	add_subdirectory(
		"${CMAKE_SOURCE_DIR}/vendor/fmt"
		"${CMAKE_SOURCE_BIN}/vendor/fmt"
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
 "include/core/math/vec2.h" "include/core/math/vec3.h" "include/core/math/vec4.h" "include/core/math/mat4.h" "include/core/math/mat3.h" "include/core/math/math.h" "include/core/log/log.cpp" "include/core/log/console_sink.h" "include/core/log/file_sink.h"   "include/core/memory/iallocator.h" "include/core/memory/allocator_types.h" "include/core/memory/allocator_utils.h" "include/core/memory/heap_allocator.h" "include/core/memory/linear_allocator.h" "include/core/memory/stack_allocator.h" "include/core/memory/pool_allocator.h")

target_include_directories(imp_core
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(imp_core
	PUBLIC 
		imperium_compile_options
		fmt::fmt
)
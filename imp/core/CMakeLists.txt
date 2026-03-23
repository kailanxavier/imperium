add_library(imp_core STATIC
	src/core.cpp
 "include/core/math/vec2.h" "include/core/math/vec3.h" "include/core/math/vec4.h" "include/core/math/mat4.h" "include/core/math/mat3.h" "include/core/math/math.h")

target_include_directories(imp_core
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_directories(imp_core
	PUBLIC imperium_compile_options
)
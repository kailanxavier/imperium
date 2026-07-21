add_executable(sandbox 
	src/main.cpp
)

target_link_libraries(sandbox PRIVATE imp_fwk imp_gfx imp_protocol)

imp_set_runtime_dir(sandbox sandbox)

IMP_COMPILE_SHADERS(
		sandbox
		"${IMP_OUTPUT_DIR}/bin/sandbox/shaders"

		${CMAKE_CURRENT_SOURCE_DIR}/shaders/vulkan/cube.frag
		${CMAKE_CURRENT_SOURCE_DIR}/shaders/vulkan/cube.vert
)

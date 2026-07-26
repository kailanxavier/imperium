add_executable(sandbox 
	src/main.cpp
)

target_link_libraries(sandbox PRIVATE imp_fwk imp_cam imp_input imp_gfx imp_protocol)

imp_set_runtime_dir(sandbox sandbox)

IMP_COMPILE_SHADERS(
		sandbox
		"${IMP_OUTPUT_DIR}/bin/sandbox/assets/shaders"

		${CMAKE_CURRENT_SOURCE_DIR}/shaders/vulkan/mesh.frag
		${CMAKE_CURRENT_SOURCE_DIR}/shaders/vulkan/mesh.vert
)

IMP_COPY_ASSETS(sandbox)

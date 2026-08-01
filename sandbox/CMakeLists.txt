add_executable(sandbox 
	src/main.cpp
	src/sandbox_app.cpp
)

target_include_directories(sandbox
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(sandbox PUBLIC imp_app PRIVATE imp_fwk imp_cam imp_input imp_gfx imp_protocol)

imp_set_runtime_dir(sandbox sandbox)

IMP_COMPILE_SHADERS(
		sandbox
		"${IMP_OUTPUT_DIR}/bin/sandbox/assets/shaders"

		${CMAKE_CURRENT_SOURCE_DIR}/shaders/vulkan/mesh.frag
		${CMAKE_CURRENT_SOURCE_DIR}/shaders/vulkan/mesh.vert
		${CMAKE_CURRENT_SOURCE_DIR}/shaders/vulkan/tonemap.vert
		${CMAKE_CURRENT_SOURCE_DIR}/shaders/vulkan/tonemap.frag
		${CMAKE_CURRENT_SOURCE_DIR}/shaders/vulkan/shadow.vert
		${CMAKE_CURRENT_SOURCE_DIR}/shaders/vulkan/shadow.frag
)

IMP_COPY_ASSETS(sandbox)

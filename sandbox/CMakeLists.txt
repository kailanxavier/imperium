add_executable(sandbox 
	src/main.cpp
	src/sandbox_app.cpp
	src/render_resources.cpp
	src/sandbox_scene.cpp
	src/scene_renderer.cpp
)

target_include_directories(sandbox
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(sandbox PUBLIC imp_app PRIVATE imp_script imp_fwk imp_cam imp_input imp_gfx imp_protocol)

imp_set_runtime_dir(sandbox sandbox)

IMP_COMPILE_SHADERS(
		sandbox
		"${IMP_OUTPUT_DIR}/bin/sandbox/assets/shaders"

		${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/vulkan/mesh.frag
		${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/vulkan/mesh.vert
		${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/vulkan/tonemap.vert
		${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/vulkan/tonemap.frag
		${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/vulkan/shadow.vert
		${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/vulkan/shadow.frag
		${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/vulkan/sky.vert
		${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/vulkan/sky.frag
		${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/vulkan/gizmo.vert
		${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/vulkan/gizmo.frag
)

IMP_COPY_ASSETS(sandbox)

target_compile_definitions(sandbox PRIVATE
		IMP_HOT_RELOAD_ENABLED=1
		IMP_SHADER_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/vulkan/"
		IMP_SCRIPT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets/scripts/"
)

if (GLSLC_EXECUTABLE)
	target_compile_definitions(sandbox PRIVATE
			IMP_SHADER_COMPILER_PATH="${GLSLC_EXECUTABLE}"
			IMP_SHADER_COMPILER_IS_GLSLANG_VALIDATOR=0
	)
else()
	target_compile_definitions(sandbox PRIVATE
			IMP_SHADER_COMPILER_PATH="${GLSLANG_VALIDATOR_EXECUTABLE}"
			IMP_SHADER_COMPILER_IS_GLSLANG_VALIDATOR=1
	)
endif()

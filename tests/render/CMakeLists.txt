add_executable(render_smoke_test
        src/main.cpp
        ${CMAKE_SOURCE_DIR}/sandbox/src/render_resources.cpp
        ${CMAKE_SOURCE_DIR}/sandbox/src/sandbox_scene.cpp
        ${CMAKE_SOURCE_DIR}/sandbox/src/scene_renderer.cpp
        ${CMAKE_SOURCE_DIR}/sandbox/src/sandbox_app.cpp
)

target_include_directories(render_smoke_test
        PRIVATE
            ${CMAKE_SOURCE_DIR}/sandbox/include
            ${CMAKE_SOURCE_DIR}/sandbox/src
)

target_link_libraries(render_smoke_test PUBLIC imp_app PRIVATE imp_script imp_fwk imp_cam imp_input imp_gfx imp_protocol)

imp_set_runtime_dir(render_smoke_test tests/render_smoke_test)

IMP_COMPILE_SHADERS(
        render_smoke_test
        "${IMP_OUTPUT_DIR}/bin/tests/render_smoke_test/assets/shaders"
        ${CMAKE_SOURCE_DIR}/sandbox/assets/shaders/vulkan/mesh.frag
        ${CMAKE_SOURCE_DIR}/sandbox/assets/shaders/vulkan/mesh.vert
        ${CMAKE_SOURCE_DIR}/sandbox/assets/shaders/vulkan/tonemap.vert
        ${CMAKE_SOURCE_DIR}/sandbox/assets/shaders/vulkan/tonemap.frag
        ${CMAKE_SOURCE_DIR}/sandbox/assets/shaders/vulkan/shadow.vert
        ${CMAKE_SOURCE_DIR}/sandbox/assets/shaders/vulkan/shadow.frag
        ${CMAKE_SOURCE_DIR}/sandbox/assets/shaders/vulkan/sky.vert
        ${CMAKE_SOURCE_DIR}/sandbox/assets/shaders/vulkan/sky.frag
)

file(COPY ${CMAKE_SOURCE_DIR}/sandbox/assets/models DESTINATION ${IMP_OUTPUT_DIR}/bin/tests/render_smoke_test/assets)
file(COPY ${CMAKE_SOURCE_DIR}/tests/render/reference DESTINATION ${IMP_OUTPUT_DIR}/bin/tests/render_smoke_test)

enable_testing()
add_test(NAME render_smoke_test COMMAND render_smoke_test WORKING_DIRECTORY ${IMP_OUTPUT_DIR}/bin/tests/render_smoke_test)

target_compile_definitions(render_smoke_test PRIVATE
		IMP_HOT_RELOAD_ENABLED=1
		IMP_SHADER_SOURCE_DIR="${CMAKE_SOURCE_DIR}/sandbox/assets/shaders/vulkan/"
		IMP_SCRIPT_SOURCE_DIR="${CMAKE_SOURCE_DIR}/sandbox/assets/scripts/"
)

if (GLSLC_EXECUTABLE)
	target_compile_definitions(render_smoke_test PRIVATE
			IMP_SHADER_COMPILER_PATH="${GLSLC_EXECUTABLE}"
			IMP_SHADER_COMPILER_IS_GLSLANG_VALIDATOR=0
	)
else()
	target_compile_definitions(render_smoke_test PRIVATE
			IMP_SHADER_COMPILER_PATH="${GLSLANG_VALIDATOR_EXECUTABLE}"
			IMP_SHADER_COMPILER_IS_GLSLANG_VALIDATOR=1
	)
endif()

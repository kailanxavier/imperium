add_executable(sandbox 
	src/main.cpp
	src/sandbox_app.cpp
	src/sandbox_scene.cpp
)

target_include_directories(sandbox
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(sandbox 
	PUBLIC 
		imp_app 
	PRIVATE 
		imp_engine 
		imp_script 
		imp_fwk 
		imp_cam 
		imp_input 
		imp_gfx 
		imp_protocol
)

imp_set_runtime_dir(sandbox sandbox)
IMP_ENGINE_DEPLOY_SHADERS(sandbox)
IMP_COPY_ASSETS(sandbox)

target_compile_definitions(sandbox PRIVATE
		IMP_HOT_RELOAD_ENABLED=1
		IMP_SHADER_SOURCE_DIR="${IMP_ENGINE_SOURCE_DIR}/assets/shaders/vulkan/"
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

set(IMP_ENGINE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "")

add_library(imp_engine STATIC
	src/render_resources.cpp
	src/scene_renderer.cpp
	src/engine_assets.cpp
)

target_include_directories(imp_engine
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(imp_engine
	PUBLIC
		imp_app
		imp_gfx
		imp_cam
		imp_sky
	PRIVATE
		imp_core
)

target_compile_definitions(imp_engine 
	PRIVATE
		IMP_HOT_RELOAD_ENABLED=1
		IMP_SHADER_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/vulkan/"
)

if (GLSLC_EXECUTABLE)
	target_compile_definitions(imp_engine
		PRIVATE
			IMP_SHADER_COMPILER_PATH="${GLSLC_EXECUTABLE}"
			IMP_SHADER_COMPILER_IS_GLSLANG_VALIDATOR=0
	)
else()
	target_compile_definitions(imp_engine
		PRIVATE
			IMP_SHADER_COMPILER_PATH="${GLSLANG_VALIDATOR_EXECUTABLE}"
			IMP_SHADER_COMPILER_IS_GLSLANG_VALIDATOR=1
	)
endif()

add_library(game_lib STATIC
	src/game_app.cpp
	src/game_scene.cpp
	src/light_control_layer.cpp
)

target_include_directories(game_lib
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(game_lib 
	PUBLIC 
		imp_app 
		imp_engine 
		imp_script 
		imp_scene
		imp_fwk 
		imp_cam 
		imp_input 
		imp_gfx 
	PRIVATE 
		imp_protocol
)

target_compile_definitions(game_lib
	PRIVATE 
		IMP_SCRIPT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets/scripts/"
)

add_executable(game
	src/main.cpp
)

target_link_libraries(game
	PRIVATE
		game_lib
		imp_app_editor_host
		imp_app_sky
		imp_app_debug
		imp_protocol
)

target_compile_definitions(game
	PRIVATE 
		IMP_SCENE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/scenes/"
)

if (TARGET imp_editor)
	target_compile_definitions(game
		PRIVATE
			IMP_EDITOR_PATH="$<TARGET_FILE:imp_editor>"
	)

	add_dependencies(game imp_editor)
endif()

imp_set_runtime_dir(game game)
IMP_ENGINE_DEPLOY_SHADERS(game)
IMP_COPY_ASSETS(game)
IMP_COPY_SCENES(game)

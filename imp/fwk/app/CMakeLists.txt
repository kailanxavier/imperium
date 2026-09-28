add_library(imp_app STATIC 
	src/application.cpp
	src/launch_options.cpp
)

target_include_directories(imp_app
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(imp_app 
	PUBLIC 
		imp_jobs 
		imp_ecs
	PRIVATE 
		imp_gfx 
		imp_fwk
		imp_input 
		imp_core 
)

target_link_libraries(imp_app PRIVATE imgui glfw)

target_sources(imp_app PRIVATE
    ${CMAKE_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_glfw.cpp
)

add_library(imp_app_editor_host STATIC
	src/editor_bridge_layer.cpp
)

target_include_directories(imp_app_editor_host
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(imp_app_editor_host
	PUBLIC
		imp_app
		imp_scene
		imp_protocol
	PRIVATE
		imp_gfx
		imp_jobs
		imp_core
)

add_library(imp_app_debug STATIC
	src/telemetry_layer.cpp
	src/gizmo_layer.cpp
)

target_include_directories(imp_app_debug
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(imp_app_debug
	PUBLIC
		imp_app
		imp_physics
	PRIVATE
		imp_protocol
		imp_gfx
		imp_cam
		imp_input
		imp_core
)

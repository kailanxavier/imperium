add_library(imp_app STATIC 
	src/application.cpp
	src/telemetry_layer.cpp
	src/light_control_layer.cpp
	src/gizmo_layer.cpp
)

target_include_directories(imp_app
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(imp_app PUBLIC imp_jobs imp_ecs PRIVATE imp_protocol imp_core imp_gfx imp_input imp_cam)

target_link_libraries(imp_app PRIVATE imgui glfw)

target_sources(imp_app PRIVATE
    ${CMAKE_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_glfw.cpp
)

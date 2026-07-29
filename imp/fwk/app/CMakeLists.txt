add_library(imp_app STATIC 
	src/application.cpp
	src/telemetry_layer.cpp
	src/render_extraction.cpp
)

target_include_directories(imp_app
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(imp_app PUBLIC imp_jobs imp_ecs PRIVATE imp_protocol imp_core imp_gfx imp_input)

add_library(imp_scene STATIC 
	src/scene.cpp
)

target_include_directories(imp_scene
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(imp_scene PUBLIC imp_core imp_ecs PRIVATE nlohmann_json::nlohmann_json)

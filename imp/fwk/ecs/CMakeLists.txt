add_library(imp_ecs STATIC 
	src/entity.cpp
	src/transform_storage.cpp
	src/renderable_storage.cpp
	src/light_storage.cpp
	src/collider_storage.cpp
	src/name_storage.cpp
)

target_include_directories(imp_ecs
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(imp_ecs PUBLIC imp_core imp_physics imp_jobs)

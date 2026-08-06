add_library(imp_physics STATIC 
	src/physics.cpp
	src/aabb.cpp
	src/aabb_collider.cpp
 	src/query.cpp
)

target_include_directories(imp_physics
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(imp_physics PUBLIC imp_core PRIVATE imp_ecs)

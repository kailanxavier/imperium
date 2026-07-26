add_library(imp_ecs STATIC 
	src/entity.cpp
)

target_include_directories(imp_ecs
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(imp_ecs PRIVATE imp_core)

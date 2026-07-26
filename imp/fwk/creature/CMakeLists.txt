add_library(imp_creature STATIC 
	src/creature.cpp
)

target_include_directories(imp_creature
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(imp_creature
	PUBLIC imp_core imp_physics imp_script
)
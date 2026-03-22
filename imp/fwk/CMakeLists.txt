add_library(imp_fwk STATIC
	src/fwk.cpp
)

target_include_directories(imp_fwk
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(imp_fwk
	PUBLIC
		imp_core
		imp_gfx
		imp_audio
		imp_physics
		imp_net
		imp_script
		imp_creature
)
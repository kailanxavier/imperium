add_library(imp_sky STATIC
	src/sky_system.cpp
	src/sky_cvars.cpp
)

target_include_directories(imp_sky
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(imp_sky
	PUBLIC
		imp_core
)

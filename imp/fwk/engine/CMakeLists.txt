set(IMP_ENGINE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "")

add_library(imp_engine STATIC
	src/render_resources.cpp
	src/scene_renderer.cpp
)

target_include_directories(imp_engine
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(imp_engine
	PUBLIC
		imp_app
		imp_gfx
		imp_cam
	PRIVATE
		imp_core
)



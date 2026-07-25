add_library(imp_app STATIC src/app.cpp)

target_include_directories(imp_app
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(imp_app PRIVATE imp_core imp_gfx imp_fwk)

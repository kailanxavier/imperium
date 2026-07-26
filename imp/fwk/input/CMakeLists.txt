add_library(imp_input STATIC 
	src/input.cpp
)

target_include_directories(imp_input
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(imp_input PRIVATE imp_core glfw)

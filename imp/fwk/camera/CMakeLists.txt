add_library(imp_cam STATIC 
	src/camera.cpp
)

target_include_directories(imp_cam
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(imp_cam PRIVATE imp_core imp_input)

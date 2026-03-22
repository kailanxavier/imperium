add_library(imp_audio STATIC 
	src/audio.cpp
)

target_include_directories(imp_audio
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(imp_audio PUBLIC imp_core)
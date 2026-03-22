add_library(imp_script STATIC src/script.cpp)
target_include_directories(imp_script
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(imp_script PUBLIC imp_core)

# lua goes here as submodule when i actually implement it
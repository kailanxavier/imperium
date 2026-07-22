add_executable(imp_perlingen 
	src/main.cpp
	src/perlingen.cpp
)

target_link_libraries(imp_perlingen PRIVATE imp_core)

target_include_directories(imp_perlingen
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

IMP_SET_RUNTIME_DIR(imp_perlingen tools)

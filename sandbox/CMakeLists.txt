add_executable(sandbox 
	src/main.cpp
)

target_link_libraries(sandbox PRIVATE imp_fwk imp_gfx imp_protocol)
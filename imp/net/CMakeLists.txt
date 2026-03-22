add_library(imp_net STATIC src/net.cpp)
target_include_directories(imp_net
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(imp_net PUBLIC imp_core)
if(WIN32)
	target_link_libraries(imp_net PRIVATE ws2_32)
endif()
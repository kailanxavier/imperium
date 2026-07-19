add_executable(test_tool_server 
	test_tool_server.cpp
)

target_link_libraries(test_tool_server PRIVATE imp_protocol GTest::gtest_main imperium_compile_options)

include(GoogleTest)

add_test(NAME test_tool_server COMMAND test_tool_server)

imp_set_runtime_dir(test_tool_server tests)

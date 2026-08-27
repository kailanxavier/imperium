add_executable(imp_script_tests
	vm_tests.cpp
	system_tests.cpp
	binding_tests.cpp
	file_watcher_tests.cpp
	script_status_tests.cpp
	world_snapshot_tests.cpp
)

target_link_libraries(imp_script_tests
	PRIVATE
		imp_script
		imp_ecs
		imp_core
		imp_protocol
		sol2::sol2
		lua_lib
		GTest::gtest_main
		imperium_compile_options
)

include(GoogleTest)

add_test(
	NAME imp_script_tests
	COMMAND imp_script_tests
)

target_include_directories(
	imp_script_tests
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}
)

IMP_SET_RUNTIME_DIR(imp_script_tests tests)

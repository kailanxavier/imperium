add_executable(entity_test 
	entity_tests.cpp
)

target_link_libraries(entity_test PRIVATE imp_ecs GTest::gtest_main imperium_compile_options)

include(GoogleTest)

add_test(NAME entity_test COMMAND entity_test)

imp_set_runtime_dir(entity_test tests)

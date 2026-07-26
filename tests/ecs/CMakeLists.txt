add_executable(imp_ecs_tests 
	entity_tests.cpp
	transform_storage_tests.cpp
)

target_link_libraries(imp_ecs_tests  PRIVATE imp_ecs GTest::gtest_main imperium_compile_options)

include(GoogleTest)

add_test(NAME imp_ecs_tests COMMAND imp_ecs_tests )

target_include_directories(imp_ecs_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

imp_set_runtime_dir(imp_ecs_tests tests)

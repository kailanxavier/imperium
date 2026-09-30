add_executable(imp_sky_tests
	test_sky_system.cpp
	test_sky_cvars.cpp
 "test_sky_system.cpp")

target_link_libraries(imp_sky_tests PRIVATE imp_sky GTest::gtest_main imperium_compile_options)
include(GoogleTest)
add_test(NAME imp_sky_tests COMMAND imp_sky_tests)

IMP_SET_RUNTIME_DIR(imp_sky_tests tests)

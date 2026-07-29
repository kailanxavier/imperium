add_executable(imp_jobs_tests 
	jobs_tests.cpp
)

target_link_libraries(imp_jobs_tests PRIVATE imp_jobs GTest::gtest_main imperium_compile_options)

include(GoogleTest)

add_test(NAME imp_jobs_tests COMMAND imp_jobs_tests )

target_include_directories(imp_jobs_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

imp_set_runtime_dir(imp_jobs_tests tests)

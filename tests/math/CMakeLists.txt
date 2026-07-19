# Add test targets
add_executable(imp_math_tests
    test_vec2.cpp
    test_vec3.cpp
    test_vec4.cpp
    test_mat3.cpp
    test_mat4.cpp
    test_quaternion.cpp
)

target_include_directories(imp_math_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(imp_math_tests PRIVATE
    imp_core
    GTest::gtest_main
)

# We want to inherit the same warning flags as the rest of the engine
target_link_libraries(imp_math_tests PRIVATE imperium_compile_options)

include(GoogleTest)
add_test(NAME imp_math_tests COMMAND imp_math_tests)

imp_set_runtime_dir(imp_math_tests tests)

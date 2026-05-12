cmake_minimum_required(VERSION 3.25)

# Try adding as a git submodule first, as this this does not require internet connection at build time
if (EXISTS "${CMAKE_SOURCE_DIR}/vendor/googletest/CMakeLists.txt")
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    add_subdirectory("${CMAKE_SOURCE_DIR}/vendor/googletest"
                     "${CMAKE_BINARY_DIR}/vendor/googletest"
                     EXCLUDE_FROM_ALL
    )
# If that doesn't work we use FetchContent, which does require internet connection at build time
else()
    message(STATUS "vendor/googletest not found - fetching via FetchContent")
    include(FetchContent)
    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.14.0
    )
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# Add test targets
add_executable(imp_math_tests
    test_vec2.cpp
    test_vec3.cpp
    test_vec4.cpp
    #test_mat.cpp
    #test_quaternion.cpp
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

# Register with GTest
include(GoogleTest)
gtest_discover_tests(imp_math_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    PROPERTIES TIMEOUT 30
)
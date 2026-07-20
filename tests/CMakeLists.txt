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

IMP_ADD_THIRD_PARTY_TARGET(gtest)
IMP_ADD_THIRD_PARTY_TARGET(gtest_main)

add_subdirectory(math)
add_subdirectory(protocol)

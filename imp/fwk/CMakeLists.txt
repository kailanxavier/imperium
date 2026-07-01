# First try using the submodule
if (EXISTS "${CMAKE_SOURCE_DIR}/vendor/glfw/CMakeLists.txt")
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

    add_subdirectory(
        "${CMAKE_SOURCE_DIR}/vendor/glfw"
        "${CMAKE_BINARY_DIR}/vendor/glfw"
        EXCLUDE_FROM_ALL
    )

# Otherwise fetch it
else()
    message(STATUS "vendor/glfw not found - fetching via FetchContent")

    include(FetchContent)

    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
    )

    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(glfw)
endif()

add_library(imp_fwk STATIC
	src/fwk.cpp
 "include/fwk/window.h" "src/window.cpp")

target_include_directories(imp_fwk
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(imp_fwk
	PUBLIC
		imp_core
		imp_audio
		imp_physics
		imp_net
		imp_script
		imp_creature
        glfw
)
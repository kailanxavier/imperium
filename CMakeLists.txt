cmake_minimum_required(VERSION 3.25)
project(Imperium VERSION 0.1.0 LANGUAGES C CXX)

find_package(Doxygen QUIET)

if (DOXYGEN_FOUND)
    configure_file(
        ${CMAKE_SOURCE_DIR}/Doxyfile.in
        ${CMAKE_BINARY_DIR}/Doxyfile
        @ONLY
    )

    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating docs..."
        VERBATIM
    )
else()
    message(STATUS "Doxygen not found. Documentation will not be generated")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(IMP_OUTPUT_DIR "${CMAKE_BINARY_DIR}/engine")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${IMP_OUTPUT_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${IMP_OUTPUT_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${IMP_OUTPUT_DIR}/bin)

foreach(CONFIG Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER ${CONFIG} CONFIG_UPPER)

    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${IMP_OUTPUT_DIR}/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${IMP_OUTPUT_DIR}/lib)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${IMP_OUTPUT_DIR}/bin)
endforeach()

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

add_library(imperium_compile_options INTERFACE
        tests/math/math_test_helper.h)

find_package(Vulkan REQUIRED)
add_library(imgui STATIC
        vendor/imgui/imgui.cpp
        vendor/imgui/imgui_draw.cpp
        vendor/imgui/imgui_tables.cpp
        vendor/imgui/imgui_widgets.cpp
)

target_include_directories(imgui
        PUBLIC vendor/imgui
)

target_sources(imgui PRIVATE
        vendor/imgui/backends/imgui_impl_glfw.cpp
        vendor/imgui/backends/imgui_impl_vulkan.cpp
)

target_link_libraries(imgui PRIVATE Vulkan::Vulkan glfw)

target_compile_options(imperium_compile_options INTERFACE
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4 /WX /MP /permissive- /Zc:__cplusplus /wd4201 /Zc:preprocessor /wd4324 /wd4100
    >

    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<NOT:$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>>>:
        -Wall -Wextra -Werror -Wpedantic
    >

    $<$<CXX_COMPILER_ID:GNU>:
        -Wall -Wextra -Werror -Wpedantic
    >
)

include(cmake/AddThirdParty.cmake)
include(cmake/AddImageLibs.cmake)
include(cmake/CompileShaders.cmake)
include(cmake/CopyAssets.cmake)

function(imp_set_runtime_dir TARGET SUBDIR)
    set(OUTPUT_DIR "${IMP_OUTPUT_DIR}/bin/${SUBDIR}")
    file(MAKE_DIRECTORY "${OUTPUT_DIR}")

    set_target_properties(${TARGET}
            PROPERTIES

            RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIR}"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIR}"
            RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${OUTPUT_DIR}"
            RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${OUTPUT_DIR}"

            DEBUG_POSTFIX "_d"
    )

endfunction()

if (EXISTS "${CMAKE_SOURCE_DIR}/vendor/json/CMakeLists.txt")
    set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
    set(JSON_Install OFF CACHE BOOL "" FORCE)
 
    add_subdirectory(
        "${CMAKE_SOURCE_DIR}/vendor/json"
        "${CMAKE_BINARY_DIR}/vendor/json"
        EXCLUDE_FROM_ALL
    )
else()
    message(STATUS "vendor/json not found - fetching via FetchContent")
 
    include(FetchContent)
 
    FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
    )
 
    set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
    set(JSON_Install OFF CACHE BOOL "" FORCE)
 
    FetchContent_MakeAvailable(json)
endif()


add_subdirectory(protocol)
add_subdirectory(imp)
add_subdirectory(tools)
add_subdirectory(sandbox)
add_subdirectory(tests)

IMP_ADD_THIRD_PARTY_TARGET(imgui)
IMP_ADD_THIRD_PARTY_TARGET(glfw)

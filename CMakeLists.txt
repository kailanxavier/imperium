cmake_minimum_required(VERSION 3.25)
project(Imperium VERSION 0.1.0 LANGUAGES CXX)

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
    $<$<CXX_COMPILER_ID:GNU,Clang>:
        -Wall -Wextra -Werror -Wpedantic
    >
)

include(cmake/AddThirdParty.cmake)
include(cmake/CompileShaders.cmake)

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

add_subdirectory(protocol)
add_subdirectory(imp)
add_subdirectory(tools)
add_subdirectory(sandbox)
add_subdirectory(tests)

IMP_ADD_THIRD_PARTY_TARGET(imgui)
IMP_ADD_THIRD_PARTY_TARGET(glfw)

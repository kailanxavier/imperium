cmake_minimum_required(VERSION 3.25)
project(Imperium VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_library(imperium_compile_options INTERFACE
        tests/math/math_test_helper.h)
target_compile_options(imperium_compile_options INTERFACE
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4 /WX /MP /permissive- /Zc:__cplusplus /wd4201 /Zc:preprocessor
    >
    $<$<CXX_COMPILER_ID:GNU,Clang>:
        -Wall -Wextra -Werror -Wpedantic
    >
)

set(IMP_GFX_BACKEND "VULKAN" CACHE STRING "Graphics backend: DX11, DX12, VULKAN")
set_property(CACHE IMP_GFX_BACKEND PROPERTY STRINGS DX11 DX12 VULKAN)
message(STATUS "Graphics backend: ${IMP_GFX_BACKEND}")

add_subdirectory(imp)
add_subdirectory(sandbox)
add_subdirectory(tests)
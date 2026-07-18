add_executable(imp_console
    src/console.cpp
)

target_include_directories(imp_console
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)
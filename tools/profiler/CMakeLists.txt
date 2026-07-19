add_executable(imp_profiler
        src/profiler.cpp
)

target_include_directories(imp_profiler
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

imp_set_runtime_dir(imp_profiler tools)

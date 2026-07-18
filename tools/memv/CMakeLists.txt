add_executable(imp_memv
        src/memv.cpp
)

target_include_directories(imp_memv
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(imp_memv
    PRIVATE imp_protocol
)

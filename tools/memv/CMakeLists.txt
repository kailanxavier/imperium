add_executable(imp_memv
        src/memv.cpp
        src/memv_app.cpp
)

imp_set_runtime_dir(imp_memv tools)

target_include_directories(imp_memv
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(imp_memv
    PRIVATE imp_protocol imp_fwk imp_gfx imp_core imgui glfw
)

if (WIN32)
    set_target_properties(imp_memv PROPERTIES
        WIN32_EXECUTABLE TRUE
    )

    target_link_options(imp_memv PRIVATE
        "/ENTRY:mainCRTStartup"
    )
elseif(APPLE)
    set_target_properties(imp_memv PROPERTIES
        MACOSX_BUNDLE TRUE
    )
endif()

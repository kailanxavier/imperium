add_executable(render_smoke_test
        src/main.cpp
)
target_link_libraries(render_smoke_test PUBLIC imp_app PRIVATE game_lib imp_engine imp_script imp_fwk imp_cam imp_input imp_gfx imp_protocol)

imp_set_runtime_dir(render_smoke_test tests/render_smoke_test)

IMP_ENGINE_DEPLOY_SHADERS(render_smoke_test)

file(COPY ${CMAKE_SOURCE_DIR}/game/assets/models DESTINATION ${IMP_OUTPUT_DIR}/bin/tests/render_smoke_test/assets)
file(COPY ${CMAKE_SOURCE_DIR}/tests/render/reference DESTINATION ${IMP_OUTPUT_DIR}/bin/tests/render_smoke_test)

enable_testing()
add_test(NAME render_smoke_test COMMAND render_smoke_test WORKING_DIRECTORY ${IMP_OUTPUT_DIR}/bin/tests/render_smoke_test)

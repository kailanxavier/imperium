find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(CMAKE_AUTOMOC ON)
add_executable(imp_editor
	src/main.cpp
	src/main_window.cpp
	src/engine_connection.cpp
	src/hierarchy_panel.cpp
	src/inspector_panel.cpp
    src/world_model.cpp
    src/entity_tree_view.cpp
	src/asset_model.cpp
	src/asset_browser_panel.cpp
	src/cvar_panel.cpp

	include/editor/main_window.h
    include/editor/engine_connection.h
    include/editor/hierarchy_panel.h
    include/editor/inspector_panel.h
    include/editor/world_model.h
    include/editor/entity_tree_view.h
	include/editor/asset_model.h
	include/editor/asset_browser_panel.h
	include/editor/cvar_panel.h
)

IMP_SET_RUNTIME_DIR(imp_editor tools/editor)

target_include_directories(imp_editor
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(imp_editor
	PRIVATE 
		imp_protocol 
		imp_core 
		Qt6::Widgets
)

if (WIN32)
    set_target_properties(imp_editor PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
elseif(APPLE)
    set_target_properties(imp_editor PROPERTIES
        MACOSX_BUNDLE TRUE
    )
endif()

if(WIN32)
	add_custom_command(TARGET imp_editor POST_BUILD

			COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"$<TARGET_FILE:Qt6::Core>"
			"$<TARGET_FILE_DIR:imp_editor>"

			COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"$<TARGET_FILE:Qt6::Gui>"
			"$<TARGET_FILE_DIR:imp_editor>"

			COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"$<TARGET_FILE:Qt6::Widgets>"
			"$<TARGET_FILE_DIR:imp_editor>"

			COMMAND ${CMAKE_COMMAND} -E make_directory
			"$<TARGET_FILE_DIR:imp_editor>/platforms"

			COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/Qt6/plugins/platforms/qwindows.dll"
			"$<TARGET_FILE_DIR:imp_editor>/platforms"
	)
elseif(APPLE)
	add_custom_command(TARGET imp_editor POST_BUILD
			COMMAND macdeployqt
			"$<TARGET_BUNDLE_DIR:imp_editor>"
	)
endif()


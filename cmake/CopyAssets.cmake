function(imp_copy_assets TARGET_NAME)
	if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
		add_custom_command(
			TARGET ${TARGET_NAME}
			POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_directory
				"${CMAKE_CURRENT_SOURCE_DIR}/assets"
				"$<TARGET_FILE_DIR:${TARGET_NAME}>/assets"
			COMMENT "Copying assets to ${TARGET_NAME}/assets"
		)
	endif()
endfunction()

function(imp_copy_scenes TARGET_NAME)
	if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/scenes")
		add_custom_command(
			TARGET ${TARGET_NAME}
			POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_directory
				"${CMAKE_CURRENT_SOURCE_DIR}/scenes"
				"$<TARGET_FILE_DIR:${TARGET_NAME}>/scenes"
			COMMENT "Copying scenes to ${TARGET_NAME}/scenes"
		)
	endif()
endfunction()

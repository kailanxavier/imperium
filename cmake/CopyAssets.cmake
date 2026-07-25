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

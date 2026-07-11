find_program(GLSLC_EXECUTABLE glslc)
find_program(GLSLANG_VALIDATOR_EXECUTABLE glslangValidator)

if(NOT GLSLC_EXECUTABLE AND NOT GLSLANG_VALIDATOR_EXECUTABLE)
	message(FATAL_ERROR
		"No GLSL->SPIR-V compiler found. Install one to continue")
endif()

function(imp_compile_shaders TARGET OUTPUT_DIR)
	set(spirv_files "")

	foreach(shader_source ${ARGN})
		get_filename_component(shader_name ${shader_source} NAME)
		set(spirv_output "${OUTPUT_DIR}/${shader_name}.spv")

		if (GLSLC_EXECUTABLE)
			add_custom_command(
				OUTPUT ${spirv_output}
				COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
				COMMAND ${GLSLC_EXECUTABLE} ${shader_source} -o ${spirv_output}
				DEPENDS ${shader_source}
				COMMENT "Compiling shader ${shader_name}"
				VERBATIM
			)
		else()
			add_custom_command(
				OUTPUT ${spirv_output}
				COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
				COMMAND ${GLSLANG_VALIDATOR_EXECUTABLE} -V ${shader_source} -o ${spirv_output}
				DEPENDS ${shader_source}
				COMMENT "Compiling shader ${shader_name} (glslangValidator)"
				VERBATIM
			)
		endif()

		list(APPEND spirv_files ${spirv_output})
	endforeach()

	set(shader_target "${TARGET}_shaders")
	add_custom_target(${shader_target} DEPENDS ${spirv_files})
	add_dependencies(${TARGET} ${shader_target})
endfunction()
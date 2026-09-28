function(imp_engine_deploy_shaders TARGET)
	set(engine_shader_dir "${IMP_ENGINE_SOURCE_DIR}/assets/shaders/vulkan")

	IMP_COMPILE_SHADERS(
		${TARGET}
		"$<TARGET_FILE_DIR:${TARGET}>/engine/shaders"

		${engine_shader_dir}/mesh.frag
		${engine_shader_dir}/mesh.vert
		${engine_shader_dir}/tonemap.vert
		${engine_shader_dir}/tonemap.frag
		${engine_shader_dir}/shadow.vert
		${engine_shader_dir}/shadow.frag
		${engine_shader_dir}/sky.vert
		${engine_shader_dir}/sky.frag
		${engine_shader_dir}/gizmo.vert
		${engine_shader_dir}/gizmo.frag
		${engine_shader_dir}/depth_normal_prepass.vert
		${engine_shader_dir}/depth_normal_prepass.frag
		${engine_shader_dir}/gtao.frag
		${engine_shader_dir}/bilateral_blur.frag
		${engine_shader_dir}/gbuffer_debug.frag
		${engine_shader_dir}/deferred_lighting.frag
		${engine_shader_dir}/taa_resolve.frag
		${engine_shader_dir}/ssgi.frag
		${engine_shader_dir}/ssgi_blur.frag
		${engine_shader_dir}/ddgi_probe_update.comp
		${engine_shader_dir}/ddgi_ray_trace.comp
		${engine_shader_dir}/ddgi_thermal_update.comp
		${engine_shader_dir}/ddgi_classify_probes.comp
		${engine_shader_dir}/thermal_update_fallback.comp
		${engine_shader_dir}/fullscreen.vert

		# DDGI probe dbg
		${engine_shader_dir}/ddgi_debug_probes.vert
		${engine_shader_dir}/ddgi_debug_probes.frag
		${engine_shader_dir}/ddgi_debug_rays.vert
		${engine_shader_dir}/ddgi_debug_rays.frag

		${engine_shader_dir}/bloom_down_sample.frag
		${engine_shader_dir}/bloom_up_sample.frag
	)

endfunction()

add_library(imp_gfx STATIC
	src/gfx.cpp
)

target_include_directories(imp_gfx
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(IMP_GFX_BACKEND STREQUAL "DX11")
	target_compile_definitions(imp_gfx PUBLIC IMP_GFX_DX11)
	target_sources(imp_gfx PRIVATE src/backend/dx11/dx11_device.cpp)
	target_link_libraries(imp_gfx PRIVATE d3d11 dxgi d3dcompiler)

elseif(IMP_GFX_BACKEND STREQUAL "DX12")
	target_compile_definitions(imp_gfx PUBLIC IMP_GFX_DX12)
	target_sources(imp_gfx PRIVATE src/backend/dx12/dx12_device.cpp)
	target_link_libraries(imp_gfx PRIVATE d3d12 dxgi d3dcompiler)

elseif(IMP_GFX_BACKEND STREQUAL "VULKAN")
	target_compile_definitions(imp_gfx PUBLIC IMP_GFX_VULKAN)
	target_sources(imp_gfx PRIVATE
			src/backend/vulkan/vk_device.cpp
			src/backend/vulkan/vk_swapchain.cpp
			src/backend/vulkan/vk_commands.cpp
			src/backend/vulkan/vk_shader.cpp
			src/backend/vulkan/vk_pipeline.cpp
			src/backend/vulkan/vk_buffer.cpp
			src/backend/vulkan/vk_allocator.cpp
			src/backend/vulkan/vk_vma_impl.cpp
	)
	find_package(Vulkan REQUIRED)
	target_link_libraries(imp_gfx PRIVATE Vulkan::Vulkan)

	target_include_directories(imp_gfx SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/vma)

	include(cmake/CompileShaders.cmake)
	IMP_COMPILE_SHADERS(imp_gfx
		${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders
		${CMAKE_CURRENT_SOURCE_DIR}/shaders/vulkan/triangle.vert
		${CMAKE_CURRENT_SOURCE_DIR}/shaders/vulkan/triangle.frag
	)

	# simple allocator test that doesn't need to go through gtest for now
	add_executable(imp_gfx_vulkan_alloc_test
		src/backend/vulkan/vk_allocator_test.cpp
	)

	target_link_libraries(imp_gfx_vulkan_alloc_test PRIVATE imp_core imp_gfx Vulkan::Vulkan)

endif()

target_link_libraries(imp_gfx
	PUBLIC 
		imp_core
		imp_fwk
)

add_library(imp_gfx STATIC
	src/gfx.cpp
)

target_include_directories(imp_gfx
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(imp_gfx
	PUBLIC 
		imp_core
		imp_fwk
)

option(IMP_VULKAN_MEM_TEST "Compile and link memory allocation tests for Vulkan backend" ON)

find_package(Vulkan REQUIRED)
target_compile_definitions(imp_gfx PRIVATE IMP_GFX_VULKAN)
target_sources(imp_gfx PRIVATE
		src/backend/vulkan/vk_device.cpp
		src/backend/vulkan/vk_swapchain.cpp
		src/backend/vulkan/vk_commands.cpp
		src/backend/vulkan/vk_shader.cpp
		src/backend/vulkan/vk_pipeline.cpp
		src/backend/vulkan/vk_buffer.cpp
		src/backend/vulkan/vk_render_target.cpp
		src/backend/vulkan/vk_command_list.cpp
		src/backend/vulkan/vk_allocator.cpp
		src/backend/vulkan/vk_vma_impl.cpp
)
target_link_libraries(imp_gfx PRIVATE Vulkan::Vulkan PUBLIC imgui)
target_include_directories(imp_gfx SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/vma)

if(IMP_VULKAN_MEM_TEST)
	# simple allocator test that doesn't need to go through gtest for now
	add_executable(imp_gfx_vulkan_alloc_test
		src/backend/vulkan/vk_allocator_test.cpp
	)

	IMP_SET_RUNTIME_DIR(imp_gfx_vulkan_alloc_test tests)
	target_link_libraries(imp_gfx_vulkan_alloc_test PRIVATE imp_core imp_gfx Vulkan::Vulkan)
endif()

if (WIN32)
	target_compile_definitions(imp_gfx PRIVATE IMP_GFX_DX12)
	target_sources(imp_gfx PRIVATE
		src/backend/dx12/dx12_device.cpp
	)

	# target_link_libraries(imp_gfx PRIVATE d3d12 dxgi d3dcompiler)
endif()

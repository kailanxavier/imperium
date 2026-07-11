#pragma once

#include <vulkan/vulkan.h>

#include <array>
#include <cstddef>

namespace imp::gfx::vulkan
{
	struct Vertex
	{
		float position[2];
		float colour[3];

		static VkVertexInputBindingDescription bindingDescription()
		{
			VkVertexInputBindingDescription binding{};
			binding.binding = 0;
			binding.stride = sizeof(Vertex);
			binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			return binding;
		}

		static std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions()
		{
			std::array<VkVertexInputAttributeDescription, 2> attrs{};

			attrs[0].location = 0;
			attrs[0].binding = 0;
			attrs[0].format = VK_FORMAT_R32G32_SFLOAT;
			attrs[0].offset = offsetof(Vertex, position);

			attrs[1].location = 1;
			attrs[1].binding = 0;
			attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attrs[1].offset = offsetof(Vertex, colour);

			return attrs;
		}
	};
}

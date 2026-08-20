#pragma once
#include <vulkan/vulkan.h>
#include <core/log/log.h>

#define VK_CHECK(expr)																					\
		do {																							\
			VkResult _vkResult = ( expr );																\
			if (_vkResult != VK_SUCCESS)																\
			{																							\
				LOG_ERROR("Vulkan", "{} failed: VkResult={}", #expr, static_cast<int>( _vkResult ));	\
				return false;																			\
			}																							\
		} while (0)																						

#define VK_CHECK_VOID(expr)																				\
		do {																							\
			VkResult _vkResult = ( expr );																\
			if (_vkResult != VK_SUCCESS)																\
			{																							\
				LOG_ERROR("Vulkan", "{} failed: VkResult={}", #expr, static_cast<int>( _vkResult ));	\
			}																							\
		} while (0)																						

#pragma once
#include <vulkan/vulkan.h>
#include <core/types/int_types.h>
#include <string>

namespace imp::gfx::vulkan
{
	bool loadDebugUtilsFunctions(VkInstance instance);

	void setDebugObjectName(VkDevice device, VkObjectType type, u64 handle, const char* name);
	void cmdBeginDebugLabel(VkCommandBuffer cmd, const char* label, float r = 1.f, float g = 1.f, float b = 1.f);
	void cmdEndDebugLabel(VkCommandBuffer cmd);

	struct ScopedDebugLabel
	{
		ScopedDebugLabel(VkCommandBuffer cmd, const char* label, float r = 1.f, float g = 1.f, float b = 1.f)
			: m_cmd(cmd) { cmdBeginDebugLabel(cmd, label, r, g, b); }
		~ScopedDebugLabel() { cmdEndDebugLabel(m_cmd); }
		VkCommandBuffer m_cmd;
	};
}

#define VK_SCOPED_LABEL(cmd, name) \
	imp::gfx::vulkan::ScopedDebugLabel _dbgLabel##__LINE__(cmd, name)

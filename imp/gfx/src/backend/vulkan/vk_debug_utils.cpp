#include "vk_debug_utils.h"

namespace imp::gfx::vulkan
{
	namespace
	{
		PFN_vkSetDebugUtilsObjectNameEXT pfnSetDebugUtilsObjectNameEXT = nullptr;
		PFN_vkCmdBeginDebugUtilsLabelEXT pfnCmdBeginDebugUtilsLabelEXT = nullptr;
		PFN_vkCmdEndDebugUtilsLabelEXT   pfnCmdEndDebugUtilsLabelEXT = nullptr;
	}

    bool loadDebugUtilsFunctions(VkInstance instance)
    {
        pfnSetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(
            vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT"));
        pfnCmdBeginDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(
            vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT"));
        pfnCmdEndDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(
            vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT"));

        return pfnSetDebugUtilsObjectNameEXT && pfnCmdBeginDebugUtilsLabelEXT && pfnCmdEndDebugUtilsLabelEXT;
    }

    void setDebugObjectName(VkDevice device, VkObjectType type, u64 handle, const char* name)
    {
        if (!pfnSetDebugUtilsObjectNameEXT || !handle) return;

        VkDebugUtilsObjectNameInfoEXT info{};
        info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        info.objectType = type;
        info.objectHandle = handle;
        info.pObjectName = name;
        pfnSetDebugUtilsObjectNameEXT(device, &info);
    }

    void cmdBeginDebugLabel(VkCommandBuffer cmd, const char* label, float r, float g, float b)
    {
        if (!pfnCmdBeginDebugUtilsLabelEXT) return;
        VkDebugUtilsLabelEXT info{};
        info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
        info.pLabelName = label;
        info.color[0] = r; info.color[1] = g; info.color[2] = b; info.color[3] = 1.f;
        pfnCmdBeginDebugUtilsLabelEXT(cmd, &info);
    }

    void cmdEndDebugLabel(VkCommandBuffer cmd)
    {
        if (!pfnCmdEndDebugUtilsLabelEXT) return;
        pfnCmdEndDebugUtilsLabelEXT(cmd);
    }
}

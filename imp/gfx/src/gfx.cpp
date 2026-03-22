#include "gfx/gfx.h"

namespace imp::gfx
{
	const char* Version() { return "imp_gfx 0.1.0"; }
	const char* ActiveBackend()
	{
#if defined(IMP_GFX_DX11)
		return "DX11";
#elif defined(IMP_GFX_DX12)
		return "DX12";
#elif defined(IMP_GFX_VULKAN)
		return "Vulkan";
#else
		return "None";
#endif
	}
}
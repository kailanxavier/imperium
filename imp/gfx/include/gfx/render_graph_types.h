#pragma once

#include <functional>
#include <string>

#include <core/types/int_types.h>

#include "gfx/render_graph_handles.h"
#include "gfx/render_graph_context.h"

#include "gfx/commands.h"
#include "gfx/resources.h"


namespace imp::gfx
{
	enum class RGAttachmentRole : u8 { None, Colour, Depth, Resolve };
	
	struct RGPassAttachment
	{
		RGAttachmentRole role = RGAttachmentRole::None;
		u32 resourceIndex = ~0u;
		RGLoadOp loadOp = RGLoadOp::DontCare;
		ClearColour clearColour{};
		float clearDepth = 1.f;
		u32 resolveSourceIndex = ~0u;
	};

	struct RGResourceDesc
	{
		std::string name;
		RGResourceType type = RGResourceType::Texture;
		bool imported = false;

		TextureDesc textureDesc{};
		BufferDesc bufferDesc{};

		IRenderTarget* resolvedTarget = nullptr;
		ITexture* resolvedTextureOnly = nullptr;
		IBuffer* resolvedBuffer = nullptr;

		u32 latestVersion = 0;
		u32 lastWritePass = ~0u;

		u32 firstPass = ~0u;
		u32 lastPass = 0;
	};

	struct RGPass
	{
		std::string name;
		std::vector<u32> reads;
		std::vector<u32> writes;
		bool hasSideEffect = false;
		bool culled = false;

		std::vector<RGPassAttachment> colours;
		RGPassAttachment depth;
		RGPassAttachment resolve;

		std::function<void(RenderGraphContext&)> execute;
	};
}

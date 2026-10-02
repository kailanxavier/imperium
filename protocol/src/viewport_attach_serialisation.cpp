#include <protocol/viewport_attach.h>
#include "viewport_generated.h"
#include <flatbuffers/flatbuffers.h>

namespace imp::protocol
{
	std::vector<u8> serialiseViewportAttach(const ViewportAttachPayload& payload)
	{
		flatbuffers::FlatBufferBuilder builder;

		viewport::ViewportAttachBuilder vb(builder);
		vb.add_native_handle(payload.nativeHandle);
		vb.add_width(payload.width);
		vb.add_height(payload.height);
		vb.add_process_id(payload.processId);
		builder.Finish(vb.Finish());

		return { builder.GetBufferPointer(), builder.GetBufferPointer() + builder.GetSize() };
	}

	std::optional<ViewportAttachPayload> deserialiseViewportAttach(std::span<const u8> payload)
	{
		flatbuffers::Verifier verifier(payload.data(), payload.size());
		if (!viewport::VerifyViewportAttachBuffer(verifier))
			return std::nullopt;

		const auto* msg = viewport::GetViewportAttach(payload.data());
		if (!msg)
			return std::nullopt;

		ViewportAttachPayload p;
		p.nativeHandle = msg->native_handle();
		p.width = msg->width();
		p.height = msg->height();
		p.processId = msg->process_id();
		return p;
	}
}

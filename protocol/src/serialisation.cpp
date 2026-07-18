#include "protocol/serialisation.h"
#include "protocol/memory_telemetry.h"
#include "memory_generated.h"
#include <flatbuffers/flatbuffers.h>

namespace imp::protocol
{
	std::vector<u8> serialiseMemoryTelemetry(const std::vector<AllocatorStatsPayload>& allocators)
	{
		flatbuffers::FlatBufferBuilder builder;

		std::vector<flatbuffers::Offset<memory::AllocatorStats>> offsets;
		offsets.reserve(allocators.size());

		for (const auto& a : allocators)
		{
			const auto nameOffset = builder.CreateString(a.name);
			const auto tagBytesOffset = builder.CreateVector(a.tagBytes);

			memory::AllocatorStatsBuilder statsBuilder(builder);
			statsBuilder.add_allocator_name(nameOffset);
			statsBuilder.add_total_allocated(a.totalAllocated);
			statsBuilder.add_total_freed(a.totalFreed);
			statsBuilder.add_current_used(a.currentUsed);
			statsBuilder.add_peak_used(a.peakUsed);
			statsBuilder.add_allocation_count(a.allocationCount);
			statsBuilder.add_free_count(a.freeCount);
			statsBuilder.add_tag_bytes(tagBytesOffset);
			offsets.push_back(statsBuilder.Finish());
		}

		const auto allocatorsVec = builder.CreateVector(offsets);

		memory::MemoryTelemetryBuilder telemetryBuilder(builder);
		telemetryBuilder.add_allocators(allocatorsVec);
		builder.Finish(telemetryBuilder.Finish());

		return {builder.GetBufferPointer(), builder.GetBufferPointer() + builder.GetSize()};
	}
}

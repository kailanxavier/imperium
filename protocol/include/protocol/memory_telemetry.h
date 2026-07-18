#pragma once
#include <string>
#include <vector>
#include "int_types.h"

namespace imp::protocol
{
	struct AllocatorStatsPayload
	{
		std::string name;
		u64 totalAllocated = 0;
		u64 totalFreed = 0;
		u64 currentUsed = 0;
		u64 peakUsed = 0;
		u64 allocationCount = 0;
		u64 freeCount = 0;
		std::vector<u64> tagBytes;
	};

	std::vector<u8> serialiseMemoryTelemetry(const std::vector<AllocatorStatsPayload>& allocators);
}

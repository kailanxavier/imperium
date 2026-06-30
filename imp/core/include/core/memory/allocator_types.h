#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <atomic>

namespace imp::memory
{
	enum class MemTag : uint8_t
	{
		Untagged = 0,

		// Engine subsystems
		Renderer,
		Audio,
		Physics,
		Animation,
		AI,
		UI,

		// Asset pipeline
		Texture,
		Mesh,
		Shader,
		Material,
		Scene,

		// General purpose
		String,
		Container,
		Scratch,
		Debug,

		Count
	};

	[[nodiscard]] constexpr std::string_view toString(MemTag tag) noexcept
	{
		switch (tag)
		{
		case MemTag::Untagged:	return "Untagged";
		case MemTag::Renderer:	return "Renderer";
		case MemTag::Audio:		return "Audio";
		case MemTag::Physics:	return "Physics";
		case MemTag::Animation: return "Animation";
		case MemTag::AI:		return "AI";
		case MemTag::UI:		return "UI";
		case MemTag::Texture:	return "Texture";
		case MemTag::Mesh:		return "Mesh";
		case MemTag::Shader:	return "Shader";
		case MemTag::Material:	return "Material";
		case MemTag::Scene:		return "Scene";
		case MemTag::String:	return "String";
		case MemTag::Container: return "Container";
		case MemTag::Scratch:	return "Scratch";
		case MemTag::Debug:		return "Debug";
		default:				return "Unknown";
		}
	}

	struct AllocatorStats
	{
		std::atomic<size_t> totalAllocated{ 0 }; // Lifetime bytes handed out
		std::atomic<size_t> totalFreed{ 0 }; // Lifetime bytes returned
		std::atomic<size_t> currentUsed{ 0 };
		std::atomic<size_t> peakUsed{ 0 }; // High watermark of currentUsed
		std::atomic<size_t> allocationCount{ 0 }; // Lifetime alloc calls
		std::atomic<size_t> freeCount{ 0 }; // Lifetime free calls

		// Per-tag breakdown (current live bytes per category)
		std::atomic<size_t> tagBytes[static_cast<size_t>( MemTag::Count )]{};
		AllocatorStats() = default;
		AllocatorStats(const AllocatorStats&) = delete;
		AllocatorStats& operator=(const AllocatorStats&) = delete;

		void recordAlloc(size_t bytes, MemTag tag) noexcept
		{
			totalAllocated.fetch_add(bytes, std::memory_order_relaxed);
			allocationCount.fetch_add(1, std::memory_order_relaxed);
			tagBytes[static_cast<size_t>( tag )].fetch_add(bytes, std::memory_order_relaxed);

			const size_t now = currentUsed.fetch_add(bytes, std::memory_order_relaxed) + bytes;

			// Update peak (relaxed CAS loop - note: contention is rare here, but possible nonetheless)
			size_t peak = peakUsed.load(std::memory_order_relaxed);
			while (now > peak && !peakUsed.compare_exchange_weak(peak, now, std::memory_order_relaxed, std::memory_order_relaxed)) {}
		}

		void recordFree(size_t bytes, MemTag tag) noexcept
		{
			totalFreed.fetch_add(bytes, std::memory_order_relaxed);
			freeCount.fetch_add(1, std::memory_order_relaxed);
			currentUsed.fetch_sub(bytes, std::memory_order_relaxed);
			tagBytes[static_cast<size_t>( tag )].fetch_sub(bytes, std::memory_order_relaxed);
		}

		void reset() noexcept
		{
			totalAllocated.store(0, std::memory_order_relaxed);
			totalFreed.store(0, std::memory_order_relaxed);
			currentUsed.store(0, std::memory_order_relaxed);
			peakUsed.store(0, std::memory_order_relaxed);
			allocationCount.store(0, std::memory_order_relaxed);
			freeCount.store(0, std::memory_order_relaxed);

			for (auto& b : tagBytes)
				b.store(0, std::memory_order_relaxed);
		}
	};

	struct AllocatorStatsSnapshot
	{
		size_t totalAllocated;
		size_t totalFreed;
		size_t currentUsed;
		size_t peakUsed;
		size_t allocationCount;
		size_t freeCount;
		size_t tagBytes[static_cast<size_t>( MemTag::Count )];
	};

	[[nodiscard]] inline AllocatorStatsSnapshot snapshot(const AllocatorStats& s) noexcept
	{
		AllocatorStatsSnapshot out{};
		out.totalAllocated = s.totalAllocated.load(std::memory_order_relaxed);
		out.totalFreed = s.totalFreed.load(std::memory_order_relaxed);
		out.currentUsed = s.currentUsed.load(std::memory_order_relaxed);
		out.peakUsed = s.peakUsed.load(std::memory_order_relaxed);
		out.allocationCount = s.allocationCount.load(std::memory_order_relaxed);
		out.freeCount = s.freeCount.load(std::memory_order_relaxed);
		for (size_t i = 0; i < static_cast<size_t>(MemTag::Count); ++i)
			out.tagBytes[i] = s.tagBytes[i].load(std::memory_order_relaxed);

		return out;
	}
}
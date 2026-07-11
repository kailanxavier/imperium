#include <core/memory/heap_allocator.h>
#include <core/log/log.h>
#include <vulkan/vulkan.h>

#include <cstring>
#include <cstdio>
#include <vector>
#include <random>
#include <cassert>

#include "vk_allocator.h"

int main()
{
    imp::log::Logger::get().initialise("heap_test.log");

    imp::memory::HeapAllocator heap("TestHeap");
    VkAllocationCallbacks cb = imp::gfx::vulkan::makeVulkanAllocationCallbacks(heap);

    std::mt19937 rng(12345);
    std::uniform_int_distribution<size_t> sizeDist(1, 4096);
    size_t aligns[] = { 1, 2, 4, 8, 16, 32, 64, 256 };
    std::uniform_int_distribution<size_t> alignIdx(0, 7);

    struct Live { void* ptr; size_t size; unsigned char pattern; };
    std::vector<Live> live;

    const int kOps = 20000;
    for (int i = 0; i < kOps; ++i)
    {
        int action = rng() % 3;

        if (action == 0 || live.empty())
        {
            size_t size = sizeDist(rng);
            size_t align = aligns[alignIdx(rng)];
            void* p = cb.pfnAllocation(cb.pUserData, size, align, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
            assert(p != nullptr);
            assert(( reinterpret_cast<uintptr_t>(p) % align ) == 0 && "returned pointer violates requested alignment");

            unsigned char pattern = static_cast<unsigned char>( rng() & 0xFF );
            std::memset(p, pattern, size); // write every byte we were promised - ASan catches any OOB here
            live.push_back({ p, size, pattern });
        }
        else if (action == 1)
        {
            size_t idx = rng() % live.size();
            // verify the data is still exactly what we wrote
            std::vector<unsigned char> expected(live[idx].size, live[idx].pattern);
            assert(std::memcmp(live[idx].ptr, expected.data(), live[idx].size) == 0 && "data corruption detected");

            cb.pfnFree(cb.pUserData, live[idx].ptr);
            live[idx] = live.back();
            live.pop_back();
        }
        else
        {
            size_t idx = rng() % live.size();
            size_t newSize = sizeDist(rng);
            size_t align = aligns[alignIdx(rng)];

            size_t oldMeaningful = std::min(live[idx].size, newSize);
            std::vector<unsigned char> expectedPrefix(oldMeaningful, live[idx].pattern);

            void* newPtr = cb.pfnReallocation(cb.pUserData, live[idx].ptr, newSize, align, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
            assert(newPtr != nullptr);
            assert(( reinterpret_cast<uintptr_t>( newPtr ) % align ) == 0);
            assert(std::memcmp(newPtr, expectedPrefix.data(), oldMeaningful) == 0 && "realloc lost old data");

            unsigned char newPattern = static_cast<unsigned char>( rng() & 0xFF );
            std::memset(newPtr, newPattern, newSize);
            live[idx] = { newPtr, newSize, newPattern };
        }
    }

    // free everything remaining
    for (auto& l : live)
        cb.pfnFree(cb.pUserData, l.ptr);
    live.clear();

    LOG_INFO("Vulkan", "PASS: {} randomised alloc/realloc/free ops, all alignment and data integrity checks held", kOps);

    imp::log::Logger::get().shutdown();
    return 0;
}

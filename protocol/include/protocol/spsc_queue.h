#pragma once

#include <new>
#include <vector>
#include <atomic>

namespace imp::protocol
{
    template <typename T, size_t Capacity>
    class SPSCQueue
    {
    public:
        SPSCQueue() : m_buffer(Capacity) {}
        bool tryPush(const T& item)
        {
            const size_t currentTail = m_tail.load(std::memory_order_relaxed);
            const size_t nextTail = increment(currentTail);

            if (nextTail == m_head.load(std::memory_order_acquire))
                return false; // queue is full, drop telemetry

            m_buffer[currentTail] = item;
            m_tail.store(nextTail, std::memory_order_release);
            return true;
        }

        bool tryPop(T& outItem)
        {
            const size_t currentHead = m_head.load(std::memory_order_relaxed);

            if (currentHead == m_tail.load(std::memory_order_acquire))
                return false; // queue is empty

            outItem = m_buffer[currentHead];
            m_head.store(increment(currentHead), std::memory_order_release);
            return true;
        }

    private:
        [[nodiscard]] static size_t increment(const size_t index) { return (index + 1) % Capacity; }

        std::vector<T> m_buffer;

        alignas(std::hardware_destructive_interference_size)
        std::atomic<size_t> m_head{ 0 };

        alignas(std::hardware_destructive_interference_size)
        std::atomic<size_t> m_tail{ 0 };
    };
}

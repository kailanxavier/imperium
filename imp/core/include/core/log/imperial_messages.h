#pragma once

#include <array>
#include <random>

namespace imp::log
{
    static const char* imperialMessage()
    {
        static constexpr std::array messages =
        {
            "The emperor is displeased.",
            "The enemy has breached the perimeter.",
            "The empire has crumbled.",
            "The situation is becoming increasingly suboptimal.",
            "The Omnissiah provides no answers.",
            "All is lost.",
            "Abandon hope.",
            "A sacrifice may be required.",
            "Our glorious campaign has encountered an unexpected obstacle.",
            "We shall pretend this was intentional.",
            "The Emperor will hear of this.",
            "This incident will be recorded in the archives.",
            "The archives will not be kind to us.",
            "Don't worry, everything is under control.",
        };

        thread_local std::mt19937 rng{ std::random_device{}() };
        std::uniform_int_distribution<std::size_t> dist(0, messages.size() - 1);

        return messages[dist(rng)];
    }
}

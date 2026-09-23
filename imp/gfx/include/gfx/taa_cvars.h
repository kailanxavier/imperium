#pragma once
#include <core/config/cvar.h>

namespace imp::gfx::taa
{
    inline CVarBool cvarEnabled{ "taa.enabled", true };
    inline CVarInt cvarJitterSamples{ "taa.jitter_samples", 8 }; // it seems like 8 is the industry standard sweet spot
                                                                 // so that's what we're going for, apparently converges
                                                                 // to slightly nicer edges, but double the time to get there

    inline CVarFloat cvarJitterScale{ "taa.jitter_scale", 1.f };
    inline CVarFloat cvarFeedbackMin{ "taa.feedback_min", 0.88f };
    inline CVarFloat cvarFeedbackMax{ "taa.feedback_max", 0.97f };

    inline CVarFloat cvarVarianceGamma{ "taa.variance_gamma", 1.25f };

    inline CVarFloat cvarRejectFeedback{ "taa.reject_feedback", 0.2f };
}

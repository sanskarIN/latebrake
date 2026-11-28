#pragma once

#include "Vehicles/CameraPresets.h"

#include <cstdint>

namespace latebrake
{
// Session-only accessibility preference for OPTIONAL camera effects.
// Fixed still follows the car, retains collision probes and chase lag;
// it does not freeze world motion or promise a medical comfort outcome.
enum class CameraMotionMode : std::uint8_t
{
    Dynamic,
    Reduced,
    Fixed,
    Count
};

constexpr bool ValidCameraMotionMode(CameraMotionMode mode) noexcept
{
    return static_cast<unsigned>(mode)<static_cast<unsigned>(CameraMotionMode::Count);
}

constexpr CameraMotionMode NextCameraMotionMode(CameraMotionMode mode) noexcept
{
    switch(mode)
    {
        case CameraMotionMode::Dynamic: return CameraMotionMode::Reduced;
        case CameraMotionMode::Reduced: return CameraMotionMode::Fixed;
        case CameraMotionMode::Fixed: return CameraMotionMode::Dynamic;
        default: return CameraMotionMode::Fixed;
    }
}

constexpr const char* CameraMotionModeName(CameraMotionMode mode) noexcept
{
    switch(mode)
    {
        case CameraMotionMode::Dynamic: return "DYNAMIC";
        case CameraMotionMode::Reduced: return "REDUCED";
        case CameraMotionMode::Fixed: return "FIXED EFFECTS";
        default: return "UNAVAILABLE";
    }
}

struct CameraMotionEffects final
{
    double FovStrength=0.0;
    double YawStrength=0.0;
    bool BlendChase=false;
};

constexpr CameraMotionEffects EffectsForCameraMotion(CameraMotionMode mode) noexcept
{
    switch(mode)
    {
        case CameraMotionMode::Dynamic: return {1.0,1.0,true};
        case CameraMotionMode::Reduced: return {0.4,0.4,true};
        default: return {};
    }
}

// Existing hood/rear exclusions and player authority remain independent.
// A reduced-effects preference cannot re-enable any prohibited animation.
constexpr bool AllowCameraMotionBlend(CameraMotionMode mode,CameraView from,
    CameraView to,bool humanLiveRace,bool lookingBack) noexcept
{
    const auto valid=[](CameraView v)
    {
        return v==CameraView::Chase || v==CameraView::CloseChase ||
            v==CameraView::Hood || v==CameraView::High;
    };
    return humanLiveRace && !lookingBack && valid(from) && valid(to) &&
        from!=CameraView::Hood && to!=CameraView::Hood &&
        EffectsForCameraMotion(mode).BlendChase;
}
} // namespace latebrake

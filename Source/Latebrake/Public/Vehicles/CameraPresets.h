#pragma once

#include <cmath>

namespace latebrake
{
// Engine-independent camera choices. C cycles these without touching driving,
// physics, checkpoint, awards, save files or Android gesture ownership.
enum class CameraView : unsigned char
{
    Chase = 0,
    CloseChase = 1,
    Hood = 2,
    High = 3
};

struct CameraPreset final
{
    double ArmLengthCm;
    double PitchDegrees;
    double ForwardOffsetCm;
    double HeightOffsetCm;
    double CameraLagSpeed;
    bool CameraLagEnabled;
    bool CollisionProbeEnabled;
};

constexpr CameraPreset GetCameraPreset(CameraView view) noexcept
{
    switch (view)
    {
    case CameraView::CloseChase:
        return {270.0, -9.0, 0.0, 110.0, 10.0, true, true};
    case CameraView::Hood:
        return {0.0, -3.0, 215.0, 86.0, 0.0, false, false};
    case CameraView::High:
        return {720.0, -40.0, 0.0, 240.0, 5.5, true, true};
    case CameraView::Chase:
    default:
        return {560.0, -12.0, 0.0, 120.0, 7.0, true, true};
    }
}

constexpr const char* CameraViewName(CameraView view) noexcept
{
    switch (view)
    {
    case CameraView::CloseChase: return "CLOSE CHASE";
    case CameraView::Hood: return "HOOD";
    case CameraView::High: return "HIGH CHASE";
    default: return "CHASE";
    }
}

// Built-in view parameters are intentionally bounded (not console-facing
// arbitrary float inputs). Fallback on unsupported enum values is Chase.
inline bool IsSafeCameraPreset(CameraPreset p) noexcept
{
    return std::isfinite(p.ArmLengthCm) &&
        std::isfinite(p.PitchDegrees) &&
        std::isfinite(p.ForwardOffsetCm) &&
        std::isfinite(p.HeightOffsetCm) &&
        std::isfinite(p.CameraLagSpeed) &&
        p.ArmLengthCm >= 0.0 && p.ArmLengthCm <= 1200.0 &&
        p.PitchDegrees >= -80.0 && p.PitchDegrees <= 5.0 &&
        p.ForwardOffsetCm >= -100.0 && p.ForwardOffsetCm <= 300.0 &&
        p.HeightOffsetCm >= 0.0 && p.HeightOffsetCm <= 350.0 &&
        p.CameraLagSpeed >= 0.0 && p.CameraLagSpeed <= 20.0 &&
        (!p.CameraLagEnabled || p.CameraLagSpeed > 0.0);
}

class CameraViewCycle final
{
public:
    CameraView Current() const noexcept { return View; }
    void Reset() noexcept { View = CameraView::Chase; }
    CameraView Next() noexcept
    {
        switch (View)
        {
        case CameraView::Chase: View = CameraView::CloseChase; break;
        case CameraView::CloseChase: View = CameraView::Hood; break;
        case CameraView::Hood: View = CameraView::High; break;
        case CameraView::High:
        default: View = CameraView::Chase; break;
        }
        return View;
    }
private:
    CameraView View = CameraView::Chase;
};
} // namespace latebrake

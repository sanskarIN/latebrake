#pragma once

#include "Vehicles/CameraPresets.h"
#include <algorithm>
#include <cmath>

namespace latebrake
{
// The actual spring-arm rig, in Unreal centimetres and degrees. This is
// presentation state only; there is no impact on Chaos, input or race logic.
struct ChaseCameraPose final
{
    double ArmLengthCm = 560.0;
    double PitchDegrees = -12.0;
    double ForwardOffsetCm = 0.0;
    double HeightOffsetCm = 120.0;
};

inline bool IsSafeChaseCameraPose(const ChaseCameraPose& p) noexcept
{
    return std::isfinite(p.ArmLengthCm) &&
        std::isfinite(p.PitchDegrees) &&
        std::isfinite(p.ForwardOffsetCm) &&
        std::isfinite(p.HeightOffsetCm) &&
        p.ArmLengthCm >= 0.0 && p.ArmLengthCm <= 1200.0 &&
        p.PitchDegrees >= -80.0 && p.PitchDegrees <= 5.0 &&
        p.ForwardOffsetCm >= -100.0 && p.ForwardOffsetCm <= 300.0 &&
        p.HeightOffsetCm >= 0.0 && p.HeightOffsetCm <= 350.0;
}

inline ChaseCameraPose ChasePoseForPreset(CameraPreset preset) noexcept
{
    const CameraPreset safe = IsSafeCameraPreset(preset)
        ? preset : GetCameraPreset(CameraView::Chase);
    return {safe.ArmLengthCm, safe.PitchDegrees,
            safe.ForwardOffsetCm, safe.HeightOffsetCm};
}

// A short deterministic ease between CHASE, CLOSE CHASE and HIGH CHASE.
// Hood switches, rear view, blocked driving and failed input/physics are
// intentionally snapped by the caller. The interpolation never rotates the
// vehicle or touches HUD, checkpoints, lap timing, rewards or saved profiles.
class ChaseCameraTransition final
{
public:
    static constexpr double DurationSeconds = 0.32;
    static constexpr double MaxFrameSeconds = 0.25;

    bool IsActive() const noexcept { return Active; }
    ChaseCameraPose Pose() const noexcept { return Current; }

    void Snap(CameraPreset target) noexcept
    {
        Current = ChasePoseForPreset(target);
        From = Current;
        To = Current;
        Elapsed = 0.0;
        Active = false;
    }

    // Repeated view switches start from what was actually being displayed,
    // never from a stale previous preset or a guessed target location.
    bool Begin(ChaseCameraPose displayed, CameraPreset target) noexcept
    {
        if (!IsSafeChaseCameraPose(displayed) || !IsSafeCameraPreset(target))
        {
            Snap(GetCameraPreset(CameraView::Chase));
            return false;
        }
        From = Current = displayed;
        To = ChasePoseForPreset(target);
        Elapsed = 0.0;
        Active =
            std::abs(From.ArmLengthCm - To.ArmLengthCm) > 1.e-9 ||
            std::abs(From.PitchDegrees - To.PitchDegrees) > 1.e-9 ||
            std::abs(From.ForwardOffsetCm - To.ForwardOffsetCm) > 1.e-9 ||
            std::abs(From.HeightOffsetCm - To.HeightOffsetCm) > 1.e-9;
        return Active;
    }

    void Cancel() noexcept
    {
        Current = To;
        Elapsed = 0.0;
        Active = false;
    }

    ChaseCameraPose Advance(double frameSeconds) noexcept
    {
        if (!Active)
        {
            return Current;
        }
        // Invalid or stalled world frames may not leave an in-between
        // camera until the next valid player Tick. Finish immediately.
        if (!std::isfinite(frameSeconds) || frameSeconds <= 0.0 ||
            frameSeconds > MaxFrameSeconds)
        {
            Cancel();
            return Current;
        }

        Elapsed = std::min(DurationSeconds, Elapsed + frameSeconds);
        const double t = Elapsed / DurationSeconds;
        const double easing = t * t * (3.0 - 2.0 * t);
        const auto Mix = [easing](double a, double b) noexcept
        {
            return a + (b - a) * easing;
        };
        Current = {
            Mix(From.ArmLengthCm, To.ArmLengthCm),
            Mix(From.PitchDegrees, To.PitchDegrees),
            Mix(From.ForwardOffsetCm, To.ForwardOffsetCm),
            Mix(From.HeightOffsetCm, To.HeightOffsetCm)};
        if (Elapsed >= DurationSeconds)
        {
            Cancel(); // End on the exact target, not rounded intermediate.
        }
        return Current;
    }

private:
    ChaseCameraPose From{};
    ChaseCameraPose To{};
    ChaseCameraPose Current{};
    double Elapsed = 0.0;
    bool Active = false;
};
} // namespace latebrake

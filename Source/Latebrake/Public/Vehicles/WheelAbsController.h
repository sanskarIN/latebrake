#pragma once

#include "Vehicles/WheelBrakeProfile.h"

#include <algorithm>
#include <cmath>

namespace latebrake
{
enum class WheelAbsPhase : unsigned
{
    Inactive,
    Applying,
    Holding,
    Releasing
};

struct WheelAbsSample final
{
    double DeltaSeconds = 0.0;
    double RoadMetersPerSecond = 0.0;
    double ModeledWheelRadiansPerSecond = 0.0;
    bool Supported = false;
    bool ServiceBrake = false;
    bool MechanicalHandbrake = false;
    double AvailablePressure = 1.0;
};

struct WheelAbsReadout final
{
    bool Valid = false;
    bool Modulating = false;
    double Pressure = 1.0;
    double ModeledBrakingSlip = 0.0;
    WheelAbsPhase Phase = WheelAbsPhase::Inactive;
};

// This controller observes MODELED rotor speed, not a real ABS wheel sensor.
// It only changes candidate brake torque; it cannot create body force,
// steering authority, tire contact or game rewards. Four independent copies
// are advanced by the wheel-brake coordinator at bounded internal steps.
class WheelAbsController final
{
public:
    void Reset() noexcept
    {
        Pressure = 1.0;
        HoldRemaining = 0.0;
        Phase = WheelAbsPhase::Inactive;
    }

    WheelAbsReadout Advance(const WheelAbsSample& s,
                            const WheelBrakeProfile& p) noexcept
    {
        if (!IsValidWheelBrakeProfile(p) ||
            !std::isfinite(s.DeltaSeconds) || s.DeltaSeconds <= 0.0 ||
            s.DeltaSeconds > 0.01 ||
            !std::isfinite(s.RoadMetersPerSecond) ||
            std::abs(s.RoadMetersPerSecond) > 1000.0 ||
            !std::isfinite(s.ModeledWheelRadiansPerSecond) ||
            std::abs(s.ModeledWheelRadiansPerSecond) > 10000.0 ||
            !std::isfinite(s.AvailablePressure) ||
            s.AvailablePressure < 0.0 || s.AvailablePressure > 1.0)
        {
            Reset();
            return {};
        }
        const double speed = std::abs(s.RoadMetersPerSecond);
        if (!p.AbsEnabled || !s.Supported || !s.ServiceBrake ||
            s.MechanicalHandbrake || speed < p.AbsMinimumSpeedMetersPerSecond)
        {
            Reset();
            return {true, false, 1.0, 0.0, WheelAbsPhase::Inactive};
        }
        // Torque feed-forward limits pressure to the current admitted
        // tire-force/load budget. Hysteresis then reduces it further when
        // modeled slip grows, instead of repeatedly locking from full
        // caliper pressure while road speed is held for a render frame.
        Pressure = std::min(Pressure, s.AvailablePressure);
        const double pressureFloor = std::min(p.MinimumPressure, s.AvailablePressure);
        const double signedTread = s.ModeledWheelRadiansPerSecond *
            p.RadiusMeters * std::copysign(1.0, s.RoadMetersPerSecond);
        const double slip = std::clamp((speed - signedTread) /
            std::max(speed, p.SlipSpeedFloorMetersPerSecond), -4.0, 4.0);
        HoldRemaining = std::max(0.0, HoldRemaining - s.DeltaSeconds);
        if (slip >= p.AbsReleaseSlip)
        {
            Phase = WheelAbsPhase::Releasing;
            Pressure = std::max(pressureFloor,
                Pressure - p.AbsReleaseRatePerSecond * s.DeltaSeconds);
            HoldRemaining = p.AbsHoldSeconds;
        }
        else if (HoldRemaining > 0.0 || slip > p.AbsReapplySlip)
        {
            Phase = WheelAbsPhase::Holding;
        }
        else
        {
            Phase = WheelAbsPhase::Applying;
            Pressure = std::min(s.AvailablePressure,
                Pressure + p.AbsApplyRatePerSecond * s.DeltaSeconds);
        }
        return {true, Pressure < 1.0 - 1.0e-9, Pressure, slip, Phase};
    }

private:
    double Pressure = 1.0;
    double HoldRemaining = 0.0;
    WheelAbsPhase Phase = WheelAbsPhase::Inactive;
};
} // namespace latebrake

#pragma once

#include <array>
#include <cmath>

namespace latebrake
{
// Original candidate parameters for the fictional garage. SI units here;
// the Unreal adapter explicitly converts cm/s and kg*cm/s^2. These values
// are not measured tire data, homologated ABS or calibrated vehicle setup.
struct WheelBrakeProfile final
{
    double RadiusMeters = 0.29;
    double InertiaKgMetersSquared = 1.8;
    double FrictionCoefficient = 1.10;
    double SlipStiffness = 9.0;
    double SlipSpeedFloorMetersPerSecond = 1.25;
    double MaximumBrakeTorqueNm = 2200.0;
    double AbsReleaseSlip = 0.22;
    double AbsReapplySlip = 0.10;
    double AbsMinimumSpeedMetersPerSecond = 2.0;
    double AbsReleaseRatePerSecond = 10.0;
    double AbsApplyRatePerSecond = 3.5;
    double AbsHoldSeconds = 0.012;
    double MinimumPressure = 0.03;
    bool AbsEnabled = true;
    bool operator==(const WheelBrakeProfile&) const = default;
};

inline bool IsValidWheelBrakeProfile(const WheelBrakeProfile& p) noexcept
{
    const std::array<double, 13> values{
        p.RadiusMeters, p.InertiaKgMetersSquared, p.FrictionCoefficient,
        p.SlipStiffness, p.SlipSpeedFloorMetersPerSecond,
        p.MaximumBrakeTorqueNm, p.AbsReleaseSlip, p.AbsReapplySlip,
        p.AbsMinimumSpeedMetersPerSecond, p.AbsReleaseRatePerSecond,
        p.AbsApplyRatePerSecond, p.AbsHoldSeconds, p.MinimumPressure};
    for (const double value : values)
        if (!std::isfinite(value)) return false;
    return p.RadiusMeters >= 0.10 && p.RadiusMeters <= 0.75 &&
        p.InertiaKgMetersSquared >= 0.10 && p.InertiaKgMetersSquared <= 20.0 &&
        p.FrictionCoefficient >= 0.05 && p.FrictionCoefficient <= 3.0 &&
        p.SlipStiffness >= 1.0 && p.SlipStiffness <= 30.0 &&
        p.SlipSpeedFloorMetersPerSecond >= 0.25 &&
        p.SlipSpeedFloorMetersPerSecond <= 5.0 &&
        p.MaximumBrakeTorqueNm >= 50.0 && p.MaximumBrakeTorqueNm <= 10000.0 &&
        p.AbsReleaseSlip >= 0.05 && p.AbsReleaseSlip <= 0.75 &&
        p.AbsReapplySlip >= 0.0 && p.AbsReapplySlip < p.AbsReleaseSlip &&
        p.AbsMinimumSpeedMetersPerSecond >= 0.5 &&
        p.AbsMinimumSpeedMetersPerSecond <= 10.0 &&
        p.AbsReleaseRatePerSecond >= 0.1 && p.AbsReleaseRatePerSecond <= 100.0 &&
        p.AbsApplyRatePerSecond >= 0.1 && p.AbsApplyRatePerSecond <= 100.0 &&
        p.AbsHoldSeconds >= 0.0 && p.AbsHoldSeconds <= 0.1 &&
        p.MinimumPressure >= 0.0 && p.MinimumPressure <= 0.5;
}

struct FourCornerBrakeProfile final
{
    std::array<WheelBrakeProfile, 4> Wheels{};
};

inline bool IsValidFourCornerBrakeProfile(const FourCornerBrakeProfile& p) noexcept
{
    for (const auto& wheel : p.Wheels)
        if (!IsValidWheelBrakeProfile(wheel)) return false;
    return true;
}

// All six current visual-wheel radii are the same 29 cm. Never infer a
// different physical radius from decorative body-kit scale. Profile torque
// and inertia vary deliberately; real driving qualification is outstanding.
inline FourCornerBrakeProfile BrakeProfileForVehicle(int id, bool arcade) noexcept
{
    struct Setup final
    {
        double Inertia;
        double FrontTorque;
        double RearTorque;
        double Friction;
        double AbsRelease;
        double AbsApply;
    };
    constexpr std::array<Setup, 6> setups{{
        {1.80, 2300.0, 1800.0, 1.10, 0.22, 3.5}, // Arrow
        {2.10, 2600.0, 1900.0, 1.15, 0.20, 3.2}, // Kestrel
        {1.65, 2200.0, 1750.0, 1.08, 0.23, 3.8}, // Phantom
        {1.90, 2800.0, 2050.0, 1.18, 0.19, 3.0}, // Tempest
        {1.55, 2050.0, 1600.0, 1.04, 0.24, 4.0}, // Glider
        {2.30, 3000.0, 2250.0, 1.20, 0.18, 2.8}, // Nova
    }};
    const auto& setup = setups[static_cast<unsigned>(id >= 0 && id < 6 ? id : 0)];
    FourCornerBrakeProfile result;
    for (unsigned i = 0; i < result.Wheels.size(); ++i)
    {
        auto& p = result.Wheels[i];
        p.InertiaKgMetersSquared = setup.Inertia;
        p.MaximumBrakeTorqueNm = i < 2 ? setup.FrontTorque : setup.RearTorque;
        p.FrictionCoefficient = setup.Friction * (arcade ? 1.0 : 0.94);
        p.AbsReleaseSlip = setup.AbsRelease;
        p.AbsReapplySlip = setup.AbsRelease * 0.45;
        p.AbsApplyRatePerSecond = setup.AbsApply;
    }
    return result;
}
} // namespace latebrake

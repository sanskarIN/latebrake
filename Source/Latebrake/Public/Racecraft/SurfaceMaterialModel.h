#pragma once
#include "Racecraft/RacecraftFrames.h"
#include "Racecraft/CircuitGeometry.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace latebrake::racecraft
{
struct SurfaceMaterial final
{
    ContactSurface Kind = ContactSurface::Unknown;
    double DryFrictionCoefficient = 0.0;
    double WetFrictionCoefficient = 0.0;
    double RollingResistanceCoefficient = 0.0;
    double Roughness = 0.0;
    double WaterFilmSensitivity = 0.0;
    double HeatTransferMultiplier = 1.0;
    double WearMultiplier = 1.0;
    double LateralGripModifier = 1.0;
    double LongitudinalGripModifier = 1.0;
    double BrakingGripModifier = 1.0;
    double SurfaceBumpHeightCm = 0.0;
    double MaximumQualifiedSpeedCmS = MaximumPhysicalSpeedCmS;
    bool Drivable = false;
    bool SupportsRaceTiming = false;
    bool RoadGeometryVerified = false;
};
struct SurfaceWeather final
{
    double AmbientC = 20.0;
    double TrackC = 35.0;
    double Wetness = 0.0;
    double StandingWaterMm = 0.0;
    double RainMmHour = 0.0;
    double Visibility = 1.0;
    double DustFraction = 0.0;
    double RubberFraction = 0.3;
    double WindCmS = 0.0;
    double WindRadians = 0.0;
    bool EnvironmentQualified = false;
};
struct SurfaceContact final
{
    SurfaceMaterial Material{};
    SurfaceWeather Weather{};
    double Friction = 0.0;
    double LateralFriction = 0.0;
    double LongitudinalFriction = 0.0;
    double BrakeFriction = 0.0;
    double RollingResistance = 0.0;
    double EffectiveRoughness = 0.0;
    double ThermalTransfer = 0.0;
    double WearRateMultiplier = 0.0;
    double HydroplaningFraction = 0.0;
    double MaximumSafeSpeedCmS = 0.0;
    double PredictedWaterDragN = 0.0;
    bool HydroplaningRisk = false;
    bool NeedsCaution = false;
    bool GeometryQualified = false;
    bool PhysicallyMeasured = false;
    bool Valid = false;
};
inline constexpr std::array<SurfaceMaterial, 9> Materials = {{
    {ContactSurface::Unknown,0.0,0.0,0.0,0.0,0.0,1.0,1.0,1.0,1.0,1.0,0.0,0.0,false,false,false},
    {ContactSurface::Asphalt,1.25,0.84,0.018,0.10,0.35,1.0,1.0,1.0,1.0,1.0,0.2,18000.0,true,true,true},
    {ContactSurface::PaintedKerb,0.95,0.56,0.018,0.22,0.55,0.90,1.2,0.92,0.95,0.89,2.0,12000.0,true,true,true},
    {ContactSurface::Grass,0.52,0.30,0.08,0.55,0.70,0.70,1.2,0.60,0.70,0.55,3.0,3000.0,true,false,true},
    {ContactSurface::Gravel,0.62,0.42,0.16,0.76,0.50,0.75,1.7,0.68,0.85,0.72,5.0,2500.0,true,false,true},
    {ContactSurface::WetAsphalt,0.88,0.70,0.023,0.17,0.70,1.15,0.90,0.91,0.88,0.87,0.3,12000.0,true,true,true},
    {ContactSurface::StandingWater,0.40,0.19,0.09,0.28,1.00,1.30,0.80,0.65,0.70,0.61,1.0,3000.0,true,false,true},
    {ContactSurface::Wall,0.0,0.0,0.0,1.0,0.0,1.0,1.0,0.0,0.0,0.0,0.0,0.0,false,false,true},
    {ContactSurface::PitConcrete,1.0,0.73,0.028,0.12,0.50,0.90,0.80,0.95,0.93,0.90,0.2,4000.0,true,false,true}
}};
inline SurfaceMaterial MaterialFor(ContactSurface kind) noexcept
{
    const std::size_t index = static_cast<std::size_t>(kind);
    return index < Materials.size() ?
        Materials[index] : Materials[0];
}
inline bool ValidSurfaceWeather(SurfaceWeather s) noexcept
{
    return Finite(s.AmbientC) && s.AmbientC >= -70.0 &&
        s.AmbientC <= 90.0 &&
        Finite(s.TrackC) && s.TrackC >= -70.0 &&
        s.TrackC <= 150.0 &&
        Finite(s.Wetness) && s.Wetness >= 0.0 &&
        s.Wetness <= 1.0 &&
        Finite(s.StandingWaterMm) &&
        s.StandingWaterMm >= 0.0 &&
        s.StandingWaterMm <= 100.0 &&
        Finite(s.RainMmHour) && s.RainMmHour >= 0.0 &&
        s.RainMmHour <= 300.0 &&
        Finite(s.Visibility) && s.Visibility >= 0.0 &&
        s.Visibility <= 1.0 &&
        Finite(s.DustFraction) &&
        s.DustFraction >= 0.0 &&
        s.DustFraction <= 1.0 &&
        Finite(s.RubberFraction) &&
        s.RubberFraction >= 0.0 &&
        s.RubberFraction <= 1.0 &&
        Finite(s.WindCmS) && s.WindCmS >= 0.0 &&
        s.WindCmS <= 15000.0 &&
        Finite(s.WindRadians);
}
inline bool ValidSurfaceMaterial(SurfaceMaterial s) noexcept
{
    return Finite(s.DryFrictionCoefficient) &&
        s.DryFrictionCoefficient >= 0.0 &&
        s.DryFrictionCoefficient <= 5.0 &&
        Finite(s.WetFrictionCoefficient) &&
        s.WetFrictionCoefficient >= 0.0 &&
        s.WetFrictionCoefficient <= s.DryFrictionCoefficient &&
        Finite(s.RollingResistanceCoefficient) &&
        s.RollingResistanceCoefficient >= 0.0 &&
        s.RollingResistanceCoefficient <= 1.0 &&
        Finite(s.Roughness) && s.Roughness >= 0.0 &&
        s.Roughness <= 1.0 &&
        Finite(s.WaterFilmSensitivity) &&
        s.WaterFilmSensitivity >= 0.0 &&
        s.WaterFilmSensitivity <= 2.0 &&
        Finite(s.HeatTransferMultiplier) &&
        s.HeatTransferMultiplier >= 0.0 &&
        Finite(s.WearMultiplier) &&
        s.WearMultiplier >= 0.0 &&
        Finite(s.LateralGripModifier) &&
        s.LateralGripModifier >= 0.0 &&
        Finite(s.LongitudinalGripModifier) &&
        s.LongitudinalGripModifier >= 0.0 &&
        Finite(s.BrakingGripModifier) &&
        s.BrakingGripModifier >= 0.0 &&
        Finite(s.SurfaceBumpHeightCm) &&
        s.SurfaceBumpHeightCm >= 0.0 &&
        Finite(s.MaximumQualifiedSpeedCmS) &&
        s.MaximumQualifiedSpeedCmS >= 0.0;
}
inline double WetFilmThicknessMm(SurfaceWeather weather) noexcept
{
    if (!ValidSurfaceWeather(weather)) return 0.0;
    return weather.StandingWaterMm +
        weather.Wetness * 0.75 +
        weather.RainMmHour / 300.0;
}
inline double HydroplaningThresholdCmS(
    double tirePressureKpa,
    double treadDepthMm,
    double waterDepthMm) noexcept
{
    if (!Finite(tirePressureKpa) ||
        !Finite(treadDepthMm) || !Finite(waterDepthMm) ||
        tirePressureKpa < 50.0 || tirePressureKpa > 800.0 ||
        treadDepthMm < 0.0 || treadDepthMm > 20.0 ||
        waterDepthMm < 0.0 || waterDepthMm > 100.0)
        return 0.0;
    const double pressurePsi = tirePressureKpa / 6.894757293;
    const double onsetMph = 10.35 * std::sqrt(pressurePsi);
    const double wetDepthFraction =
        Clamp01((waterDepthMm - 0.25 * treadDepthMm) / 7.0);
    const double mphToCmS = 44.704;
    const double threshold = onsetMph * mphToCmS;
    return wetDepthFraction <= 0.001 ?
        MaximumPhysicalSpeedCmS : threshold;
}
inline double HydroplaningSeverity(double speedCmS,
                                   double pressureKpa,
                                   double treadDepthMm,
                                   double waterDepthMm) noexcept
{
    if (!Finite(speedCmS) || speedCmS < 0.0) return 0.0;
    const double threshold = HydroplaningThresholdCmS(
        pressureKpa, treadDepthMm, waterDepthMm);
    if (threshold <= PhysicsEpsilon ||
        threshold >= MaximumPhysicalSpeedCmS)
        return 0.0;
    const double depthFactor =
        Smoothstep(0.5, 4.0, waterDepthMm - 0.25 * treadDepthMm);
    return Smoothstep(threshold * 0.75,
                      threshold * 1.30,
                      speedCmS) * depthFactor;
}
inline double TemperatureGripFraction(double tireC,
                                      double surfaceC,
                                      TireCompound compound) noexcept
{
    if (!Finite(tireC) || !Finite(surfaceC)) return 0.0;
    double target = 95.0;
    switch (compound)
    {
    case TireCompound::Hard: target = 110.0; break;
    case TireCompound::Medium: target = 96.0; break;
    case TireCompound::Soft: target = 82.0; break;
    case TireCompound::Intermediate: target = 65.0; break;
    case TireCompound::Wet: target = 50.0; break;
    default: return 0.0;
    }
    const double deviation = std::abs(tireC - target);
    const double profile = std::exp(-0.5 *
        Square(deviation / 58.0));
    const double roadPenalty =
        std::max(0.0, surfaceC - 120.0) * 0.002;
    return std::clamp(0.45 + 0.55 * profile -
                      roadPenalty, 0.20, 1.15);
}
inline double WearGripFraction(double wear,
                               TireCompound compound) noexcept
{
    if (!Finite(wear) || wear < 0.0 || wear > 1.0 ||
        compound == TireCompound::Unknown) return 0.0;
    const double exponent =
        compound == TireCompound::Soft ? 1.7 :
        compound == TireCompound::Hard ? 0.95 : 1.2;
    return std::clamp(
        1.0 - 0.64 * std::pow(wear, exponent),
        0.25, 1.0);
}
inline double TireWetSuitability(TireCompound compound,
                                 double wetness) noexcept
{
    if (!Finite(wetness) || wetness < 0.0 ||
        wetness > 1.0) return 0.0;
    const double dryBias =
        compound == TireCompound::Soft ? 1.08 :
        compound == TireCompound::Hard ? 0.98 :
        compound == TireCompound::Medium ? 1.0 : 0.74;
    const double wetBias =
        compound == TireCompound::Wet ? 1.05 :
        compound == TireCompound::Intermediate ? 0.95 :
        compound == TireCompound::Hard ? 0.55 : 0.45;
    return Lerp(dryBias, wetBias, wetness);
}
inline SurfaceContact EvaluateSurfaceContact(
    SurfaceMaterial material, SurfaceWeather weather,
    TireCompound compound, double tireTemperatureC,
    double tireWear, double tirePressureKpa,
    double speedCmS, double loadN) noexcept
{
    SurfaceContact result;
    result.Material = material;
    result.Weather = weather;
    if (!ValidSurfaceMaterial(material) ||
        !ValidSurfaceWeather(weather) ||
        !Finite(tireTemperatureC) ||
        !Finite(tireWear) || tireWear < 0.0 ||
        tireWear > 1.0 ||
        !Finite(tirePressureKpa) ||
        tirePressureKpa < 50.0 ||
        tirePressureKpa > 800.0 ||
        !Finite(speedCmS) || speedCmS < 0.0 ||
        speedCmS > MaximumPhysicalSpeedCmS ||
        !Finite(loadN) || loadN < 0.0 ||
        loadN > 300000.0 ||
        compound == TireCompound::Unknown ||
        !material.Drivable) return result;
    const double base = Lerp(
        material.DryFrictionCoefficient,
        material.WetFrictionCoefficient,
        weather.Wetness);
    const double wetSuitability = TireWetSuitability(
        compound, weather.Wetness);
    const double heat = TemperatureGripFraction(
        tireTemperatureC, weather.TrackC, compound);
    const double wear = WearGripFraction(tireWear, compound);
    const double hydro = HydroplaningSeverity(
        speedCmS, tirePressureKpa,
        8.0 * (1.0 - tireWear),
        WetFilmThicknessMm(weather));
    const double dust = 1.0 -
        0.32 * weather.DustFraction;
    const double rubber =
        1.0 + 0.08 * weather.RubberFraction *
        (1.0 - weather.Wetness);
    const double grip = base * heat * wear *
        wetSuitability * dust * rubber *
        (1.0 - 0.83 * hydro);
    result.Friction = std::clamp(grip, 0.02, 3.0);
    result.LateralFriction = result.Friction *
        material.LateralGripModifier;
    result.LongitudinalFriction = result.Friction *
        material.LongitudinalGripModifier;
    result.BrakeFriction = result.Friction *
        material.BrakingGripModifier;
    result.RollingResistance =
        material.RollingResistanceCoefficient *
        (1.0 + 0.8 * weather.Wetness);
    result.EffectiveRoughness =
        material.Roughness *
        (1.0 + 0.25 * weather.StandingWaterMm);
    result.ThermalTransfer =
        material.HeatTransferMultiplier *
        (1.0 + weather.Wetness);
    result.WearRateMultiplier =
        material.WearMultiplier * (1.0 +
        0.3 * weather.TrackC / 100.0);
    result.HydroplaningFraction = hydro;
    result.HydroplaningRisk = hydro > 0.2;
    result.MaximumSafeSpeedCmS =
        material.MaximumQualifiedSpeedCmS *
        (1.0 - 0.60 * hydro);
    result.PredictedWaterDragN =
        0.000005 * weather.StandingWaterMm *
        speedCmS * speedCmS;
    result.NeedsCaution =
        result.HydroplaningRisk ||
        result.Friction < 0.45 ||
        weather.Visibility < 0.3;
    result.GeometryQualified = material.RoadGeometryVerified;
    result.PhysicallyMeasured =
        weather.EnvironmentQualified;
    result.Valid = Finite(result.Friction) &&
        Finite(result.PredictedWaterDragN);
    return result;
}
inline SurfaceContact EvaluateWheelSurface(
    WheelObservation wheel, SurfaceWeather weather,
    TireCompound compound, double speedCmS) noexcept
{
    if (!wheel.ContactVerified || !wheel.PhysicalContact ||
        wheel.NormalLoadN <= 0.0) return {};
    return EvaluateSurfaceContact(
        MaterialFor(wheel.Surface),
        weather, compound, wheel.TemperatureC,
        wheel.WearFraction, wheel.PressureKpa,
        speedCmS, wheel.NormalLoadN);
}
struct SurfaceTransition final
{
    ContactSurface From = ContactSurface::Unknown;
    ContactSurface To = ContactSurface::Unknown;
    double GripRatio = 0.0;
    double BrakeGripRatio = 0.0;
    double RollingResistanceDelta = 0.0;
    double RecommendedThrottleCeiling = 0.0;
    double RecommendedSpeedCeilingCmS = 0.0;
    bool MoreSlippery = false;
    bool NeedsImmediateReduction = false;
    bool Valid = false;
};
inline SurfaceTransition CompareSurfaceContact(
    SurfaceContact previous, SurfaceContact next) noexcept
{
    SurfaceTransition out;
    out.From = previous.Material.Kind;
    out.To = next.Material.Kind;
    if (!previous.Valid || !next.Valid ||
        previous.Friction <= PhysicsEpsilon) return out;
    out.GripRatio = next.Friction / previous.Friction;
    out.BrakeGripRatio = SafeDivision(
        next.BrakeFriction, previous.BrakeFriction);
    out.RollingResistanceDelta =
        next.RollingResistance -
        previous.RollingResistance;
    out.MoreSlippery = out.GripRatio < 0.95;
    out.NeedsImmediateReduction = out.GripRatio < 0.55 ||
        next.HydroplaningRisk;
    out.RecommendedThrottleCeiling =
        std::clamp(out.GripRatio, 0.0, 1.0);
    out.RecommendedSpeedCeilingCmS = std::min(
        previous.MaximumSafeSpeedCmS,
        next.MaximumSafeSpeedCmS);
    out.Valid = true;
    return out;
}
class SurfaceTransitionMonitor final
{
public:
    void Reset() noexcept
    {
        Last = {};
        Previous = {};
        Qualified = false;
        Changes = 0;
        Severe = 0;
        WetTransitions = 0;
    }
    SurfaceTransition Observe(SurfaceContact current) noexcept
    {
        if (!current.Valid)
        {
            Reset();
            return {};
        }
        if (!Qualified)
        {
            Previous = current;
            Qualified = true;
            return {};
        }
        Last = CompareSurfaceContact(Previous, current);
        if (Last.Valid && Last.From != Last.To)
            ++Changes;
        if (Last.NeedsImmediateReduction) ++Severe;
        if (Previous.Weather.Wetness < 0.3 &&
            current.Weather.Wetness >= 0.3)
            ++WetTransitions;
        Previous = current;
        return Last;
    }
    SurfaceTransition Snapshot() const noexcept { return Last; }
    std::uint64_t SurfaceChanges() const noexcept { return Changes; }
    std::uint64_t SevereTransitions() const noexcept { return Severe; }
    std::uint64_t WetFronts() const noexcept { return WetTransitions; }
private:
    SurfaceContact Previous{};
    SurfaceTransition Last{};
    std::uint64_t Changes = 0;
    std::uint64_t Severe = 0;
    std::uint64_t WetTransitions = 0;
    bool Qualified = false;
};
} // namespace latebrake::racecraft

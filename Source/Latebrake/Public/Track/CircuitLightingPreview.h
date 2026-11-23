#pragma once
#include <cmath>
#include <cstdint>

namespace latebrake
{
// A two-state, purely visual preview for the actual UE starter circuit.
// This is intentionally NOT a weather or time-of-day simulation; it never
// grants vehicle tuning, race flags, credits, score or SaveGame authority.
enum class CircuitLightingMood : std::uint8_t {Day, Dusk};
struct CircuitLightRgb final
{
    double R=1.0,G=1.0,B=1.0;
};
struct CircuitLightingPreset final
{
    double SunPitchDeg=-55.0;
    double SunYawDeg=-30.0;
    double SunIntensity=6.0;
    CircuitLightRgb SunColor{};
    double SkyIntensity=1.0;
    CircuitLightRgb SkyColor{};
    bool PreviewOnly=true;
};
inline constexpr CircuitLightingPreset LightingForMood(
    CircuitLightingMood mood) noexcept
{
    if (mood==CircuitLightingMood::Dusk)
        return {-10.0,-30.0,0.68,{1.0,0.66,0.44},
                0.40,{0.47,0.61,0.91},true};
    return {-55.0,-30.0,6.0,{1.0,0.96,0.88},
            1.0,{0.85,0.92,1.0},true};
}
inline constexpr bool CanChangeCircuitLighting(
    bool menuOpen, bool settingsPage, bool focusSuspended,
    bool manualPauseSuspended, bool realSunAndSkyReady) noexcept
{
    return menuOpen && settingsPage && !focusSuspended &&
        !manualPauseSuspended && realSunAndSkyReady;
}
inline bool ValidCircuitLightingPreset(CircuitLightingPreset p) noexcept
{
    const auto rgb=[](CircuitLightRgb c){
        return std::isfinite(c.R) && std::isfinite(c.G) &&
            std::isfinite(c.B) &&
            c.R>=0.0 && c.R<=1.0 &&
            c.G>=0.0 && c.G<=1.0 &&
            c.B>=0.0 && c.B<=1.0;
    };
    return p.PreviewOnly &&
        std::isfinite(p.SunPitchDeg) &&
        p.SunPitchDeg>=-75.0 && p.SunPitchDeg<=0.0 &&
        std::isfinite(p.SunYawDeg) &&
        p.SunYawDeg>=-180.0 && p.SunYawDeg<=180.0 &&
        std::isfinite(p.SunIntensity) &&
        p.SunIntensity>=0.0 && p.SunIntensity<=10.0 &&
        std::isfinite(p.SkyIntensity) &&
        p.SkyIntensity>=0.0 && p.SkyIntensity<=3.0 &&
        rgb(p.SunColor) && rgb(p.SkyColor);
}
static_assert(CanChangeCircuitLighting(true,true,false,false,true));
static_assert(!CanChangeCircuitLighting(false,true,false,false,true));
static_assert(!CanChangeCircuitLighting(true,false,false,false,true));
static_assert(LightingForMood(CircuitLightingMood::Day).SunIntensity >
              LightingForMood(CircuitLightingMood::Dusk).SunIntensity);
} // namespace latebrake

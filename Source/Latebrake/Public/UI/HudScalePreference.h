#pragma once

#include "UI/HudVerticalPlan.h"
#include "UI/HudTextBounds.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace latebrake
{
// Race presentation only. Keep menu geometry and its independent input
// hit tests on CalculateHudLayout, regardless of this session setting.
enum class HudScalePreference : std::uint8_t
{
    Automatic, Smaller, Larger
};

constexpr bool ValidHudScalePreference(HudScalePreference preference) noexcept
{
    switch(preference)
    {
    case HudScalePreference::Automatic:
    case HudScalePreference::Smaller:
    case HudScalePreference::Larger:return true;
    default:return false;
    }
}

constexpr HudScalePreference NextHudScalePreference(HudScalePreference preference) noexcept
{
    switch(preference)
    {
    case HudScalePreference::Automatic:return HudScalePreference::Smaller;
    case HudScalePreference::Smaller:return HudScalePreference::Larger;
    default:return HudScalePreference::Automatic;
    }
}

constexpr const char* HudScalePreferenceName(HudScalePreference preference) noexcept
{
    switch(preference)
    {
    case HudScalePreference::Automatic:return "AUTO";
    case HudScalePreference::Smaller:return "SMALLER";
    case HudScalePreference::Larger:return "LARGER";
    default:return "AUTO";
    }
}

inline constexpr double HudPreferenceMinimumScale=0.60;
inline constexpr double HudPreferenceMaximumScale=1.20;
inline constexpr int HudPreferenceMinimumTelemetryRows=6;

struct HudScaleResolution final
{
    HudLayout Layout{};
    bool Applied=false;
    bool Limited=false;
};

// Try requested size within the existing UI geometry contract. Larger
// text must not consume essential telemetry rows or collide with touch
// controls. If the requested size does not fit, search down to the
// baseline, never invent a new viewport or change physical touch pads.
inline HudScaleResolution ResolveHudScalePreference(double width,double height,
    HudScalePreference preference,bool menuOpen,bool touchUI) noexcept
{
    const HudLayout baseline=CalculateHudLayout(width,height);
    HudScaleResolution out{baseline,false,false};
    if(menuOpen || !ValidHudScalePreference(preference) ||
        preference==HudScalePreference::Automatic)return out;
    if(!std::isfinite(width) || !std::isfinite(height) ||
        width<240 || height<240)
    {
        out.Limited=true;
        return out;
    }
    const auto baseVertical=CalculateHudVerticalPlan(width,height,baseline,touchUI);
    const auto baseHorizontal=CalculateHudTextBounds(width,height,baseline,touchUI);
    if(!baseVertical.Valid || !baseHorizontal.Valid)
    {
        out.Limited=true;
        return out;
    }
    const double requested=baseline.Scale*(preference==HudScalePreference::Smaller?0.85:1.15);
    const double bounded=std::clamp(requested,HudPreferenceMinimumScale,
        HudPreferenceMaximumScale);
    const int minimumRows=std::min(baseVertical.MaxTelemetryLines,
        HudPreferenceMinimumTelemetryRows);
    const auto candidate=[&](double scale)
    {
        HudLayout layout=baseline;
        layout.Scale=scale;
        layout.Left=24*scale;
        layout.Top=18*scale;
        layout.LineHeight=(baseline.Compact?23.0:26.0)*scale;
        return layout;
    };
    const auto fits=[&](const HudLayout& layout)
    {
        const auto vertical=CalculateHudVerticalPlan(width,height,layout,touchUI);
        const auto horizontal=CalculateHudTextBounds(width,height,layout,touchUI);
        return vertical.Valid && horizontal.Valid &&
            vertical.MaxTelemetryLines>=minimumRows &&
            horizontal.MaxWidthPixels>=std::min(140.0*layout.Scale,
                baseHorizontal.MaxWidthPixels);
    };
    const auto proposed=candidate(bounded);
    if(fits(proposed))
        out.Layout=proposed;
    else
    {
        // Geometry is monotone in scale for the existing race lanes.
        // Fixed iteration count makes the result deterministic and bounded.
        double low=std::min(baseline.Scale,bounded);
        double high=std::max(baseline.Scale,bounded);
        for(int i=0;i<16;++i)
        {
            const double mid=(low+high)*0.5;
            if(fits(candidate(mid)))low=mid;
            else high=mid;
        }
        const auto limited=candidate(low);
        if(fits(limited) && preference==HudScalePreference::Larger)
            out.Layout=limited;
    }
    out.Applied=std::abs(out.Layout.Scale-baseline.Scale)>1e-7;
    out.Limited=std::abs(out.Layout.Scale-requested)>1e-5;
    return out;
}
} // namespace latebrake

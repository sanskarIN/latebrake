#pragma once

#include "UI/HudLayout.h"

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Desktop-only, read-only visual instrumentation. Neither the dimensions nor
// meter values may be used to authorize driving, checkpoint credit or rewards.
struct InstrumentRect final
{
    double X = 0.0;
    double Y = 0.0;
    double Width = 0.0;
    double Height = 0.0;
};

// Independently anchored percentage typography prevents long telemetry
// labels from pushing live 0–100% values out of a small laptop panel.
// Unreal supplies the percent's ACTUAL measured font width to this portable
// layout; no character-count or monospaced-width assumptions are made.
struct HudInstrumentMeterText final
{
    bool Valid = false;
    double LabelX = 0.0;
    double PercentX = 0.0;
    double LabelMaxWidth = 0.0;
    double PercentRight = 0.0;
};

inline HudInstrumentMeterText CalculateHudInstrumentMeterText(
    InstrumentRect panel, double scale,
    double measuredPercentWidth) noexcept
{
    if (!std::isfinite(panel.X) || !std::isfinite(panel.Y) ||
        !std::isfinite(panel.Width) || !std::isfinite(panel.Height) ||
        panel.Width <= 0.0 || panel.Height <= 0.0 ||
        !std::isfinite(scale) || scale <= 0.0 ||
        !std::isfinite(measuredPercentWidth) ||
        measuredPercentWidth <= 0.0)
    {
        return {};
    }

    const double inset = 16.0 * scale;
    const double gap = std::max(8.0, 10.0 * scale);
    const double left = panel.X + inset;
    const double percentRight = panel.X + panel.Width - inset;
    const double percentX = percentRight - measuredPercentWidth;
    const double labelWidth = percentX - gap - left;
    if (!std::isfinite(left) || !std::isfinite(percentRight) ||
        !std::isfinite(percentX) || !std::isfinite(labelWidth) ||
        left < panel.X || percentRight > panel.X + panel.Width ||
        percentX <= left || labelWidth < 4.0 * scale)
    {
        return {};
    }
    return {true, left, percentX, labelWidth, percentRight};
}

struct HudInstrumentCluster final
{
    bool Visible = false;
    InstrumentRect Panel;
    double Scale = 1.0;
    double SpeedKmH = 0.0;
    double NitroFraction = 0.0;
    double EngineHeatFraction = 0.0;
    double ChassisConditionFraction = 1.0;
};

inline HudInstrumentCluster CalculateHudInstrumentCluster(
    double width, double height, HudLayout layout,
    bool touchUI, bool menuOpen, bool raceFinished,
    double speedKmH, double nitroFraction,
    double engineHeatFraction, double collisionDamageFraction) noexcept
{
    // The existing compact/mobile HUD owns all touch regions; never draw a
    // visual-only widget over driving pads, menu text or a finished race.
    // For supported 900px laptop windows retain the existing gauge: its
    // independent measured right-hand lane and footer guard still apply.
    if (touchUI || menuOpen || raceFinished || layout.Compact ||
        !std::isfinite(width) || !std::isfinite(height) ||
        width < 900.0 || height < 600.0 ||
        !std::isfinite(layout.Scale) || layout.Scale <= 0.0 ||
        !std::isfinite(layout.Left) || layout.Left < 0.0 ||
        !std::isfinite(speedKmH) || !std::isfinite(nitroFraction) ||
        !std::isfinite(engineHeatFraction) ||
        !std::isfinite(collisionDamageFraction))
    {
        return {};
    }

    // Keep speed and meter labels readable at laptop resolutions even when
    // the ordinary text HUD uses smaller telemetry typography.
    const double scale = std::clamp(layout.Scale, 0.85, 1.2);
    const double panelWidth = 278.0 * scale;
    const double panelHeight = 214.0 * scale;
    const double rightMargin = std::max(18.0 * scale, width * 0.018);
    const double x = width - rightMargin - panelWidth;
    const double y = height - (56.0 * scale) - panelHeight;
    // Fail closed if a future layout change makes the widget collide with
    // the left information lane or cross a screen edge.
    if (!std::isfinite(x) || !std::isfinite(y) ||
        x < std::max(width * 0.53, layout.Left + 300.0 * scale) ||
        y < 0.0 || x + panelWidth > width || y + panelHeight > height)
    {
        return {};
    }

    return {
        true, {x, y, panelWidth, panelHeight}, scale,
        std::clamp(speedKmH, 0.0, 999.0),
        std::clamp(nitroFraction, 0.0, 1.0),
        std::clamp(engineHeatFraction, 0.0, 1.0),
        1.0 - std::clamp(collisionDamageFraction, 0.0, 1.0)
    };
}
} // namespace latebrake

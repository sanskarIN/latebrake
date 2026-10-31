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
    if (touchUI || menuOpen || raceFinished || layout.Compact ||
        !std::isfinite(width) || !std::isfinite(height) ||
        width < 1100.0 || height < 680.0 ||
        !std::isfinite(layout.Scale) || layout.Scale <= 0.0 ||
        !std::isfinite(layout.Left) || layout.Left < 0.0 ||
        !std::isfinite(speedKmH) || !std::isfinite(nitroFraction) ||
        !std::isfinite(engineHeatFraction) ||
        !std::isfinite(collisionDamageFraction))
    {
        return {};
    }

    const double scale = std::clamp(layout.Scale, 0.6, 1.2);
    const double panelWidth = 278.0 * scale;
    const double panelHeight = 178.0 * scale;
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

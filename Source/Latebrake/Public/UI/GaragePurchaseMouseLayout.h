#pragma once

#include "Garage/GaragePurchasePanel.h"
#include "UI/HudMenuMouseControls.h"
#include "UI/GarageShowroomLane.h"

namespace latebrake
{
// One compact action strip ABOVE the 3D aperture. Its geometry is derived
// from the same showroom lane used by the HUD and pointer admission.
constexpr DesktopMenuMouseAction GarageOfferAction(std::size_t index) noexcept
{
    return index == 0 ? DesktopMenuMouseAction::GarageUnlockOffer :
        index == 1 ? DesktopMenuMouseAction::GarageEngineOffer :
        index == 2 ? DesktopMenuMouseAction::GarageGripOffer :
        DesktopMenuMouseAction::None;
}
constexpr int GarageOfferIndex(DesktopMenuMouseAction action) noexcept
{
    switch (action)
    {
    case DesktopMenuMouseAction::GarageUnlockOffer: return 0;
    case DesktopMenuMouseAction::GarageEngineOffer: return 1;
    case DesktopMenuMouseAction::GarageGripOffer: return 2;
    default: return -1;
    }
}
struct GaragePurchaseMouseLayout final
{
    bool Valid = false;
    std::array<DesktopMenuMouseButton, GaragePurchaseOfferCount> Buttons{};
};
inline GaragePurchaseMouseLayout CalculateGaragePurchaseMouseLayout(
    const GarageShowroomLane& lane, double width, double height, HudLayout hud) noexcept
{
    if (!lane.Valid || hud.Compact || !std::isfinite(width) ||
        !std::isfinite(height) || !std::isfinite(hud.Scale) ||
        hud.Scale <= 0.0 || width < 900.0 || height < 600.0)
        return {};
    const auto& a = lane.Aperture;
    const double scale = std::clamp(hud.Scale,0.6,1.2);
    const double gap = std::max(6.0, 8.0 * scale);
    const double h = std::max(44.0, 48.0 * scale);
    const double w = (a.Width - 2.0 * gap) / 3.0;
    const double y = a.Y - gap - h;
    if (!std::isfinite(a.X) || !std::isfinite(a.Y) ||
        !std::isfinite(w) || !std::isfinite(y) ||
        !std::isfinite(a.X + a.Width) ||
        a.X < 0.0 || a.X + a.Width > width || a.Y > height ||
        y < 22.0 * scale || w < 60.0 || y + h > a.Y)
        return {};
    GaragePurchaseMouseLayout result;
    result.Valid = true;
    for (std::size_t i = 0; i < result.Buttons.size(); ++i)
        result.Buttons[i] = {GarageOfferAction(i),
            {a.X + static_cast<double>(i) * (w + gap),y,w,h}};
    return result;
}
inline DesktopMenuMouseAction HitTestGaragePurchaseMouse(
    const GaragePurchaseMouseLayout& layout, double x, double y) noexcept
{
    if (!layout.Valid || !std::isfinite(x) || !std::isfinite(y))
        return DesktopMenuMouseAction::None;
    DesktopMenuMouseAction found = DesktopMenuMouseAction::None;
    for (std::size_t i = 0; i < layout.Buttons.size(); ++i)
    {
        const auto& b = layout.Buttons[i];
        const auto& r = b.Bounds;
        if (b.Action != GarageOfferAction(i) ||
            !std::isfinite(r.X) || !std::isfinite(r.Y) ||
            !std::isfinite(r.Width) || !std::isfinite(r.Height) ||
            !std::isfinite(r.X+r.Width) || !std::isfinite(r.Y+r.Height) ||
            r.X < 0.0 || r.Y < 0.0 || r.Width < 60.0 || r.Height < 44.0 ||
            r.X+r.Width <= r.X || r.Y+r.Height <= r.Y)
            return DesktopMenuMouseAction::None;
        for (std::size_t j=0; j<i; ++j)
        {
            const auto& prior=layout.Buttons[j].Bounds;
            if (r.X < prior.X+prior.Width && r.X+r.Width > prior.X &&
                r.Y < prior.Y+prior.Height && r.Y+r.Height > prior.Y)
                return DesktopMenuMouseAction::None;
        }
        if (r.Contains(x,y)) found=b.Action;
    }
    return found;
}
// An input event may arrive between HUD frames. A newly staged offer must
// first be presented with its confirmation label; it is never enough to
// recompute a new price/status from an old rectangle at click time.
struct GaragePresentedOffer final
{
    bool Visible = false;
    DesktopMenuMouseRect Bounds{};
    GaragePurchasePanelOffer Offer{};
};
using GaragePresentedOffers = std::array<GaragePresentedOffer,GaragePurchaseOfferCount>;

inline bool CanClickPresentedGarageOffer(
    const GaragePresentedOffer& shown, const DesktopMenuMouseButton& button,
    const GaragePurchasePanelOffer& current) noexcept
{
    const auto& a = shown.Bounds;
    const auto& b = button.Bounds;
    if (!shown.Visible || !current.Enabled() || !shown.Offer.Enabled() ||
        a.X != b.X || a.Y != b.Y || a.Width != b.Width || a.Height != b.Height ||
        shown.Offer.Kind != current.Kind || shown.Offer.Subject != current.Subject ||
        shown.Offer.Price != current.Price || shown.Offer.Status != current.Status)
        return false;
    if (current.Status == GarageOfferStatus::Confirm)
        return std::isfinite(current.SecondsRemaining) &&
            std::isfinite(shown.Offer.SecondsRemaining) &&
            current.SecondsRemaining >= 0.0 &&
            current.SecondsRemaining <= shown.Offer.SecondsRemaining;
    return current.Status == GarageOfferStatus::Preview;
}
} // namespace latebrake

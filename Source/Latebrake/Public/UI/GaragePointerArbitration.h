#pragma once

#include "UI/GarageShowroomLane.h"
#include "UI/GarageShowroomQuickView.h"
#include "UI/GaragePurchaseMouseLayout.h"
#include <array>
#include <cstddef>

namespace latebrake
{
// A physical hit is deliberately independent of permission. A disabled
// button still occupies its rectangle; it may not pass a click through
// to an overlapping enabled button. No entry here mutates the GameMode.
struct GaragePointerClaim final
{
    DesktopMenuMouseAction Action = DesktopMenuMouseAction::None;
    bool Authorized = false;
};

enum class GaragePointerGroup : std::size_t
{
    Navigation, Style, QuickView, Orbit, Purchase, Count
};

inline constexpr std::size_t GaragePointerGroupCount =
    static_cast<std::size_t>(GaragePointerGroup::Count);
using GaragePointerClaims = std::array<GaragePointerClaim, GaragePointerGroupCount>;

constexpr bool IsGarageBrowseAction(DesktopMenuMouseAction action) noexcept
{
    return action == DesktopMenuMouseAction::GarageBrowsePrevious ||
           action == DesktopMenuMouseAction::GarageBrowseNext;
}

constexpr bool IsGarageStyleAction(DesktopMenuMouseAction action) noexcept
{
    return action == DesktopMenuMouseAction::GaragePaint ||
           action == DesktopMenuMouseAction::GarageStripe ||
           action == DesktopMenuMouseAction::GarageWheels ||
           action == DesktopMenuMouseAction::GarageReset;
}

constexpr bool ValidGarageGroupClaim(GaragePointerGroup group,
    DesktopMenuMouseAction action) noexcept
{
    if (action == DesktopMenuMouseAction::None) return true;
    switch (group)
    {
    case GaragePointerGroup::Navigation:
        return action == DesktopMenuMouseAction::PreviousPage ||
               action == DesktopMenuMouseAction::NextPage ||
               action == DesktopMenuMouseAction::DetailsUp ||
               action == DesktopMenuMouseAction::DetailsDown ||
               action == DesktopMenuMouseAction::Activate ||
               action == DesktopMenuMouseAction::Close;
    case GaragePointerGroup::Style:
        return IsGarageBrowseAction(action) || IsGarageStyleAction(action);
    case GaragePointerGroup::QuickView:
        return GarageQuickViewIndex(action) >= 0;
    case GaragePointerGroup::Orbit:
        return GarageOrbitIndex(action) >= 0;
    case GaragePointerGroup::Purchase:
        return GarageOfferIndex(action) >= 0;
    default:
        return false;
    }
}

// Resolve all geometrical claims at once rather than giving one group
// implicit precedence. This also guards future viewport/dock changes.
constexpr DesktopMenuMouseAction ResolveGaragePointerClaims(
    const GaragePointerClaims& claims) noexcept
{
    DesktopMenuMouseAction action = DesktopMenuMouseAction::None;
    bool authorized = false;
    for (std::size_t i = 0; i < claims.size(); ++i)
    {
        const auto& claim = claims[i];
        if (!ValidGarageGroupClaim(static_cast<GaragePointerGroup>(i),
                                    claim.Action))
            return DesktopMenuMouseAction::None;
        if (claim.Action == DesktopMenuMouseAction::None)
        {
            if (claim.Authorized) return DesktopMenuMouseAction::None;
            continue;
        }
        if (action != DesktopMenuMouseAction::None)
            return DesktopMenuMouseAction::None; // disabled claims count too
        action = claim.Action;
        authorized = claim.Authorized;
    }
    return authorized ? action : DesktopMenuMouseAction::None;
}

struct GaragePointerPermissions final
{
    DesktopMenuMouseAvailability Navigation;
    bool Browse = false;
    bool Style = false;
    bool Orbit = false;
    std::array<bool, GaragePurchaseOfferCount> Purchase{};
};

// Geometry is computed by the same source plans used by the HUD. All raw
// hits are collected with admission enabled, then authority is applied.
inline GaragePointerClaims CollectGaragePointerClaims(
    const DesktopMenuMouseLayout& navigation,
    const GarageStyleMouseLayout& style,
    const GarageQuickViewLayout& quick,
    const GarageOrbitButtons& orbit,
    const GaragePurchaseMouseLayout& purchase,
    double x, double y, GaragePointerPermissions permission) noexcept
{
    const auto navAction = HitTestDesktopMenuMouse(navigation, x, y,
        {true, true, true});
    const auto styleAction = HitTestGarageStyleMouse(style, x, y, true, true);
    const auto quickAction = HitTestGarageQuickView(quick, x, y, true);
    const auto orbitAction = HitTestGarageOrbitButtons(orbit, x, y, true);

    const auto purchaseAction = HitTestGaragePurchaseMouse(purchase, x, y);
    const int purchaseIndex = GarageOfferIndex(purchaseAction);

    return {{
        {navAction, navAction != DesktopMenuMouseAction::None &&
            DesktopMenuMouseActionEnabled(navAction, permission.Navigation)},
        {styleAction, styleAction != DesktopMenuMouseAction::None &&
            (IsGarageBrowseAction(styleAction) ? permission.Browse : permission.Style)},
        {quickAction, quickAction != DesktopMenuMouseAction::None && permission.Browse},
        {orbitAction, orbitAction != DesktopMenuMouseAction::None && permission.Orbit},
        {purchaseAction, purchaseIndex >= 0 &&
            permission.Purchase[static_cast<std::size_t>(purchaseIndex)]}
    }};
}

static_assert(ValidGarageGroupClaim(GaragePointerGroup::Navigation,
    DesktopMenuMouseAction::Close));
static_assert(!ValidGarageGroupClaim(GaragePointerGroup::QuickView,
    DesktopMenuMouseAction::Activate));
} // namespace latebrake

#pragma once

#include "UI/GarageShowroomLane.h"
#include "UI/GarageShowroomQuickView.h"
#include "UI/GaragePurchaseMouseLayout.h"
#include "UI/GarageAppearancePalette.h"
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
    Navigation, Style, QuickView, Orbit, Purchase, Palette, Count
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
    case GaragePointerGroup::Palette:
        return IsGaragePaletteAction(action);
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

// Validate enabled layouts independently of the pointer position. A
// malformed active group must never disappear into an ordinary no-hit:
// its invalid rectangle or overlapping controls could otherwise pass a
// click to a different group. An inactive layout is intentionally empty.
template<std::size_t N,typename ExpectedAction>
inline bool ValidGaragePointerLayout(bool active,
    const std::array<DesktopMenuMouseButton,N>& buttons,
    ExpectedAction expected,double minWidth,double minHeight) noexcept
{
    if(!active)return true;
    for(std::size_t i=0;i<N;++i)
    {
        const auto& button=buttons[i];
        const auto& r=button.Bounds;
        const double right=r.X+r.Width,bottom=r.Y+r.Height;
        if(button.Action!=expected(i)||
            !std::isfinite(r.X)||!std::isfinite(r.Y)||
            !std::isfinite(r.Width)||!std::isfinite(r.Height)||
            !std::isfinite(right)||!std::isfinite(bottom)||
            r.X<0.0||r.Y<0.0||r.Width<minWidth||r.Height<minHeight||
            right<=r.X||bottom<=r.Y)
            return false;
        for(std::size_t j=0;j<i;++j)
        {
            const auto& p=buttons[j].Bounds;
            if(r.X<p.X+p.Width&&right>p.X&&r.Y<p.Y+p.Height&&bottom>p.Y)
                return false;
        }
    }
    return true;
}

inline bool ValidGaragePointerPlans(const DesktopMenuMouseLayout& nav,
    const GarageStyleMouseLayout& style,const GarageQuickViewLayout& quick,
    const GarageOrbitButtons& orbit,const GaragePurchaseMouseLayout& purchase) noexcept
{
    constexpr std::array navActions{
        DesktopMenuMouseAction::PreviousPage,DesktopMenuMouseAction::NextPage,
        DesktopMenuMouseAction::DetailsUp,DesktopMenuMouseAction::DetailsDown,
        DesktopMenuMouseAction::Activate,DesktopMenuMouseAction::Close};
    constexpr std::array styleActions{
        DesktopMenuMouseAction::GaragePaint,DesktopMenuMouseAction::GarageStripe,
        DesktopMenuMouseAction::GarageWheels,DesktopMenuMouseAction::GarageReset,
        DesktopMenuMouseAction::GarageBrowsePrevious,DesktopMenuMouseAction::GarageBrowseNext};
    return ValidGaragePointerLayout(nav.Valid,nav.Buttons,
            [&](std::size_t i){return navActions[i];},44.0,44.0)&&
        ValidGaragePointerLayout(style.Valid,style.Buttons,
            [&](std::size_t i){return styleActions[i];},44.0,44.0)&&
        ValidGaragePointerLayout(quick.Valid,quick.Buttons,
            [](std::size_t i){return GarageQuickViewAction(static_cast<int>(i));},25.0,20.0)&&
        ValidGaragePointerLayout(orbit.Valid,orbit.Buttons,
            [](std::size_t i){return GarageOrbitAction(static_cast<int>(i));},44.0,44.0)&&
        ValidGaragePointerLayout(purchase.Valid,purchase.Buttons,
            [](std::size_t i){return GarageOfferAction(i);},60.0,44.0);
}

// Geometry is computed by the same source plans used by the HUD. All raw
// hits are collected with admission enabled, then authority is applied.
inline GaragePointerClaims CollectGaragePointerClaims(
    const DesktopMenuMouseLayout& navigation,
    const GarageStyleMouseLayout& style,
    const GarageQuickViewLayout& quick,
    const GarageOrbitButtons& orbit,
    const GaragePurchaseMouseLayout& purchase,
    double x, double y, GaragePointerPermissions permission,
    GaragePointerClaim palette = {}) noexcept
{
    if(!std::isfinite(x)||!std::isfinite(y)||
        !ValidGaragePointerPlans(navigation,style,quick,orbit,purchase))
        return {}; // reject the complete malformed active surface
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
            permission.Purchase[static_cast<std::size_t>(purchaseIndex)]},
        palette
    }};
}

static_assert(ValidGarageGroupClaim(GaragePointerGroup::Navigation,
    DesktopMenuMouseAction::Close));
static_assert(!ValidGarageGroupClaim(GaragePointerGroup::QuickView,
    DesktopMenuMouseAction::Activate));
} // namespace latebrake

#pragma once

#include "UI/MenuNavigation.h"

namespace latebrake
{
// Controller buttons are intentionally reused between driving and the
// pause-only Garage UI. A garage callback may not consume the event's
// driving input or invoke GameMode unless the visible open page is Garage.
//
// This is a routing/focus check, NOT authorization to spend currency or
// edit paint. GameMode independently validates ownership, future SaveGame
// protection, career race restrictions and current preview selection.
inline constexpr bool CanConsumeGarageShortcut(
    bool menuOpen, MenuPage visiblePage,
    bool externallySuspended = false) noexcept
{
    return !externallySuspended && menuOpen &&
           visiblePage == MenuPage::Garage;
}

// The existing E hotkey may cycle owned cars during an eligible quick
// race when no menu is open. A paused menu routes it ONLY on Garage.
// This is focus, not approval to own or purchase any car.
inline constexpr bool CanUseGarageVehicleHotkey(
    bool menuOpen, MenuPage visiblePage,
    bool standaloneRacePaused = false,
    bool externallySuspended = false) noexcept
{
    // Unreal pauses the world for an ordinary open Garage. That world
    // pause is allowed, but explicit manual-pause/focus-loss latches are
    // independent safety gates, even if the Garage is later opened.
    // A suspended session cannot stage purchases or equip a different car.
    return !externallySuspended &&
        (menuOpen ? visiblePage == MenuPage::Garage
                  : !standaloneRacePaused);
}

// Money and upgrade actions MUST require the visible Garage, even
// during otherwise eligible quick-race driving. The E owned-car cycling
// shortcut intentionally has a different, legacy gameplay policy.
// Unlike an ordinary world pause caused by opening Garage, explicit
// manual suspension / OS focus loss always revokes paid action focus.
// This is only a routing predicate: ownership, save version, available
// credits, two-key consent and transaction rollback remain GameMode rules.
inline constexpr bool CanUseGaragePurchaseHotkey(
    bool menuOpen, MenuPage visiblePage,
    bool externallySuspended = false) noexcept
{
    return !externallySuspended && menuOpen &&
           visiblePage == MenuPage::Garage;
}
} // namespace latebrake

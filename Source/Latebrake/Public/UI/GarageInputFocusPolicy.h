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
    bool menuOpen, MenuPage visiblePage) noexcept
{
    return menuOpen && visiblePage == MenuPage::Garage;
}

// The existing E hotkey may cycle owned cars during an eligible quick
// race when no menu is open. A paused menu routes it ONLY on Garage.
// This is focus, not approval to own or purchase any car.
inline constexpr bool CanUseGarageVehicleHotkey(
    bool menuOpen, MenuPage visiblePage,
    bool standaloneRacePaused = false) noexcept
{
    // A separate Pause key, OS deactivation or manually suspended race
    // must not grant spending/equip access merely because E/Q/U/I now
    // execute when Unreal is paused.
    return menuOpen ? visiblePage == MenuPage::Garage
                    : !standaloneRacePaused;
}
} // namespace latebrake

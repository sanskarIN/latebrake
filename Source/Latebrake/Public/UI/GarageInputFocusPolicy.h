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
} // namespace latebrake

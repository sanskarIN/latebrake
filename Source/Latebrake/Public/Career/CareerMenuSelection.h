#pragma once

#include "Career/CareerProgress.h"
#include "UI/MenuNavigation.h"

namespace latebrake
{
// One read-only selection policy for the menu label, roadmap, browse
// cursor and launch action. Page changes must not show a different
// event from the one SELECT will actually attempt to start.
constexpr bool IsCareerMenuPage(MenuPage page) noexcept
{
    return page == MenuPage::Career || page == MenuPage::Championships ||
           page == MenuPage::Challenges;
}

constexpr bool IsEventInCareerMenu(MenuPage page, int id) noexcept
{
    const auto* event = GetCareerEvent(id);
    if (!event) return false;
    switch (page)
    {
    case MenuPage::Career: return true;
    case MenuPage::Championships: return event->Series >= 0;
    case MenuPage::Challenges: return event->Series < 0;
    default: return false;
    }
}

enum class CareerMenuTargetOrigin
{
    Unavailable,
    ExplicitSelection,
    FirstAvailable,
    ReplayFallback
};

struct CareerMenuTarget final
{
    int EventId = -1;
    CareerMenuTargetOrigin Origin = CareerMenuTargetOrigin::Unavailable;
    CareerStatus Status = CareerStatus::Locked;

    constexpr bool Valid() const noexcept
    {
        return IsCareerId(EventId) && Origin != CareerMenuTargetOrigin::Unavailable;
    }
};

inline CareerMenuTarget ResolveCareerMenuTarget(
    const CareerProgress& career, MenuPage page, int selection) noexcept
{
    if (!IsCareerMenuPage(page)) return {};
    // Keep the explicitly selected eligible event, even if it is already
    // mastered: selecting a replay must never launch an unrelated round.
    if (IsEventInCareerMenu(page, selection) &&
        career.Status(selection) != CareerStatus::Locked)
    {
        return {selection, CareerMenuTargetOrigin::ExplicitSelection,
                career.Status(selection)};
    }
    // Page entry with a stale cursor: prefer uncompleted available events.
    // This matches the established championship/challenge launch fallback.
    for (int id = 0; id < CareerEventCount; ++id)
    {
        if (IsEventInCareerMenu(page, id) &&
            career.Status(id) == CareerStatus::Available)
            return {id, CareerMenuTargetOrigin::FirstAvailable, CareerStatus::Available};
    }
    // All available first medals have been collected, or all remaining
    // events depend on locked prerequisites in another page.
    for (int id = 0; id < CareerEventCount; ++id)
    {
        if (!IsEventInCareerMenu(page, id)) continue;
        const auto status = career.Status(id);
        if (status != CareerStatus::Locked)
            return {id, CareerMenuTargetOrigin::ReplayFallback, status};
    }
    return {};
}

inline CareerMenuTarget BrowseCareerMenuTarget(
    const CareerProgress& career, MenuPage page, int selection) noexcept
{
    const auto current = ResolveCareerMenuTarget(career, page, selection);
    if (!current.Valid()) return {};
    // Browse from the visible effective target, not the previous page's
    // unrelated cursor. Include the same event last if it is the only one.
    for (int delta = 1; delta <= CareerEventCount; ++delta)
    {
        const int id = (current.EventId + delta) % CareerEventCount;
        if (IsEventInCareerMenu(page, id) &&
            career.Status(id) != CareerStatus::Locked)
            return {id, CareerMenuTargetOrigin::ExplicitSelection, career.Status(id)};
    }
    return current;
}
} // namespace latebrake

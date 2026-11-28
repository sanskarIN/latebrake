#include "Career/CareerMenuSelection.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool good, const char* message)
{
    if (!good)
    {
        std::cerr << "CareerMenuSelection FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace latebrake;
    CareerProgress career;
    const auto freshChampionship = ResolveCareerMenuTarget(career, MenuPage::Championships, 6);
    Require(freshChampionship.EventId == 0 &&
            freshChampionship.Origin == CareerMenuTargetOrigin::FirstAvailable,
            "challenge cursor cannot launch a championship outside its page");
    const auto freshChallenge = ResolveCareerMenuTarget(career, MenuPage::Challenges, 0);
    Require(freshChallenge.EventId == 6 &&
            freshChallenge.Origin == CareerMenuTargetOrigin::FirstAvailable,
            "championship cursor cannot mislabel the challenge launch");
    Require(BrowseCareerMenuTarget(career, MenuPage::Championships, 6).EventId == 0,
            "single eligible round wraps to itself");
    Require(BrowseCareerMenuTarget(career, MenuPage::Challenges, 0).EventId == 6,
            "single eligible challenge wraps to itself");

    CareerSnapshot sample{};
    sample.Medals[0] = 1;
    sample.Medals[1] = 2;
    career.Load(sample);
    auto target = ResolveCareerMenuTarget(career, MenuPage::Championships, 6);
    Require(target.EventId == 2 && target.Status == CareerStatus::Available,
            "prefer unfinished available round when entering from another page");
    target = ResolveCareerMenuTarget(career, MenuPage::Championships, 0);
    Require(target.EventId == 0 && target.Origin == CareerMenuTargetOrigin::ExplicitSelection,
            "completed round intentionally selected for replay is retained");
    Require(BrowseCareerMenuTarget(career, MenuPage::Championships, 6).EventId == 0,
            "browse starts at displayed effective target");
    Require(BrowseCareerMenuTarget(career, MenuPage::Challenges, 0).EventId == 7,
            "browse from first displayed challenge advances to next eligible one");

    sample.Medals.fill(3);
    career.Load(sample);
    target = ResolveCareerMenuTarget(career, MenuPage::Championships, 9);
    Require(target.EventId == 0 && target.Status == CareerStatus::Mastered &&
            target.Origin == CareerMenuTargetOrigin::ReplayFallback,
            "fully mastered championship exposes an actual replay");
    target = ResolveCareerMenuTarget(career, MenuPage::Challenges, 3);
    Require(target.EventId == 6 && target.Origin == CareerMenuTargetOrigin::ReplayFallback,
            "fully mastered challenges expose a valid replay");

    constexpr std::array<MenuPage, 3> pages{
        MenuPage::Career, MenuPage::Championships, MenuPage::Challenges
    };
    // Exhaust all raw completion masks. CareerProgress::Load sanitizes
    // impossible prerequisite states before the read-only menu uses them.
    for (unsigned mask = 0; mask < (1u << CareerEventCount); ++mask)
    {
        CareerSnapshot snapshot{};
        for (int id = 0; id < CareerEventCount; ++id)
            snapshot.Medals[static_cast<std::size_t>(id)] =
                mask & (1u << id) ? static_cast<std::uint8_t>(1 + id % 3) : 0;
        career.Load(snapshot);
        const auto before = career.Snapshot();
        for (auto page : pages)
        {
            for (int cursor = -2; cursor <= CareerEventCount + 2; ++cursor)
            {
                const auto resolved = ResolveCareerMenuTarget(career, page, cursor);
                Require(resolved.Valid(), "every existing page has at least one eligible event");
                Require(IsEventInCareerMenu(page, resolved.EventId),
                        "display and launch target belongs to current page");
                Require(resolved.Status == career.Status(resolved.EventId) &&
                        resolved.Status != CareerStatus::Locked,
                        "target always describes actual eligible profile state");
                if (IsEventInCareerMenu(page, cursor) &&
                    career.Status(cursor) != CareerStatus::Locked)
                    Require(resolved.EventId == cursor, "valid explicit cursor retained");
                else
                {
                    int firstAvailable = -1;
                    int firstEligible = -1;
                    for (int id = 0; id < CareerEventCount; ++id)
                    {
                        if (!IsEventInCareerMenu(page, id)) continue;
                        if (firstAvailable < 0 && career.Status(id) == CareerStatus::Available)
                            firstAvailable = id;
                        if (firstEligible < 0 && career.Status(id) != CareerStatus::Locked)
                            firstEligible = id;
                    }
                    Require(resolved.EventId == (firstAvailable >= 0 ? firstAvailable : firstEligible),
                            "fallback prefers first available, then first eligible");
                }
                const auto next = BrowseCareerMenuTarget(career, page, cursor);
                Require(next.Valid() && IsEventInCareerMenu(page, next.EventId),
                        "browse remains within eligible page events");
                for (int delta = 1; delta <= CareerEventCount; ++delta)
                {
                    const int id = (resolved.EventId + delta) % CareerEventCount;
                    if (!IsEventInCareerMenu(page, id) || career.Status(id) == CareerStatus::Locked)
                        continue;
                    Require(next.EventId == id, "browse advances from displayed cursor, including wrap");
                    break;
                }
            }
        }
        const auto after = career.Snapshot();
        Require(before.Schema == after.Schema && before.Medals == after.Medals &&
                before.SelectedEvent == after.SelectedEvent,
                "resolving and browsing never mutate persistent selection or medals");
    }
    for (auto invalidPage : {MenuPage::Closed, MenuPage::Garage, MenuPage::QuickRace,
                             static_cast<MenuPage>(255)})
    {
        Require(!ResolveCareerMenuTarget(career, invalidPage, 0).Valid() &&
                !BrowseCareerMenuTarget(career, invalidPage, 0).Valid(),
                "unrelated menu never authorizes a career target");
    }
    Require(!IsEventInCareerMenu(MenuPage::Career, std::numeric_limits<int>::min()) &&
            !IsEventInCareerMenu(MenuPage::Career, std::numeric_limits<int>::max()),
            "malformed event identifiers rejected");
    std::cout << "Career menu selection exhaustive regression passed.\n";
}

#pragma once

#include "Career/CareerMenuSelection.h"
#include "Career/CareerRoadmap.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake
{
// All events remain inspectable, even if the launch cursor deliberately
// skips locked events. Each route is an independent conditional scenario;
// never add their overlapping projected rewards together.
struct CareerCatalogueRow final
{
    CareerRoadmapEvent Event{};
    int FirstRouteEvent=-1;
    int MissingPrerequisiteMedals=0;
    int ConditionalRouteLaps=0;
    std::int64_t ConditionalAdditionalFunding=0;
};

struct CareerCatalogueSection final
{
    bool Valid=false;
    MenuPage Page=MenuPage::Closed;
    int Count=0;
    int Completed=0;
    int Gold=0;
    int Ready=0;
    int ReadyFirstMedals=0;
    int LockedPrerequisite=0;
    int LockedCar=0;
    int RemainingFirstCredits=0;
    int RemainingSeriesBonus=0;
    std::array<int,CareerEventCount> RowIds{};
};

struct CareerCatalogue final
{
    bool Valid=false;
    std::array<CareerCatalogueRow,CareerEventCount> Rows{};
    std::array<CareerCatalogueSection,3> Sections{};

    const CareerCatalogueSection* Section(MenuPage page) const noexcept
    {
        if(!Valid) return nullptr;
        for(const auto& section:Sections)
            if(section.Valid && section.Page==page) return &section;
        return nullptr;
    }
};

inline CareerCatalogue BuildCareerCatalogue(const CareerSnapshot& saved,
    GarageSnapshot garage) noexcept
{
    CareerProgress progress;
    progress.Load(saved);
    GarageState state;
    state.Load(garage);
    const auto career=progress.Snapshot();
    const auto canonicalGarage=state.Snapshot();
    const auto overview=BuildCareerRoadmapOverview(career,canonicalGarage);
    CareerCatalogue out;
    for(int id=0;id<CareerEventCount;++id)
    {
        auto& row=out.Rows[static_cast<std::size_t>(id)];
        row.Event=overview.Events[static_cast<std::size_t>(id)];
        const auto route=BuildCareerRoadmap(career,canonicalGarage,id);
        if(!row.Event.Valid || !route.Valid || route.StepCount<1 ||
            route.Target.EventId!=id || route.Steps[0].EventId<0)
            return {};
        row.FirstRouteEvent=route.Steps[0].EventId;
        row.MissingPrerequisiteMedals=route.StepCount-1;
        row.ConditionalRouteLaps=route.RequiredRaceLaps;
        row.ConditionalAdditionalFunding=route.MinimumAdditionalFunding;
    }
    constexpr std::array<MenuPage,3> pages{
        MenuPage::Career,MenuPage::Championships,MenuPage::Challenges};
    for(std::size_t i=0;i<pages.size();++i)
    {
        auto& section=out.Sections[i];
        section.Page=pages[i];
        section.RowIds.fill(-1);
        for(int id=0;id<CareerEventCount;++id)
        {
            if(!IsEventInCareerMenu(section.Page,id)) continue;
            const auto& event=out.Rows[static_cast<std::size_t>(id)].Event;
            section.RowIds[static_cast<std::size_t>(section.Count++)]=id;
            if(event.Medal!=CareerMedal::None) ++section.Completed;
            if(event.Medal==CareerMedal::Gold) ++section.Gold;
            section.RemainingFirstCredits+=event.FirstCredits;
            switch(event.Readiness)
            {
                case CareerRoadmapReadiness::Ready:
                    ++section.Ready;
                    if(event.Medal==CareerMedal::None) ++section.ReadyFirstMedals;
                    break;
                case CareerRoadmapReadiness::PrerequisiteMissing:
                    ++section.LockedPrerequisite;break;
                case CareerRoadmapReadiness::RequiredCarMissing:
                    ++section.LockedCar;break;
                default:return {};
            }
        }
        if(section.Page!=MenuPage::Challenges)
            for(const auto& series:Championships)
                if(!progress.ChampionshipComplete(series.Id))
                    section.RemainingSeriesBonus+=series.FirstCompletionBonus;
        section.Valid=section.Count>0 &&
            section.Ready+section.LockedCar+section.LockedPrerequisite==section.Count;
        if(!section.Valid) return {};
    }
    out.Valid=true;
    return out;
}

// HUD-owned cache: input normalization uses the same persistence policies
// as the planners. The cursor is deliberately excluded because inspecting
// a different event does not change this catalogue or its unlock routes.
class CareerCatalogueCache final
{
public:
    const CareerCatalogue& Read(const CareerSnapshot& saved,GarageSnapshot garage) noexcept
    {
        CareerProgress progress;
        progress.Load(saved);
        GarageState state;
        state.Load(garage);
        const auto career=progress.Snapshot();
        const auto canonical=state.Snapshot();
        if(!Catalogue.Valid || LastMedals!=career.Medals ||
            LastGarage.Credits!=canonical.Credits ||
            LastGarage.OwnedVehicleMask!=canonical.OwnedVehicleMask)
        {
            Catalogue=BuildCareerCatalogue(career,canonical);
            LastMedals=career.Medals;
            LastGarage=canonical;
            if(Rebuilds!=std::numeric_limits<std::uint64_t>::max()) ++Rebuilds;
        }
        return Catalogue;
    }

    void Reset() noexcept { *this={}; }
    std::uint64_t RebuildCount() const noexcept { return Rebuilds; }

private:
    CareerCatalogue Catalogue{};
    std::array<std::uint8_t,CareerEventCount> LastMedals{};
    GarageSnapshot LastGarage{};
    std::uint64_t Rebuilds=0;
};
} // namespace latebrake

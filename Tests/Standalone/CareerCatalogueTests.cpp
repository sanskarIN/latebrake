#include "Career/CareerCatalogue.h"

#include <cstdlib>
#include <iostream>

namespace
{
using namespace latebrake;
void Require(bool condition,const char* why)
{
    if(!condition)
    {
        std::cerr<<"CareerCatalogue: "<<why<<'\n';
        std::exit(EXIT_FAILURE);
    }
}

void FreshAndProgressed()
{
    CareerSnapshot career;
    GarageSnapshot garage;
    auto catalogue=BuildCareerCatalogue(career,garage);
    const auto* all=catalogue.Section(MenuPage::Career);
    const auto* cups=catalogue.Section(MenuPage::Championships);
    const auto* challenges=catalogue.Section(MenuPage::Challenges);
    Require(catalogue.Valid && all && cups && challenges,"all page sections");
    Require(all->Count==12 && all->Ready==2 && all->LockedPrerequisite==10 &&
        all->Completed==0 && all->RemainingSeriesBonus==900,"initial whole career");
    Require(cups->Count==6 && cups->Ready==1 && cups->LockedPrerequisite==5 &&
        cups->RemainingSeriesBonus==900,"cup partition");
    Require(challenges->Count==6 && challenges->Ready==1 &&
        challenges->LockedPrerequisite==5 && challenges->RemainingSeriesBonus==0,
        "challenge partition excludes cup bonus");
    const auto& finale=catalogue.Rows[5];
    Require(finale.Event.Readiness==CareerRoadmapReadiness::PrerequisiteMissing &&
        finale.FirstRouteEvent==0 && finale.MissingPrerequisiteMedals==5 &&
        finale.ConditionalRouteLaps==14,"locked route visible without changing launch cursor");
    const auto& car=catalogue.Rows[10];
    Require(car.Event.RequiredVehicle==1 && car.Event.RequiredUnlockCost==600 &&
        car.FirstRouteEvent==0 && car.MissingPrerequisiteMedals==3 &&
        car.ConditionalAdditionalFunding==0,"conditional earlier rewards distinguished from current funds");
    career.Medals[0]=1;career.Medals[1]=2;career.Medals[2]=3;
    catalogue=BuildCareerCatalogue(career,garage);
    all=catalogue.Section(MenuPage::Career);
    Require(all && all->Completed==3 && all->Gold==1 && all->LockedCar==1 &&
        all->RemainingSeriesBonus==550,"completed cup reward excluded");
    const auto& locked=catalogue.Rows[10];
    Require(locked.Event.Readiness==CareerRoadmapReadiness::RequiredCarMissing &&
        locked.FirstRouteEvent==10 && locked.MissingPrerequisiteMedals==0 &&
        locked.ConditionalAdditionalFunding==600,"owned car required even after prior medals");
    garage.Credits=600;
    catalogue=BuildCareerCatalogue(career,garage);
    Require(catalogue.Rows[10].Event.Readiness==CareerRoadmapReadiness::RequiredCarMissing &&
        catalogue.Rows[10].ConditionalAdditionalFunding==0,"affordability does not confer ownership");
    garage.OwnedVehicleMask|=2;
    catalogue=BuildCareerCatalogue(career,garage);
    Require(catalogue.Rows[10].Event.Readiness==CareerRoadmapReadiness::Ready &&
        catalogue.Rows[10].Event.RequiredUnlockCost==0,"actual ownership unlocks eligibility");
    Require(!catalogue.Section(MenuPage::Garage) && !catalogue.Section(static_cast<MenuPage>(255)),
        "unrelated pages excluded");
}

void AllProfilesAndPartitioning()
{
    for(unsigned mask=0;mask<(1u<<CareerEventCount);++mask)
    {
        CareerSnapshot saved;
        for(int id=0;id<CareerEventCount;++id)
            if(mask&(1u<<id))saved.Medals[static_cast<std::size_t>(id)]=
                static_cast<std::uint8_t>(1+id%3);
        GarageSnapshot garage{static_cast<int>(mask%1100),static_cast<int>(mask%64),0,0,0};
        const auto c=BuildCareerCatalogue(saved,garage);
        Require(c.Valid,"valid catalogue on sanitized profile");
        const auto& all=*c.Section(MenuPage::Career);
        const auto& cups=*c.Section(MenuPage::Championships);
        const auto& challenges=*c.Section(MenuPage::Challenges);
        Require(all.Count==cups.Count+challenges.Count &&
            all.Completed==cups.Completed+challenges.Completed &&
            all.Gold==cups.Gold+challenges.Gold &&
            all.Ready==cups.Ready+challenges.Ready &&
            all.ReadyFirstMedals==cups.ReadyFirstMedals+challenges.ReadyFirstMedals &&
            all.LockedCar==cups.LockedCar+challenges.LockedCar &&
            all.LockedPrerequisite==cups.LockedPrerequisite+challenges.LockedPrerequisite &&
            all.RemainingFirstCredits==cups.RemainingFirstCredits+challenges.RemainingFirstCredits &&
            all.RemainingSeriesBonus==cups.RemainingSeriesBonus,
            "partition has no duplicated rewards or events");
        CareerProgress progress;
        progress.Load(saved);
        Require(all.Completed==progress.CompletedCount(),"authoritative completed count");
        for(const auto& section:c.Sections)
        {
            unsigned seen=0;
            for(int n=0;n<section.Count;++n)
            {
                const int id=section.RowIds[static_cast<std::size_t>(n)];
                Require(IsEventInCareerMenu(section.Page,id) && !(seen&(1u<<id)),
                    "each valid event exactly once");
                seen|=1u<<id;
                const auto& row=c.Rows[static_cast<std::size_t>(id)];
                Require(row.Event.Medal==progress.Medal(id) && row.Event.Status==progress.Status(id),
                    "actual sanitized medal status");
                const auto route=BuildCareerRoadmap(saved,garage,id);
                Require(route.Valid && row.FirstRouteEvent==route.Steps[0].EventId &&
                    row.MissingPrerequisiteMedals==route.StepCount-1 &&
                    row.ConditionalRouteLaps==route.RequiredRaceLaps &&
                    row.ConditionalAdditionalFunding==route.MinimumAdditionalFunding,
                    "exact independent route summary");
            }
            for(int n=section.Count;n<CareerEventCount;++n)
                Require(section.RowIds[static_cast<std::size_t>(n)]==-1,"unused row not actionable");
        }
    }
}

void CacheUsesOnlyRelevantInputs()
{
    CareerCatalogueCache cache;
    CareerSnapshot s;
    GarageSnapshot g;
    Require(cache.Read(s,g).Valid && cache.RebuildCount()==1,"initial build");
    for(int i=0;i<30;++i)cache.Read(s,g);
    Require(cache.RebuildCount()==1,"no repeated frame planning");
    s.SelectedEvent=6;g.EngineLevel=3;g.GripLevel=2;
    cache.Read(s,g);
    Require(cache.RebuildCount()==1,"irrelevant selection and tuning do not rebuild");
    s.Medals[5]=3;cache.Read(s,g);
    Require(cache.RebuildCount()==1,"impossible medal sanitized");
    s.Medals[0]=1;cache.Read(s,g);
    Require(cache.RebuildCount()==2,"new medal invalidates");
    s.Medals[0]=2;cache.Read(s,g);
    Require(cache.RebuildCount()==3,"upgraded medal invalidates");
    g.Credits=300;cache.Read(s,g);
    Require(cache.RebuildCount()==4,"new funds invalidate");
    g.OwnedVehicleMask|=2;cache.Read(s,g);
    Require(cache.RebuildCount()==5,"new ownership invalidates");
    cache.Reset();
    Require(cache.RebuildCount()==0 && cache.Read(s,g).Valid &&
        cache.RebuildCount()==1,"explicit reset rebuilds");
}
}
int main()
{
    FreshAndProgressed();
    AllProfilesAndPartitioning();
    CacheUsesOnlyRelevantInputs();
    std::cout<<"Career catalogue and page partition tests passed\n";
}

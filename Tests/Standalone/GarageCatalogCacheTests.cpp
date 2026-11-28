#include "UI/GarageVehicleCards.h"
#include "UI/GarageComparisonPanel.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <string_view>

namespace
{
using namespace latebrake;
void Require(bool ok,const char* why)
{
    if(!ok){std::cerr<<"GarageCatalogCache: "<<why<<'\n';std::exit(EXIT_FAILURE);}
}
bool SamePerformance(VehiclePerformance a,VehiclePerformance b)
{
    return a.Acceleration==b.Acceleration && a.Grip==b.Grip && a.TopSpeed==b.TopSpeed;
}
void Equal(const GarageCatalogView& a,const GarageCatalogView& b)
{
    Require(a.SelectedVehicle==b.SelectedVehicle &&
        a.OwnedVehicleCount==b.OwnedVehicleCount && a.Credits==b.Credits &&
        a.NextUnlockId==b.NextUnlockId && a.EngineLevel==b.EngineLevel &&
        a.GripLevel==b.GripLevel,"canonical catalog identity");
    for(std::size_t i=0;i<a.Cars.size();++i)
    {
        const auto& x=a.Cars[i];const auto& y=b.Cars[i];
        Require(x.VehicleId==y.VehicleId && std::string_view(x.Name)==y.Name &&
            x.Ownership==y.Ownership && x.UnlockCost==y.UnlockCost &&
            x.CreditsShort==y.CreditsShort && SamePerformance(x.Effective,y.Effective) &&
            x.RouteValid==y.RouteValid && x.RouteCars==y.RouteCars &&
            x.RouteCost==y.RouteCost && x.RouteCreditsShort==y.RouteCreditsShort &&
            x.RouteBudgetReady==y.RouteBudgetReady && x.IsNextUnlock==y.IsNextUnlock,
            "every catalogue field matches fresh authoritative calculation");
    }
}
void AllEconomyFieldsInvalidate()
{
    GarageCatalogCache cache;
    GarageSnapshot saved{1000,1,0,0,0};
    Equal(cache.Read(saved),BuildGarageCatalogView(saved));
    Require(cache.RebuildCount()==1,"first read builds");
    for(int i=0;i<1000;++i)Equal(cache.Read(saved),BuildGarageCatalogView(saved));
    Require(cache.RebuildCount()==1,"unchanged reads share a single build");
    for(int n=0;n<5;++n)
    {
        switch(n)
        {
        case 0:++saved.Credits;break;
        case 1:saved.OwnedVehicleMask=3;break;
        case 2:saved.SelectedVehicle=1;break;
        case 3:++saved.EngineLevel;break;
        case 4:++saved.GripLevel;break;
        }
        Equal(cache.Read(saved),BuildGarageCatalogView(saved));
        Require(cache.RebuildCount()==static_cast<unsigned>(n+2),"every canonical change rebuilds");
    }
    auto equivalent=saved;equivalent.OwnedVehicleMask|=1024;
    Equal(cache.Read(equivalent),BuildGarageCatalogView(saved));
    Require(cache.RebuildCount()==6,"ignored future bits do not invalidate canonical key");
    cache.Reset();
    Require(cache.RebuildCount()==0,"reset clears metrics");
    Equal(cache.Read(saved),BuildGarageCatalogView(saved));
    Require(cache.RebuildCount()==1,"reset forces fresh view");
}
void SharedConsumerAndAppearanceChanges()
{
    GarageCatalogCache cache;
    GarageSnapshot saved{9500,1,0,0,0};
    GarageAppearanceState looks;
    for(int frame=0;frame<120;++frame)
    {
        const int viewed=frame%VehicleCount;
        const auto snap=BuildGarageVehicleCardsSnapshot(true,true,false,false,
            viewed,static_cast<std::uint64_t>(frame+1),saved,looks);
        const auto cards=DescribeGarageVehicleCards(snap,&cache);
        const auto comparison=ReadGarageComparisonPanel(cache.Read(saved),viewed);
        const auto& text=cache.Read(saved);
        Require(comparison.Valid && comparison.ViewedId==viewed &&
            text.Credits==saved.Credits && cache.RebuildCount()==1,
            "three drawing consumers share one six-car simulation");
        for(int id=0;id<VehicleCount;++id)
        {
            const auto& card=cards[static_cast<std::size_t>(id)];
            Require(card.Valid && card.Id==id && card.Viewed==(id==viewed) &&
                SameGarageLook(card.Look,looks.Get(id)) &&
                card.RouteCost==text.Cars[static_cast<std::size_t>(id)].RouteCost,
                "fresh cursor and appearance use cached economy only");
        }
        Require(looks.Cycle(viewed,GarageLookLayer::Paint),"change visual source each frame");
    }
    auto invalid=BuildGarageVehicleCardsSnapshot(true,true,false,false,0,1,saved,looks);
    invalid.Looks[0]=255;
    for(const auto& card:DescribeGarageVehicleCards(invalid,&cache))
        Require(!card.Valid,"corrupt appearance cannot borrow cached identity");
    Require(cache.RebuildCount()==1,"invalid snapshot never contaminates cache");
}
void RandomSanitizedTransitions()
{
    std::mt19937 rng(670219);
    GarageCatalogCache cache;
    for(int i=0;i<5000;++i)
    {
        GarageSnapshot saved{static_cast<int>(rng()%30000)-100,
            static_cast<int>(rng()%150)-5,static_cast<int>(rng()%12)-3,
            static_cast<int>(rng()%8)-2,static_cast<int>(rng()%8)-2};
        const auto fresh=BuildGarageCatalogView(saved);
        Equal(cache.Read(saved),fresh);
        const auto rebuilds=cache.RebuildCount();
        GarageState garage;garage.Load(saved);
        Equal(cache.Read(garage.Snapshot()),fresh);
        Require(cache.RebuildCount()==rebuilds,"canonical equivalent read remains cached");
        const auto next=garage.NextLockedVehicle();
        if(next>=0 && garage.CanUnlock(next))
        {
            Require(garage.Unlock(next),"real state mutation");
            Equal(cache.Read(garage.Snapshot()),BuildGarageCatalogView(garage.Snapshot()));
            Require(cache.RebuildCount()==rebuilds+1,"actual ownership and credit change observed");
        }
    }
}
} // namespace
int main()
{
    AllEconomyFieldsInvalidate();SharedConsumerAndAppearanceChanges();RandomSanitizedTransitions();
    std::cout<<"Shared garage catalogue caching and fresh appearance tests passed\n";
}

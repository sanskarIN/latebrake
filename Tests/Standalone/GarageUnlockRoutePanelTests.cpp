#include "UI/GarageUnlockRoutePanel.h"
#include "UI/GarageShowroomLane.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

namespace
{
using namespace latebrake;
void Require(bool ok,const char* why)
{
    if(!ok){std::cerr<<"GarageUnlockRoutePanel: "<<why<<'\n';std::exit(EXIT_FAILURE);}
}
bool Near(double a,double b){return std::abs(a-b)<1e-7;}

void VerifyAgainstGarage(GarageSnapshot saved,int target)
{
    GarageState initial;initial.Load(saved);
    const auto canonical=initial.Snapshot();
    const auto data=ReadGarageUnlockRoute(saved,target);
    Require(data.Valid && data.Viewed==target && data.Equipped==initial.SelectedVehicle() &&
        data.Credits==initial.Credits(),"canonical source fields");
    int total=0,count=0,funded=0;
    if(!initial.Owns(target))
        for(int id=1;id<=target;++id)
        {
            if(initial.Owns(id))continue;
            const auto& row=data.Steps[static_cast<std::size_t>(count)];
            const int cost=GarageState::Definition(id).UnlockCost;
            total+=cost;
            const int shortfall=std::max(0,total-canonical.Credits);
            Require(row.Vehicle==id && row.Cost==cost && row.CumulativeCost==total &&
                row.CumulativeShortfall==shortfall && row.Funded==(shortfall==0) &&
                row.Next==(count==0),"independent sequential route oracle");
            if(shortfall==0)++funded;
            ++count;
        }
    Require(data.Count==count && data.FundedCount==funded && data.TotalCost==total &&
        data.Shortfall==std::max(0,total-canonical.Credits),"route totals conserve credits");
    const auto expected=count==0?(target==canonical.SelectedVehicle?
        GarageUnlockRouteState::AlreadyEquipped:GarageUnlockRouteState::Owned):
        funded==count?GarageUnlockRouteState::FullyFunded:
        funded>0?GarageUnlockRouteState::PartlyFunded:GarageUnlockRouteState::NeedCredits;
    Require(data.State==expected,"read-only funding state");
    if(count>0)
    {
        const DesktopMenuMouseRect track{12,20,300,7};
        const auto bar=BuildGarageUnlockRouteBar(track,data);
        Require(bar.Valid && bar.BoundaryCount==count-1 &&
            Near(bar.Funded.Width,track.Width*std::min(canonical.Credits,total)/total),
            "funding bar is proportional to credits");
        for(int i=0;i<bar.BoundaryCount;++i)
            Require(Near(bar.Boundaries[static_cast<std::size_t>(i)],
                track.X+track.Width*data.Steps[static_cast<std::size_t>(i)].CumulativeCost/total),
                "each divider represents an exact cumulative price");
    }
    else Require(!BuildGarageUnlockRouteBar({12,20,300,7},data).Valid,"no fabricated purchase bar");
    Require(initial.Snapshot().Credits==canonical.Credits &&
        initial.Snapshot().OwnedVehicleMask==canonical.OwnedVehicleMask,
        "read-only original economy remains intact");
}

void ExhaustOwnershipAndCreditBoundaries()
{
    for(int mask=0;mask<=KnownVehicleMask;++mask)
        for(int target=0;target<VehicleCount;++target)
        {
            int cumulative=0;
            VerifyAgainstGarage({0,mask,0,1,2},target);
            for(int id=1;id<=target;++id)
            {
                if((mask&(1<<id))!=0)continue;
                cumulative+=GarageState::Definition(id).UnlockCost;
                for(int offset:{-1,0,1})
                    VerifyAgainstGarage({cumulative+offset,mask,target,3,3},target);
            }
            VerifyAgainstGarage({std::numeric_limits<int>::max(),mask,target,0,0},target);
        }
    for(int bad:{-1,VehicleCount,std::numeric_limits<int>::max()})
        Require(!ReadGarageUnlockRoute({},bad).Valid,"invalid viewed id rejected");
    VerifyAgainstGarage({-1,-1,999,99,-1},5);
    VerifyAgainstGarage({9500,1024|1,5,2,1},5);
}

void ActualPurchaseProgression()
{
    GarageState live;live.Load({2000,1,0,0,0});
    const auto before=ReadGarageUnlockRoute(live.Snapshot(),5);
    Require(before.TotalCost==9500 && before.FundedCount==2 && before.Shortfall==7500,
        "actual full route from starter");
    for(int i=0;i<2;++i)
    {
        const auto prior=ReadGarageUnlockRoute(live.Snapshot(),5);
        const auto first=prior.Steps[0];
        Require(first.Next && first.Funded && live.NextLockedVehicle()==first.Vehicle,
            "preview matches authorized next purchase");
        Require(live.Unlock(first.Vehicle),"simulate separately authorized purchase");
        const auto next=ReadGarageUnlockRoute(live.Snapshot(),5);
        Require(next.Count==prior.Count-1 && next.TotalCost==prior.TotalCost-first.Cost &&
            next.Credits==prior.Credits-first.Cost && next.Shortfall==prior.Shortfall &&
            next.FundedCount==prior.FundedCount-1,"route advances only after actual purchase");
    }
    Require(!live.CanUnlock(live.NextLockedVehicle()),"remaining step not affordable");
    live.EarnCredits(7500);
    auto data=ReadGarageUnlockRoute(live.Snapshot(),5);
    Require(data.FundedCount==data.Count && data.Shortfall==0,"real earned credits fund remainder");
    while(live.NextLockedVehicle()>=0)
        Require(live.Unlock(live.NextLockedVehicle()),"remaining separately funded purchases");
    data=ReadGarageUnlockRoute(live.Snapshot(),5);
    Require(data.Count==0 && data.State==GarageUnlockRouteState::Owned &&
        live.SelectedVehicle()==0,"ownership never automatically equips");
    Require(live.Select(5),"explicit equip");
    Require(ReadGarageUnlockRoute(live.Snapshot(),5).State==GarageUnlockRouteState::AlreadyEquipped,
        "actual equipped identity");
}

void CacheChanges()
{
    GarageUnlockRouteCache cache;
    GarageSnapshot s{2000,1,0,0,0};
    Require(cache.Read(s,5).Valid && cache.RebuildCount()==1,"initial read");
    for(int i=0;i<1000;++i)Require(cache.Read(s,5).Count==5,"stable route");
    Require(cache.RebuildCount()==1,"no repeated frame simulation");
    for(int fault=0;fault<5;++fault)
    {
        switch(fault)
        {
        case 0:++s.Credits;break;
        case 1:s.OwnedVehicleMask=3;break;
        case 2:s.SelectedVehicle=1;break;
        case 3:++s.EngineLevel;break;
        case 4:++s.GripLevel;break;
        }
        Require(cache.Read(s,5).Valid && cache.RebuildCount()==static_cast<unsigned>(fault+2),
            "each real economy change invalidates cache");
    }
    Require(cache.Read(s,4).Viewed==4 && cache.RebuildCount()==7,"view identity changes");
    auto equivalent=s;equivalent.OwnedVehicleMask|=1024;
    Require(cache.Read(equivalent,4).Valid && cache.RebuildCount()==7,"sanitized equivalent key reused");
    Require(!cache.Read(s,-1).Valid,"invalid target never returns stale data");
    cache.Reset();
    Require(cache.RebuildCount()==0 && cache.Read(s,5).Valid && cache.RebuildCount()==1,"cache reset");
}

struct View final
{
    double W,H,Safe;
    HudLayout Hud{};
    HudMenuVerticalPlan Vertical{};
    HudMenuReadablePanel Readable{};
    GarageShowroomLane Showroom{};
    DesktopMenuMouseLayout Dock{};
    View(double w=1280,double h=720):W(w),H(h),Safe(0)
    {
        Hud=CalculateHudLayout(w,h);
        Vertical=CalculateHudMenuVerticalPlan(w,h,Hud,false);
        const auto text=CalculateHudTextBounds(w,h,Hud,false);
        Dock=CalculateDesktopMenuMouseLayout(w,h,Hud,false,true);
        Showroom=CalculateGarageShowroomLane(w,h,Hud,false,true,MenuPage::Garage);
        Safe=text.MaxWidthPixels;
        if(Dock.Valid)Safe=std::min(Safe,Dock.Left-text.Left-12*Hud.Scale);
        if(Showroom.Valid)Safe=std::min(Safe,Showroom.TextWidth);
        Readable=CalculateHudMenuReadablePanel(w,h,Hud,Vertical,Safe,true);
    }
    GarageUnlockRouteLayout Plan(int count,int pinned=2,bool garage=true,bool touch=false) const
    {
        return CalculateGarageUnlockRouteLayout(Hud,Vertical,Readable,Safe,
            Hud.Top+pinned*28*Hud.Scale,pinned,count,garage,touch);
    }
};
GarageUnlockRouteMeasurements Measures(const GarageUnlockRouteLayout& p)
{
    GarageUnlockRouteMeasurements m;
    const auto size=[](DesktopMenuMouseRect r){return HudRaceStatusTextSize{r.Width*0.8,r.Height*0.8};};
    m.Header=size(p.Header);m.Summary=size(p.Summary);
    for(int i=0;i<p.Count;++i)m.Steps[static_cast<std::size_t>(i)]=size(p.Steps[static_cast<std::size_t>(i)]);
    return m;
}
void CheckLayout(const View& v,const GarageUnlockRouteLayout& p,int pinned)
{
    Require(p.Valid && p.Panel.Width<=v.Safe+1e-7 &&
        HudRadarContains({0,0,v.W,v.H},p.Panel),"bounded route panel");
    const int remaining=v.Vertical.MaxDetailsRows-pinned-p.ReservedRows;
    Require(remaining>=GarageUnlockRouteMinimumDetails &&
        p.NextDetailY>=p.Panel.Y+p.Panel.Height &&
        p.NextDetailY+(remaining-1)*28*v.Hud.Scale<=v.Vertical.OverflowNoticeY-1.25*28*v.Hud.Scale+1e-7,
        "preserve detail pager and save warning");
    if(v.Showroom.Valid)Require(!HudRadarOverlaps(v.Showroom.Aperture,p.Panel),"live showroom clear");
    if(v.Dock.Valid)
        for(const auto& b:v.Dock.Buttons)
            Require(!HudRadarOverlaps(b.Bounds,p.Panel),"navigation dock clear");
    Require(CanDrawGarageUnlockRoute(p,Measures(p)),"complete visible captions");
}
void ResponsiveAndMalformed()
{
    const View v;
    for(int count=1;count<VehicleCount;++count)
    {
        const auto p=v.Plan(count);
        CheckLayout(v,p,2);
        auto m=Measures(p);m.Summary.Width=p.Summary.Width+1;
        Require(!CanDrawGarageUnlockRoute(p,m),"summary clipping refused");
        m=Measures(p);m.Steps[static_cast<std::size_t>(count-1)].Height=0;
        Require(!CanDrawGarageUnlockRoute(p,m),"missing final step refused");
    }
    Require(!v.Plan(0).Valid && !v.Plan(VehicleCount).Valid &&
        !v.Plan(2,2,false).Valid && !v.Plan(2,2,true,true).Valid,"unsupported layouts fallback");
    auto narrow=v;narrow.Safe=100;
    Require(!narrow.Plan(2).Valid,"too narrow");
    auto shortPage=v;shortPage.Vertical.MaxDetailsRows=6;
    Require(!shortPage.Plan(5).Valid,"preserve details");
    const auto data=ReadGarageUnlockRoute({2000,1,0,0,0},5);
    const auto track=v.Plan(5).Track;
    for(int fault=0;fault<9;++fault)
    {
        auto bad=data;
        switch(fault)
        {
        case 0:bad.TotalCost=0;break;
        case 1:bad.Shortfall=1;break;
        case 2:bad.Steps[0].Cost=0;break;
        case 3:bad.Steps[1].Vehicle=1;break;
        case 4:bad.Steps[0].CumulativeCost=1;break;
        case 5:bad.Steps[0].Next=false;break;
        case 6:bad.FundedCount=0;break;
        case 7:bad.Viewed=4;break;
        case 8:bad.Steps[0].CumulativeShortfall=1;break;
        }
        Require(!BuildGarageUnlockRouteBar(track,bad).Valid,"malformed chart data rejected");
    }
}
void RandomViewports()
{
    std::mt19937 rng(91528);
    int visible=0;
    for(int i=0;i<7000;++i)
    {
        const View v(240+rng()%3800,240+rng()%2000);
        const int count=1+static_cast<int>(rng()%(VehicleCount-1));
        const int pinned=2+static_cast<int>(rng()%7);
        const auto p=v.Plan(count,pinned);
        if(!p.Valid)continue;
        ++visible;CheckLayout(v,p,pinned);
        const auto data=ReadGarageUnlockRoute({static_cast<int>(rng()%10000),1,0,0,0},count);
        Require(data.Count==count && BuildGarageUnlockRouteBar(p.Track,data).Valid,
            "actual route funding fits scaled track");
    }
    Require(visible>2000,"broad desktop coverage");
}
} // namespace
int main()
{
    ExhaustOwnershipAndCreditBoundaries();ActualPurchaseProgression();CacheChanges();
    ResponsiveAndMalformed();RandomViewports();
    std::cout<<"Garage unlock roadmap, funding and cache tests passed\n";
}

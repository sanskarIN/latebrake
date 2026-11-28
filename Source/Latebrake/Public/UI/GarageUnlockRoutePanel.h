#pragma once

#include "Garage/GaragePurchasePlanner.h"
#include "UI/HudRaceStatusPanel.h"
#include "UI/HudMenuReadablePanel.h"

#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace latebrake
{
// Read-only roadmap to the currently viewed car. The actual Q purchase
// still targets only GarageState::NextLockedVehicle(), one transaction
// at a time. No projected ownership is treated as current ownership.
enum class GarageUnlockRouteState : std::uint8_t
{
    AlreadyEquipped, Owned, FullyFunded, PartlyFunded, NeedCredits
};

struct GarageUnlockRouteStep final
{
    int Vehicle=-1;
    int Cost=0;
    int CumulativeCost=0;
    int CumulativeShortfall=0;
    bool Funded=false;
    bool Next=false;
};

struct GarageUnlockRouteReadout final
{
    bool Valid=false;
    int Viewed=-1;
    int Equipped=-1;
    int Credits=0;
    int TotalCost=0;
    int Shortfall=0;
    int Count=0;
    int FundedCount=0;
    GarageUnlockRouteState State=GarageUnlockRouteState::NeedCredits;
    std::array<GarageUnlockRouteStep,VehicleCount-1> Steps{};
};

inline const char* GarageUnlockRouteStateName(GarageUnlockRouteState state) noexcept
{
    switch(state)
    {
    case GarageUnlockRouteState::AlreadyEquipped:return "ALREADY EQUIPPED";
    case GarageUnlockRouteState::Owned:return "OWNED / SELECT TO EQUIP";
    case GarageUnlockRouteState::FullyFunded:return "ROUTE FUNDED";
    case GarageUnlockRouteState::PartlyFunded:return "PART OF ROUTE FUNDED";
    case GarageUnlockRouteState::NeedCredits:return "SAVE CREDITS FOR ROUTE";
    default:return "ROUTE UNAVAILABLE";
    }
}

inline GarageUnlockRouteReadout ReadGarageUnlockRoute(GarageSnapshot persisted,
    int viewed) noexcept
{
    if(!GarageState::IsVehicle(viewed))return {};
    GarageState garage;
    garage.Load(persisted);
    const auto current=garage.Snapshot();
    const auto path=PlanGaragePurchasePath(current,
        {viewed,current.EngineLevel,current.GripLevel,0});
    if(path.Status==GaragePlanStatus::Invalid || !path.ReadOnly ||
        path.AdditionalCarsUnlocked<0 || path.AdditionalCarsUnlocked>=VehicleCount ||
        path.ActionCount>path.Actions.size() || path.EngineCost!=0 ||
        path.GripCost!=0 || path.TotalCost!=path.UnlockCost || path.TotalCost<0)
        return {};
    GarageUnlockRouteReadout out;
    out.Viewed=viewed;out.Equipped=current.SelectedVehicle;
    out.Credits=current.Credits;out.TotalCost=path.UnlockCost;
    out.Shortfall=std::max(0,out.TotalCost-out.Credits);
    int previous=-1,cumulative=0;
    for(unsigned i=0;i<path.ActionCount;++i)
    {
        const auto& action=path.Actions[i];
        if(action.Kind==GaragePlanActionKind::SelectOwned)
        {
            if(i+1!=path.ActionCount || action.Target!=viewed ||
                action.Cost!=0 || action.RequiresPurchaseConfirmation)
                return {};
            continue;
        }
        if(action.Kind!=GaragePlanActionKind::UnlockNext ||
            !GarageState::IsVehicle(action.Target) || action.Target<=previous ||
            action.Target>viewed || garage.Owns(action.Target) ||
            action.Cost!=GarageState::Definition(action.Target).UnlockCost ||
            action.Cost<=0 || !action.RequiresPurchaseConfirmation ||
            out.Count>=static_cast<int>(out.Steps.size()) ||
            action.Cost>std::numeric_limits<int>::max()-cumulative)
            return {};
        cumulative+=action.Cost;
        if(cumulative!=action.CumulativeCost)return {};
        const int shortfall=std::max(0,cumulative-out.Credits);
        const bool funded=shortfall==0;
        out.Steps[static_cast<std::size_t>(out.Count)]={action.Target,
            action.Cost,cumulative,shortfall,funded,out.Count==0};
        if(funded)++out.FundedCount;
        ++out.Count;
        previous=action.Target;
    }
    if(out.Count!=path.AdditionalCarsUnlocked || cumulative!=out.TotalCost ||
        out.Shortfall!=path.CreditsShort ||
        (out.Count==0)!=garage.Owns(viewed) ||
        (out.Count>0 && (previous!=viewed ||
            out.Steps[0].Vehicle!=garage.NextLockedVehicle())))
        return {};
    out.State=out.Count==0?(viewed==out.Equipped?
        GarageUnlockRouteState::AlreadyEquipped:GarageUnlockRouteState::Owned):
        out.FundedCount==out.Count?GarageUnlockRouteState::FullyFunded:
        out.FundedCount>0?GarageUnlockRouteState::PartlyFunded:GarageUnlockRouteState::NeedCredits;
    out.Valid=true;
    return out;
}

// Cache only the displayed route, independently of the larger tuning
// advisor. Every economy field is part of the key; unrelated styling
// does not force a new planning simulation.
class GarageUnlockRouteCache final
{
public:
    const GarageUnlockRouteReadout& Read(GarageSnapshot saved,int viewed) noexcept
    {
        GarageState garage;garage.Load(saved);
        const auto current=garage.Snapshot();
        if(!Cached.Valid || Cached.Viewed!=viewed ||
            current.Credits!=Key.Credits || current.OwnedVehicleMask!=Key.OwnedVehicleMask ||
            current.SelectedVehicle!=Key.SelectedVehicle ||
            current.EngineLevel!=Key.EngineLevel || current.GripLevel!=Key.GripLevel)
        {
            Key=current;Cached=ReadGarageUnlockRoute(current,viewed);
            if(Rebuilds!=std::numeric_limits<std::uint64_t>::max())++Rebuilds;
        }
        return Cached;
    }
    std::uint64_t RebuildCount() const noexcept{return Rebuilds;}
    void Reset() noexcept{Key={};Cached={};Rebuilds=0;}
private:
    GarageSnapshot Key{};
    GarageUnlockRouteReadout Cached{};
    std::uint64_t Rebuilds=0;
};

inline constexpr int GarageUnlockRouteMinimumDetails=6;
struct GarageUnlockRouteLayout final
{
    bool Valid=false;
    double Scale=1;
    int Count=0,ReservedRows=0;
    double NextDetailY=0;
    DesktopMenuMouseRect Panel{},Header{},Summary{},Track{};
    std::array<DesktopMenuMouseRect,VehicleCount-1> Steps{};
};

// Compact horizontal route funding strip plus individual step labels.
// Uses the protected garage text lane and may fall back to the same
// detailed text rows if the remaining menu height is insufficient.
inline GarageUnlockRouteLayout CalculateGarageUnlockRouteLayout(
    HudLayout hud,HudMenuVerticalPlan vertical,const HudMenuReadablePanel& readable,
    double safeWidth,double y,int drawnRows,int count,bool garage,bool touch) noexcept
{
    if(!garage || touch || hud.Compact || !vertical.Valid || !readable.Valid ||
        count<1 || count>=VehicleCount ||
        !std::isfinite(hud.Scale) || hud.Scale<0.6 || hud.Scale>1.2 ||
        !std::isfinite(hud.Left) || !std::isfinite(hud.Top) ||
        !std::isfinite(safeWidth) || safeWidth<=0 || !std::isfinite(y) ||
        drawnRows<0 || drawnRows>vertical.MaxDetailsRows || vertical.MaxDetailsRows>90 ||
        !std::isfinite(vertical.OverflowNoticeY) || !std::isfinite(vertical.SaveWarningY) ||
        vertical.OverflowNoticeY>=vertical.SaveWarningY)return {};
    const double s=hud.Scale,step=28*s;
    if(std::abs(y-hud.Top-drawnRows*step)>0.02)return {};
    const double width=std::min(safeWidth,readable.X+readable.Width-hud.Left);
    if(width<242*s)return {};
    const double panelHeight=(59+19*count)*s;
    const int reserve=static_cast<int>(std::ceil((panelHeight+6*s)/step));
    const int remaining=vertical.MaxDetailsRows-drawnRows-reserve;
    if(remaining<GarageUnlockRouteMinimumDetails)return {};
    GarageUnlockRouteLayout out;
    out.Scale=s;out.Count=count;out.ReservedRows=reserve;out.NextDetailY=y+reserve*step;
    out.Panel={hud.Left,y,width,panelHeight};
    const DesktopMenuMouseRect owner{readable.X,readable.Y,readable.Width,readable.Height};
    if(!HudRadarContains(owner,out.Panel) || out.NextDetailY<y+panelHeight ||
        out.NextDetailY+(remaining-1)*step>vertical.OverflowNoticeY-1.25*step+1e-7)
        return {};
    const double x=hud.Left+8*s,w=width-16*s;
    out.Header={x,y+5*s,w,16*s};
    out.Summary={x,y+23*s,w,15*s};
    out.Track={x,y+42*s,w,6*s};
    for(int i=0;i<count;++i)
        out.Steps[static_cast<std::size_t>(i)]={x,y+(53+19*i)*s,w,16*s};
    std::array<DesktopMenuMouseRect,VehicleCount+2> parts{};
    parts[0]=out.Header;parts[1]=out.Summary;parts[2]=out.Track;
    for(int i=0;i<count;++i)parts[static_cast<std::size_t>(i+3)]=out.Steps[static_cast<std::size_t>(i)];
    for(int i=0;i<count+3;++i)
    {
        if(!HudRadarContains(out.Panel,parts[static_cast<std::size_t>(i)]))return {};
        for(int j=0;j<i;++j)
            if(HudRadarOverlaps(parts[static_cast<std::size_t>(i)],parts[static_cast<std::size_t>(j)]))
                return {};
    }
    out.Valid=true;
    return out;
}

struct GarageUnlockRouteBar final
{
    bool Valid=false;
    DesktopMenuMouseRect Funded{};
    std::array<double,VehicleCount-2> Boundaries{};
    int BoundaryCount=0;
};
inline GarageUnlockRouteBar BuildGarageUnlockRouteBar(const DesktopMenuMouseRect& track,
    const GarageUnlockRouteReadout& data) noexcept
{
    if(!ValidHudRadarRect(track) || !data.Valid || data.Count<=0 ||
        data.Count>=VehicleCount || data.TotalCost<=0 || data.Credits<0 ||
        data.Shortfall!=std::max(0,data.TotalCost-data.Credits))return {};
    GarageUnlockRouteBar out;
    int cumulative=0,fundedCount=0,last=-1;
    for(int i=0;i<data.Count;++i)
    {
        const auto& s=data.Steps[static_cast<std::size_t>(i)];
        if(!GarageState::IsVehicle(s.Vehicle) || s.Vehicle<=last ||
            s.Cost<=0 || s.Cost!=GarageState::Definition(s.Vehicle).UnlockCost ||
            s.Cost>data.TotalCost-cumulative || s.Next!=(i==0))return {};
        cumulative+=s.Cost;
        if(s.CumulativeCost!=cumulative ||
            s.CumulativeShortfall!=std::max(0,cumulative-data.Credits) ||
            s.Funded!=(s.CumulativeShortfall==0))return {};
        if(s.Funded)++fundedCount;
        if(i+1<data.Count)
            out.Boundaries[static_cast<std::size_t>(out.BoundaryCount++)]=
                track.X+track.Width*static_cast<double>(cumulative)/data.TotalCost;
        last=s.Vehicle;
    }
    if(cumulative!=data.TotalCost || fundedCount!=data.FundedCount ||
        last!=data.Viewed)return {};
    out.Funded={track.X,track.Y,track.Width*static_cast<double>(
        std::min(data.Credits,data.TotalCost))/data.TotalCost,track.Height};
    if(out.Funded.Width>0 && !HudRadarContains(track,out.Funded))return {};
    out.Valid=true;
    return out;
}

struct GarageUnlockRouteMeasurements final
{
    HudRaceStatusTextSize Header{},Summary{};
    std::array<HudRaceStatusTextSize,VehicleCount-1> Steps{};
};
inline bool CanDrawGarageUnlockRoute(const GarageUnlockRouteLayout& p,
    const GarageUnlockRouteMeasurements& m) noexcept
{
    if(!p.Valid || p.Count<=0 || p.Count>=VehicleCount ||
        !HudRaceStatusTextFits(p.Header,m.Header) ||
        !HudRaceStatusTextFits(p.Summary,m.Summary))return false;
    for(int i=0;i<p.Count;++i)
        if(!HudRaceStatusTextFits(p.Steps[static_cast<std::size_t>(i)],
            m.Steps[static_cast<std::size_t>(i)]))return false;
    return true;
}
} // namespace latebrake

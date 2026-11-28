#pragma once

#include "Career/CareerProgress.h"
#include "Garage/GarageState.h"
#include "UI/HudRaceStatusPanel.h"
#include "UI/HudMenuReadablePanel.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace latebrake
{
// Read-only profile view. Counts are never treated as newly earned rewards.
enum class ProfileOverviewStatus : std::uint8_t
{
    Current, ReadOnly, SaveFailed
};
inline const char* ProfileOverviewStatusLabel(ProfileOverviewStatus status) noexcept
{
    switch(status)
    {
    case ProfileOverviewStatus::Current:return "LOCAL PROFILE";
    case ProfileOverviewStatus::ReadOnly:return "READ-ONLY PROFILE";
    case ProfileOverviewStatus::SaveFailed:return "SAVE FAILED / CHANGES MAY BE UNSAVED";
    default:return "PROFILE STATUS UNAVAILABLE";
    }
}
struct ProfileOverviewSource final
{
    CareerSnapshot Career{};
    GarageSnapshot Garage{};
    int CompletedRaces=0;
    double CircuitBest=0;
    double BestLap=0;
    bool ReadOnly=false;
    bool SaveFailed=false;
};
struct ProfileOverviewReadout final
{
    bool Valid=false;
    ProfileOverviewStatus Status=ProfileOverviewStatus::Current;
    int CompletedRaces=0;
    int Credits=0;
    int EquippedCar=0;
    int OwnedCars=0;
    int CareerCompleted=0;
    int Gold=0;
    int Silver=0;
    int Bronze=0;
    int EngineLevel=0;
    int GripLevel=0;
    bool CircuitBestValid=false;
    double CircuitBest=0;
    bool BestLapValid=false;
    double BestLap=0;
};

inline ProfileOverviewReadout ReadProfileOverview(const ProfileOverviewSource& source) noexcept
{
    CareerProgress career;
    career.Load(source.Career);
    GarageState garage;
    garage.Load(source.Garage);
    ProfileOverviewReadout out;
    out.Status=source.SaveFailed?ProfileOverviewStatus::SaveFailed:
        source.ReadOnly?ProfileOverviewStatus::ReadOnly:ProfileOverviewStatus::Current;
    out.CompletedRaces=SanitizeCompletedRaces(source.CompletedRaces);
    out.Credits=garage.Credits();
    out.EquippedCar=garage.SelectedVehicle();
    out.EngineLevel=garage.EngineLevel();
    out.GripLevel=garage.GripLevel();
    for(int id=0;id<VehicleCount;++id)
        if(garage.Owns(id))++out.OwnedCars;
    for(int id=0;id<CareerEventCount;++id)
    {
        switch(career.Medal(id))
        {
        case CareerMedal::Gold:++out.Gold;break;
        case CareerMedal::Silver:++out.Silver;break;
        case CareerMedal::Bronze:++out.Bronze;break;
        default:break;
        }
    }
    out.CareerCompleted=out.Gold+out.Silver+out.Bronze;
    // Unknown profile versions must not imply that their opaque fields
    // are known persisted record values.
    if(!source.ReadOnly && !source.SaveFailed)
    {
        out.CircuitBest=SanitizeBestTime(source.CircuitBest);
        out.BestLap=SanitizeBestTime(source.BestLap);
        out.CircuitBestValid=out.CircuitBest>0;
        out.BestLapValid=out.BestLap>0;
    }
    out.Valid=out.OwnedCars>=1 && out.OwnedCars<=VehicleCount &&
        out.CareerCompleted==career.CompletedCount();
    return out;
}

inline constexpr int ProfileOverviewTiles=6;
inline constexpr int ProfileOverviewMinDetails=3;
struct ProfileOverviewTile final
{
    DesktopMenuMouseRect Bounds{};
    DesktopMenuMouseRect Label{};
    DesktopMenuMouseRect Value{};
};
struct ProfileOverviewLayout final
{
    bool Valid=false;
    double Scale=1;
    int ReservedRows=0;
    double NextDetailY=0;
    DesktopMenuMouseRect Panel{};
    DesktopMenuMouseRect Header{};
    std::array<ProfileOverviewTile,ProfileOverviewTiles> Tiles{};
    DesktopMenuMouseRect MedalLabel{};
    DesktopMenuMouseRect MedalTrack{};
    DesktopMenuMouseRect VehicleLabel{};
    DesktopMenuMouseRect VehicleTrack{};
};

inline ProfileOverviewLayout CalculateProfileOverviewLayout(HudLayout hud,
    HudMenuVerticalPlan vertical,const HudMenuReadablePanel& readable,
    double safeWidth,double y,int drawnRows,bool profile,bool touch) noexcept
{
    if(!profile || touch || hud.Compact || !vertical.Valid || !readable.Valid ||
        !std::isfinite(hud.Scale) || hud.Scale<0.6 || hud.Scale>1.2 ||
        !std::isfinite(hud.Left) || !std::isfinite(hud.Top) ||
        !std::isfinite(safeWidth) || safeWidth<=0 ||
        !std::isfinite(y) || drawnRows<0 || drawnRows>vertical.MaxDetailsRows ||
        vertical.MaxDetailsRows>90 || !std::isfinite(vertical.OverflowNoticeY) ||
        !std::isfinite(vertical.SaveWarningY) ||
        vertical.OverflowNoticeY>=vertical.SaveWarningY)
        return {};
    const double s=hud.Scale,step=28*s;
    if(std::abs(y-hud.Top-drawnRows*step)>0.02)return {};
    const double width=std::min({safeWidth,readable.X+readable.Width-hud.Left,720*s});
    if(width<310*s)return {};
    constexpr int reserve=8;
    const int remaining=vertical.MaxDetailsRows-drawnRows-reserve;
    if(remaining<ProfileOverviewMinDetails)return {};
    ProfileOverviewLayout out;
    out.Scale=s;out.ReservedRows=reserve;out.NextDetailY=y+reserve*step;
    out.Panel={hud.Left,y,width,211*s};
    const DesktopMenuMouseRect owner{readable.X,readable.Y,readable.Width,readable.Height};
    if(!HudRadarContains(owner,out.Panel) || out.NextDetailY<y+out.Panel.Height ||
        out.NextDetailY+(remaining-1)*step>vertical.OverflowNoticeY-1.25*step+1e-7)
        return {};
    const double pad=8*s,gap=6*s,x=hud.Left+pad,w=width-2*pad;
    const double tw=(w-2*gap)/3,th=51*s;
    out.Header={x,y+5*s,w,17*s};
    for(int i=0;i<ProfileOverviewTiles;++i)
    {
        auto& t=out.Tiles[static_cast<std::size_t>(i)];
        t.Bounds={x+(i%3)*(tw+gap),y+28*s+(i/3)*(th+gap),tw,th};
        t.Label={t.Bounds.X+4*s,t.Bounds.Y+4*s,tw-8*s,13*s};
        t.Value={t.Bounds.X+4*s,t.Bounds.Y+20*s,tw-8*s,25*s};
        if(!HudRadarContains(out.Panel,t.Bounds) || !HudRadarContains(t.Bounds,t.Label) ||
            !HudRadarContains(t.Bounds,t.Value) || HudRadarOverlaps(t.Label,t.Value))
            return {};
    }
    out.MedalLabel={x,y+142*s,w,14*s};
    out.MedalTrack={x,y+159*s,w,6*s};
    out.VehicleLabel={x,y+174*s,w,14*s};
    out.VehicleTrack={x,y+191*s,w,6*s};
    const std::array<DesktopMenuMouseRect,5> regions{{out.Header,out.MedalLabel,
        out.MedalTrack,out.VehicleLabel,out.VehicleTrack}};
    for(std::size_t i=0;i<regions.size();++i)
    {
        if(!HudRadarContains(out.Panel,regions[i]))return {};
        for(std::size_t j=0;j<i;++j)
            if(HudRadarOverlaps(regions[i],regions[j]))return {};
        for(const auto& tile:out.Tiles)
            if(HudRadarOverlaps(regions[i],tile.Bounds))return {};
    }
    for(std::size_t i=0;i<out.Tiles.size();++i)
        for(std::size_t j=0;j<i;++j)
            if(HudRadarOverlaps(out.Tiles[i].Bounds,out.Tiles[j].Bounds))return {};
    out.Valid=true;
    return out;
}

struct ProfileOverviewMeasurements final
{
    HudRaceStatusTextSize Header{},Medals{},Vehicles{};
    std::array<HudRaceStatusTextSize,ProfileOverviewTiles> Labels{},Values{};
};
inline bool CanDrawProfileOverview(const ProfileOverviewLayout& layout,
    const ProfileOverviewMeasurements& m) noexcept
{
    if(!layout.Valid || !HudRaceStatusTextFits(layout.Header,m.Header) ||
        !HudRaceStatusTextFits(layout.MedalLabel,m.Medals) ||
        !HudRaceStatusTextFits(layout.VehicleLabel,m.Vehicles))return false;
    for(std::size_t i=0;i<layout.Tiles.size();++i)
        if(!HudRaceStatusTextFits(layout.Tiles[i].Label,m.Labels[i]) ||
            !HudRaceStatusTextFits(layout.Tiles[i].Value,m.Values[i]))return false;
    return true;
}
struct ProfileOverviewBars final
{
    bool Valid=false;
    // Bronze, silver, gold and remaining uncompleted events.
    std::array<DesktopMenuMouseRect,4> Medals{};
    DesktopMenuMouseRect Vehicles{};
};
inline ProfileOverviewBars BuildProfileOverviewBars(const ProfileOverviewLayout& layout,
    const ProfileOverviewReadout& data) noexcept
{
    if(!layout.Valid || !data.Valid || !ValidHudRadarRect(layout.MedalTrack) ||
        !ValidHudRadarRect(layout.VehicleTrack) || data.CareerCompleted<0 ||
        data.CareerCompleted>CareerEventCount || data.Bronze<0 || data.Silver<0 ||
        data.Gold<0 || data.Bronze>CareerEventCount || data.Silver>CareerEventCount ||
        data.Gold>CareerEventCount ||
        data.Bronze+data.Silver+data.Gold!=data.CareerCompleted ||
        data.OwnedCars<1 || data.OwnedCars>VehicleCount)return {};
    const std::array<int,4> counts{{data.Bronze,data.Silver,data.Gold,
        CareerEventCount-data.CareerCompleted}};
    ProfileOverviewBars out;
    int prior=0;
    const auto& m=layout.MedalTrack;
    for(std::size_t i=0;i<counts.size();++i)
    {
        const double left=m.X+m.Width*prior/CareerEventCount;
        prior+=counts[i];
        const double right=m.X+m.Width*prior/CareerEventCount;
        out.Medals[i]={left,m.Y,right-left,m.Height};
        if(counts[i]>0 && !HudRadarContains(m,out.Medals[i]))return {};
    }
    const auto& v=layout.VehicleTrack;
    out.Vehicles={v.X,v.Y,v.Width*data.OwnedCars/VehicleCount,v.Height};
    if(!HudRadarContains(v,out.Vehicles))return {};
    out.Valid=true;
    return out;
}
} // namespace latebrake

#pragma once

#include "Career/CareerMenuSelection.h"
#include "Career/CareerRoadmap.h"
#include "UI/SettingsMenuPanel.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace latebrake
{
inline constexpr int CareerCardsMinimumDetailRows=4;

struct CareerEventCardsSnapshot final
{
    bool MenuOpen=false;
    MenuPage Page=MenuPage::Closed;
    bool LocalDriver=false;
    bool FocusSuspended=false;
    bool ReadOnly=false;
    int Selected=-1;
    std::uint64_t Revision=0;
    std::array<std::uint8_t,CareerEventCount> Medals{};
    int OwnedVehicleMask=1;

    bool CanBrowse() const noexcept
    {
        return MenuOpen && IsCareerMenuPage(Page) && LocalDriver &&
            !FocusSuspended && Revision!=0;
    }
    constexpr bool operator==(const CareerEventCardsSnapshot&) const noexcept=default;
};

inline CareerEventCardsSnapshot BuildCareerEventCardsSnapshot(
    bool open,MenuPage page,bool local,bool focus,bool readOnly,int selection,
    std::uint64_t revision,const CareerSnapshot& career,GarageSnapshot garage) noexcept
{
    CareerProgress progress;
    progress.Load(career);
    GarageState state;
    state.Load(garage);
    const auto target=ResolveCareerMenuTarget(progress,page,selection);
    return {open,page,local,focus,readOnly,target.Valid()?target.EventId:-1,
        revision,progress.Snapshot().Medals,state.Snapshot().OwnedVehicleMask};
}

enum class CareerCardReadiness : std::uint8_t
{
    LockedPrerequisite, NeedsCar, Ready, Replay, Mastered
};
inline const char* CareerCardReadinessLabel(CareerCardReadiness readiness) noexcept
{
    switch(readiness)
    {
    case CareerCardReadiness::LockedPrerequisite:return "LOCKED";
    case CareerCardReadiness::NeedsCar:return "CAR REQUIRED";
    case CareerCardReadiness::Ready:return "READY";
    case CareerCardReadiness::Replay:return "REPLAY";
    case CareerCardReadiness::Mastered:return "GOLD";
    default:return "UNAVAILABLE";
    }
}
inline const char* CareerCardMedalLabel(CareerMedal medal) noexcept
{
    switch(medal)
    {
    case CareerMedal::Bronze:return "BRONZE";
    case CareerMedal::Silver:return "SILVER";
    case CareerMedal::Gold:return "GOLD";
    default:return "NO MEDAL";
    }
}

struct CareerEventCardDescription final
{
    bool Valid=false;
    bool Selectable=false;
    bool Selected=false;
    int EventId=-1;
    const char* Name="";
    int Laps=0;
    bool Rival=false;
    CareerObjective Objective=CareerObjective::Time;
    CareerMedal Medal=CareerMedal::None;
    CareerCardReadiness Readiness=CareerCardReadiness::LockedPrerequisite;
    int Prerequisite=-1;
    int RequiredVehicle=-1;
    int FirstCredits=0;
};

inline CareerEventCardDescription DescribeCareerEventCard(
    const CareerEventCardsSnapshot& snapshot,int id) noexcept
{
    const auto* event=GetCareerEvent(id);
    if(!event || !IsEventInCareerMenu(snapshot.Page,id) ||
        snapshot.OwnedVehicleMask<0 ||
        (snapshot.OwnedVehicleMask&~KnownVehicleMask)!=0 ||
        (snapshot.OwnedVehicleMask&1)==0)
        return {};
    for(int i=0;i<CareerEventCount;++i)
    {
        const auto medal=snapshot.Medals[static_cast<std::size_t>(i)];
        const auto* definition=GetCareerEvent(i);
        if(medal>3 || (medal!=0 && definition->Prerequisite>=0 &&
            snapshot.Medals[static_cast<std::size_t>(definition->Prerequisite)]==0))
            return {};
    }
    CareerSnapshot canonical;
    canonical.Medals=snapshot.Medals;
    CareerProgress progress;
    progress.Load(canonical);
    const auto medal=progress.Medal(id);
    const auto status=progress.Status(id);
    const bool carMissing=event->RequiredVehicle>=0 &&
        (!GarageState::IsVehicle(event->RequiredVehicle) ||
            (snapshot.OwnedVehicleMask&(1<<event->RequiredVehicle))==0);
    CareerCardReadiness readiness=CareerCardReadiness::Ready;
    if(status==CareerStatus::Locked)readiness=CareerCardReadiness::LockedPrerequisite;
    else if(carMissing)readiness=CareerCardReadiness::NeedsCar;
    else if(status==CareerStatus::Mastered)readiness=CareerCardReadiness::Mastered;
    else if(status==CareerStatus::Completed)readiness=CareerCardReadiness::Replay;
    // A missing required car can be selected for inspection, but only the
    // existing StageCareerLaunch can authorize actual event entry.
    return {true,snapshot.CanBrowse() && status!=CareerStatus::Locked,
        snapshot.Selected==id,id,event->Name,event->Laps,event->SpawnRival,
        event->Objective,medal,readiness,event->Prerequisite,event->RequiredVehicle,
        medal==CareerMedal::None?event->FirstCredits:0};
}

struct CareerEventCard final
{
    int EventId=-1;
    DesktopMenuMouseRect Bounds{};
    DesktopMenuMouseRect Name{};
    DesktopMenuMouseRect Detail{};
    DesktopMenuMouseRect State{};
};

struct CareerEventCardsLayout final
{
    bool Valid=false;
    double Width=0,Height=0,Scale=1;
    MenuPage Page=MenuPage::Closed;
    int Count=0,Columns=0,ReservedRows=0;
    double NextDetailY=0;
    DesktopMenuMouseRect Panel{};
    DesktopMenuMouseRect Header{};
    std::array<CareerEventCard,CareerEventCount> Cards{};
};

inline bool ValidCareerEventCardsLayout(const CareerEventCardsLayout& plan) noexcept
{
    if(!plan.Valid || !IsCareerMenuPage(plan.Page) ||
        !std::isfinite(plan.Width) || !std::isfinite(plan.Height) ||
        plan.Width<900 || plan.Height<600 ||
        !std::isfinite(plan.Scale) || plan.Scale<0.6 || plan.Scale>1.2 ||
        plan.Count<=0 || plan.Count>CareerEventCount ||
        plan.Columns<2 || plan.Columns>3 ||
        plan.ReservedRows<1 || plan.ReservedRows>90 ||
        !std::isfinite(plan.NextDetailY) ||
        !ContainsSettingsRect({0,0,plan.Width,plan.Height},plan.Panel) ||
        !ContainsSettingsRect(plan.Panel,plan.Header) ||
        plan.NextDetailY<plan.Panel.Y+plan.Panel.Height)
        return false;
    int expected=0;
    for(int id=0;id<CareerEventCount;++id)
    {
        if(!IsEventInCareerMenu(plan.Page,id))continue;
        if(expected>=plan.Count)return false;
        const auto& card=plan.Cards[static_cast<std::size_t>(expected)];
        if(card.EventId!=id || card.Bounds.Width<100 || card.Bounds.Height<44 ||
            !ContainsSettingsRect(plan.Panel,card.Bounds) ||
            !ContainsSettingsRect(card.Bounds,card.Name) ||
            !ContainsSettingsRect(card.Bounds,card.Detail) ||
            !ContainsSettingsRect(card.Bounds,card.State) ||
            SettingsRectsOverlap(card.Name,card.Detail) ||
            SettingsRectsOverlap(card.Name,card.State) ||
            SettingsRectsOverlap(card.Detail,card.State) ||
            SettingsRectsOverlap(card.Bounds,plan.Header))
            return false;
        for(int j=0;j<expected;++j)
            if(SettingsRectsOverlap(card.Bounds,plan.Cards[static_cast<std::size_t>(j)].Bounds))
                return false;
        ++expected;
    }
    return expected==plan.Count;
}

inline CareerEventCardsLayout CalculateCareerEventCardsLayout(double width,double height,
    HudLayout hud,HudMenuVerticalPlan vertical,const HudMenuReadablePanel& readable,
    double safeWidth,double currentY,int drawnRows,bool touch,MenuPage page) noexcept
{
    if(!IsCareerMenuPage(page) || touch || hud.Compact || !vertical.Valid ||
        !readable.Valid || !std::isfinite(width) || !std::isfinite(height) ||
        width<900 || height<600 || width>20000 || height>20000 ||
        !std::isfinite(hud.Scale) || hud.Scale<0.6 || hud.Scale>1.2 ||
        !std::isfinite(hud.Left) || !std::isfinite(hud.Top) ||
        !std::isfinite(safeWidth) || safeWidth<=0 ||
        !std::isfinite(currentY) || drawnRows<0 || drawnRows>vertical.MaxDetailsRows ||
        vertical.MaxDetailsRows>90 || !std::isfinite(vertical.OverflowNoticeY) ||
        !std::isfinite(vertical.SaveWarningY) ||
        vertical.OverflowNoticeY>=vertical.SaveWarningY)
        return {};
    const double s=hud.Scale,step=28*s;
    if(std::abs(currentY-hud.Top-drawnRows*step)>0.02)return {};
    const double w=std::min({safeWidth,readable.X+readable.Width-hud.Left,800*s});
    const double pad=7*s,gap=6*s,header=20*s,usable=w-2*pad;
    const double cardHeight=std::max(52.0,64*s);
    int count=0;
    for(int id=0;id<CareerEventCount;++id)if(IsEventInCareerMenu(page,id))++count;
    const DesktopMenuMouseRect owner{readable.X,readable.Y,readable.Width,readable.Height};
    for(int columns:{3,2})
    {
        const int rows=(count+columns-1)/columns;
        const double cardWidth=(usable-(columns-1)*gap)/columns;
        if(cardWidth<std::max(100.0,142*s))continue;
        const double panelHeight=2*pad+header+gap+rows*cardHeight+(rows-1)*gap;
        const int reserve=static_cast<int>(std::ceil((panelHeight+gap)/step));
        const int remaining=vertical.MaxDetailsRows-drawnRows-reserve;
        if(remaining<CareerCardsMinimumDetailRows)continue;
        CareerEventCardsLayout out;
        out.Valid=true;out.Width=width;out.Height=height;out.Scale=s;
        out.Page=page;out.Count=count;out.Columns=columns;out.ReservedRows=reserve;
        out.NextDetailY=currentY+reserve*step;
        out.Panel={hud.Left,currentY,w,panelHeight};
        out.Header={hud.Left+pad,currentY+pad,usable,header};
        if(!ContainsSettingsRect(owner,out.Panel) ||
            out.NextDetailY+(remaining-1)*step>vertical.OverflowNoticeY-1.25*step+1e-7)
            continue;
        const double top=out.Header.Y+header+gap;
        int index=0;
        for(int id=0;id<CareerEventCount;++id)
        {
            if(!IsEventInCareerMenu(page,id))continue;
            auto& card=out.Cards[static_cast<std::size_t>(index)];
            card.EventId=id;
            card.Bounds={hud.Left+pad+(index%columns)*(cardWidth+gap),
                top+(index/columns)*(cardHeight+gap),cardWidth,cardHeight};
            const double inset=5*s,x=card.Bounds.X+inset,y=card.Bounds.Y,
                inner=cardWidth-2*inset;
            card.Name={x,y+3*s,inner,cardHeight*0.34-3*s};
            card.Detail={x,y+cardHeight*0.34,inner,cardHeight*0.32-2*s};
            card.State={x,y+cardHeight*0.66,inner,cardHeight*0.34-2*s};
            ++index;
        }
        if(ValidCareerEventCardsLayout(out))return out;
    }
    return {};
}

struct CareerEventCardsPresentation final
{
    bool Valid=false;
    CareerEventCardsLayout Layout{};
    CareerEventCardsSnapshot Snapshot{};
    std::array<bool,CareerEventCount> Rendered{};
};
struct CareerEventCardsHit final
{
    bool Consumed=false;
    int EventId=-1;
};

inline CareerEventCardsHit HitPresentedCareerEventCards(
    const CareerEventCardsPresentation& shown,const CareerEventCardsSnapshot& current,
    double width,double height,double x,double y) noexcept
{
    if(!shown.Valid || !shown.Layout.Panel.Contains(x,y))return {};
    if(!current.CanBrowse() || shown.Snapshot!=current ||
        !ValidCareerEventCardsLayout(shown.Layout) || shown.Layout.Page!=current.Page ||
        shown.Layout.Width!=width || shown.Layout.Height!=height)
        return {true,-1};
    for(int i=0;i<shown.Layout.Count;++i)
    {
        const auto& card=shown.Layout.Cards[static_cast<std::size_t>(i)];
        if(!card.Bounds.Contains(x,y))continue;
        const auto desc=DescribeCareerEventCard(current,card.EventId);
        return {true,shown.Rendered[static_cast<std::size_t>(i)] &&
            desc.Valid && desc.Selectable?card.EventId:-1};
    }
    return {true,-1};
}
} // namespace latebrake

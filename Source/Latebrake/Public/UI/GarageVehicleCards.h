#pragma once

#include "Garage/GarageCatalogView.h"
#include "Garage/GarageAppearance.h"
#include "UI/SettingsMenuPanel.h"
#include "Vehicles/VehiclePaintRecipe.h"

#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace latebrake
{
inline constexpr int GarageVehicleCardsMinimumDetails=6;

// The selection receipt describes what was actually rendered, including
// changes in ownership, credits, tuning, appearance and viewed vehicle.
// Browsing does not authorize any of those mutations.
struct GarageVehicleCardsSnapshot final
{
    bool Open=false;
    bool Local=false;
    bool FocusSuspended=false;
    bool ReadOnly=false;
    int Viewed=-1;
    int Equipped=0;
    int OwnedMask=1;
    int Credits=0;
    int Engine=0;
    int Grip=0;
    std::uint64_t Revision=0;
    GarageAppearanceState::Packed Looks{};
    constexpr bool operator==(const GarageVehicleCardsSnapshot&) const noexcept=default;
    bool CanBrowse() const noexcept
    {
        return Open && Local && !FocusSuspended && Revision!=0 &&
            GarageState::IsVehicle(Viewed);
    }
};

inline GarageVehicleCardsSnapshot BuildGarageVehicleCardsSnapshot(
    bool open,bool local,bool focus,bool readOnly,int viewed,std::uint64_t revision,
    GarageSnapshot saved,const GarageAppearanceState& looks) noexcept
{
    GarageState garage;
    garage.Load(saved);
    const auto g=garage.Snapshot();
    return {open,local,focus,readOnly,viewed,g.SelectedVehicle,
        g.OwnedVehicleMask,g.Credits,g.EngineLevel,g.GripLevel,revision,looks.Serialize()};
}

enum class GarageCardStatus : std::uint8_t
{
    Equipped, Owned, UnlockReady, RouteRequired, CreditsRequired
};

inline const char* GarageCardStatusName(GarageCardStatus status) noexcept
{
    switch(status)
    {
    case GarageCardStatus::Equipped:return "EQUIPPED";
    case GarageCardStatus::Owned:return "OWNED";
    case GarageCardStatus::UnlockReady:return "NEXT UNLOCK";
    case GarageCardStatus::RouteRequired:return "EARLIER CARS FIRST";
    case GarageCardStatus::CreditsRequired:return "LOCKED / NEED CREDITS";
    default:return "UNAVAILABLE";
    }
}

struct GarageVehicleCardData final
{
    bool Valid=false;
    int Id=-1;
    const char* Name="";
    GarageCardStatus Status=GarageCardStatus::CreditsRequired;
    bool Viewed=false;
    bool CanBrowse=false;
    GarageLook Look{};
    VehiclePaintRgb Swatch{0,0,0};
    int StickerCost=0;
    int RouteCost=0;
    int RouteShortfall=0;
    int RouteCars=0;
    bool Customized=false;
};

inline std::array<GarageVehicleCardData,VehicleCount> DescribeGarageVehicleCards(
    const GarageVehicleCardsSnapshot& s,GarageCatalogCache* cache=nullptr) noexcept
{
    std::array<GarageVehicleCardData,VehicleCount> out{};
    if(!GarageState::IsVehicle(s.Viewed) || !GarageState::IsVehicle(s.Equipped) ||
        s.OwnedMask<0 || (s.OwnedMask&~KnownVehicleMask)!=0 ||
        (s.OwnedMask&1)==0 || (s.OwnedMask&(1<<s.Equipped))==0 ||
        s.Credits<0 || s.Engine<0 || s.Engine>3 || s.Grip<0 || s.Grip>3)
        return out;
    GarageAppearanceState appearance;
    if(!appearance.Load(s.Looks.data(),s.Looks.size()))return out;
    const GarageSnapshot saved{s.Credits,s.OwnedMask,s.Equipped,s.Engine,s.Grip};
    GarageCatalogView uncached;
    const auto& catalog=cache?cache->Read(saved):(uncached=BuildGarageCatalogView(saved));
    for(int id=0;id<VehicleCount;++id)
    {
        const auto& c=catalog.Cars[static_cast<std::size_t>(id)];
        if(c.VehicleId!=id || !c.RouteValid)return {};
        GarageCardStatus status=GarageCardStatus::CreditsRequired;
        if(c.Ownership==GarageCatalogOwnership::Equipped)status=GarageCardStatus::Equipped;
        else if(c.Ownership==GarageCatalogOwnership::Owned)status=GarageCardStatus::Owned;
        else if(c.RouteCreditsShort>0)status=GarageCardStatus::CreditsRequired;
        else if(c.IsNextUnlock)status=GarageCardStatus::UnlockReady;
        else status=GarageCardStatus::RouteRequired;
        const auto look=appearance.Get(id);
        out[static_cast<std::size_t>(id)]={true,id,c.Name,status,s.Viewed==id,
            s.CanBrowse(),look,PaintForGarageLook(id,look).Body,c.UnlockCost,
            c.RouteCost,c.RouteCreditsShort,c.RouteCars,
            !SameGarageLook(look,FactoryGarageLook(id))};
    }
    return out;
}

struct GarageVehicleCardLayout final
{
    int Id=-1;
    DesktopMenuMouseRect Bounds{},Name{},Swatch{},State{},Cost{};
};

struct GarageVehicleCardsLayout final
{
    bool Valid=false;
    double Width=0,Height=0,Scale=1;
    int Columns=0,ReservedRows=0;
    double NextDetailY=0;
    DesktopMenuMouseRect Panel{},Header{};
    std::array<GarageVehicleCardLayout,VehicleCount> Cards{};
};

inline bool ValidGarageVehicleCardsLayout(const GarageVehicleCardsLayout& p) noexcept
{
    if(!p.Valid || !std::isfinite(p.Width) || !std::isfinite(p.Height) ||
        p.Width<900 || p.Height<600 || !std::isfinite(p.Scale) ||
        p.Scale<0.6 || p.Scale>1.2 || (p.Columns!=2 && p.Columns!=3) ||
        p.ReservedRows<=0 || p.ReservedRows>90 || !std::isfinite(p.NextDetailY) ||
        !ContainsSettingsRect({0,0,p.Width,p.Height},p.Panel) ||
        !ContainsSettingsRect(p.Panel,p.Header) || p.NextDetailY<p.Panel.Y+p.Panel.Height)
        return false;
    for(std::size_t i=0;i<p.Cards.size();++i)
    {
        const auto& c=p.Cards[i];
        if(c.Id!=static_cast<int>(i) || c.Bounds.Width<100 || c.Bounds.Height<44 ||
            !ContainsSettingsRect(p.Panel,c.Bounds) || SettingsRectsOverlap(c.Bounds,p.Header))
            return false;
        const std::array<DesktopMenuMouseRect,4> parts{{c.Name,c.Swatch,c.State,c.Cost}};
        for(std::size_t j=0;j<parts.size();++j)
        {
            if(!ContainsSettingsRect(c.Bounds,parts[j]))return false;
            for(std::size_t k=0;k<j;++k)
                if(SettingsRectsOverlap(parts[j],parts[k]))return false;
        }
        for(std::size_t j=0;j<i;++j)
            if(SettingsRectsOverlap(c.Bounds,p.Cards[j].Bounds))return false;
    }
    return true;
}

inline GarageVehicleCardsLayout CalculateGarageVehicleCardsLayout(double width,double height,
    HudLayout hud,HudMenuVerticalPlan vertical,const HudMenuReadablePanel& readable,
    double safeWidth,double y,int drawnRows,bool garage,bool touch) noexcept
{
    if(!garage || touch || hud.Compact || !vertical.Valid || !readable.Valid ||
        !std::isfinite(width) || !std::isfinite(height) ||
        width<900 || height<600 || width>20000 || height>20000 ||
        !std::isfinite(hud.Scale) || hud.Scale<0.6 || hud.Scale>1.2 ||
        !std::isfinite(hud.Left) || !std::isfinite(hud.Top) ||
        !std::isfinite(safeWidth) || safeWidth<=0 || !std::isfinite(y) ||
        drawnRows<0 || drawnRows>vertical.MaxDetailsRows || vertical.MaxDetailsRows>90 ||
        !std::isfinite(vertical.OverflowNoticeY) || !std::isfinite(vertical.SaveWarningY) ||
        vertical.OverflowNoticeY>=vertical.SaveWarningY)return {};
    const double s=hud.Scale,step=28*s;
    if(std::abs(y-hud.Top-drawnRows*step)>0.02)return {};
    const double w=std::min({safeWidth,readable.X+readable.Width-hud.Left,720*s});
    const double pad=7*s,gap=6*s,header=18*s,usable=w-2*pad;
    const double ch=std::max(50.0,60*s);
    const DesktopMenuMouseRect owner{readable.X,readable.Y,readable.Width,readable.Height};
    for(int columns:{3,2})
    {
        const int rows=(VehicleCount+columns-1)/columns;
        const double cw=(usable-(columns-1)*gap)/columns;
        if(cw<std::max(100.0,132*s))continue;
        const double ph=2*pad+header+gap+rows*ch+(rows-1)*gap;
        const int reserve=static_cast<int>(std::ceil((ph+gap)/step));
        const int remaining=vertical.MaxDetailsRows-drawnRows-reserve;
        if(remaining<GarageVehicleCardsMinimumDetails)continue;
        GarageVehicleCardsLayout out;
        out.Valid=true;out.Width=width;out.Height=height;out.Scale=s;
        out.Columns=columns;out.ReservedRows=reserve;out.NextDetailY=y+reserve*step;
        out.Panel={hud.Left,y,w,ph};
        out.Header={hud.Left+pad,y+pad,usable,header};
        if(!ContainsSettingsRect(owner,out.Panel) ||
            out.NextDetailY+(remaining-1)*step>vertical.OverflowNoticeY-1.25*step+1e-7)
            continue;
        const double top=out.Header.Y+header+gap;
        for(int id=0;id<VehicleCount;++id)
        {
            auto& c=out.Cards[static_cast<std::size_t>(id)];
            c.Id=id;c.Bounds={hud.Left+pad+(id%columns)*(cw+gap),
                top+(id/columns)*(ch+gap),cw,ch};
            const double x=c.Bounds.X+5*s,cy=c.Bounds.Y,inner=cw-10*s;
            c.Name={x,cy+3*s,inner-18*s,ch*0.34-3*s};
            c.Swatch={x+inner-14*s,cy+4*s,14*s,11*s};
            c.State={x,cy+ch*0.34,inner,ch*0.32-2*s};
            c.Cost={x,cy+ch*0.66,inner,ch*0.34-3*s};
        }
        if(ValidGarageVehicleCardsLayout(out))return out;
    }
    return {};
}

struct GarageVehicleCardsPresentation final
{
    bool Valid=false;
    GarageVehicleCardsLayout Layout{};
    GarageVehicleCardsSnapshot Snapshot{};
    std::array<bool,VehicleCount> Rendered{};
};
struct GarageVehicleCardsHit final
{
    bool Consumed=false;
    int Vehicle=-1;
};

inline GarageVehicleCardsHit HitPresentedGarageVehicleCards(
    const GarageVehicleCardsPresentation& shown,const GarageVehicleCardsSnapshot& current,
    double width,double height,double x,double y,bool paletteOpen) noexcept
{
    if(!shown.Valid || !shown.Layout.Panel.Contains(x,y))return {};
    if(paletteOpen || !current.CanBrowse() || shown.Snapshot!=current ||
        !ValidGarageVehicleCardsLayout(shown.Layout) ||
        shown.Layout.Width!=width || shown.Layout.Height!=height)return {true,-1};
    const auto data=DescribeGarageVehicleCards(current);
    for(std::size_t i=0;i<shown.Layout.Cards.size();++i)
        if(shown.Layout.Cards[i].Bounds.Contains(x,y))
            return {true,shown.Rendered[i] && data[i].Valid && data[i].CanBrowse?
                static_cast<int>(i):-1};
    return {true,-1};
}
} // namespace latebrake

#pragma once

#include "Garage/GarageDirectAppearance.h"
#include "UI/GarageShowroomLane.h"
#include "Vehicles/VehiclePaintRecipe.h"

namespace latebrake
{
inline constexpr std::size_t GaragePaletteMaxChoices = GaragePaintOptions;
static_assert(GaragePaintOptions == 8 && GarageStripeOptions <= GaragePaintOptions &&
    GarageWheelOptions <= GaragePaintOptions && GaragePresetOptions <= GaragePaintOptions);

constexpr DesktopMenuMouseAction GaragePaletteChoiceAction(int index) noexcept
{
    return index >= 0 && index < static_cast<int>(GaragePaletteMaxChoices)
        ? static_cast<DesktopMenuMouseAction>(
            static_cast<int>(DesktopMenuMouseAction::GarageLookChoice0) + index)
        : DesktopMenuMouseAction::None;
}
constexpr int GaragePaletteChoiceIndex(DesktopMenuMouseAction action) noexcept
{
    constexpr int first=static_cast<int>(DesktopMenuMouseAction::GarageLookChoice0);
    const int value=static_cast<int>(action);
    return value>=first && value<first+static_cast<int>(GaragePaletteMaxChoices)
        ? value-first : -1;
}
constexpr bool IsGaragePaletteAction(DesktopMenuMouseAction action) noexcept
{
    return GaragePaletteChoiceIndex(action)>=0 ||
        action==DesktopMenuMouseAction::GaragePaletteClose;
}
constexpr bool IsGaragePaletteTab(DesktopMenuMouseAction action) noexcept
{
    return action==DesktopMenuMouseAction::GaragePaint ||
           action==DesktopMenuMouseAction::GarageStripe ||
           action==DesktopMenuMouseAction::GarageWheels;
}

struct GaragePaletteSession final
{
    bool Open = false;
    int VehicleId = -1;
    GarageLook Look{};
    GarageLookLayer Layer = GarageLookLayer::Paint;

    constexpr bool Matches(int viewedId, GarageLook current, bool canStyle) const noexcept
    {
        return Open && canStyle && GarageState::IsVehicle(VehicleId) &&
            VehicleId==viewedId && ValidGarageLook(Look) && ValidGarageLook(current) &&
            SameGarageLook(Look,current) && GarageLookChoiceCount(Layer)>0;
    }
};

struct GaragePaletteChoice final
{
    int Index = -1;
    const char* Label = "";
    const char* ShortLabel = "";
    VehiclePaintRgb Swatch{0,0,0};
    bool Selected = false;
    bool Factory = false;
};

constexpr GaragePaletteChoice DescribeGaragePaletteChoice(
    int vehicleId, GarageLook current, GarageLookLayer layer, int choice) noexcept
{
    if(!GarageState::IsVehicle(vehicleId) || !ValidGarageLook(current) ||
        choice<0 || choice>=GarageLookChoiceCount(layer)) return {};
    const auto decision=PreviewGarageDirectAppearance(vehicleId,current,
        {vehicleId,current,layer,choice});
    constexpr const char* shortPaint[]={"BLUE","ORANGE","VIOLET","SCARLET",
        "SEA GLASS","YELLOW","GRAPHITE","PEARL"};
    const auto swatch=PaintForGarageLook(vehicleId,decision.Result).Body;
    return {choice,GarageLookChoiceName(layer,choice),
        layer==GarageLookLayer::Paint?shortPaint[choice]:
        layer==GarageLookLayer::Preset?GarageLookPresets[static_cast<std::size_t>(choice)].ShortName:
        GarageLookChoiceName(layer,choice),
        swatch,GarageLookChoice(current,layer)==choice,
        GarageLookChoice(FactoryGarageLook(vehicleId),layer)==choice};
}

struct GaragePaletteLayout final
{
    bool Valid = false;
    DesktopMenuMouseRect Panel{};
    DesktopMenuMouseButton Close{};
    std::array<DesktopMenuMouseButton,GaragePaletteMaxChoices> Choices{};
    int Count = 0;
};

// The chooser is an intentionally temporary overlay inside the showroom
// aperture. It cannot occupy the left details, right dock, buy strip or
// orbit controls. Picking an option dismisses it to reveal the car again.
inline GaragePaletteLayout CalculateGaragePaletteLayout(
    const GarageShowroomLane& lane, double width, double height,
    HudLayout hud, GarageLookLayer layer) noexcept
{
    const int count=GarageLookChoiceCount(layer);
    const auto& a=lane.Aperture;
    if(!lane.Valid || hud.Compact || count<=0 ||
        count>static_cast<int>(GaragePaletteMaxChoices) ||
        !std::isfinite(width)||!std::isfinite(height)||
        !std::isfinite(hud.Scale)||hud.Scale<0.6||hud.Scale>1.2||
        !std::isfinite(a.X)||!std::isfinite(a.Y)||
        !std::isfinite(a.Width)||!std::isfinite(a.Height)||
        !std::isfinite(a.X+a.Width)||!std::isfinite(a.Y+a.Height)||
        a.X<0||a.Y<0||a.X+a.Width>width||a.Y+a.Height>height)
        return {};
    const double gap=std::max(6.0,8.0*hud.Scale);
    const double pad=gap;
    const double rowH=std::max(44.0,52.0*hud.Scale);
    const int columns=layer==GarageLookLayer::Paint?2:1;
    const int rows=(count+columns-1)/columns;
    const double panelW=std::min(a.Width,420.0*hud.Scale);
    const double panelH=2*pad+rowH*(rows+1)+gap*rows;
    const double buttonW=(panelW-2*pad-gap*(columns-1))/columns;
    if(!std::isfinite(panelW)||!std::isfinite(panelH)||
        buttonW<68.0 || panelH>a.Height || panelW>a.Width)
        return {};
    const double x=a.X+(a.Width-panelW)*0.5;
    const double y=a.Y+(a.Height-panelH)*0.5;
    GaragePaletteLayout result;
    result.Panel={x,y,panelW,panelH};
    result.Close={DesktopMenuMouseAction::GaragePaletteClose,
        {x+panelW-pad-60.0,y+pad,60.0,rowH}};
    result.Count=count;
    for(int i=0;i<count;++i)
        result.Choices[static_cast<std::size_t>(i)]={GaragePaletteChoiceAction(i),
            {x+pad+(i%columns)*(buttonW+gap),
             y+pad+(1+i/columns)*(rowH+gap),buttonW,rowH}};
    result.Valid=true;
    return result;
}

inline bool ValidGaragePaletteRect(const DesktopMenuMouseRect& r,
    const DesktopMenuMouseRect& panel) noexcept
{
    return std::isfinite(r.X)&&std::isfinite(r.Y)&&
        std::isfinite(r.Width)&&std::isfinite(r.Height)&&
        std::isfinite(r.X+r.Width)&&std::isfinite(r.Y+r.Height)&&
        r.Width>=44.0&&r.Height>=44.0&&
        r.X+r.Width>r.X&&r.Y+r.Height>r.Y&&
        r.X>=panel.X&&r.Y>=panel.Y&&
        r.X+r.Width<=panel.X+panel.Width&&r.Y+r.Height<=panel.Y+panel.Height;
}

inline bool GaragePaletteRectOverlap(const DesktopMenuMouseRect& a,
    const DesktopMenuMouseRect& b) noexcept
{
    return a.X<b.X+b.Width&&a.X+a.Width>b.X&&
        a.Y<b.Y+b.Height&&a.Y+a.Height>b.Y;
}

// Shared by mouse admission, keyboard admission and final dispatch.
// Matching two malformed presentations is not evidence of a visible target.
inline bool ValidGaragePaletteLayout(const GaragePaletteLayout& layout) noexcept
{
    const auto& p=layout.Panel;
    if(!layout.Valid || layout.Count<=0 ||
        layout.Count>static_cast<int>(layout.Choices.size()) ||
        !std::isfinite(p.X)||!std::isfinite(p.Y)||
        !std::isfinite(p.Width)||!std::isfinite(p.Height)||
        !std::isfinite(p.X+p.Width)||!std::isfinite(p.Y+p.Height)||
        p.X<0.0||p.Y<0.0||p.Width<44.0||p.Height<44.0||
        p.X+p.Width<=p.X||p.Y+p.Height<=p.Y||
        layout.Close.Action!=DesktopMenuMouseAction::GaragePaletteClose||
        !ValidGaragePaletteRect(layout.Close.Bounds,p))
        return false;
    for(int i=0;i<layout.Count;++i)
    {
        const auto& b=layout.Choices[static_cast<std::size_t>(i)];
        if(b.Action!=GaragePaletteChoiceAction(i)||
            !ValidGaragePaletteRect(b.Bounds,p)||
            GaragePaletteRectOverlap(b.Bounds,layout.Close.Bounds))
            return false;
        for(int j=0;j<i;++j)
            if(GaragePaletteRectOverlap(b.Bounds,layout.Choices[static_cast<std::size_t>(j)].Bounds))
                return false;
    }
    return true;
}

inline bool SameGaragePaletteRect(const DesktopMenuMouseRect& a,
    const DesktopMenuMouseRect& b) noexcept
{
    return a.X==b.X&&a.Y==b.Y&&a.Width==b.Width&&a.Height==b.Height;
}

inline bool SameValidGaragePaletteLayout(const GaragePaletteLayout& before,
    const GaragePaletteLayout& now) noexcept
{
    if(!ValidGaragePaletteLayout(before)||!ValidGaragePaletteLayout(now)||
        before.Count!=now.Count||
        !SameGaragePaletteRect(before.Panel,now.Panel)||
        before.Close.Action!=now.Close.Action||
        !SameGaragePaletteRect(before.Close.Bounds,now.Close.Bounds))
        return false;
    for(int i=0;i<now.Count;++i)
    {
        const auto index=static_cast<std::size_t>(i);
        if(before.Choices[index].Action!=now.Choices[index].Action||
            !SameGaragePaletteRect(before.Choices[index].Bounds,now.Choices[index].Bounds))
            return false;
    }
    return true;
}

inline DesktopMenuMouseAction HitTestGaragePalette(
    const GaragePaletteLayout& layout, double x, double y) noexcept
{
    if(!ValidGaragePaletteLayout(layout)||!layout.Panel.Contains(x,y))
        return DesktopMenuMouseAction::None;
    if(layout.Close.Bounds.Contains(x,y))
        return layout.Close.Action;
    for(int i=0;i<layout.Count;++i)
    {
        const auto& b=layout.Choices[static_cast<std::size_t>(i)];
        if(b.Bounds.Contains(x,y)) return b.Action;
    }
    return DesktopMenuMouseAction::None;
}

// An open modal owns its background, not just its buttons. A corrupt
// active plan blocks all pointer dispatch until the next valid draw.
inline bool GaragePaletteBlocksUnderlyingPointer(bool open,
    const GaragePaletteLayout& layout,double x,double y,
    DesktopMenuMouseAction paletteAction) noexcept
{
    if(!open) return false;
    if(!ValidGaragePaletteLayout(layout)) return true;
    if(!layout.Panel.Contains(x,y)) return false;
    return paletteAction==DesktopMenuMouseAction::None;
}

struct GaragePresentedPalette final
{
    GaragePaletteSession Session{};
    GaragePaletteLayout Layout{};
    std::array<bool,GaragePaletteMaxChoices> ChoiceVisible{};
    bool CloseVisible=false;
};

inline bool CanClickPresentedGaragePalette(
    const GaragePresentedPalette& shown,const GaragePaletteSession& session,
    const GaragePaletteLayout& current,DesktopMenuMouseAction action) noexcept
{
    if(!shown.Session.Matches(session.VehicleId,session.Look,session.Open)||
        shown.Session.Layer!=session.Layer||
        !SameValidGaragePaletteLayout(shown.Layout,current)||
        current.Count!=GarageLookChoiceCount(session.Layer)) return false;
    const int index=GaragePaletteChoiceIndex(action);
    const bool close=action==DesktopMenuMouseAction::GaragePaletteClose;
    if(!close&&(index<0||index>=current.Count))return false;
    const auto& before=close?shown.Layout.Close:shown.Layout.Choices[static_cast<std::size_t>(index)];
    const auto& now=close?current.Close:current.Choices[static_cast<std::size_t>(index)];
    const auto& a=before.Bounds;
    const auto& b=now.Bounds;
    return (close?shown.CloseVisible:shown.ChoiceVisible[static_cast<std::size_t>(index)])&&
        before.Action==action&&now.Action==action&&
        a.X==b.X&&a.Y==b.Y&&a.Width==b.Width&&a.Height==b.Height;
}
} // namespace latebrake

#pragma once

#include "UI/SettingsMenuPanel.h"
#include "UI/HudButtonCaptionFit.h"

#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace latebrake
{
inline constexpr int MainMenuDestinationCount=9;
inline constexpr int MainMenuMinimumRemainingRows=3;
struct MainMenuDestination final
{
    MenuPage Page=MenuPage::Closed;
    const char* Title="";
    const char* ShortTitle="";
    const char* Description="";
};
inline constexpr std::array<MainMenuDestination,MainMenuDestinationCount> MainMenuDestinations{{
    {MenuPage::QuickRace,"QUICK RACE","QUICK RACE","Choose a race or practice"},
    {MenuPage::Career,"CAREER","CAREER","Progress through events"},
    {MenuPage::Championships,"CHAMPIONSHIPS","SERIES","Explore championship rounds"},
    {MenuPage::Challenges,"CHALLENGES","CHALLENGES","Browse driving challenges"},
    {MenuPage::Garage,"GARAGE","GARAGE","Browse and style your cars"},
    {MenuPage::Results,"RACE RESULTS","RESULTS","Review your latest attempt"},
    {MenuPage::Pause,"PAUSE MENU","PAUSE","View pause and resume options"},
    {MenuPage::Settings,"SETTINGS","SETTINGS","Adjust HUD and controls"},
    {MenuPage::Profile,"PROFILE","PROFILE","Review saved progress"}
}};

constexpr bool ValidMainMenuDestination(MenuPage page) noexcept
{
    for(const auto& d:MainMenuDestinations)
        if(d.Page==page)return true;
    return false;
}

struct MainMenuDashboardSnapshot final
{
    bool MenuOpen=false;
    MenuPage Page=MenuPage::Closed;
    bool LocalDriver=false;
    bool FocusSuspended=false;
    std::uint64_t Revision=0;
    constexpr bool AllowsNavigation() const noexcept
    {
        return MenuOpen && Page==MenuPage::Main && LocalDriver &&
            !FocusSuspended && Revision!=0;
    }
    constexpr bool operator==(const MainMenuDashboardSnapshot&) const noexcept=default;
};

struct MainMenuCard final
{
    MenuPage Destination=MenuPage::Closed;
    DesktopMenuMouseRect Bounds{};
    DesktopMenuMouseRect Title{};
    DesktopMenuMouseRect Description{};
};

struct MainMenuDashboardLayout final
{
    bool Valid=false;
    double Width=0,Height=0,Scale=1;
    int Columns=0,ReservedRows=0;
    double NextDetailY=0;
    DesktopMenuMouseRect Panel{};
    DesktopMenuMouseRect Header{};
    std::array<MainMenuCard,MainMenuDestinationCount> Cards{};
};

inline bool ValidMainMenuDashboardLayout(const MainMenuDashboardLayout& plan) noexcept
{
    if(!plan.Valid || !std::isfinite(plan.Width) || !std::isfinite(plan.Height) ||
        plan.Width<900 || plan.Height<600 || !std::isfinite(plan.Scale) ||
        plan.Scale<0.6 || plan.Scale>1.2 ||
        (plan.Columns!=2 && plan.Columns!=3) ||
        plan.ReservedRows<=0 || plan.ReservedRows>90 ||
        !std::isfinite(plan.NextDetailY) ||
        !ContainsSettingsRect({0,0,plan.Width,plan.Height},plan.Panel) ||
        !ContainsSettingsRect(plan.Panel,plan.Header) ||
        plan.NextDetailY<plan.Panel.Y+plan.Panel.Height)
        return false;
    for(std::size_t i=0;i<plan.Cards.size();++i)
    {
        const auto& card=plan.Cards[i];
        if(card.Destination!=MainMenuDestinations[i].Page ||
            card.Bounds.Width<100 || card.Bounds.Height<44 ||
            !ContainsSettingsRect(plan.Panel,card.Bounds) ||
            !ContainsSettingsRect(card.Bounds,card.Title) ||
            !ContainsSettingsRect(card.Bounds,card.Description) ||
            SettingsRectsOverlap(card.Title,card.Description) ||
            SettingsRectsOverlap(card.Bounds,plan.Header))
            return false;
        for(std::size_t j=0;j<i;++j)
            if(SettingsRectsOverlap(card.Bounds,plan.Cards[j].Bounds))return false;
    }
    return true;
}

// Keep the existing navigation dock intact. The dashboard only uses
// the measured left text lane after the pinned menu heading and controls.
// The existing text pager retains its original logical content.
inline MainMenuDashboardLayout CalculateMainMenuDashboardLayout(
    double width,double height,HudLayout hud,HudMenuVerticalPlan vertical,
    const HudMenuReadablePanel& readable,double safeWidth,double currentY,
    int drawnRows,bool touch,MenuPage page) noexcept
{
    if(page!=MenuPage::Main || touch || hud.Compact || !vertical.Valid ||
        !readable.Valid || !std::isfinite(width) || !std::isfinite(height) ||
        width<900 || height<600 || width>20000 || height>20000 ||
        !std::isfinite(hud.Scale) || hud.Scale<0.6 || hud.Scale>1.2 ||
        !std::isfinite(hud.Left) || !std::isfinite(hud.Top) ||
        !std::isfinite(safeWidth) || safeWidth<=0 ||
        !std::isfinite(currentY) || drawnRows<0 || drawnRows>vertical.MaxDetailsRows ||
        vertical.MaxDetailsRows>90 ||
        !std::isfinite(vertical.OverflowNoticeY) ||
        !std::isfinite(vertical.SaveWarningY) ||
        vertical.OverflowNoticeY>=vertical.SaveWarningY)
        return {};
    const double s=hud.Scale,step=28*s;
    if(std::abs(currentY-(hud.Top+drawnRows*step))>0.02)return {};
    const double w=std::min({safeWidth,readable.X+readable.Width-hud.Left,720*s});
    const double pad=8*s,gap=7*s,header=20*s;
    const double usable=w-2*pad;
    const double cardHeight=std::max(44.0,58*s);
    const DesktopMenuMouseRect owner{readable.X,readable.Y,readable.Width,readable.Height};
    for(int columns:{3,2})
    {
        const int rows=(MainMenuDestinationCount+columns-1)/columns;
        const double cardWidth=(usable-(columns-1)*gap)/columns;
        if(cardWidth<std::max(100.0,136*s))continue;
        const double panelHeight=2*pad+header+gap+rows*cardHeight+(rows-1)*gap;
        const int reserve=static_cast<int>(std::ceil((panelHeight+gap)/step));
        const int remaining=vertical.MaxDetailsRows-drawnRows-reserve;
        if(remaining<MainMenuMinimumRemainingRows)continue;
        MainMenuDashboardLayout out;
        out.Valid=true;out.Width=width;out.Height=height;out.Scale=s;
        out.Columns=columns;out.ReservedRows=reserve;
        out.NextDetailY=currentY+reserve*step;
        out.Panel={hud.Left,currentY,w,panelHeight};
        out.Header={hud.Left+pad,currentY+pad,usable,header};
        if(!ContainsSettingsRect(owner,out.Panel) ||
            out.NextDetailY+(remaining-1)*step>
                vertical.OverflowNoticeY-1.25*step+1e-7)
            continue;
        const double top=out.Header.Y+header+gap;
        for(int i=0;i<MainMenuDestinationCount;++i)
        {
            auto& c=out.Cards[static_cast<std::size_t>(i)];
            c.Destination=MainMenuDestinations[static_cast<std::size_t>(i)].Page;
            c.Bounds={hud.Left+pad+(i%columns)*(cardWidth+gap),
                top+(i/columns)*(cardHeight+gap),cardWidth,cardHeight};
            const double inset=6*s;
            c.Title={c.Bounds.X+inset,c.Bounds.Y+4*s,
                cardWidth-2*inset,cardHeight*0.46-4*s};
            c.Description={c.Bounds.X+inset,c.Bounds.Y+cardHeight*0.51,
                cardWidth-2*inset,cardHeight*0.49-4*s};
        }
        if(ValidMainMenuDashboardLayout(out))return out;
    }
    return {};
}

struct MainMenuDashboardPresentation final
{
    bool Valid=false;
    MainMenuDashboardLayout Layout{};
    MainMenuDashboardSnapshot Snapshot{};
    std::array<bool,MainMenuDestinationCount> Rendered{};
};

struct MainMenuDashboardHit final
{
    bool Consumed=false;
    MenuPage Destination=MenuPage::Closed;
};

inline MainMenuDashboardHit HitPresentedMainMenuDashboard(
    const MainMenuDashboardPresentation& presented,
    const MainMenuDashboardSnapshot& current,double width,double height,double x,double y) noexcept
{
    if(!presented.Valid || !presented.Layout.Panel.Contains(x,y))return {};
    // Stale input is consumed in the old lane, not delegated to a hidden
    // purchase, start-race action or an unrelated menu destination.
    if(!current.AllowsNavigation() || presented.Snapshot!=current ||
        !ValidMainMenuDashboardLayout(presented.Layout) ||
        width!=presented.Layout.Width || height!=presented.Layout.Height)
        return {true,MenuPage::Closed};
    for(std::size_t i=0;i<presented.Layout.Cards.size();++i)
    {
        const auto& card=presented.Layout.Cards[i];
        if(card.Bounds.Contains(x,y))
            return {true,presented.Rendered[i]?card.Destination:MenuPage::Closed};
    }
    return {true,MenuPage::Closed};
}
} // namespace latebrake

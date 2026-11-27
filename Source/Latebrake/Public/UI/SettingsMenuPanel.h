#pragma once

#include "UI/SettingsMenuSelection.h"
#include "UI/HudMenuMouseControls.h"
#include "UI/HudMenuReadablePanel.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace latebrake
{
inline bool ValidSettingsRect(DesktopMenuMouseRect r) noexcept
{
    return std::isfinite(r.X) && std::isfinite(r.Y) &&
        std::isfinite(r.Width) && std::isfinite(r.Height) &&
        r.Width>0.0 && r.Height>0.0 &&
        std::isfinite(r.X+r.Width) && std::isfinite(r.Y+r.Height) &&
        r.X+r.Width>r.X && r.Y+r.Height>r.Y;
}

inline bool SettingsRectsOverlap(DesktopMenuMouseRect a,DesktopMenuMouseRect b) noexcept
{
    return ValidSettingsRect(a) && ValidSettingsRect(b) &&
        a.X<b.X+b.Width && b.X<a.X+a.Width &&
        a.Y<b.Y+b.Height && b.Y<a.Y+a.Height;
}

inline bool ContainsSettingsRect(DesktopMenuMouseRect outer,DesktopMenuMouseRect inner) noexcept
{
    return ValidSettingsRect(outer) && ValidSettingsRect(inner) &&
        inner.X>=outer.X && inner.Y>=outer.Y &&
        inner.X+inner.Width<=outer.X+outer.Width+1e-6 &&
        inner.Y+inner.Height<=outer.Y+outer.Height+1e-6;
}

struct SettingsPanelRow final
{
    SettingsControl Control=SettingsControl::Count;
    DesktopMenuMouseRect Bounds{};
    DesktopMenuMouseRect Title{};
    DesktopMenuMouseRect Value{};
    DesktopMenuMouseRect Status{};
};

struct SettingsPanelLayout final
{
    bool Valid=false;
    double ViewportWidth=0.0,ViewportHeight=0.0,Scale=1.0;
    DesktopMenuMouseRect Panel{};
    DesktopMenuMouseRect Header{};
    std::array<SettingsPanelRow,SettingsControlCount> Rows{};
};

// Optional desktop card lane below the existing menu dock. Compact and
// phone layouts use the exact same Browse/Select state and text rows.
// The panel never covers the left text lane or creates a new touch zone.
inline SettingsPanelLayout CalculateSettingsPanelLayout(double width,double height,
    HudLayout hud,const DesktopMenuMouseLayout& dock,
    const HudMenuReadablePanel& text,bool touch,MenuPage page) noexcept
{
    if(page!=MenuPage::Settings || touch || !dock.Valid || !text.Valid ||
        !std::isfinite(width) || !std::isfinite(height) || width<900 || height<600 ||
        !std::isfinite(hud.Scale) || hud.Scale<0.25 || hud.Scale>4.0)
        return {};
    const double s=std::clamp(hud.Scale,0.65,1.15);
    double left=width,right=0,bottom=0;
    for(const auto& button:dock.Buttons)
    {
        if(!ContainsSettingsRect({0,0,width,height},button.Bounds))return {};
        left=std::min(left,button.Bounds.X);
        right=std::max(right,button.Bounds.X+button.Bounds.Width);
        bottom=std::max(bottom,button.Bounds.Y+button.Bounds.Height);
    }
    const double pad=8.0*s,gap=6.0*s,header=26.0*s;
    const double rowHeight=std::max(60.0,72.0*s);
    const double panelHeight=2.0*pad+header+SettingsControlCount*(rowHeight+gap);
    const double top=bottom+12.0*s;
    const DesktopMenuMouseRect panel{left,top,right-left,panelHeight};
    const DesktopMenuMouseRect textRect{text.X,text.Y,text.Width,text.Height};
    if(!ContainsSettingsRect({0,0,width,height-16.0*s},panel) ||
        !ContainsSettingsRect({0,0,width,height},textRect) ||
        SettingsRectsOverlap(panel,textRect) || panel.Width<220.0)
        return {};
    SettingsPanelLayout out;
    out.ViewportWidth=width;
    out.ViewportHeight=height;
    out.Scale=s;
    out.Panel=panel;
    out.Header={left+pad,top+pad,panel.Width-2.0*pad,header};
    const double start=out.Header.Y+header+gap;
    for(int i=0;i<SettingsControlCount;++i)
    {
        auto& row=out.Rows[static_cast<std::size_t>(i)];
        row.Control=SettingsControls[static_cast<std::size_t>(i)];
        row.Bounds={left+pad,start+i*(rowHeight+gap),panel.Width-2.0*pad,rowHeight};
        const auto& b=row.Bounds;
        const double textWidth=b.Width-2.0*pad;
        row.Title={b.X+pad,b.Y+2.0*s,textWidth,b.Height*0.30-3.0*s};
        row.Value={b.X+pad,b.Y+b.Height*0.30,textWidth,b.Height*0.37-1.0*s};
        row.Status={b.X+pad,b.Y+b.Height*0.67,textWidth,b.Height*0.33-2.0*s};
        if(!ContainsSettingsRect(panel,b) || !ContainsSettingsRect(b,row.Title) ||
            !ContainsSettingsRect(b,row.Value) || !ContainsSettingsRect(b,row.Status))return {};
    }
    out.Valid=ContainsSettingsRect(panel,out.Header);
    return out;
}

struct SettingsPanelPresentation final
{
    bool Valid=false;
    SettingsPanelLayout Layout{};
    SettingsMenuSnapshot Snapshot{};
    std::array<bool,SettingsControlCount> Rendered{};
};

struct SettingsPanelHit final
{
    bool Consumed=false;
    SettingsControl Action=SettingsControl::Count;
};

inline SettingsPanelHit HitPresentedSettingsPanel(const SettingsPanelPresentation& shown,
    const SettingsMenuSnapshot& current,double width,double height,
    double x,double y) noexcept
{
    if(!shown.Valid || !shown.Layout.Valid || !shown.Layout.Panel.Contains(x,y))
        return {};
    // A stale presentation consumes clicks inside its old lane rather
    // than passing through into a hidden purchase or another action.
    if(!current.Access.OwnsInput() || shown.Snapshot!=current ||
        shown.Layout.ViewportWidth!=width || shown.Layout.ViewportHeight!=height)
        return {true,SettingsControl::Count};
    for(std::size_t i=0;i<shown.Layout.Rows.size();++i)
    {
        const auto& row=shown.Layout.Rows[i];
        if(!row.Bounds.Contains(x,y))continue;
        const auto control=SettingsControls[i];
        if(row.Control!=control || !shown.Rendered[i] || !current.Access.Allows(control))
            return {true,SettingsControl::Count};
        return {true,control};
    }
    return {true,SettingsControl::Count};
}
} // namespace latebrake

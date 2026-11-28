#pragma once

#include "Racing/PracticeTrialSetup.h"
#include "Racing/PracticeExperimentPlan.h"
#include "UI/QuickRaceChoicePanel.h"

#include <array>
#include <cstddef>

namespace latebrake
{
// A secondary, independent panel beneath the eight mode cards. It is
// intentionally absent when the existing readable text or viewport would
// be crowded. Keyboard setup remains available in that case.
inline constexpr std::size_t PracticeTrialControlCount=13;

struct PracticeTrialControl final
{
    PracticeTrialAction Action=PracticeTrialAction::Toggle;
    int Vehicle=-1;
    DesktopMenuMouseRect Bounds{};
};

struct PracticeTrialPanelLayout final
{
    bool Valid=false;
    double Width=0,Height=0,Scale=1;
    DesktopMenuMouseRect Panel{};
    DesktopMenuMouseRect Header{};
    DesktopMenuMouseRect Summary{};
    std::array<PracticeTrialControl,PracticeTrialControlCount> Controls{};
};

inline bool ValidPracticeTrialPanelLayout(const PracticeTrialPanelLayout& layout) noexcept
{
    if(!layout.Valid || !std::isfinite(layout.Width) || !std::isfinite(layout.Height) ||
        !std::isfinite(layout.Scale) || layout.Scale<0.65 || layout.Scale>1.15 ||
        layout.Width<900 || layout.Height<600 ||
        !ContainsSettingsRect({0,0,layout.Width,layout.Height},layout.Panel) ||
        !ContainsSettingsRect(layout.Panel,layout.Header) ||
        !ContainsSettingsRect(layout.Panel,layout.Summary) ||
        SettingsRectsOverlap(layout.Header,layout.Summary))return false;
    constexpr PracticeTrialAction expected[]={
        PracticeTrialAction::Toggle,PracticeTrialAction::Reset,
        PracticeTrialAction::NextCar,PracticeTrialAction::NextEngine,
        PracticeTrialAction::NextGrip};
    for(std::size_t i=0;i<layout.Controls.size();++i)
    {
        const auto& button=layout.Controls[i];
        if(button.Bounds.Width<32 || button.Bounds.Height<30 ||
            !ContainsSettingsRect(layout.Panel,button.Bounds) ||
            SettingsRectsOverlap(layout.Header,button.Bounds) ||
            SettingsRectsOverlap(layout.Summary,button.Bounds))return false;
        if(i<5)
        {
            if(button.Action!=expected[i] || button.Vehicle!=-1)return false;
        }
        else if(i<11)
        {
            if(button.Action!=PracticeTrialAction::ChooseCar ||
                button.Vehicle!=static_cast<int>(i-5))return false;
        }
        else if(button.Vehicle!=-1 ||
            button.Action!=(i==11?PracticeTrialAction::NextTest:PracticeTrialAction::RetestBest))
            return false;
        for(std::size_t j=0;j<i;++j)
            if(SettingsRectsOverlap(layout.Controls[j].Bounds,button.Bounds))return false;
    }
    return true;
}

inline PracticeTrialPanelLayout CalculatePracticeTrialPanel(
    const QuickRaceChoicePanelLayout& choices,const HudMenuReadablePanel& text) noexcept
{
    if(!ValidQuickRaceChoiceLayout(choices) || !text.Valid)return {};
    const double s=choices.Scale,pad=6*s,gap=5*s;
    const double left=choices.Panel.X,top=choices.Panel.Y+choices.Panel.Height+8*s;
    const double width=choices.Panel.Width;
    const double header=24*s,summary=23*s,row=std::max(32.0,35*s);
    const double height=2*pad+header+summary+4*row+5*gap;
    const DesktopMenuMouseRect viewport{0,0,choices.Width,choices.Height-8*s};
    const DesktopMenuMouseRect textRect{text.X,text.Y,text.Width,text.Height};
    PracticeTrialPanelLayout out;
    out.Valid=true;out.Width=choices.Width;out.Height=choices.Height;out.Scale=s;
    out.Panel={left,top,width,height};
    if(!ContainsSettingsRect(viewport,out.Panel) ||
        SettingsRectsOverlap(out.Panel,textRect) ||
        SettingsRectsOverlap(out.Panel,choices.Panel))return {};
    const double x=left+pad,usable=width-2*pad;
    double y=top+pad;
    out.Header={x,y,usable,header};y+=header+gap;
    out.Summary={x,y,usable,summary};y+=summary+gap;
    const double half=(usable-gap)/2;
    out.Controls[0]={PracticeTrialAction::Toggle,-1,{x,y,half,row}};
    out.Controls[1]={PracticeTrialAction::Reset,-1,{x+half+gap,y,half,row}};
    y+=row+gap;
    const double third=(usable-2*gap)/3;
    out.Controls[2]={PracticeTrialAction::NextCar,-1,{x,y,third,row}};
    out.Controls[3]={PracticeTrialAction::NextEngine,-1,{x+third+gap,y,third,row}};
    out.Controls[4]={PracticeTrialAction::NextGrip,-1,{x+2*(third+gap),y,third,row}};
    y+=row+gap;
    const double sixth=(usable-5*gap)/VehicleCount;
    for(int i=0;i<VehicleCount;++i)
        out.Controls[static_cast<std::size_t>(5+i)]={PracticeTrialAction::ChooseCar,i,
            {x+i*(sixth+gap),y,sixth,row}};
    y+=row+gap;
    out.Controls[11]={PracticeTrialAction::NextTest,-1,{x,y,half,row}};
    out.Controls[12]={PracticeTrialAction::RetestBest,-1,{x+half+gap,y,half,row}};
    return ValidPracticeTrialPanelLayout(out)?out:PracticeTrialPanelLayout{};
}

struct PracticeTrialPanelSnapshot final
{
    QuickRaceChoiceSnapshot Choice{};
    bool ManualPause=false;
    bool TrialEnabled=false;
    GarageTuningKey Key{};
    bool Owned=false;
    PracticeExperimentChoices Experiments{};

    bool CanEdit() const noexcept
    {
        return Choice.CanSelect() && !ManualPause && Choice.Selected>=4 &&
            ValidPracticeTrialKey(Key);
    }

    bool operator==(const PracticeTrialPanelSnapshot&) const noexcept=default;
};

inline PracticeTrialPanelSnapshot BuildPracticeTrialPanelSnapshot(
    QuickRaceChoiceSnapshot choice,PracticeTrialReadout trial,bool manualPause,
    PracticeExperimentChoices experiments={}) noexcept
{
    return {choice,manualPause,trial.Enabled,trial.Key,trial.Owned,experiments};
}

inline bool PracticeTrialControlEnabled(const PracticeTrialPanelSnapshot& snapshot,
    const PracticeTrialControl& control) noexcept
{
    if(!snapshot.CanEdit())return false;
    if(control.Action==PracticeTrialAction::Toggle)return control.Vehicle==-1;
    if(!snapshot.TrialEnabled)return false;
    switch(control.Action)
    {
    case PracticeTrialAction::NextTest:
        return control.Vehicle==-1 && snapshot.Experiments.NextValid &&
            ValidPracticeExperimentReason(snapshot.Experiments.Reason) &&
            ValidPracticeTrialKey(snapshot.Experiments.Next) &&
            !(snapshot.Experiments.Next==snapshot.Key);
    case PracticeTrialAction::RetestBest:
        return control.Vehicle==-1 && snapshot.Experiments.BestValid &&
            IsValidFinishedRaceTime(snapshot.Experiments.BestSeconds) &&
            ValidPracticeTrialKey(snapshot.Experiments.Best) &&
            !(snapshot.Experiments.Best==snapshot.Key);
    case PracticeTrialAction::ChooseCar:
        return GarageState::IsVehicle(control.Vehicle) &&
            control.Vehicle!=snapshot.Key.Selected;
    case PracticeTrialAction::NextCar:
    case PracticeTrialAction::NextEngine:
    case PracticeTrialAction::NextGrip:
    case PracticeTrialAction::Reset:return control.Vehicle==-1;
    default:return false;
    }
}

struct PracticeTrialPanelPresentation final
{
    bool Valid=false;
    PracticeTrialPanelLayout Layout{};
    PracticeTrialPanelSnapshot Snapshot{};
    std::array<bool,PracticeTrialControlCount> Rendered{};
};

struct PracticeTrialPanelHit final
{
    bool Consumed=false;
    bool Accepted=false;
    PracticeTrialAction Action=PracticeTrialAction::Toggle;
    int Vehicle=-1;
};

inline PracticeTrialPanelHit HitPresentedPracticeTrial(
    const PracticeTrialPanelPresentation& shown,const PracticeTrialPanelSnapshot& current,
    double width,double height,double x,double y) noexcept
{
    if(!shown.Valid || !shown.Layout.Panel.Contains(x,y))return {};
    if(!ValidPracticeTrialPanelLayout(shown.Layout) || !current.CanEdit() ||
        !(shown.Snapshot==current) || width!=shown.Layout.Width || height!=shown.Layout.Height)
        return {true,false};
    for(std::size_t i=0;i<shown.Layout.Controls.size();++i)
    {
        const auto& button=shown.Layout.Controls[i];
        if(!button.Bounds.Contains(x,y))continue;
        return {true,shown.Rendered[i] && PracticeTrialControlEnabled(current,button),
            button.Action,button.Vehicle};
    }
    return {true,false};
}
} // namespace latebrake

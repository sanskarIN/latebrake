#pragma once

#include "Racing/QuickRaceModePicker.h"
#include "UI/SettingsMenuPanel.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace latebrake
{
// Direct pointer selection is the same pending choice used by the
// existing Browse/Select inputs. It never starts a race or changes the
// active opponent difficulty, route, garage or persistent profile.
inline constexpr int QuickRaceChoiceCount=8;

constexpr QuickRaceMenuChoice QuickRaceChoiceAt(int index) noexcept
{
    switch(index)
    {
    case 0:return {RivalDifficultyPreset::Standard,false};
    case 1:return {RivalDifficultyPreset::Expert,false};
    case 2:return {RivalDifficultyPreset::Expert,true};
    case 3:return {RivalDifficultyPreset::Rookie,false};
    case 4:return PracticeMenuChoice(QuickPracticePreset::SoloThree);
    case 5:return PracticeMenuChoice(QuickPracticePreset::SoloFive);
    case 6:return PracticeMenuChoice(QuickPracticePreset::RivalFive);
    case 7:return PracticeMenuChoice(QuickPracticePreset::RivalTen);
    default:return {static_cast<RivalDifficultyPreset>(255),false};
    }
}

constexpr int QuickRaceChoiceIndex(QuickRaceMenuChoice choice) noexcept
{
    if(!IsValidQuickRaceMenuChoice(choice))return -1;
    if(choice.Practice!=QuickPracticePreset::None)
    {
        switch(choice.Practice)
        {
        case QuickPracticePreset::SoloThree:return 4;
        case QuickPracticePreset::SoloFive:return 5;
        case QuickPracticePreset::RivalFive:return 6;
        case QuickPracticePreset::RivalTen:return 7;
        default:return -1;
        }
    }
    if(choice.SoloTimeAttack)return 2;
    switch(choice.Difficulty)
    {
    case RivalDifficultyPreset::Standard:return 0;
    case RivalDifficultyPreset::Expert:return 1;
    case RivalDifficultyPreset::Rookie:return 3;
    default:return -1;
    }
}

struct QuickRaceChoiceSnapshot final
{
    bool MenuOpen=false;
    MenuPage Page=MenuPage::Closed;
    bool LocalPlayer=false;
    bool FocusSuspended=false;
    bool ReadOnlyProfile=false;
    bool ExperimentalFleetFlag=false;
    int Selected=-1;
    std::uint64_t PageRevision=0;
    std::uint64_t ChoiceRevision=0;

    constexpr bool CanSelect() const noexcept
    {
        return MenuOpen && Page==MenuPage::QuickRace && LocalPlayer &&
            !FocusSuspended && Selected>=0 && Selected<QuickRaceChoiceCount &&
            PageRevision!=0 && ChoiceRevision!=0;
    }

    constexpr bool operator==(const QuickRaceChoiceSnapshot&) const noexcept=default;
};

struct QuickRaceChoiceDescription final
{
    bool Valid=false;
    bool Enabled=false;
    bool Selected=false;
    bool Practice=false;
    int Laps=0;
    int RequestedRivals=0;
    bool ProfileRecordsPossible=false;
    const char* Title="";
    const char* ShortTitle="";
    const char* Status="";
};

constexpr QuickRaceChoiceDescription DescribeQuickRaceChoice(
    const QuickRaceChoiceSnapshot& snapshot,int index) noexcept
{
    const auto choice=QuickRaceChoiceAt(index);
    if(!IsValidQuickRaceMenuChoice(choice))return {};
    QuickRaceChoiceDescription out;
    out.Valid=true;
    out.Enabled=snapshot.CanSelect();
    out.Selected=snapshot.Selected==index;
    const auto practice=GetQuickPracticeDefinition(choice.Practice);
    out.Practice=practice.Valid;
    if(practice.Valid)
    {
        out.Laps=practice.RequiredLaps;
        out.RequestedRivals=practice.SpawnRival?1:0;
        out.Status="SESSION ONLY";
        switch(choice.Practice)
        {
        case QuickPracticePreset::SoloThree:out.Title="SOLO PRACTICE 3";out.ShortTitle="SOLO 3";break;
        case QuickPracticePreset::SoloFive:out.Title="SOLO PRACTICE 5";out.ShortTitle="SOLO 5";break;
        case QuickPracticePreset::RivalFive:out.Title="RIVAL PRACTICE 5";out.ShortTitle="RIVAL 5";break;
        case QuickPracticePreset::RivalTen:out.Title="RIVAL PRACTICE 10";out.ShortTitle="RIVAL 10";break;
        default:return {};
        }
    }
    else
    {
        const auto event=GetRaceEvent(QuickRaceMenuEvent(choice));
        out.Laps=event.RequiredLaps;
        const bool fleet=event.SpawnRival && snapshot.ExperimentalFleetFlag;
        out.RequestedRivals=fleet?5:event.SpawnRival?1:0;
        out.ProfileRecordsPossible=!fleet && !snapshot.ReadOnlyProfile;
        out.Status=fleet?"FLEET TEST":snapshot.ReadOnlyProfile?"READ ONLY":"SAVED IF VALID";
        switch(index)
        {
        case 0:out.Title="STANDARD CIRCUIT";out.ShortTitle="STANDARD";break;
        case 1:out.Title="EXPERT CIRCUIT";out.ShortTitle="EXPERT";break;
        case 2:out.Title="SOLO TIME ATTACK";out.ShortTitle="TIME ATTACK";break;
        case 3:out.Title="ROOKIE CIRCUIT";out.ShortTitle="ROOKIE";break;
        default:return {};
        }
    }
    if(!out.Enabled)out.Status="INPUT LOCKED";
    return out;
}

struct QuickRaceChoiceCard final
{
    int Index=-1;
    DesktopMenuMouseRect Bounds{};
    DesktopMenuMouseRect Title{};
    DesktopMenuMouseRect Detail{};
    DesktopMenuMouseRect Status{};
};

struct QuickRaceChoicePanelLayout final
{
    bool Valid=false;
    double Width=0.0,Height=0.0,Scale=1.0;
    DesktopMenuMouseRect Panel{};
    DesktopMenuMouseRect Header{};
    std::array<QuickRaceChoiceCard,QuickRaceChoiceCount> Cards{};
};

inline bool ValidQuickRaceChoiceLayout(const QuickRaceChoicePanelLayout& plan) noexcept
{
    if(!plan.Valid || !std::isfinite(plan.Width) || !std::isfinite(plan.Height) ||
        plan.Width<900 || plan.Height<600 || !std::isfinite(plan.Scale) ||
        plan.Scale<0.65 || plan.Scale>1.15 ||
        !ContainsSettingsRect({0,0,plan.Width,plan.Height},plan.Panel) ||
        !ContainsSettingsRect(plan.Panel,plan.Header))
        return false;
    for(std::size_t i=0;i<plan.Cards.size();++i)
    {
        const auto& card=plan.Cards[i];
        if(card.Index!=static_cast<int>(i) || card.Bounds.Width<96 ||
            card.Bounds.Height<44 || !ContainsSettingsRect(plan.Panel,card.Bounds) ||
            !ContainsSettingsRect(card.Bounds,card.Title) ||
            !ContainsSettingsRect(card.Bounds,card.Detail) ||
            !ContainsSettingsRect(card.Bounds,card.Status) ||
            SettingsRectsOverlap(card.Title,card.Detail) ||
            SettingsRectsOverlap(card.Title,card.Status) ||
            SettingsRectsOverlap(card.Detail,card.Status) ||
            SettingsRectsOverlap(plan.Header,card.Bounds))
            return false;
        for(std::size_t j=0;j<i;++j)
            if(SettingsRectsOverlap(card.Bounds,plan.Cards[j].Bounds))return false;
    }
    return true;
}

inline QuickRaceChoicePanelLayout CalculateQuickRaceChoicePanel(double width,double height,
    HudLayout hud,const DesktopMenuMouseLayout& dock,const HudMenuReadablePanel& text,
    bool touch,MenuPage page) noexcept
{
    if(touch || page!=MenuPage::QuickRace || !dock.Valid || !text.Valid ||
        !std::isfinite(width) || !std::isfinite(height) || width<900 || height<600 ||
        !std::isfinite(hud.Scale) || hud.Scale<0.25 || hud.Scale>4)
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
    const double pad=6*s,gap=6*s,header=26*s;
    const double top=bottom+28*s;
    const double cardWidth=(right-left-2*pad-gap)*0.5;
    const double cardHeight=std::max(58.0,68*s);
    const double panelHeight=2*pad+header+gap+4*cardHeight+3*gap;
    QuickRaceChoicePanelLayout out;
    out.Valid=true;out.Width=width;out.Height=height;out.Scale=s;
    out.Panel={left,top,right-left,panelHeight};
    out.Header={left+pad,top+pad,right-left-2*pad,header};
    const DesktopMenuMouseRect textRect{text.X,text.Y,text.Width,text.Height};
    if(!ContainsSettingsRect({0,0,width,height-12*s},out.Panel) ||
        !ContainsSettingsRect({0,0,width,height},textRect) ||
        SettingsRectsOverlap(out.Panel,textRect))return {};
    const double start=out.Header.Y+header+gap;
    for(int i=0;i<QuickRaceChoiceCount;++i)
    {
        auto& card=out.Cards[static_cast<std::size_t>(i)];
        card.Index=i;
        card.Bounds={left+pad+(i%2)*(cardWidth+gap),
            start+(i/2)*(cardHeight+gap),cardWidth,cardHeight};
        const auto b=card.Bounds;
        const double inset=4*s,w=b.Width-2*inset;
        card.Title={b.X+inset,b.Y+2*s,w,b.Height*0.34-3*s};
        card.Detail={b.X+inset,b.Y+b.Height*0.34,w,b.Height*0.32-2*s};
        card.Status={b.X+inset,b.Y+b.Height*0.66,w,b.Height*0.34-2*s};
    }
    if(!ValidQuickRaceChoiceLayout(out))return {};
    return out;
}

struct QuickRaceChoicePresentation final
{
    bool Valid=false;
    QuickRaceChoicePanelLayout Layout{};
    QuickRaceChoiceSnapshot Snapshot{};
    std::array<bool,QuickRaceChoiceCount> Rendered{};
};

struct QuickRaceChoiceHit final
{
    bool Consumed=false;
    int Index=-1;
};

inline QuickRaceChoiceHit HitPresentedQuickRaceChoice(const QuickRaceChoicePresentation& shown,
    const QuickRaceChoiceSnapshot& current,double width,double height,double x,double y) noexcept
{
    if(!shown.Valid || !shown.Layout.Panel.Contains(x,y))return {};
    if(!ValidQuickRaceChoiceLayout(shown.Layout) || !current.CanSelect() ||
        shown.Snapshot!=current || width!=shown.Layout.Width || height!=shown.Layout.Height)
        return {true,-1};
    for(std::size_t i=0;i<shown.Layout.Cards.size();++i)
    {
        const auto& card=shown.Layout.Cards[i];
        if(!card.Bounds.Contains(x,y))continue;
        return {true,shown.Rendered[i]?card.Index:-1};
    }
    return {true,-1};
}
} // namespace latebrake

#pragma once

#include "UI/GarageAppearancePalette.h"

namespace latebrake
{
// Keyboard/controller focus is transient UI state. It cannot stage a
// purchase, select a race car, or update a persisted appearance.
enum class GaragePaletteNavigationInput { Previous, Next, Confirm, Cancel };
enum class GaragePaletteNavigationEffect { Ignore, Move, Confirm, Cancel };

struct GaragePaletteNavigationDecision final
{
    GaragePaletteNavigationEffect Effect=GaragePaletteNavigationEffect::Ignore;
    int Choice=-1;
};

constexpr GarageLookLayer NextGaragePaletteLayer(GarageLookLayer layer) noexcept
{
    switch(layer)
    {
    case GarageLookLayer::Paint: return GarageLookLayer::Stripe;
    case GarageLookLayer::Stripe: return GarageLookLayer::Wheels;
    case GarageLookLayer::Wheels: return GarageLookLayer::Paint;
    default: return GarageLookLayer::Paint;
    }
}

constexpr int InitialGaragePaletteFocus(const GaragePaletteSession& session) noexcept
{
    if(!session.Open||!GarageState::IsVehicle(session.VehicleId)||
        !ValidGarageLook(session.Look))return -1;
    const int choice=GarageLookChoice(session.Look,session.Layer);
    return choice>=0&&choice<GarageLookChoiceCount(session.Layer)?choice:-1;
}

constexpr bool GaragePaletteFocusInRange(int index,int count) noexcept
{
    return count>0&&count<=static_cast<int>(GaragePaletteMaxChoices)&&
        index>=0&&index<count;
}

inline bool SameGaragePalettePresentation(const GaragePresentedPalette& shown,
    const GaragePaletteSession& current,const GaragePaletteLayout& layout) noexcept
{
    if(!shown.Session.Matches(current.VehicleId,current.Look,current.Open)||
        shown.Session.Layer!=current.Layer||
        layout.Count!=GarageLookChoiceCount(current.Layer))
        return false;
    return SameValidGaragePaletteLayout(shown.Layout,layout);
}

// Moving focus never commits a choice. A confirm is permitted only when
// the currently focused button was visibly drawn as focused, and the
// entire presentation still corresponds to the exact current session.
// This prevents rapid controller input from equipping a car, choosing a
// not-yet-drawn color, or applying a choice from a previous viewport.
inline GaragePaletteNavigationDecision NavigateGaragePalette(
    const GaragePaletteSession& session,const GaragePaletteLayout& layout,
    const GaragePresentedPalette& shown,int focused,int drawnFocus,
    GaragePaletteNavigationInput input) noexcept
{
    if(!session.Open)return {};
    if(input==GaragePaletteNavigationInput::Cancel)
        return {GaragePaletteNavigationEffect::Cancel,-1};
    if(!SameGaragePalettePresentation(shown,session,layout))return {};
    const int count=layout.Count;
    const auto visible=[&](int index)
    {
        return GaragePaletteFocusInRange(index,count)&&
            CanClickPresentedGaragePalette(shown,session,layout,GaragePaletteChoiceAction(index));
    };
    if(input==GaragePaletteNavigationInput::Confirm)
    {
        if(focused==drawnFocus&&visible(focused))
            return {GaragePaletteNavigationEffect::Confirm,focused};
        return {};
    }
    if(input!=GaragePaletteNavigationInput::Previous&&input!=GaragePaletteNavigationInput::Next)
        return {};
    const int step=input==GaragePaletteNavigationInput::Next?1:-1;
    int index=GaragePaletteFocusInRange(focused,count)?focused:(step>0?count-1:0);
    for(int attempts=0;attempts<count;++attempts)
    {
        index=(index+step+count)%count;
        if(visible(index))
            return {GaragePaletteNavigationEffect::Move,index};
    }
    return {};
}
} // namespace latebrake

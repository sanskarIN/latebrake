#pragma once

#include "Racing/FleetReplayCapture.h"
#include "Track/CircuitMap.h"
#include "UI/HudMenuMouseControls.h"
#include "UI/HudMenuReadablePanel.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace latebrake
{
// Replay controls are presentation state only. They never touch a
// simulation clock, physical actor, camera, save, or race authority.
enum class FleetReplayAction
{
    None,
    PlayPause,
    Previous,
    Next,
    NextHighlight,
    NextFocus,
    Speed
};

inline constexpr double FleetReplaySeekSeconds=0.5;
inline constexpr double FleetReplayMaxUiDeltaSeconds=0.25;
inline constexpr std::array<double,3> FleetReplaySpeeds{{0.5,1.0,2.0}};

struct FleetReplayViewerState final
{
    bool Valid=false;
    bool Playing=false;
    std::uint64_t Attempt=0;
    double FirstSeconds=0.0;
    double LastSeconds=0.0;
    double CursorSeconds=0.0;
    int FocusSlot=0;
    int SpeedIndex=1;
    int SelectedHighlight=-1;
    unsigned HighlightCount=0;
    double ClipStartSeconds=0.0;
    double ClipEndSeconds=0.0;
    bool ClipPrimed=false;
    double ClipEventSeconds=0.0;
    ReplayBroadcastEventKind ClipKind=ReplayBroadcastEventKind::Unknown;
    double ClipSampledSpeedKmh=0.0;
    unsigned ClipObservations=0;
};

inline bool ValidFleetReplayReview(const FleetReplayReview& review) noexcept
{
    const auto& c=review.Coverage;
    return review.Valid && review.Attempt!=0 && c.Frozen && c.HasReplay &&
        c.RetainedFrames>=2 && c.RetainedFrames<=PhysicalReplayFrameCapacity &&
        c.RecordedFrames>=c.RetainedFrames &&
        std::isfinite(c.FirstClockSeconds) && std::isfinite(c.LastClockSeconds) &&
        c.FirstClockSeconds>0.0 && c.LastClockSeconds>c.FirstClockSeconds &&
        c.LastClockSeconds<=86400.0 &&
        review.HighlightCount<=PhysicalReplayMaximumHighlights;
}

inline bool ValidFleetReplayHighlight(const FleetReplayReview& review,
    unsigned index) noexcept
{
    if(!ValidFleetReplayReview(review) || index>=review.HighlightCount)
        return false;
    const auto& h=review.Highlights[index];
    return h.Valid && h.FocusSlot>=0 &&
        h.FocusSlot<static_cast<int>(PhysicalReplayCarCount) &&
        PhysicalReplayEventPriority(h.Kind)>0u &&
        h.Observations>0u && std::isfinite(h.SampledSpeedKmh) &&
        h.SampledSpeedKmh>=0.0 &&
        h.SampledSpeedKmh<=PhysicalReplayMaximumSpeedCmps*0.036 &&
        std::isfinite(h.StartSeconds) && std::isfinite(h.EndSeconds) &&
        std::isfinite(h.EventSeconds) &&
        h.StartSeconds>=review.Coverage.FirstClockSeconds &&
        h.EndSeconds<=review.Coverage.LastClockSeconds &&
        h.StartSeconds<h.EndSeconds &&
        h.EventSeconds>=h.StartSeconds && h.EventSeconds<=h.EndSeconds;
}

class FleetReplayViewer final
{
public:
    void Reset() noexcept
    {
        State={};
        LastWall=0.0;
        HaveWall=false;
    }

    bool Matches(const FleetReplayReview& review) const noexcept
    {
        return State.Valid && ValidFleetReplayReview(review) &&
            State.Attempt==review.Attempt &&
            State.FirstSeconds==review.Coverage.FirstClockSeconds &&
            State.LastSeconds==review.Coverage.LastClockSeconds &&
            State.HighlightCount==review.HighlightCount;
    }

    bool Synchronize(const FleetReplayReview& review,bool visible) noexcept
    {
        if(!visible || !ValidFleetReplayReview(review))
        {
            Reset();
            return false;
        }
        if(Matches(review))
        {
            if(State.SelectedHighlight>=0 && !SelectedClipMatches(review))
            {
                ClearClip();
                Pause();
            }
            return true;
        }
        Reset();
        State.Valid=true;
        State.Attempt=review.Attempt;
        State.FirstSeconds=review.Coverage.FirstClockSeconds;
        State.LastSeconds=review.Coverage.LastClockSeconds;
        State.CursorSeconds=State.FirstSeconds;
        State.HighlightCount=review.HighlightCount;
        return true;
    }

    FleetReplayViewerState Readout() const noexcept {return State;}

    double Speed() const noexcept
    {
        return FleetReplaySpeeds[static_cast<std::size_t>(State.SpeedIndex)];
    }

    double Fraction() const noexcept
    {
        if(!State.Valid) return 0.0;
        return std::clamp((State.CursorSeconds-State.FirstSeconds)/
            (State.LastSeconds-State.FirstSeconds),0.0,1.0);
    }

    bool CanAct(FleetReplayAction action,const FleetReplayReview& review) const noexcept
    {
        if(!Matches(review)) return false;
        switch(action)
        {
            case FleetReplayAction::PlayPause:
                return State.SelectedHighlight<0 || SelectedClipMatches(review);
            case FleetReplayAction::NextFocus:
            case FleetReplayAction::Speed:
                return true;
            case FleetReplayAction::Previous:
                return State.CursorSeconds>State.FirstSeconds;
            case FleetReplayAction::Next:
                return State.CursorSeconds<State.LastSeconds;
            case FleetReplayAction::NextHighlight:
                for(unsigned i=0;i<review.HighlightCount;++i)
                    if(ValidFleetReplayHighlight(review,i)) return true;
                return false;
            default: return false;
        }
    }

    bool Act(FleetReplayAction action,const FleetReplayReview& review) noexcept
    {
        if(!CanAct(action,review)) return false;
        switch(action)
        {
            case FleetReplayAction::PlayPause:
                if(!State.Playing)
                {
                    if(State.SelectedHighlight>=0)
                    {
                        if(State.ClipPrimed || State.CursorSeconds>=State.ClipEndSeconds ||
                           State.CursorSeconds<State.ClipStartSeconds)
                            State.CursorSeconds=State.ClipStartSeconds;
                        State.ClipPrimed=false;
                    }
                    else if(State.CursorSeconds>=State.LastSeconds)
                        State.CursorSeconds=State.FirstSeconds;
                }
                State.Playing=!State.Playing;
                HaveWall=false;
                return true;
            case FleetReplayAction::Previous:
                return SeekSeconds(State.CursorSeconds-FleetReplaySeekSeconds);
            case FleetReplayAction::Next:
                return SeekSeconds(State.CursorSeconds+FleetReplaySeekSeconds);
            case FleetReplayAction::NextFocus:
                State.FocusSlot=(State.FocusSlot+1)%static_cast<int>(PhysicalReplayCarCount);
                if(State.SelectedHighlight>=0)
                {
                    ClearClip();
                    Pause();
                }
                return true;
            case FleetReplayAction::Speed:
                State.SpeedIndex=(State.SpeedIndex+1)%static_cast<int>(FleetReplaySpeeds.size());
                HaveWall=false;
                return true;
            case FleetReplayAction::NextHighlight:
            {
                // Cycle only valid candidates in this exact frozen window.
                for(unsigned step=1;step<=review.HighlightCount;++step)
                {
                    const unsigned i=static_cast<unsigned>(State.SelectedHighlight+
                        static_cast<int>(step))%review.HighlightCount;
                    if(!ValidFleetReplayHighlight(review,i)) continue;
                    const auto& h=review.Highlights[i];
                    State.SelectedHighlight=static_cast<int>(i);
                    State.FocusSlot=h.FocusSlot;
                    State.ClipStartSeconds=h.StartSeconds;
                    State.ClipEndSeconds=h.EndSeconds;
                    State.ClipPrimed=true;
                    State.ClipEventSeconds=h.EventSeconds;
                    State.ClipKind=h.Kind;
                    State.ClipSampledSpeedKmh=h.SampledSpeedKmh;
                    State.ClipObservations=h.Observations;
                    return SeekSeconds(h.EventSeconds,false);
                }
                return false;
            }
            default: return false;
        }
    }

    bool SelectFocus(int slot,double presentedSeconds,const FleetReplayReview& review) noexcept
    {
        if(!Matches(review) || slot<0 || slot>=static_cast<int>(PhysicalReplayCarCount) ||
            !std::isfinite(presentedSeconds) || presentedSeconds!=State.CursorSeconds)
            return false;
        State.FocusSlot=slot;
        ClearClip();
        Pause();
        return true;
    }

    bool SeekFraction(double fraction,const FleetReplayReview& review) noexcept
    {
        if(!Matches(review) || !std::isfinite(fraction) || fraction<0.0 || fraction>1.0)
            return false;
        return SeekSeconds(State.FirstSeconds+
            fraction*(State.LastSeconds-State.FirstSeconds));
    }

    void Pause() noexcept
    {
        State.Playing=false;
        HaveWall=false;
    }

    // Real UI time may continue while Unreal's race world is paused.
    // No elapsed time is carried across a focus loss, draw hitch or
    // hidden menu. Never call the authoritative race clock from here.
    void Tick(double wallSeconds,bool focused) noexcept
    {
        if(!State.Valid || !focused || !std::isfinite(wallSeconds) || wallSeconds<0.0)
        {
            Pause();
            return;
        }
        if(!State.Playing)
        {
            HaveWall=false;
            return;
        }
        if(!HaveWall)
        {
            LastWall=wallSeconds;
            HaveWall=true;
            return;
        }
        const double dt=wallSeconds-LastWall;
        LastWall=wallSeconds;
        if(dt<0.0 || dt>FleetReplayMaxUiDeltaSeconds)
        {
            Pause();
            return;
        }
        const double end=State.SelectedHighlight>=0 ? State.ClipEndSeconds : State.LastSeconds;
        State.CursorSeconds=std::min(end,State.CursorSeconds+dt*Speed());
        if(State.CursorSeconds>=end) Pause();
    }

private:
    bool SelectedClipMatches(const FleetReplayReview& review) const noexcept
    {
        if(State.SelectedHighlight<0) return false;
        const auto i=static_cast<unsigned>(State.SelectedHighlight);
        if(!ValidFleetReplayHighlight(review,i)) return false;
        const auto& h=review.Highlights[i];
        return h.FocusSlot==State.FocusSlot &&
            h.StartSeconds==State.ClipStartSeconds &&
            h.EndSeconds==State.ClipEndSeconds &&
            h.EventSeconds==State.ClipEventSeconds && h.Kind==State.ClipKind &&
            h.SampledSpeedKmh==State.ClipSampledSpeedKmh && h.Observations==State.ClipObservations;
    }

    void ClearClip() noexcept
    {
        State.SelectedHighlight=-1;
        State.ClipStartSeconds=0.0;
        State.ClipEndSeconds=0.0;
        State.ClipPrimed=false;
        State.ClipEventSeconds=0.0;
        State.ClipKind=ReplayBroadcastEventKind::Unknown;
        State.ClipSampledSpeedKmh=0.0;
        State.ClipObservations=0;
    }

    bool SeekSeconds(double seconds,bool clearHighlight=true) noexcept
    {
        if(!State.Valid || !std::isfinite(seconds)) return false;
        State.CursorSeconds=std::clamp(seconds,State.FirstSeconds,State.LastSeconds);
        if(clearHighlight) ClearClip();
        Pause();
        return true;
    }

    FleetReplayViewerState State{};
    double LastWall=0.0;
    bool HaveWall=false;
};

struct FleetReplayPanelButton final
{
    FleetReplayAction Action=FleetReplayAction::None;
    DesktopMenuMouseRect Bounds{};
};

struct FleetReplayPanelLayout final
{
    bool Valid=false;
    double ViewportWidth=0.0;
    double ViewportHeight=0.0;
    double Scale=1.0;
    DesktopMenuMouseRect Panel{};
    DesktopMenuMouseRect Map{};
    DesktopMenuMouseRect Scrub{};
    DesktopMenuMouseRect Header{};
    DesktopMenuMouseRect Status{};
    DesktopMenuMouseRect ControlsStatus{};
    std::array<FleetReplayPanelButton,6> Buttons{};
};

inline bool ValidFleetReplayRect(const DesktopMenuMouseRect& r) noexcept
{
    return std::isfinite(r.X) && std::isfinite(r.Y) &&
        std::isfinite(r.Width) && std::isfinite(r.Height) &&
        r.X>=0.0 && r.Y>=0.0 && r.Width>0.0 && r.Height>0.0 &&
        std::isfinite(r.X+r.Width) && std::isfinite(r.Y+r.Height);
}

inline bool FleetReplayRectsOverlap(const DesktopMenuMouseRect& a,
    const DesktopMenuMouseRect& b) noexcept
{
    return ValidFleetReplayRect(a) && ValidFleetReplayRect(b) &&
        a.X<b.X+b.Width && b.X<a.X+a.Width &&
        a.Y<b.Y+b.Height && b.Y<a.Y+a.Height;
}

inline bool FleetReplayContainsRect(const DesktopMenuMouseRect& outer,
    const DesktopMenuMouseRect& inner) noexcept
{
    return ValidFleetReplayRect(outer) && ValidFleetReplayRect(inner) &&
        inner.X>=outer.X && inner.Y>=outer.Y &&
        inner.X+inner.Width<=outer.X+outer.Width+1e-7 &&
        inner.Y+inner.Height<=outer.Y+outer.Height+1e-7;
}

// Use the existing real desktop dock, not an independent screen guess.
// The compact/touch path retains text review and ordinary menu targets.
inline FleetReplayPanelLayout CalculateFleetReplayPanel(double width,double height,
    HudLayout hud,const DesktopMenuMouseLayout& dock,const HudMenuReadablePanel& text,
    bool touch,bool results) noexcept
{
    FleetReplayPanelLayout out;
    if(!results || touch || hud.Compact || !dock.Valid || !text.Valid ||
        !std::isfinite(width) || !std::isfinite(height) ||
        width<900.0 || height<600.0 || width>20000.0 || height>20000.0 ||
        !std::isfinite(hud.Scale) || hud.Scale<0.25 || hud.Scale>4.0)
        return out;
    const double s=std::clamp(hud.Scale,0.6,1.2);
    double left=width,right=0,bottom=0;
    for(const auto& button:dock.Buttons)
    {
        if(!FleetReplayContainsRect({0,0,width,height},button.Bounds)) return out;
        left=std::min(left,button.Bounds.X);
        right=std::max(right,button.Bounds.X+button.Bounds.Width);
        bottom=std::max(bottom,button.Bounds.Y+button.Bounds.Height);
    }
    const double top=bottom+34.0*s;
    const double usable=height-top-16.0*s;
    const double panelWidth=right-left;
    const double inset=8.0*s;
    const double headerHeight=22.0*s;
    const double statusHeight=20.0*s;
    const double scrubHeight=18.0*s;
    const double buttonHeight=std::max(32.0,32.0*s);
    const double gap=5.0*s;
    const double fixed=2.0*inset+headerHeight+2.0*statusHeight+scrubHeight+
        2.0*buttonHeight+6.0*gap;
    const double mapHeight=std::min((panelWidth-2.0*inset)*6000.0/8400.0,usable-fixed);
    if(panelWidth<216.0 || !std::isfinite(mapHeight) || mapHeight<72.0)
        return out;
    const double panelHeight=fixed+mapHeight;
    out.Panel={left,top,panelWidth,panelHeight};
    const DesktopMenuMouseRect textRect{text.X,text.Y,text.Width,text.Height};
    if(!FleetReplayContainsRect({0,0,width,height},out.Panel) ||
        !ValidFleetReplayRect(textRect) || FleetReplayRectsOverlap(out.Panel,textRect))
        return {};
    out.ViewportWidth=width;
    out.ViewportHeight=height;
    out.Scale=s;
    const double x=left+inset;
    const double w=panelWidth-2.0*inset;
    double y=top+inset;
    out.Header={x,y,w,headerHeight};
    y+=headerHeight+gap;
    const double actualMapWidth=std::min(w,mapHeight*8400.0/6000.0);
    out.Map={x+(w-actualMapWidth)*0.5,y,actualMapWidth,mapHeight};
    y+=mapHeight+gap;
    out.Status={x,y,w,statusHeight};
    y+=statusHeight+gap;
    out.ControlsStatus={x,y,w,statusHeight};
    y+=statusHeight+gap;
    out.Scrub={x,y,w,scrubHeight};
    y+=scrubHeight+gap;
    constexpr std::array<FleetReplayAction,6> actions{{
        FleetReplayAction::Previous,FleetReplayAction::PlayPause,FleetReplayAction::Next,
        FleetReplayAction::NextHighlight,FleetReplayAction::NextFocus,FleetReplayAction::Speed}};
    const double bw=(w-2.0*gap)/3.0;
    for(std::size_t i=0;i<actions.size();++i)
    {
        const double bx=x+static_cast<double>(i%3)*(bw+gap);
        const double by=y+static_cast<double>(i/3)*(buttonHeight+gap);
        out.Buttons[i]={actions[i],{bx,by,bw,buttonHeight}};
        if(!FleetReplayContainsRect(out.Panel,out.Buttons[i].Bounds))
            return {};
    }
    if(!FleetReplayContainsRect(out.Panel,out.Header) ||
        !FleetReplayContainsRect(out.Panel,out.Map) ||
        !FleetReplayContainsRect(out.Panel,out.Status) ||
        !FleetReplayContainsRect(out.Panel,out.ControlsStatus) ||
        !FleetReplayContainsRect(out.Panel,out.Scrub))
        return {};
    out.Valid=true;
    return out;
}

struct FleetReplayPointerHit final
{
    bool Consumed=false;
    bool Seek=false;
    double Fraction=0.0;
    FleetReplayAction Action=FleetReplayAction::None;
};

inline FleetReplayPointerHit HitFleetReplayPanel(const FleetReplayPanelLayout& layout,
    double x,double y) noexcept
{
    if(!layout.Valid || !layout.Panel.Contains(x,y)) return {};
    if(layout.Scrub.Contains(x,y))
        return {true,true,std::clamp((x-layout.Scrub.X)/layout.Scrub.Width,0.0,1.0),
            FleetReplayAction::None};
    for(const auto& button:layout.Buttons)
        if(button.Bounds.Contains(x,y)) return {true,false,0.0,button.Action};
    return {true,false,0.0,FleetReplayAction::None};
}

struct FleetReplayMapPoint final
{
    bool Valid=false;
    double X=0.0;
    double Y=0.0;
};

inline FleetReplayMapPoint ProjectFleetReplayMap(CircuitPoint point,
    const DesktopMenuMouseRect& map) noexcept
{
    if(!ValidFleetReplayRect(map) || !std::isfinite(point.X) ||
        !std::isfinite(point.Y) || std::abs(point.X)>4200.0 ||
        std::abs(point.Y)>3000.0) return {};
    const auto uv=NormalizeCircuitMap(point);
    return {true,map.X+uv.U*map.Width,map.Y+(1.0-uv.V)*map.Height};
}

struct FleetReplayMapCar final
{
    bool Visible=false;
    int Slot=-1;
    FleetReplayMapPoint Position{};
    FleetReplayMapPoint Nose{};
    double SpeedKmh=0.0;
    bool Observed=false;
    double Throttle=0.0;
    double Brake=0.0;
    double Steering=0.0;
};

struct FleetReplayMapReadout final
{
    bool Valid=false;
    int VisibleCars=0;
    std::array<FleetReplayMapCar,PhysicalReplayCarCount> Cars{};
};

inline FleetReplayMapReadout BuildFleetReplayMap(
    const PhysicalReplayPlaybackFrame& frame,const FleetReplayPanelLayout& layout,
    const FleetReplayViewerState& viewer) noexcept
{
    FleetReplayMapReadout out;
    if(!layout.Valid || !FleetReplayContainsRect(layout.Panel,layout.Map) ||
        !std::isfinite(layout.Scale) || layout.Scale<0.6 || layout.Scale>1.2 ||
        !viewer.Valid || !frame.Valid || frame.Session!=viewer.Attempt ||
        !frame.NoCollisionAuthority || !frame.NoOfficialTimingAuthority ||
        !frame.SourceOnly || !std::isfinite(frame.ActiveClockSeconds) ||
        std::abs(frame.ActiveClockSeconds-viewer.CursorSeconds)>1.0e-6)
        return out;
    for(std::size_t i=0;i<frame.Cars.size();++i)
    {
        const auto& source=frame.Cars[i];
        if(!source.ProvenanceMatched || source.Actor==0 ||
            !std::isfinite(source.Xcm) || !std::isfinite(source.Ycm) ||
            !std::isfinite(source.HeadingRadians) ||
            !std::isfinite(source.ObservedSpeedCmps) || source.ObservedSpeedCmps<0.0 ||
            source.ObservedSpeedCmps>PhysicalReplayMaximumSpeedCmps ||
            !std::isfinite(source.ThrottleNormalized) || source.ThrottleNormalized<0.0 || source.ThrottleNormalized>1.0 ||
            !std::isfinite(source.BrakeNormalized) || source.BrakeNormalized<0.0 || source.BrakeNormalized>1.0 ||
            !std::isfinite(source.SteeringNormalized) || std::abs(source.SteeringNormalized)>1.0)
            return {};
        for(std::size_t j=0;j<i;++j)
            if(source.Actor==frame.Cars[j].Actor) return {};
        auto& car=out.Cars[i];
        car.Slot=static_cast<int>(i);
        car.Observed=true;
        car.SpeedKmh=source.ObservedSpeedCmps*0.036;
        car.Throttle=source.ThrottleNormalized;
        car.Brake=source.BrakeNormalized;
        car.Steering=source.SteeringNormalized;
        car.Position=ProjectFleetReplayMap({source.Xcm,source.Ycm},layout.Map);
        if(!car.Position.Valid) continue;
        car.Visible=true;
        const double length=7.0*layout.Scale;
        car.Nose={true,std::clamp(car.Position.X+length*std::cos(source.HeadingRadians),
            layout.Map.X,layout.Map.X+layout.Map.Width),
            std::clamp(car.Position.Y-length*std::sin(source.HeadingRadians),
            layout.Map.Y,layout.Map.Y+layout.Map.Height)};
        ++out.VisibleCars;
    }
    out.Valid=true;
    return out;
}
// Select a uniquely nearest visible marker from the exact presented map.
// A crowded overlap is intentionally ambiguous; the FOCUS button remains
// available to cycle the six identities without guessing a hit target.
inline int HitFleetReplayMapCar(const FleetReplayPanelLayout& layout,
    const FleetReplayMapReadout& map,double x,double y) noexcept
{
    if(!layout.Valid || !map.Valid || !FleetReplayContainsRect(layout.Panel,layout.Map) ||
        !layout.Map.Contains(x,y) || !std::isfinite(layout.Scale) ||
        layout.Scale<0.6 || layout.Scale>1.2)
        return -1;
    const double radius=std::max(8.0,10.0*layout.Scale);
    double nearest=radius*radius;
    int found=-1;
    bool ambiguous=false;
    for(std::size_t i=0;i<map.Cars.size();++i)
    {
        const auto& car=map.Cars[i];
        if(!car.Visible) continue;
        if(!car.Observed || car.Slot!=static_cast<int>(i) ||
            !car.Position.Valid || !layout.Map.Contains(car.Position.X,car.Position.Y))
            return -1;
        const double dx=car.Position.X-x;
        const double dy=car.Position.Y-y;
        const double distance=dx*dx+dy*dy;
        if(distance>radius*radius) continue;
        if(found<0 || distance<nearest-1.0e-7)
        {
            nearest=distance;
            found=car.Slot;
            ambiguous=false;
        }
        else if(std::abs(distance-nearest)<=1.0e-7)
            ambiguous=true;
    }
    return ambiguous?-1:found;
}
} // namespace latebrake

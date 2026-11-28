#pragma once

#include "UI/HudRaceRadar.h"
#include "UI/HudVerticalPlan.h"
#include "UI/RaceGateProgressGauge.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace latebrake
{
// A glanceable presentation of the same authoritative race clock and
// accepted gate counts used by the existing text HUD. This is not a
// race position, distance estimate, timer owner or input surface.
inline constexpr int HudRaceStatusTileCount=4;
inline constexpr int HudRaceStatusMaximumSegments=12;
inline constexpr int HudRaceStatusMinimumRemainingRows=8;
inline constexpr double HudRaceStatusMaximumElapsed=3599999.0;
inline constexpr double HudRaceStatusMaximumSpeedKmh=540.0;

enum class HudRaceStatusTile : std::uint8_t
{
    Clock, Lap, NextGate, Speed
};

struct HudRaceStatusSource final
{
    int CompletedLaps=0;
    int RequiredLaps=0;
    int NextCheckpoint=0;
    int CheckpointCount=0;
    double ElapsedSeconds=0.0;
    double SpeedKmh=0.0;
    bool MilesPerHour=false;
    bool Finished=false;
    bool Running=false;
    bool SpeedObserved=false;
};

struct HudRaceStatusClock final
{
    int Minutes=0;
    int Seconds=0;
    int Hundredths=0;
};

inline HudRaceStatusClock SplitHudRaceStatusClock(double elapsed) noexcept
{
    if(!std::isfinite(elapsed) || elapsed<0.0 || elapsed>HudRaceStatusMaximumElapsed)
        return {};
    // Round the whole clock before splitting, so 59.999 cannot print
    // as 00:60.00. Integer arithmetic also keeps the display stable at
    // the exact minute boundary and for long practice attempts.
    const auto ticks=static_cast<std::int64_t>(std::llround(elapsed*100.0));
    return {static_cast<int>(ticks/6000),static_cast<int>((ticks/100)%60),
        static_cast<int>(ticks%100)};
}

struct HudRaceStatusReadout final
{
    bool Valid=false;
    HudRaceStatusClock Clock{};
    int CurrentLap=0;
    int RequiredLaps=0;
    int NextGate=0;
    int GateCount=0;
    int DisplaySpeed=0;
    bool MilesPerHour=false;
    RaceGateProgressGauge Progress{};
};

inline HudRaceStatusReadout ReadHudRaceStatus(const HudRaceStatusSource& source) noexcept
{
    if(!source.Running || source.Finished || !source.SpeedObserved ||
        !std::isfinite(source.ElapsedSeconds) || source.ElapsedSeconds<0.0 ||
        source.ElapsedSeconds>HudRaceStatusMaximumElapsed ||
        !std::isfinite(source.SpeedKmh) || source.SpeedKmh<0.0 ||
        source.SpeedKmh>HudRaceStatusMaximumSpeedKmh)
        return {};
    const auto progress=CalculateRaceGateProgressGauge(source.CompletedLaps,
        source.NextCheckpoint,source.RequiredLaps,source.CheckpointCount,source.Finished);
    if(!progress.Valid)return {};
    const double speed=source.SpeedKmh*(source.MilesPerHour?0.621371:1.0);
    return {true,SplitHudRaceStatusClock(source.ElapsedSeconds),
        source.CompletedLaps+1,source.RequiredLaps,source.NextCheckpoint+1,
        source.CheckpointCount,static_cast<int>(std::lround(speed)),
        source.MilesPerHour,progress};
}

struct HudRaceStatusTileLayout final
{
    DesktopMenuMouseRect Background{};
    DesktopMenuMouseRect Label{};
    DesktopMenuMouseRect Value{};
};

struct HudRaceStatusLayout final
{
    bool Valid=false;
    double Scale=1.0;
    int ReservedRows=0;
    int RemainingRows=0;
    double TelemetryTop=0.0;
    DesktopMenuMouseRect Panel{};
    std::array<HudRaceStatusTileLayout,HudRaceStatusTileCount> Tiles{};
    DesktopMenuMouseRect ProgressLabel{};
    DesktopMenuMouseRect ProgressTrack{};
};

// Use the already protected left text lane, not a fraction of the
// viewport guessed independently of the radar and right instrument
// panels. Reserve integral baseline rows from the existing vertical
// budget; the bottom status and footer never move.
inline HudRaceStatusLayout CalculateHudRaceStatusLayout(double width,double height,
    HudLayout hud,HudVerticalPlan vertical,HudTextBounds horizontal,double safeTextWidth,
    bool touch,bool menuOpen,bool raceFinished,bool running) noexcept
{
    if(touch || menuOpen || raceFinished || !running || hud.Compact ||
        !vertical.Valid || !horizontal.Valid ||
        !std::isfinite(width) || !std::isfinite(height) ||
        width<900.0 || height<600.0 || width>20000.0 || height>20000.0 ||
        !std::isfinite(hud.Scale) || hud.Scale<0.6 || hud.Scale>1.2 ||
        !std::isfinite(hud.Left) || !std::isfinite(hud.Top) ||
        !std::isfinite(hud.LineHeight) || hud.LineHeight<=0.0 ||
        !std::isfinite(safeTextWidth) || safeTextWidth<=0.0 ||
        !std::isfinite(horizontal.Left) || !std::isfinite(horizontal.Right) ||
        !std::isfinite(horizontal.MaxWidthPixels) ||
        horizontal.Left!=hud.Left || horizontal.Right>width ||
        horizontal.MaxWidthPixels>horizontal.Right-horizontal.Left+1e-7 ||
        !std::isfinite(vertical.SaveWarningY) || !std::isfinite(vertical.RaceStatusY) ||
        !std::isfinite(vertical.FooterY) || vertical.SaveWarningY>=vertical.RaceStatusY ||
        vertical.RaceStatusY>=vertical.FooterY || vertical.FooterY>=height ||
        vertical.MaxTelemetryLines<HudRaceStatusMinimumRemainingRows ||
        vertical.MaxTelemetryLines>80)
        return {};
    const double s=hud.Scale;
    const double panelWidth=std::min({safeTextWidth,horizontal.MaxWidthPixels,
        horizontal.Right-horizontal.Left,720.0*s});
    if(panelWidth<480.0*s)return {};
    const double panelHeight=94.0*s;
    const double rows=std::ceil((panelHeight+10.0*s)/hud.LineHeight);
    if(!std::isfinite(rows) || rows<1 || rows>80)return {};
    HudRaceStatusLayout out;
    out.Scale=s;
    out.ReservedRows=static_cast<int>(rows);
    out.RemainingRows=vertical.MaxTelemetryLines-out.ReservedRows;
    if(out.RemainingRows<HudRaceStatusMinimumRemainingRows)return {};
    out.TelemetryTop=hud.Top+out.ReservedRows*hud.LineHeight;
    out.Panel={hud.Left,hud.Top,panelWidth,panelHeight};
    if(!HudRadarContains({0,0,width,height},out.Panel) ||
        out.TelemetryTop<out.Panel.Y+out.Panel.Height ||
        out.TelemetryTop+(out.RemainingRows-1)*hud.LineHeight>
            vertical.SaveWarningY-1.25*hud.LineHeight+1e-7)
        return {};
    const double pad=8.0*s,gap=6.0*s;
    const double usable=panelWidth-2.0*pad-3.0*gap;
    // Clock gets a wider lane because it may contain long minute
    // counts. All four numbers must fit in full or the panel falls back.
    constexpr std::array<double,HudRaceStatusTileCount> weights{{0.34,0.21,0.21,0.24}};
    double x=out.Panel.X+pad;
    for(std::size_t i=0;i<out.Tiles.size();++i)
    {
        const double w=usable*weights[i];
        auto& tile=out.Tiles[i];
        tile.Background={x,out.Panel.Y+pad,w,52.0*s};
        tile.Label={x+5.0*s,tile.Background.Y+4.0*s,w-10.0*s,14.0*s};
        tile.Value={x+5.0*s,tile.Background.Y+20.0*s,w-10.0*s,27.0*s};
        if(!HudRadarContains(out.Panel,tile.Background) ||
            !HudRadarContains(tile.Background,tile.Label) ||
            !HudRadarContains(tile.Background,tile.Value) ||
            HudRadarOverlaps(tile.Label,tile.Value))
            return {};
        for(std::size_t j=0;j<i;++j)
            if(HudRadarOverlaps(tile.Background,out.Tiles[j].Background))return {};
        x+=w+gap;
    }
    out.ProgressLabel={out.Panel.X+pad,out.Panel.Y+64.0*s,panelWidth-2.0*pad,14.0*s};
    out.ProgressTrack={out.Panel.X+pad,out.Panel.Y+82.0*s,panelWidth-2.0*pad,5.0*s};
    if(!HudRadarContains(out.Panel,out.ProgressLabel) ||
        !HudRadarContains(out.Panel,out.ProgressTrack) ||
        HudRadarOverlaps(out.ProgressLabel,out.ProgressTrack))
        return {};
    for(const auto& tile:out.Tiles)
        if(HudRadarOverlaps(tile.Background,out.ProgressLabel) ||
            HudRadarOverlaps(tile.Background,out.ProgressTrack))
            return {};
    out.Valid=true;
    return out;
}

struct HudRaceStatusTextSize final
{
    double Width=0.0;
    double Height=0.0;
};

inline bool HudRaceStatusTextFits(const DesktopMenuMouseRect& rect,
    HudRaceStatusTextSize measure) noexcept
{
    return ValidHudRadarRect(rect) && std::isfinite(measure.Width) &&
        std::isfinite(measure.Height) && measure.Width>0.0 && measure.Height>0.0 &&
        measure.Width<=rect.Width && measure.Height<=rect.Height;
}

struct HudRaceStatusMeasurements final
{
    std::array<HudRaceStatusTextSize,HudRaceStatusTileCount> Labels{};
    std::array<HudRaceStatusTextSize,HudRaceStatusTileCount> Values{};
    HudRaceStatusTextSize Progress{};
};

// The native renderer measures the complete actual strings at each font
// size. Suppression of legacy text is transactional: either all essential
// fields fit, or none of the card is drawn and old telemetry remains.
inline bool CanDrawHudRaceStatus(const HudRaceStatusLayout& layout,
    const HudRaceStatusMeasurements& measured) noexcept
{
    if(!layout.Valid || !HudRaceStatusTextFits(layout.ProgressLabel,measured.Progress))
        return false;
    for(std::size_t i=0;i<layout.Tiles.size();++i)
        if(!HudRaceStatusTextFits(layout.Tiles[i].Label,measured.Labels[i]) ||
            !HudRaceStatusTextFits(layout.Tiles[i].Value,measured.Values[i]))
            return false;
    return true;
}

struct HudRaceStatusSegment final
{
    DesktopMenuMouseRect Track{};
    DesktopMenuMouseRect Fill{};
    bool Credited=false;
};

struct HudRaceStatusProgressPlan final
{
    bool Valid=false;
    bool Segmented=false;
    int Count=0;
    std::array<HudRaceStatusSegment,HudRaceStatusMaximumSegments> Segments{};
};

// On short events show each accepted gate as a segment. Long events
// receive a continuous progress bar, avoiding tiny unreadable slivers.
// Never use interpolated vehicle motion as an earned checkpoint.
inline HudRaceStatusProgressPlan BuildHudRaceStatusProgress(
    const HudRaceStatusLayout& layout,const RaceGateProgressGauge& progress) noexcept
{
    HudRaceStatusProgressPlan out;
    if(!layout.Valid || !ValidHudRadarRect(layout.ProgressTrack) ||
        !progress.Valid || progress.TotalGates<=0 || progress.CreditedGates<0 ||
        progress.CreditedGates>progress.TotalGates ||
        !std::isfinite(progress.Fraction) ||
        std::abs(progress.Fraction-static_cast<double>(progress.CreditedGates)/
            progress.TotalGates)>1e-9)
        return out;
    const auto& track=layout.ProgressTrack;
    out.Segmented=progress.TotalGates<=HudRaceStatusMaximumSegments;
    out.Count=out.Segmented?progress.TotalGates:1;
    const double gap=out.Segmented?3.0*layout.Scale:0.0;
    const double w=(track.Width-(out.Count-1)*gap)/out.Count;
    if(!std::isfinite(w) || w<=0.0)return {};
    for(int i=0;i<out.Count;++i)
    {
        auto& segment=out.Segments[static_cast<std::size_t>(i)];
        segment.Track={track.X+i*(w+gap),track.Y,w,track.Height};
        const double fraction=out.Segmented?(i<progress.CreditedGates?1.0:0.0):
            static_cast<double>(progress.CreditedGates)/progress.TotalGates;
        segment.Credited=fraction==1.0;
        if(fraction>0.0)
            segment.Fill={segment.Track.X,segment.Track.Y,w*fraction,track.Height};
        if(!HudRadarContains(track,segment.Track) ||
            (fraction>0.0 && !HudRadarContains(segment.Track,segment.Fill)))
            return {};
    }
    out.Valid=true;
    return out;
}
} // namespace latebrake

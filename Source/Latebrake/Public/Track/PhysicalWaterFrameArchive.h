#pragma once
#include "Track/WorldScaleObservedWaterFilm.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
namespace latebrake {
// 32-frame immutable evidence ring for actual source-witnessed 256-cell
// water estimates. The ring cannot forecast rain, replace an engine
// weather component, alter tire friction or edit the live racing clock.
// All interpolation is labelled VISUAL, and unobserved times are rejected.
inline constexpr unsigned PhysicalWaterArchiveCapacity=32u;
enum class PhysicalWaterArchiveFault : std::uint8_t {
    None, InvalidFrame, WrongRoad, WrongSession, TimeRollback,
    FrameGap, ImpossibleWaterChange, InvalidBudget
};
struct PhysicalWaterArchiveSettings final {
    double MinimumFrameSeconds=0.001;
    double MaximumFrameSeconds=0.2;
    double MaximumClockDeltaSkewSeconds=0.04;
    double MaximumSensorChangeSlackMm=0.35;
    double MaximumExpectedRainMmH=ObservedWaterMaximumRainMmH;
    double MaximumFilmPerCellMm=ObservedWaterMaximumMm;
    double MaximumMassResidualMm=0.01;
    double MaximumActiveClockSeconds=86400.0;
    unsigned MaximumSkippedSourceFrames=8u;
    bool RequireOfficialSessionIdentity=true;
    bool KeepSourceSamplesImmutable=true;
    bool NeverInterpolateForPhysics=true;
    bool NeverAdjustOriginalRaceClock=true;
};
struct PhysicalWaterArchiveReceipt final {
    bool Accepted=false;
    PhysicalWaterArchiveFault Fault=PhysicalWaterArchiveFault::InvalidFrame;
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    std::uint64_t Session=0u;
    std::uint64_t SourceFrame=0u;
    std::size_t RetainedFrames=0u;
    double LatestClockSeconds=0.0;
    double LargestObservedCellChangeMm=0.0;
    bool NoLiveWeatherChanges=true;
};
struct PhysicalWaterPlaybackFrame final {
    bool Valid=false;
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    std::uint64_t Session=0u;
    std::uint64_t Revision=0u;
    std::uint64_t LowerFrame=0u;
    std::uint64_t UpperFrame=0u;
    double SourceClockSeconds=0.0;
    double InterpolationFraction=0.0;
    double MeanFilmMm=0.0;
    double MinimumFilmMm=0.0;
    double MaximumFilmMm=0.0;
    unsigned WetSections=0u;
    std::array<double,ObservedWaterCellCount> DisplayFilmMm{};
    bool SourceOnly=true;
    bool VisualInterpolationOnly=true;
    bool NoOfficialPhysicalForceAuthority=true;
};
inline bool ValidPhysicalWaterArchiveSettings(
    PhysicalWaterArchiveSettings s) noexcept
{
    return std::isfinite(s.MinimumFrameSeconds) &&
        s.MinimumFrameSeconds>=0.001 &&
        s.MinimumFrameSeconds<=0.02 &&
        std::isfinite(s.MaximumFrameSeconds) &&
        s.MaximumFrameSeconds>=0.05 &&
        s.MaximumFrameSeconds<=0.2 &&
        s.MaximumFrameSeconds>s.MinimumFrameSeconds &&
        std::isfinite(s.MaximumClockDeltaSkewSeconds) &&
        s.MaximumClockDeltaSkewSeconds>=0.0 &&
        s.MaximumClockDeltaSkewSeconds<=0.05 &&
        std::isfinite(s.MaximumSensorChangeSlackMm) &&
        s.MaximumSensorChangeSlackMm>=0.1 &&
        s.MaximumSensorChangeSlackMm<=1.0 &&
        std::isfinite(s.MaximumExpectedRainMmH) &&
        s.MaximumExpectedRainMmH>=100.0 &&
        s.MaximumExpectedRainMmH<=300.0 &&
        std::isfinite(s.MaximumFilmPerCellMm) &&
        s.MaximumFilmPerCellMm>=5.0 &&
        s.MaximumFilmPerCellMm<=ObservedWaterMaximumMm &&
        std::isfinite(s.MaximumMassResidualMm) &&
        s.MaximumMassResidualMm>=0.001 &&
        s.MaximumMassResidualMm<=0.05 &&
        std::isfinite(s.MaximumActiveClockSeconds) &&
        s.MaximumActiveClockSeconds>=1200.0 &&
        s.MaximumActiveClockSeconds<=86400.0 &&
        s.MaximumSkippedSourceFrames>=1u &&
        s.MaximumSkippedSourceFrames<=16u &&
        s.RequireOfficialSessionIdentity &&
        s.KeepSourceSamplesImmutable &&
        s.NeverInterpolateForPhysics &&
        s.NeverAdjustOriginalRaceClock;
}
inline bool ValidPhysicalWaterArchiveFrame(
    const ObservedWaterResult& frame,
    PhysicalWaterArchiveSettings settings={}) noexcept
{
    if(!ValidPhysicalWaterArchiveSettings(settings) ||
       !frame.Valid ||
       static_cast<unsigned>(frame.Circuit)>=WorldScaleVariantCount ||
       frame.Fault!=ObservedWaterFault::None ||
       frame.Session==0u ||
       frame.Revision==0u ||
       frame.SourceFrame==0u ||
       !frame.RenderAdvisoryOnly ||
       !frame.OriginalRaceUnaffected ||
       !std::isfinite(frame.ClockSeconds) ||
       frame.ClockSeconds<=0.0 ||
       frame.ClockSeconds>settings.MaximumActiveClockSeconds ||
       !std::isfinite(frame.StepSeconds) ||
       frame.StepSeconds<settings.MinimumFrameSeconds ||
       frame.StepSeconds>settings.MaximumFrameSeconds ||
       !std::isfinite(frame.CellLengthCm) ||
       frame.CellLengthCm<=0.0 ||
       !std::isfinite(frame.Budget.MassResidualMm) ||
       std::abs(frame.Budget.MassResidualMm)>
           settings.MaximumMassResidualMm)
        return false;
    for(double film:frame.FilmMm)
        if(!std::isfinite(film) ||
           film<0.0 ||
           film>settings.MaximumFilmPerCellMm)
            return false;
    return true;
}
class PhysicalWaterFrameArchive final {
public:
    void Reset() noexcept {
        Size=0u;
        Next=0u;
        Session=0u;
        Revision=0u;
        LastFault=PhysicalWaterArchiveFault::None;
    }
    std::size_t Count() const noexcept {return Size;}
    bool HasPlayback() const noexcept {return Size>=2u;}
    PhysicalWaterArchiveFault LastRejection() const noexcept {
        return LastFault;
    }
    double EarliestClock() const noexcept {
        return Size>0u ? FrameAt(0u).ClockSeconds:0.0;
    }
    double LatestClock() const noexcept {
        return Size>0u ? FrameAt(Size-1u).ClockSeconds:0.0;
    }
    bool TryRead(std::size_t index,ObservedWaterResult& result) const noexcept {
        result={};
        if(index>=Size)return false;
        result=FrameAt(index);
        return true;
    }
    PhysicalWaterArchiveReceipt Record(
        const ObservedWaterResult& frame,
        PhysicalWaterArchiveSettings settings={}) noexcept
    {
        PhysicalWaterArchiveReceipt receipt;
        receipt.RetainedFrames=Size;
        if(!ValidPhysicalWaterArchiveFrame(frame,settings)){
            receipt.Fault=PhysicalWaterArchiveFault::InvalidFrame;
            LastFault=receipt.Fault;
            return receipt;
        }
        double maximumChange=0.0;
        if(Size>0u){
            const auto& previous=FrameAt(Size-1u);
            if(frame.Circuit!=previous.Circuit){
                receipt.Fault=PhysicalWaterArchiveFault::WrongRoad;
                LastFault=receipt.Fault;
                return receipt;
            }
            if(frame.Session!=Session ||
               frame.Revision!=Revision){
                receipt.Fault=PhysicalWaterArchiveFault::WrongSession;
                LastFault=receipt.Fault;
                return receipt;
            }
            if(frame.SourceFrame<=previous.SourceFrame ||
               frame.SourceFrame-previous.SourceFrame>
                   settings.MaximumSkippedSourceFrames){
                receipt.Fault=PhysicalWaterArchiveFault::FrameGap;
                LastFault=receipt.Fault;
                return receipt;
            }
            const double dt=frame.ClockSeconds-previous.ClockSeconds;
            if(!std::isfinite(dt) ||
               dt<settings.MinimumFrameSeconds ||
               dt>settings.MaximumFrameSeconds ||
               std::abs(dt-frame.StepSeconds)>
                   settings.MaximumClockDeltaSkewSeconds){
                receipt.Fault=PhysicalWaterArchiveFault::TimeRollback;
                LastFault=receipt.Fault;
                return receipt;
            }
            const double plausibleMaximum=dt*
                settings.MaximumExpectedRainMmH/3600.0+
                settings.MaximumSensorChangeSlackMm;
            for(std::size_t i=0u;i<ObservedWaterCellCount;++i){
                const double change=std::abs(
                    frame.FilmMm[i]-previous.FilmMm[i]);
                maximumChange=std::max(maximumChange,change);
                if(!std::isfinite(change) ||
                   change>plausibleMaximum){
                    receipt.Fault=
                        PhysicalWaterArchiveFault::ImpossibleWaterChange;
                    LastFault=receipt.Fault;
                    return receipt;
                }
            }
        }else{
            Session=frame.Session;
            Revision=frame.Revision;
        }
        Frames[Next]=frame;
        Next=(Next+1u)%PhysicalWaterArchiveCapacity;
        if(Size<PhysicalWaterArchiveCapacity)++Size;
        receipt.Accepted=true;
        receipt.Fault=PhysicalWaterArchiveFault::None;
        receipt.Circuit=frame.Circuit;
        receipt.Session=Session;
        receipt.SourceFrame=frame.SourceFrame;
        receipt.RetainedFrames=Size;
        receipt.LatestClockSeconds=frame.ClockSeconds;
        receipt.LargestObservedCellChangeMm=maximumChange;
        LastFault=PhysicalWaterArchiveFault::None;
        return receipt;
    }
    PhysicalWaterPlaybackFrame DisplayAt(
        double activeClockSeconds) const noexcept
    {
        PhysicalWaterPlaybackFrame out;
        if(!HasPlayback() || !std::isfinite(activeClockSeconds))
            return out;
        // Exact recorded frame times can differ by one machine ulp from
        // user-supplied decimal query times. Admit a tiny numeric epsilon,
        // clamp to the witnessed range, and never extrapolate a real frame.
        constexpr double TimeRoundingEpsilon=1.0e-8;
        if(activeClockSeconds<EarliestClock()-TimeRoundingEpsilon ||
           activeClockSeconds>LatestClock()+TimeRoundingEpsilon)
            return out;
        const double query=std::clamp(activeClockSeconds,
            EarliestClock(),LatestClock());
        std::size_t first=0u;
        std::size_t last=Size-1u;
        while(first<last){
            const std::size_t mid=first+(last-first)/2u;
            if(FrameAt(mid).ClockSeconds<query)
                first=mid+1u;
            else
                last=mid;
        }
        const std::size_t top=first;
        const std::size_t bottom=top==0u?0u:top-1u;
        const auto& a=FrameAt(bottom);
        const auto& b=FrameAt(top);
        const double dt=b.ClockSeconds-a.ClockSeconds;
        double alpha=0.0;
        if(top!=bottom){
            if(!std::isfinite(dt) ||
               dt<0.001 ||
               dt>0.2)
                return {};
            alpha=(query-a.ClockSeconds)/dt;
            if(!std::isfinite(alpha) ||
               alpha<0.0 ||
               alpha>1.0)return {};
        }
        if(a.Session!=b.Session ||
           a.Revision!=b.Revision ||
           a.Circuit!=b.Circuit)
            return {};
        out.Circuit=a.Circuit;
        out.Session=a.Session;
        out.Revision=a.Revision;
        out.LowerFrame=a.SourceFrame;
        out.UpperFrame=b.SourceFrame;
        out.SourceClockSeconds=query;
        out.InterpolationFraction=alpha;
        out.MinimumFilmMm=
            std::numeric_limits<double>::infinity();
        double sum=0.0;
        for(std::size_t i=0u;i<ObservedWaterCellCount;++i){
            const double film=(1.0-alpha)*a.FilmMm[i]+
                alpha*b.FilmMm[i];
            if(!std::isfinite(film) ||
               film<0.0 ||
               film>ObservedWaterMaximumMm)
                return {};
            out.DisplayFilmMm[i]=film;
            sum+=film;
            out.MinimumFilmMm=std::min(
                out.MinimumFilmMm,film);
            out.MaximumFilmMm=std::max(
                out.MaximumFilmMm,film);
            if(film>0.05)++out.WetSections;
        }
        out.MeanFilmMm=sum/
            static_cast<double>(ObservedWaterCellCount);
        out.Valid=std::isfinite(out.MeanFilmMm);
        return out;
    }
private:
    const ObservedWaterResult& FrameAt(
        std::size_t chronologicalIndex) const noexcept
    {
        const std::size_t oldest=
            (Next+PhysicalWaterArchiveCapacity-Size)%
            PhysicalWaterArchiveCapacity;
        return Frames[(oldest+chronologicalIndex)%
            PhysicalWaterArchiveCapacity];
    }
    std::array<ObservedWaterResult,
        PhysicalWaterArchiveCapacity> Frames{};
    std::size_t Size=0u;
    std::size_t Next=0u;
    std::uint64_t Session=0u;
    std::uint64_t Revision=0u;
    PhysicalWaterArchiveFault LastFault=PhysicalWaterArchiveFault::None;
};
} // namespace latebrake

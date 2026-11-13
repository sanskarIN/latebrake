#pragma once

#include "Telemetry/TelemetryFilters.h"
#include "Telemetry/TelemetryTrack.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
enum class PaceSegmentQuality : std::uint8_t
{
    Missing,
    Incomplete,
    Discontinuous,
    Qualified
};

struct DistancePaceSample final
{
    double DistanceCm = 0.0;
    double ElapsedSeconds = 0.0;
    double SpeedCmS = 0.0;
    double LongitudinalAccelerationG = 0.0;
    double LateralAccelerationG = 0.0;
    double Throttle = 0.0;
    double Brake = 0.0;
    double Steering = 0.0;
    double CrossTrackCm = 0.0;
    double SlipAngleRadians = 0.0;
    int Sector = -1;
    int Segment = -1;
    bool OnRoad = false;
    bool Qualified = false;
};

struct DistancePaceSettings final
{
    double BinLengthCm = 200.0;
    double MaximumTrackLengthCm = 500000.0;
    double MaximumTimeGapSeconds = 0.25;
    double MaximumDistanceGapCm = 800.0;
    double MinimumTravelSpeedCmS = 60.0;
    double MinimumBinOccupancySeconds = 0.005;
    double MaximumCrossTrackErrorCm = 1200.0;
    int MinimumSamplesPerBin = 2;
    bool RejectOffRoad = true;
    bool RejectReverseMovement = true;
    bool ClampToTrackLength = true;
};

inline bool ValidDistancePaceSettings(
    DistancePaceSettings settings) noexcept
{
    return Finite(settings.BinLengthCm) &&
           settings.BinLengthCm >= 20.0 &&
           settings.BinLengthCm <= 2000.0 &&
           Finite(settings.MaximumTrackLengthCm) &&
           settings.MaximumTrackLengthCm >
               settings.BinLengthCm &&
           settings.MaximumTrackLengthCm <= 1000000.0 &&
           Finite(settings.MaximumTimeGapSeconds) &&
           settings.MaximumTimeGapSeconds > 0.0 &&
           settings.MaximumTimeGapSeconds <= 2.0 &&
           Finite(settings.MaximumDistanceGapCm) &&
           settings.MaximumDistanceGapCm >=
               settings.BinLengthCm &&
           Finite(settings.MinimumTravelSpeedCmS) &&
           settings.MinimumTravelSpeedCmS >= 0.0 &&
           Finite(settings.MinimumBinOccupancySeconds) &&
           settings.MinimumBinOccupancySeconds > 0.0 &&
           Finite(settings.MaximumCrossTrackErrorCm) &&
           settings.MaximumCrossTrackErrorCm >= 0.0 &&
           settings.MinimumSamplesPerBin >= 1 &&
           settings.MinimumSamplesPerBin <= 64;
}

struct DistancePaceBin final
{
    double StartCm = 0.0;
    double EndCm = 0.0;
    double CenterCm = 0.0;
    double OccupiedSeconds = 0.0;
    double WeightedSpeedCmS = 0.0;
    double MeanSpeedCmS = 0.0;
    double PeakSpeedCmS = 0.0;
    double MinimumSpeedCmS = 0.0;
    double MeanThrottle = 0.0;
    double MeanBrake = 0.0;
    double MeanAbsSteering = 0.0;
    double MeanLateralG = 0.0;
    double MeanLongitudinalG = 0.0;
    double MeanCrossTrackCm = 0.0;
    double MeanSlipAngleRadians = 0.0;
    double OffRoadSeconds = 0.0;
    double BrakingSeconds = 0.0;
    double AcceleratingSeconds = 0.0;
    double CoastingSeconds = 0.0;
    int Samples = 0;
    int Discontinuities = 0;
    int Sector = -1;
    PaceSegmentQuality Quality = PaceSegmentQuality::Missing;
    bool Valid = false;
};

struct PaceProfileSummary final
{
    double TrackLengthCm = 0.0;
    double QualifiedDistanceCm = 0.0;
    double MeanWeightedSpeedCmS = 0.0;
    double MinimumObservedSpeedCmS = 0.0;
    double MaximumObservedSpeedCmS = 0.0;
    double MeanBrakeFraction = 0.0;
    double MeanThrottleFraction = 0.0;
    double MeanCrossTrackDeviationCm = 0.0;
    double MeanSlipAngleRadians = 0.0;
    double MinimumBinCoverageFraction = 0.0;
    int Bins = 0;
    int QualifiedBins = 0;
    int MissingBins = 0;
    int RejectedSamples = 0;
    int DiscontinuousSamples = 0;
    int RecordedSamples = 0;
    bool Complete = false;
    bool Qualified = false;
};

template<std::size_t MaximumBins>
class DistancePaceProfiler final
{
    static_assert(MaximumBins >= 8 && MaximumBins <= 8192);
public:
    void Reset() noexcept
    {
        Bins.fill({});
        Settings = {};
        LengthCm = 0.0;
        BinCount = 0;
        HasPrevious = false;
        Previous = {};
        Rejected = 0;
        Discontinuous = 0;
        Accepted = 0;
        Session = 0;
        Actor = 0;
        LapIndex = -1;
        Ready = false;
    }

    bool Configure(double trackLengthCm,
                   DistancePaceSettings settings = {}) noexcept
    {
        if (!ValidDistancePaceSettings(settings) ||
            !Finite(trackLengthCm) ||
            trackLengthCm <= settings.BinLengthCm ||
            trackLengthCm > settings.MaximumTrackLengthCm)
        {
            return false;
        }
        const double requiredBins =
            std::ceil(trackLengthCm / settings.BinLengthCm);
        if (!Finite(requiredBins) ||
            requiredBins > static_cast<double>(MaximumBins))
        {
            return false;
        }
        Reset();
        Settings = settings;
        LengthCm = trackLengthCm;
        BinCount = static_cast<int>(requiredBins);
        for (int i = 0; i < BinCount; ++i)
        {
            DistancePaceBin& b =
                Bins[static_cast<std::size_t>(i)];
            b.StartCm = i * settings.BinLengthCm;
            b.EndCm = std::min(
                b.StartCm + settings.BinLengthCm,
                LengthCm);
            b.CenterCm = 0.5 * (b.StartCm + b.EndCm);
            b.MinimumSpeedCmS =
                std::numeric_limits<double>::infinity();
        }
        Ready = true;
        return true;
    }

    bool StartLap(
        std::uint64_t sessionToken,
        std::uint64_t actorToken, int lap) noexcept
    {
        if (!Ready || sessionToken == 0 ||
            actorToken == 0 || lap < 0 ||
            lap > MaximumTelemetryLaps)
        {
            return false;
        }
        for (int i = 0; i < BinCount; ++i)
        {
            const double begin =
                Bins[static_cast<std::size_t>(i)].StartCm;
            const double end =
                Bins[static_cast<std::size_t>(i)].EndCm;
            Bins[static_cast<std::size_t>(i)] = {};
            DistancePaceBin& b =
                Bins[static_cast<std::size_t>(i)];
            b.StartCm = begin;
            b.EndCm = end;
            b.CenterCm = 0.5 * (begin + end);
            b.MinimumSpeedCmS =
                std::numeric_limits<double>::infinity();
        }
        Session = sessionToken;
        Actor = actorToken;
        LapIndex = lap;
        HasPrevious = false;
        Rejected = 0;
        Discontinuous = 0;
        Accepted = 0;
        return true;
    }

    bool Observe(
        DistancePaceSample sample,
        std::uint64_t sessionToken,
        std::uint64_t actorToken, int lap) noexcept
    {
        if (!Ready || Session != sessionToken ||
            Actor != actorToken || LapIndex != lap ||
            !ValidSample(sample))
        {
            ++Rejected;
            return false;
        }
        if (Settings.RejectOffRoad && !sample.OnRoad)
        {
            ++Rejected;
            HasPrevious = false;
            return false;
        }
        if (!HasPrevious)
        {
            Previous = sample;
            HasPrevious = true;
            return true;
        }
        const double dt =
            sample.ElapsedSeconds - Previous.ElapsedSeconds;
        const double distance =
            sample.DistanceCm - Previous.DistanceCm;
        if (dt <= 0.0 ||
            dt > Settings.MaximumTimeGapSeconds ||
            !Finite(distance) ||
            std::abs(distance) >
                Settings.MaximumDistanceGapCm ||
            (distance < 0.0 &&
             Settings.RejectReverseMovement))
        {
            ++Discontinuous;
            Previous = sample;
            return false;
        }
        if (distance == 0.0 ||
            sample.SpeedCmS <
                Settings.MinimumTravelSpeedCmS)
        {
            Previous = sample;
            return true;
        }
        const double begin =
            std::min(Previous.DistanceCm, sample.DistanceCm);
        const double end =
            std::max(Previous.DistanceCm, sample.DistanceCm);
        const double traveled =
            std::max(Tiny, end - begin);
        bool touched = false;
        for (int i = 0; i < BinCount; ++i)
        {
            DistancePaceBin& bin =
                Bins[static_cast<std::size_t>(i)];
            const double overlap = std::max(
                0.0, std::min(end, bin.EndCm) -
                     std::max(begin, bin.StartCm));
            if (overlap <= 0.0)
            {
                continue;
            }
            const double fraction = overlap / traveled;
            const double weight = dt * fraction;
            const double crossingPosition =
                0.5 * (std::max(begin, bin.StartCm) +
                       std::min(end, bin.EndCm));
            const double interpolation = Clamp01(
                (crossingPosition - begin) / traveled);
            const double speed = Lerp(
                Previous.SpeedCmS, sample.SpeedCmS,
                interpolation);
            bin.OccupiedSeconds += weight;
            bin.WeightedSpeedCmS += speed * weight;
            bin.MeanThrottle += Lerp(
                Previous.Throttle, sample.Throttle,
                interpolation) * weight;
            bin.MeanBrake += Lerp(
                Previous.Brake, sample.Brake,
                interpolation) * weight;
            bin.MeanAbsSteering += Lerp(
                std::abs(Previous.Steering),
                std::abs(sample.Steering),
                interpolation) * weight;
            bin.MeanLongitudinalG += Lerp(
                Previous.LongitudinalAccelerationG,
                sample.LongitudinalAccelerationG,
                interpolation) * weight;
            bin.MeanLateralG += Lerp(
                std::abs(Previous.LateralAccelerationG),
                std::abs(sample.LateralAccelerationG),
                interpolation) * weight;
            bin.MeanCrossTrackCm += Lerp(
                std::abs(Previous.CrossTrackCm),
                std::abs(sample.CrossTrackCm),
                interpolation) * weight;
            bin.MeanSlipAngleRadians += Lerp(
                std::abs(Previous.SlipAngleRadians),
                std::abs(sample.SlipAngleRadians),
                interpolation) * weight;
            bin.PeakSpeedCmS = std::max(
                bin.PeakSpeedCmS, speed);
            bin.MinimumSpeedCmS = std::min(
                bin.MinimumSpeedCmS, speed);
            bin.OffRoadSeconds += !sample.OnRoad ?
                weight : 0.0;
            bin.BrakingSeconds += sample.Brake >= 0.18 ?
                weight : 0.0;
            bin.AcceleratingSeconds +=
                sample.Throttle >= 0.18 ? weight : 0.0;
            bin.CoastingSeconds +=
                sample.Throttle < 0.18 &&
                sample.Brake < 0.18 ? weight : 0.0;
            bin.Sector = sample.Sector;
            ++bin.Samples;
            bin.Quality = bin.Samples >=
                Settings.MinimumSamplesPerBin ?
                PaceSegmentQuality::Qualified :
                PaceSegmentQuality::Incomplete;
            bin.Valid = true;
            touched = true;
        }
        Previous = sample;
        ++Accepted;
        return touched;
    }

    DistancePaceBin Bin(int index) const noexcept
    {
        if (!Ready || index < 0 || index >= BinCount)
        {
            return {};
        }
        DistancePaceBin result =
            Bins[static_cast<std::size_t>(index)];
        if (result.OccupiedSeconds > Tiny)
        {
            const double inv = 1.0 /
                result.OccupiedSeconds;
            result.MeanSpeedCmS =
                result.WeightedSpeedCmS * inv;
            result.MeanThrottle *= inv;
            result.MeanBrake *= inv;
            result.MeanAbsSteering *= inv;
            result.MeanLongitudinalG *= inv;
            result.MeanLateralG *= inv;
            result.MeanCrossTrackCm *= inv;
            result.MeanSlipAngleRadians *= inv;
        }
        if (!Finite(result.MinimumSpeedCmS))
        {
            result.MinimumSpeedCmS = 0.0;
        }
        return result;
    }

    PaceProfileSummary Summary() const noexcept
    {
        PaceProfileSummary out;
        if (!Ready)
        {
            return out;
        }
        out.TrackLengthCm = LengthCm;
        out.Bins = BinCount;
        out.RecordedSamples = Accepted;
        out.RejectedSamples = Rejected;
        out.DiscontinuousSamples = Discontinuous;
        double weightedSpeed = 0.0;
        double weightedThrottle = 0.0;
        double weightedBrake = 0.0;
        double weightedLine = 0.0;
        double weightedSlip = 0.0;
        double totalTime = 0.0;
        out.MinimumObservedSpeedCmS =
            std::numeric_limits<double>::infinity();
        for (int i = 0; i < BinCount; ++i)
        {
            const DistancePaceBin bin = Bin(i);
            if (bin.Quality ==
                PaceSegmentQuality::Qualified)
            {
                ++out.QualifiedBins;
            }
            else
            {
                ++out.MissingBins;
            }
            if (!bin.Valid)
            {
                continue;
            }
            const double dt = bin.OccupiedSeconds;
            weightedSpeed += bin.MeanSpeedCmS * dt;
            weightedThrottle += bin.MeanThrottle * dt;
            weightedBrake += bin.MeanBrake * dt;
            weightedLine += bin.MeanCrossTrackCm * dt;
            weightedSlip += bin.MeanSlipAngleRadians * dt;
            totalTime += dt;
            out.QualifiedDistanceCm +=
                bin.EndCm - bin.StartCm;
            out.MinimumObservedSpeedCmS = std::min(
                out.MinimumObservedSpeedCmS,
                bin.MinimumSpeedCmS);
            out.MaximumObservedSpeedCmS = std::max(
                out.MaximumObservedSpeedCmS,
                bin.PeakSpeedCmS);
        }
        out.MeanWeightedSpeedCmS = SafeRatio(
            weightedSpeed, totalTime);
        out.MeanThrottleFraction = SafeRatio(
            weightedThrottle, totalTime);
        out.MeanBrakeFraction = SafeRatio(
            weightedBrake, totalTime);
        out.MeanCrossTrackDeviationCm = SafeRatio(
            weightedLine, totalTime);
        out.MeanSlipAngleRadians = SafeRatio(
            weightedSlip, totalTime);
        if (!Finite(out.MinimumObservedSpeedCmS))
        {
            out.MinimumObservedSpeedCmS = 0.0;
        }
        out.MinimumBinCoverageFraction = SafeRatio(
            static_cast<double>(out.QualifiedBins),
            static_cast<double>(BinCount));
        out.Complete = out.QualifiedBins == BinCount;
        out.Qualified = out.QualifiedBins > 0 &&
            out.DiscontinuousSamples == 0;
        return out;
    }

    int BinCountValue() const noexcept
    {
        return BinCount;
    }

    double TrackLengthCm() const noexcept
    {
        return LengthCm;
    }

    bool IsReady() const noexcept { return Ready; }

private:
    bool ValidSample(
        const DistancePaceSample& s) const noexcept
    {
        return s.Qualified &&
            Finite(s.DistanceCm) &&
            Finite(s.ElapsedSeconds) &&
            Finite(s.SpeedCmS) &&
            Finite(s.LongitudinalAccelerationG) &&
            Finite(s.LateralAccelerationG) &&
            Finite(s.Throttle) &&
            Finite(s.Brake) &&
            Finite(s.Steering) &&
            Finite(s.CrossTrackCm) &&
            Finite(s.SlipAngleRadians) &&
            s.DistanceCm >= 0.0 &&
            s.DistanceCm <= LengthCm &&
            s.ElapsedSeconds >= 0.0 &&
            s.SpeedCmS >= 0.0 &&
            s.SpeedCmS <= MaximumPlanarSpeedCmS &&
            s.Throttle >= 0.0 && s.Throttle <= 1.0 &&
            s.Brake >= 0.0 && s.Brake <= 1.0 &&
            s.Steering >= -1.0 && s.Steering <= 1.0 &&
            std::abs(s.CrossTrackCm) <=
                Settings.MaximumCrossTrackErrorCm &&
            s.Sector >= -1 &&
            s.Sector < MaximumTelemetrySectors;
    }

    std::array<DistancePaceBin, MaximumBins> Bins{};
    DistancePaceSettings Settings{};
    DistancePaceSample Previous{};
    double LengthCm = 0.0;
    int BinCount = 0;
    int Accepted = 0;
    int Rejected = 0;
    int Discontinuous = 0;
    std::uint64_t Session = 0;
    std::uint64_t Actor = 0;
    int LapIndex = -1;
    bool HasPrevious = false;
    bool Ready = false;
};
} // namespace latebrake::engineering

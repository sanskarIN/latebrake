#pragma once

#include "Telemetry/TelemetryDistancePace.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
struct RaceLineSample final
{
    double TrackDistanceCm = 0.0;
    double LateralOffsetCm = 0.0;
    double RoadHalfWidthCm = 0.0;
    double VehicleHalfWidthCm = 100.0;
    double SpeedCmS = 0.0;
    double YawErrorRadians = 0.0;
    double Steering = 0.0;
    double SlipAngleRadians = 0.0;
    double DeltaSeconds = 0.0;
    int Sector = -1;
    bool OnOriginalPavement = false;
    bool CollisionFree = false;
    bool EngineQualified = false;
};

struct RaceLineSettings final
{
    double SegmentLengthCm = 250.0;
    double MinimumRoadEdgeMarginCm = 80.0;
    double MaximumYawErrorRadians = 1.8;
    double MaximumSlipAngleRadians = 1.2;
    double MinimumSpeedCmS = 80.0;
    double MaximumSpeedCmS = 16000.0;
    double MaximumLateralOffsetCm = 2500.0;
    int MinimumConfidenceSamples = 3;
    int MinimumSamplesForStableLine = 8;
    bool RejectKnownCollisions = true;
    bool RejectOffPavement = true;
};

inline bool ValidRaceLineSettings(RaceLineSettings settings) noexcept
{
    return Finite(settings.SegmentLengthCm) &&
        settings.SegmentLengthCm >= 20.0 &&
        settings.SegmentLengthCm <= 5000.0 &&
        Finite(settings.MinimumRoadEdgeMarginCm) &&
        settings.MinimumRoadEdgeMarginCm >= 0.0 &&
        Finite(settings.MaximumYawErrorRadians) &&
        settings.MaximumYawErrorRadians > 0.0 &&
        settings.MaximumYawErrorRadians <= Pi &&
        Finite(settings.MaximumSlipAngleRadians) &&
        settings.MaximumSlipAngleRadians > 0.0 &&
        settings.MaximumSlipAngleRadians <= Pi &&
        Finite(settings.MinimumSpeedCmS) &&
        settings.MinimumSpeedCmS >= 0.0 &&
        Finite(settings.MaximumSpeedCmS) &&
        settings.MaximumSpeedCmS > settings.MinimumSpeedCmS &&
        Finite(settings.MaximumLateralOffsetCm) &&
        settings.MaximumLateralOffsetCm > 0.0 &&
        settings.MinimumConfidenceSamples >= 1 &&
        settings.MinimumConfidenceSamples <= 64 &&
        settings.MinimumSamplesForStableLine >=
            settings.MinimumConfidenceSamples;
}

struct RaceLineSegment final
{
    double StartCm = 0.0;
    double EndCm = 0.0;
    double BestFastOffsetCm = 0.0;
    double MeanOffsetCm = 0.0;
    double MeanAbsoluteOffsetCm = 0.0;
    double OffsetStandardDeviationCm = 0.0;
    double MeanHeadingErrorRadians = 0.0;
    double MeanSlipAngleRadians = 0.0;
    double MeanRoadMarginCm = 0.0;
    double WorstRoadMarginCm = 0.0;
    double BestFastSpeedCmS = 0.0;
    double MeanSpeedCmS = 0.0;
    double MinimumSpeedCmS = 0.0;
    double MaximumSpeedCmS = 0.0;
    double MeanSteeringMagnitude = 0.0;
    double TotalDwellSeconds = 0.0;
    double StableFraction = 0.0;
    double RoadRiskFraction = 0.0;
    double OffsetVarianceCm2 = 0.0;
    int Observations = 0;
    int Rejections = 0;
    int StableObservations = 0;
    int EdgeRiskObservations = 0;
    int FastSamples = 0;
    int Sector = -1;
    bool HasBestFastSample = false;
    bool Valid = false;
};

struct RaceLineReview final
{
    double TrackLengthCm = 0.0;
    double CoveredTrackCm = 0.0;
    double MeanAbsoluteLateralCm = 0.0;
    double MeanRoadMarginCm = 0.0;
    double MeanLineVariationCm = 0.0;
    double MeanHeadingErrorRadians = 0.0;
    double MeanSlipAngleRadians = 0.0;
    double RiskyEdgeFraction = 0.0;
    double StableFraction = 0.0;
    double CoverageFraction = 0.0;
    int SegmentCount = 0;
    int QualifiedSegments = 0;
    int StableSegments = 0;
    int InsufficientSegments = 0;
    int RejectedSamples = 0;
    int AcceptedSamples = 0;
    int WorstDeviationSegment = -1;
    int HighestEdgeRiskSegment = -1;
    bool Qualified = false;
    bool AuthoritativeCollisionEvidence = false;
};

template<std::size_t MaximumSegments>
class RaceLineProfiler final
{
    static_assert(MaximumSegments >= 8 && MaximumSegments <= 4096);
public:
    void Reset() noexcept
    {
        Segments.fill({});
        Settings = {};
        TrackLengthCm = 0.0;
        Count = 0;
        Accepted = 0;
        Rejected = 0;
        Ready = false;
    }

    bool Configure(double trackLengthCm,
                   RaceLineSettings settings = {}) noexcept
    {
        if (!ValidRaceLineSettings(settings) ||
            !Finite(trackLengthCm) ||
            trackLengthCm <= settings.SegmentLengthCm ||
            trackLengthCm > 1000000.0)
        {
            return false;
        }
        const int n = static_cast<int>(
            std::ceil(trackLengthCm / settings.SegmentLengthCm));
        if (n <= 0 ||
            n > static_cast<int>(MaximumSegments))
        {
            return false;
        }
        Reset();
        Settings = settings;
        TrackLengthCm = trackLengthCm;
        Count = n;
        for (int i = 0; i < Count; ++i)
        {
            RaceLineSegment& segment =
                Segments[static_cast<std::size_t>(i)];
            segment.StartCm =
                i * settings.SegmentLengthCm;
            segment.EndCm = std::min(
                trackLengthCm,
                segment.StartCm + settings.SegmentLengthCm);
            segment.WorstRoadMarginCm =
                std::numeric_limits<double>::infinity();
            segment.MinimumSpeedCmS =
                std::numeric_limits<double>::infinity();
        }
        Ready = true;
        return true;
    }

    bool Observe(RaceLineSample sample) noexcept
    {
        if (!Ready || !ValidSample(sample))
        {
            ++Rejected;
            return false;
        }
        const int index = std::clamp(
            static_cast<int>(
                sample.TrackDistanceCm /
                Settings.SegmentLengthCm),
            0, Count - 1);
        RaceLineSegment& segment =
            Segments[static_cast<std::size_t>(index)];
        if (Settings.RejectKnownCollisions &&
            !sample.CollisionFree)
        {
            ++segment.Rejections;
            ++Rejected;
            return false;
        }
        if (Settings.RejectOffPavement &&
            !sample.OnOriginalPavement)
        {
            ++segment.Rejections;
            ++Rejected;
            return false;
        }
        const double margin =
            sample.RoadHalfWidthCm -
            std::abs(sample.LateralOffsetCm) -
            sample.VehicleHalfWidthCm;
        const bool roadRisk =
            margin < Settings.MinimumRoadEdgeMarginCm;
        const bool stable =
            std::abs(sample.YawErrorRadians) < 0.25 &&
            std::abs(sample.SlipAngleRadians) < 0.15 &&
            !roadRisk;
        const double speed = sample.SpeedCmS;
        const int n = ++segment.Observations;
        const double weight = 1.0 / static_cast<double>(n);
        const double previousMean = segment.MeanOffsetCm;
        segment.MeanOffsetCm +=
            (sample.LateralOffsetCm -
             segment.MeanOffsetCm) * weight;
        const double deltaAfter =
            sample.LateralOffsetCm -
            segment.MeanOffsetCm;
        segment.OffsetVarianceCm2 +=
            (sample.LateralOffsetCm -
             previousMean) * deltaAfter;
        segment.MeanAbsoluteOffsetCm +=
            (std::abs(sample.LateralOffsetCm) -
             segment.MeanAbsoluteOffsetCm) * weight;
        segment.MeanHeadingErrorRadians +=
            (std::abs(sample.YawErrorRadians) -
             segment.MeanHeadingErrorRadians) * weight;
        segment.MeanSlipAngleRadians +=
            (std::abs(sample.SlipAngleRadians) -
             segment.MeanSlipAngleRadians) * weight;
        segment.MeanRoadMarginCm +=
            (margin - segment.MeanRoadMarginCm) * weight;
        segment.MeanSpeedCmS +=
            (speed - segment.MeanSpeedCmS) * weight;
        segment.MeanSteeringMagnitude +=
            (std::abs(sample.Steering) -
             segment.MeanSteeringMagnitude) * weight;
        segment.WorstRoadMarginCm =
            std::min(segment.WorstRoadMarginCm, margin);
        segment.MinimumSpeedCmS =
            std::min(segment.MinimumSpeedCmS, speed);
        segment.MaximumSpeedCmS =
            std::max(segment.MaximumSpeedCmS, speed);
        segment.TotalDwellSeconds += sample.DeltaSeconds;
        segment.Sector = sample.Sector;
        segment.StableObservations += stable ? 1 : 0;
        segment.EdgeRiskObservations += roadRisk ? 1 : 0;
        segment.StableFraction = SafeRatio(
            segment.StableObservations, n);
        segment.RoadRiskFraction = SafeRatio(
            segment.EdgeRiskObservations, n);
        if (stable &&
            (!segment.HasBestFastSample ||
             speed > segment.BestFastSpeedCmS))
        {
            segment.BestFastSpeedCmS = speed;
            segment.BestFastOffsetCm =
                sample.LateralOffsetCm;
            segment.HasBestFastSample = true;
            ++segment.FastSamples;
        }
        segment.OffsetStandardDeviationCm =
            n > 1 ? std::sqrt(
                std::max(0.0, segment.OffsetVarianceCm2 /
                         static_cast<double>(n - 1))) : 0.0;
        segment.Valid = n >= Settings.MinimumConfidenceSamples;
        ++Accepted;
        return true;
    }

    RaceLineSegment Segment(int index) const noexcept
    {
        return index >= 0 && index < Count ?
            Segments[static_cast<std::size_t>(index)] :
            RaceLineSegment{};
    }

    RaceLineReview Summarize() const noexcept
    {
        RaceLineReview result;
        if (!Ready)
        {
            return result;
        }
        result.TrackLengthCm = TrackLengthCm;
        result.SegmentCount = Count;
        result.RejectedSamples = Rejected;
        result.AcceptedSamples = Accepted;
        double totalWeight = 0.0;
        double deviationSum = 0.0;
        double marginSum = 0.0;
        double variationSum = 0.0;
        double headingSum = 0.0;
        double slipSum = 0.0;
        double riskSum = 0.0;
        double stableSum = 0.0;
        double maximumDeviation = -1.0;
        double maximumRisk = -1.0;
        for (int i = 0; i < Count; ++i)
        {
            const RaceLineSegment& s =
                Segments[static_cast<std::size_t>(i)];
            if (!s.Valid)
            {
                ++result.InsufficientSegments;
                continue;
            }
            ++result.QualifiedSegments;
            if (s.Observations >=
                    Settings.MinimumSamplesForStableLine &&
                s.StableFraction >= 0.8)
            {
                ++result.StableSegments;
            }
            const double width =
                s.EndCm - s.StartCm;
            result.CoveredTrackCm += width;
            totalWeight += width;
            deviationSum +=
                s.MeanAbsoluteOffsetCm * width;
            marginSum += s.MeanRoadMarginCm * width;
            variationSum +=
                s.OffsetStandardDeviationCm * width;
            headingSum +=
                s.MeanHeadingErrorRadians * width;
            slipSum +=
                s.MeanSlipAngleRadians * width;
            riskSum += s.RoadRiskFraction * width;
            stableSum += s.StableFraction * width;
            if (s.MeanAbsoluteOffsetCm > maximumDeviation)
            {
                maximumDeviation =
                    s.MeanAbsoluteOffsetCm;
                result.WorstDeviationSegment = i;
            }
            if (s.RoadRiskFraction > maximumRisk)
            {
                maximumRisk = s.RoadRiskFraction;
                result.HighestEdgeRiskSegment = i;
            }
        }
        result.MeanAbsoluteLateralCm =
            SafeRatio(deviationSum, totalWeight);
        result.MeanRoadMarginCm =
            SafeRatio(marginSum, totalWeight);
        result.MeanLineVariationCm =
            SafeRatio(variationSum, totalWeight);
        result.MeanHeadingErrorRadians =
            SafeRatio(headingSum, totalWeight);
        result.MeanSlipAngleRadians =
            SafeRatio(slipSum, totalWeight);
        result.RiskyEdgeFraction =
            SafeRatio(riskSum, totalWeight);
        result.StableFraction =
            SafeRatio(stableSum, totalWeight);
        result.CoverageFraction =
            SafeRatio(result.CoveredTrackCm,
                      result.TrackLengthCm);
        result.Qualified = result.QualifiedSegments > 0;
        result.AuthoritativeCollisionEvidence = false;
        return result;
    }

    int SegmentCount() const noexcept { return Count; }
    bool ReadyToObserve() const noexcept { return Ready; }

private:
    bool ValidSample(const RaceLineSample& s) const noexcept
    {
        return s.EngineQualified &&
            Finite(s.TrackDistanceCm) &&
            Finite(s.LateralOffsetCm) &&
            Finite(s.RoadHalfWidthCm) &&
            Finite(s.VehicleHalfWidthCm) &&
            Finite(s.SpeedCmS) &&
            Finite(s.YawErrorRadians) &&
            Finite(s.Steering) &&
            Finite(s.SlipAngleRadians) &&
            Finite(s.DeltaSeconds) &&
            s.TrackDistanceCm >= 0.0 &&
            s.TrackDistanceCm <= TrackLengthCm &&
            std::abs(s.LateralOffsetCm) <=
                Settings.MaximumLateralOffsetCm &&
            s.RoadHalfWidthCm > 0.0 &&
            s.VehicleHalfWidthCm > 0.0 &&
            s.VehicleHalfWidthCm < s.RoadHalfWidthCm &&
            s.SpeedCmS >= Settings.MinimumSpeedCmS &&
            s.SpeedCmS <= Settings.MaximumSpeedCmS &&
            std::abs(s.YawErrorRadians) <=
                Settings.MaximumYawErrorRadians &&
            std::abs(s.SlipAngleRadians) <=
                Settings.MaximumSlipAngleRadians &&
            s.Steering >= -1.0 && s.Steering <= 1.0 &&
            s.DeltaSeconds > 0.0 &&
            s.DeltaSeconds <= MaximumFrameDeltaSeconds &&
            s.Sector >= -1 &&
            s.Sector < MaximumTelemetrySectors;
    }

    std::array<RaceLineSegment, MaximumSegments> Segments{};
    RaceLineSettings Settings{};
    double TrackLengthCm = 0.0;
    int Count = 0;
    int Accepted = 0;
    int Rejected = 0;
    bool Ready = false;
};
} // namespace latebrake::engineering

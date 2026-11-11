#pragma once

#include "Telemetry/TelemetryReplayBuffer.h"
#include "Telemetry/TelemetrySectorAnalysis.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
inline constexpr int MaximumDeltaReferencePoints = 2048;

enum class DeltaRejectReason : std::uint8_t
{
    None,
    NoReference,
    InvalidReference,
    InvalidCurrentSample,
    WrongSession,
    WrongTrack,
    WrongLap,
    OutOfOrder,
    BackwardsDistance,
    NonMonotonicTime,
    SparseSamples,
    ReferenceIncomplete,
    OutsideBounds,
    IncompatibleFormat
};

struct DeltaSettings final
{
    double MaximumInterpolationGapCm = 400.0;
    double MaximumLapTimeSeconds = 7200.0;
    double MaximumLapDistanceCm = 2000000.0;
    double MinimumLapDistanceCm = 1000.0;
    double MaximumTimeGapSeconds = 1.0;
    double MaximumLateralDifferenceCm = 1300.0;
    double MaximumSpeedDifferenceCmS = 14000.0;
    double MaximumReferenceErrorCm = 200.0;
    double MinimumDistanceStepCm = 1.0;
    int MaximumReferencePoints = MaximumDeltaReferencePoints;
    bool RequireEngineReference = true;
    bool RejectMixedSectors = true;
    bool RequireSameTrackLength = true;
};

inline bool ValidDeltaSettings(
    const DeltaSettings& s) noexcept
{
    return Finite(s.MaximumInterpolationGapCm) &&
           s.MaximumInterpolationGapCm > 0.0 &&
           Finite(s.MaximumLapTimeSeconds) &&
           s.MaximumLapTimeSeconds > 0.0 &&
           Finite(s.MaximumLapDistanceCm) &&
           s.MaximumLapDistanceCm > s.MinimumLapDistanceCm &&
           Finite(s.MinimumLapDistanceCm) &&
           s.MinimumLapDistanceCm > 0.0 &&
           Finite(s.MaximumTimeGapSeconds) &&
           s.MaximumTimeGapSeconds > 0.0 &&
           Finite(s.MaximumLateralDifferenceCm) &&
           s.MaximumLateralDifferenceCm > 0.0 &&
           Finite(s.MaximumSpeedDifferenceCmS) &&
           s.MaximumSpeedDifferenceCmS > 0.0 &&
           Finite(s.MaximumReferenceErrorCm) &&
           s.MaximumReferenceErrorCm >= 0.0 &&
           Finite(s.MinimumDistanceStepCm) &&
           s.MinimumDistanceStepCm > 0.0 &&
           s.MaximumReferencePoints > 1 &&
           s.MaximumReferencePoints <= MaximumDeltaReferencePoints;
}

struct DeltaReferencePoint final
{
    double DistanceCm = 0.0;
    double ElapsedSeconds = 0.0;
    double SpeedCmS = 0.0;
    double Throttle = 0.0;
    double Brake = 0.0;
    double Steering = 0.0;
    double LateralErrorCm = 0.0;
    double SlipRadians = 0.0;
    double GripUtilization = 0.0;
    double EngineTemperatureC = 0.0;
    double TireWear = 0.0;
    int SectorIndex = -1;
    std::uint64_t FrameOrdinal = 0;
    bool NativeObserved = false;
    bool Valid = false;
};

struct DeltaReferenceInfo final
{
    std::uint64_t SessionToken = 0;
    std::uint64_t ActorToken = 0;
    std::uint64_t TrackFingerprint = 0;
    double TotalDistanceCm = 0.0;
    double FinishedLapSeconds = 0.0;
    double StartingRaceSeconds = 0.0;
    double FinishedRaceSeconds = 0.0;
    double MaximumSampleGapCm = 0.0;
    int CompletedLapIndex = -1;
    int SectorCount = 0;
    int PointCount = 0;
    bool OfficialLapQualified = false;
    bool FullySampled = false;
    bool Valid = false;
};

struct DeltaSample final
{
    DeltaRejectReason Reason = DeltaRejectReason::NoReference;
    double DistanceCm = 0.0;
    double CurrentLapSeconds = 0.0;
    double ReferenceLapSeconds = 0.0;
    double DeltaSeconds = 0.0;
    double DeltaSlopeSecondsPerMeter = 0.0;
    double ReferenceSpeedCmS = 0.0;
    double CurrentSpeedCmS = 0.0;
    double SpeedDifferenceCmS = 0.0;
    double ReferenceThrottle = 0.0;
    double CurrentThrottle = 0.0;
    double ReferenceBrake = 0.0;
    double CurrentBrake = 0.0;
    double ReferenceSteering = 0.0;
    double CurrentSteering = 0.0;
    double ReferenceLateralErrorCm = 0.0;
    double CurrentLateralErrorCm = 0.0;
    double ReferenceGripUtilization = 0.0;
    double ReferenceSlipRadians = 0.0;
    int SectorIndex = -1;
    bool GainingTime = false;
    bool LosingTime = false;
    bool BrakingLater = false;
    bool AcceleratingEarlier = false;
    bool DifferentRacingLine = false;
    bool Qualified = false;
};

inline bool ValidDeltaPoint(
    DeltaReferencePoint point,
    const DeltaSettings& settings) noexcept
{
    return point.Valid &&
           Finite(point.DistanceCm) &&
           Finite(point.ElapsedSeconds) &&
           Finite(point.SpeedCmS) &&
           Finite(point.Throttle) &&
           Finite(point.Brake) &&
           Finite(point.Steering) &&
           Finite(point.LateralErrorCm) &&
           Finite(point.SlipRadians) &&
           Finite(point.GripUtilization) &&
           Finite(point.EngineTemperatureC) &&
           Finite(point.TireWear) &&
           point.DistanceCm >= 0.0 &&
           point.DistanceCm <= settings.MaximumLapDistanceCm &&
           point.ElapsedSeconds >= 0.0 &&
           point.ElapsedSeconds <= settings.MaximumLapTimeSeconds &&
           point.SpeedCmS >= 0.0 &&
           point.SpeedCmS <= MaximumPlanarSpeedCmS &&
           point.Throttle >= 0.0 && point.Throttle <= 1.0 &&
           point.Brake >= 0.0 && point.Brake <= 1.0 &&
           point.Steering >= -1.0 && point.Steering <= 1.0 &&
           std::abs(point.LateralErrorCm) <=
               settings.MaximumLateralDifferenceCm * 4.0 &&
           point.SectorIndex >= 0 &&
           point.SectorIndex < MaximumTelemetrySectors &&
           (!settings.RequireEngineReference || point.NativeObserved);
}

inline DeltaReferencePoint InterpolateDeltaReference(
    const DeltaReferencePoint& a,
    const DeltaReferencePoint& b,
    double fraction) noexcept
{
    const double t = Clamp01(fraction);
    DeltaReferencePoint out = a;
    out.DistanceCm = Lerp(a.DistanceCm, b.DistanceCm, t);
    out.ElapsedSeconds = Lerp(a.ElapsedSeconds, b.ElapsedSeconds, t);
    out.SpeedCmS = Lerp(a.SpeedCmS, b.SpeedCmS, t);
    out.Throttle = Lerp(a.Throttle, b.Throttle, t);
    out.Brake = Lerp(a.Brake, b.Brake, t);
    out.Steering = Lerp(a.Steering, b.Steering, t);
    out.LateralErrorCm = Lerp(
        a.LateralErrorCm, b.LateralErrorCm, t);
    out.SlipRadians = Lerp(a.SlipRadians, b.SlipRadians, t);
    out.GripUtilization = Lerp(
        a.GripUtilization, b.GripUtilization, t);
    out.EngineTemperatureC = Lerp(
        a.EngineTemperatureC, b.EngineTemperatureC, t);
    out.TireWear = Lerp(a.TireWear, b.TireWear, t);
    out.SectorIndex = t < 0.5 ? a.SectorIndex : b.SectorIndex;
    out.NativeObserved = false;
    out.Valid = a.Valid && b.Valid;
    return out;
}

class LapDeltaReference final
{
public:
    bool Configure(DeltaSettings settings) noexcept
    {
        if (!ValidDeltaSettings(settings))
        {
            return false;
        }
        Settings = settings;
        Reset();
        return true;
    }

    void Reset() noexcept
    {
        Points.fill({});
        Info = {};
        Count = 0;
        MaximumGapCm = 0.0;
        RejectedPoints = 0;
        HasStart = false;
        Complete = false;
    }

    bool Begin(
        std::uint64_t session, std::uint64_t actor,
        std::uint64_t trackFingerprint,
        int lapIndex, double trackLengthCm,
        double startRaceSeconds) noexcept
    {
        if (session == 0 || actor == 0 ||
            trackFingerprint == 0 || lapIndex < 0 ||
            lapIndex >= MaximumTelemetryLaps ||
            !Finite(trackLengthCm) ||
            trackLengthCm < Settings.MinimumLapDistanceCm ||
            trackLengthCm > Settings.MaximumLapDistanceCm ||
            !Finite(startRaceSeconds) || startRaceSeconds < 0.0)
        {
            return false;
        }
        Reset();
        Info.SessionToken = session;
        Info.ActorToken = actor;
        Info.TrackFingerprint = trackFingerprint;
        Info.CompletedLapIndex = lapIndex;
        Info.TotalDistanceCm = trackLengthCm;
        Info.StartingRaceSeconds = startRaceSeconds;
        HasStart = true;
        return true;
    }

    bool Append(DeltaReferencePoint point) noexcept
    {
        if (!HasStart || Complete ||
            !ValidDeltaPoint(point, Settings) ||
            Count >= Settings.MaximumReferencePoints)
        {
            ++RejectedPoints;
            return false;
        }
        if (Count > 0)
        {
            const DeltaReferencePoint& previous =
                Points[static_cast<std::size_t>(Count - 1)];
            const double distanceStep =
                point.DistanceCm - previous.DistanceCm;
            const double secondsStep =
                point.ElapsedSeconds - previous.ElapsedSeconds;
            if (distanceStep < Settings.MinimumDistanceStepCm ||
                secondsStep <= 0.0 ||
                secondsStep > Settings.MaximumTimeGapSeconds ||
                (Settings.RejectMixedSectors &&
                 point.SectorIndex <
                     previous.SectorIndex &&
                 !(previous.SectorIndex ==
                     MaximumTelemetrySectors - 1 &&
                   point.SectorIndex == 0)))
            {
                ++RejectedPoints;
                return false;
            }
            MaximumGapCm = std::max(
                MaximumGapCm, distanceStep);
        }
        else if (point.DistanceCm >
                 Settings.MaximumReferenceErrorCm)
        {
            ++RejectedPoints;
            return false;
        }
        Points[static_cast<std::size_t>(Count++)] = point;
        Info.PointCount = Count;
        return true;
    }

    bool Finish(
        double lapSeconds, int sectors,
        bool officialLapQualified) noexcept
    {
        if (!HasStart || Count < 2 || Complete ||
            !Finite(lapSeconds) || lapSeconds <= 0.0 ||
            lapSeconds > Settings.MaximumLapTimeSeconds ||
            sectors < 1 || sectors > MaximumTelemetrySectors ||
            !officialLapQualified)
        {
            return false;
        }
        const DeltaReferencePoint& last =
            Points[static_cast<std::size_t>(Count - 1)];
        if (std::abs(last.DistanceCm - Info.TotalDistanceCm) >
                Settings.MaximumReferenceErrorCm ||
            std::abs(last.ElapsedSeconds - lapSeconds) >
                Settings.MaximumTimeGapSeconds)
        {
            return false;
        }
        Info.FinishedLapSeconds = lapSeconds;
        Info.FinishedRaceSeconds =
            Info.StartingRaceSeconds + lapSeconds;
        Info.MaximumSampleGapCm = MaximumGapCm;
        Info.SectorCount = sectors;
        Info.OfficialLapQualified = officialLapQualified;
        Info.FullySampled =
            MaximumGapCm <= Settings.MaximumInterpolationGapCm &&
            Points[0].DistanceCm <= Settings.MaximumReferenceErrorCm;
        Info.Valid = Info.FullySampled &&
            Info.OfficialLapQualified &&
            Count >= 2;
        Complete = Info.Valid;
        return Complete;
    }

    DeltaReferencePoint Point(int index) const noexcept
    {
        return index >= 0 && index < Count ?
            Points[static_cast<std::size_t>(index)] :
            DeltaReferencePoint{};
    }

    DeltaReferencePoint AtDistance(
        double distanceCm) const noexcept
    {
        if (!Complete || !Finite(distanceCm) ||
            distanceCm < Points[0].DistanceCm ||
            distanceCm > Points[static_cast<std::size_t>(
                Count - 1)].DistanceCm)
        {
            return {};
        }
        int lower = 0;
        int upper = Count - 1;
        while (lower < upper)
        {
            const int middle = lower + (upper - lower) / 2;
            if (Points[static_cast<std::size_t>(middle)]
                    .DistanceCm < distanceCm)
            {
                lower = middle + 1;
            }
            else
            {
                upper = middle;
            }
        }
        if (lower == 0)
        {
            return Points[0];
        }
        const DeltaReferencePoint& a =
            Points[static_cast<std::size_t>(lower - 1)];
        const DeltaReferencePoint& b =
            Points[static_cast<std::size_t>(lower)];
        if (b.DistanceCm - a.DistanceCm >
                Settings.MaximumInterpolationGapCm)
        {
            return {};
        }
        return InterpolateDeltaReference(
            a, b, SafeRatio(
                distanceCm - a.DistanceCm,
                b.DistanceCm - a.DistanceCm));
    }

    bool IsComplete() const noexcept { return Complete; }
    int Size() const noexcept { return Count; }
    int RejectedSamples() const noexcept { return RejectedPoints; }
    DeltaReferenceInfo Metadata() const noexcept { return Info; }
    double MaximumSpacingCm() const noexcept { return MaximumGapCm; }

private:
    DeltaSettings Settings{};
    std::array<DeltaReferencePoint,
               MaximumDeltaReferencePoints> Points{};
    DeltaReferenceInfo Info{};
    int Count = 0;
    int RejectedPoints = 0;
    double MaximumGapCm = 0.0;
    bool HasStart = false;
    bool Complete = false;
};

struct DeltaComparisonStatistics final
{
    double LargestGainSeconds = 0.0;
    double LargestLossSeconds = 0.0;
    double LatestDeltaSeconds = 0.0;
    double MeanAbsoluteDeltaSeconds = 0.0;
    double SpeedDeficitIntegralCm = 0.0;
    double AdditionalBrakingDistanceCm = 0.0;
    double EarlierThrottleDistanceCm = 0.0;
    double DifferentLineDistanceCm = 0.0;
    double TimeGainingDistanceCm = 0.0;
    double TimeLosingDistanceCm = 0.0;
    double MaximumSpeedDeficitCmS = 0.0;
    double MaximumLateralDifferenceCm = 0.0;
    int QualifiedSamples = 0;
    int RejectedSamples = 0;
    int GainingSegments = 0;
    int LosingSegments = 0;
    bool HasReference = false;
};

class LiveDeltaAnalyzer final
{
public:
    bool Configure(
        const LapDeltaReference* reference,
        DeltaSettings settings = {}) noexcept
    {
        if (!reference || !reference->IsComplete() ||
            !ValidDeltaSettings(settings))
        {
            return false;
        }
        Reference = reference;
        Settings = settings;
        Reset();
        return true;
    }

    void Reset() noexcept
    {
        Last = {};
        Stats = {};
        LastDistanceCm = -1.0;
        PreviousDelta = 0.0;
        HasPrevious = false;
    }

    DeltaSample Compare(
        double distanceCm,
        double lapSeconds,
        double speedCmS, double throttle, double brake,
        double steering, double lateralErrorCm,
        double gripUtilization, int sectorIndex,
        std::uint64_t trackFingerprint) noexcept
    {
        DeltaSample result;
        if (!Reference || !Reference->IsComplete())
        {
            return Reject(DeltaRejectReason::NoReference, result);
        }
        const DeltaReferenceInfo info = Reference->Metadata();
        if (trackFingerprint != info.TrackFingerprint)
        {
            return Reject(DeltaRejectReason::WrongTrack, result);
        }
        if (!Finite(distanceCm) || !Finite(lapSeconds) ||
            !Finite(speedCmS) || !Finite(throttle) ||
            !Finite(brake) || !Finite(steering) ||
            !Finite(lateralErrorCm) || !Finite(gripUtilization) ||
            distanceCm < 0.0 || lapSeconds < 0.0 ||
            speedCmS < 0.0 || speedCmS > MaximumPlanarSpeedCmS ||
            throttle < 0.0 || throttle > 1.0 ||
            brake < 0.0 || brake > 1.0 ||
            std::abs(steering) > 1.0)
        {
            return Reject(DeltaRejectReason::InvalidCurrentSample,
                          result);
        }
        if (HasPrevious &&
            distanceCm < LastDistanceCm)
        {
            return Reject(DeltaRejectReason::BackwardsDistance, result);
        }
        const DeltaReferencePoint ref =
            Reference->AtDistance(distanceCm);
        if (!ref.Valid)
        {
            return Reject(DeltaRejectReason::OutsideBounds, result);
        }
        if (Settings.RejectMixedSectors &&
            ref.SectorIndex != sectorIndex)
        {
            return Reject(DeltaRejectReason::IncompatibleFormat, result);
        }
        result.Reason = DeltaRejectReason::None;
        result.DistanceCm = distanceCm;
        result.CurrentLapSeconds = lapSeconds;
        result.ReferenceLapSeconds = ref.ElapsedSeconds;
        result.DeltaSeconds = lapSeconds - ref.ElapsedSeconds;
        result.ReferenceSpeedCmS = ref.SpeedCmS;
        result.CurrentSpeedCmS = speedCmS;
        result.SpeedDifferenceCmS = speedCmS - ref.SpeedCmS;
        result.ReferenceThrottle = ref.Throttle;
        result.CurrentThrottle = throttle;
        result.ReferenceBrake = ref.Brake;
        result.CurrentBrake = brake;
        result.ReferenceSteering = ref.Steering;
        result.CurrentSteering = steering;
        result.ReferenceLateralErrorCm = ref.LateralErrorCm;
        result.CurrentLateralErrorCm = lateralErrorCm;
        result.ReferenceGripUtilization = ref.GripUtilization;
        result.ReferenceSlipRadians = ref.SlipRadians;
        result.SectorIndex = sectorIndex;
        result.BrakingLater = brake < 0.15 && ref.Brake > 0.4;
        result.AcceleratingEarlier =
            throttle > 0.5 && ref.Throttle < 0.25;
        result.DifferentRacingLine =
            std::abs(lateralErrorCm - ref.LateralErrorCm) > 100.0;
        result.Qualified = true;
        if (HasPrevious)
        {
            const double step = distanceCm - LastDistanceCm;
            result.DeltaSlopeSecondsPerMeter =
                SafeRatio(result.DeltaSeconds - PreviousDelta, step / 100.0);
            result.GainingTime =
                result.DeltaSeconds + 0.001 < PreviousDelta;
            result.LosingTime =
                result.DeltaSeconds > PreviousDelta + 0.001;
            Stats.TimeGainingDistanceCm +=
                result.GainingTime ? step : 0.0;
            Stats.TimeLosingDistanceCm +=
                result.LosingTime ? step : 0.0;
            Stats.AdditionalBrakingDistanceCm +=
                brake > ref.Brake + 0.15 ? step : 0.0;
            Stats.EarlierThrottleDistanceCm +=
                result.AcceleratingEarlier ? step : 0.0;
            Stats.DifferentLineDistanceCm +=
                result.DifferentRacingLine ? step : 0.0;
            Stats.SpeedDeficitIntegralCm +=
                std::max(0.0, -result.SpeedDifferenceCmS) * step;
            Stats.GainingSegments += result.GainingTime ? 1 : 0;
            Stats.LosingSegments += result.LosingTime ? 1 : 0;
        }
        Stats.QualifiedSamples += 1;
        Stats.HasReference = true;
        Stats.LatestDeltaSeconds = result.DeltaSeconds;
        Stats.LargestGainSeconds = std::min(
            Stats.LargestGainSeconds, result.DeltaSeconds);
        Stats.LargestLossSeconds = std::max(
            Stats.LargestLossSeconds, result.DeltaSeconds);
        Stats.MaximumSpeedDeficitCmS = std::max(
            Stats.MaximumSpeedDeficitCmS,
            -result.SpeedDifferenceCmS);
        Stats.MaximumLateralDifferenceCm = std::max(
            Stats.MaximumLateralDifferenceCm,
            std::abs(lateralErrorCm - ref.LateralErrorCm));
        Stats.MeanAbsoluteDeltaSeconds +=
            (std::abs(result.DeltaSeconds) -
             Stats.MeanAbsoluteDeltaSeconds) /
            static_cast<double>(Stats.QualifiedSamples);
        PreviousDelta = result.DeltaSeconds;
        LastDistanceCm = distanceCm;
        HasPrevious = true;
        Last = result;
        return result;
    }

    DeltaSample LastComparison() const noexcept { return Last; }
    DeltaComparisonStatistics History() const noexcept { return Stats; }
    bool Ready() const noexcept
    {
        return Reference && Reference->IsComplete() && Stats.HasReference;
    }

private:
    DeltaSample Reject(
        DeltaRejectReason reason,
        DeltaSample result) noexcept
    {
        result.Reason = reason;
        result.Qualified = false;
        ++Stats.RejectedSamples;
        Last = result;
        return result;
    }

    const LapDeltaReference* Reference = nullptr;
    DeltaSettings Settings{};
    DeltaSample Last{};
    DeltaComparisonStatistics Stats{};
    double LastDistanceCm = -1.0;
    double PreviousDelta = 0.0;
    bool HasPrevious = false;
};
} // namespace latebrake::engineering

#pragma once

#include "Telemetry/TelemetryCoach.h"
#include "Telemetry/TelemetryRaceStrategy.h"
#include "Telemetry/TelemetryDriverComparison.h"
#include "Telemetry/TelemetryStintPerformance.h"
#include "Telemetry/TelemetryReliability.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace latebrake::engineering
{
enum class DashboardPanel : std::uint8_t
{
    Session,
    LapTiming,
    SectorTiming,
    Chassis,
    Brake,
    Corner,
    Grip,
    Fuel,
    VehicleHealth,
    Opponents,
    Strategy,
    SensorStatus,
    Coaching,
    Comparison,
    Replay,
    PitReview
};

enum class DashboardDataQuality : std::uint8_t
{
    Unavailable,
    OfflineEstimate,
    IncompleteNative,
    QualifiedNative,
    Stale
};

struct DashboardSettings final
{
    double MaximumFrameAgeSeconds = 0.5;
    double MaximumSnapshotAgeSeconds = 2.0;
    double MaximumStaleSeconds = 5.0;
    double MinimumSensorConfidence = 0.7;
    double SpeedFilterSeconds = 0.12;
    double ThreatHoldSeconds = 0.5;
    double MaximumSteeringMagnitude = 1.0;
    double MaximumTemperatureC = 1500.0;
    double MaximumAbsoluteDeltaSeconds = 300.0;
    int MaximumPanelCount = 16;
    bool HideUnverifiedSensors = true;
    bool ShowAdvisoryLabels = true;
    bool RequireSameNativeSession = true;
};

inline bool ValidDashboardSettings(
    const DashboardSettings& s) noexcept
{
    return Finite(s.MaximumFrameAgeSeconds) &&
           s.MaximumFrameAgeSeconds > 0.0 &&
           Finite(s.MaximumSnapshotAgeSeconds) &&
           s.MaximumSnapshotAgeSeconds >
               s.MaximumFrameAgeSeconds &&
           Finite(s.MaximumStaleSeconds) &&
           s.MaximumStaleSeconds >
               s.MaximumSnapshotAgeSeconds &&
           Finite(s.MinimumSensorConfidence) &&
           s.MinimumSensorConfidence >= 0.0 &&
           s.MinimumSensorConfidence <= 1.0 &&
           Finite(s.SpeedFilterSeconds) &&
           s.SpeedFilterSeconds > 0.0 &&
           Finite(s.ThreatHoldSeconds) &&
           s.ThreatHoldSeconds >= 0.0 &&
           Finite(s.MaximumSteeringMagnitude) &&
           s.MaximumSteeringMagnitude > 0.0 &&
           Finite(s.MaximumTemperatureC) &&
           s.MaximumTemperatureC > 0.0 &&
           Finite(s.MaximumAbsoluteDeltaSeconds) &&
           s.MaximumAbsoluteDeltaSeconds > 0.0 &&
           s.MaximumPanelCount >= 1 &&
           s.MaximumPanelCount <= 16;
}

struct DashboardPanelStatus final
{
    DashboardPanel Panel = DashboardPanel::Session;
    DashboardDataQuality Quality =
        DashboardDataQuality::Unavailable;
    double UpdatedAtSeconds = 0.0;
    double AgeSeconds = 0.0;
    double Confidence = 0.0;
    std::uint64_t SampleCount = 0;
    bool Available = false;
    bool NativeObserved = false;
    bool MayDisplay = false;
    bool SafetyCritical = false;
};

struct DashboardRaceView final
{
    std::uint64_t SessionToken = 0;
    std::uint64_t ActorToken = 0;
    std::uint64_t SourceFrameOrdinal = 0;
    double MonotonicSeconds = 0.0;
    double RaceClockSeconds = 0.0;
    double CurrentLapSeconds = 0.0;
    double BestLapSeconds = 0.0;
    double LastLapSeconds = 0.0;
    double CurrentSectorSeconds = 0.0;
    double EstimatedLapDeltaSeconds = 0.0;
    double DrivenDistanceCm = 0.0;
    double TrackDistanceCm = 0.0;
    double CrossTrackErrorCm = 0.0;
    double SpeedKmh = 0.0;
    double ForwardSpeedKmh = 0.0;
    double LateralSpeedKmh = 0.0;
    double Throttle = 0.0;
    double Brake = 0.0;
    double Steering = 0.0;
    double EngineRpm = 0.0;
    double EngineTemperatureC = 0.0;
    double BrakeTemperatureC = 0.0;
    double FuelFraction = 0.0;
    double EstimatedLapsOfFuel = 0.0;
    double TireWearFraction = 0.0;
    double GripUtilization = 0.0;
    double SlipAngleDegrees = 0.0;
    double LongitudinalG = 0.0;
    double LateralG = 0.0;
    double EstimatedBrakeDistanceCm = 0.0;
    double NearestRivalDistanceCm = 0.0;
    double TimeToPotentialContactSeconds = 0.0;
    double RiskEventHoldSeconds = 0.0;
    int LapIndex = 0;
    int SectorIndex = 0;
    int RacePosition = 0;
    int NearbyRivals = 0;
    int ThreateningRivals = 0;
    int Gear = 0;
    TelemetryPhase Phase = TelemetryPhase::PreGrid;
    GripState GripCondition = GripState::Unobserved;
    HealthCondition Health = HealthCondition::Unavailable;
    FuelForecastConfidence FuelQuality =
        FuelForecastConfidence::Missing;
    StrategyKind SuggestedReview = StrategyKind::Unavailable;
    CoachingTopic PrimaryCoachingTopic = CoachingTopic::None;
    DashboardDataQuality OverallData =
        DashboardDataQuality::Unavailable;
    std::array<DashboardPanelStatus, 16> Panels{};
    bool NativeData = false;
    bool IsPaused = false;
    bool InPitLane = false;
    bool OnRoad = false;
    bool ReadyForDisplay = false;
    bool AnyCriticalWarning = false;
    bool AdvisoryOnly = true;
};

struct DashboardSources final
{
    TelemetryFrame Frame{};
    TelemetryKinematics Kinematics{};
    TrackProjection Track{};
    BrakeLiveMetrics Brake{};
    CornerLiveMetrics Corner{};
    GripTelemetry Grip{};
    FuelStintSummary Fuel{};
    VehicleHealthSnapshot Health{};
    FleetTelemetrySnapshot Opponents{};
    ReliabilitySnapshot Reliability{};
    StrategyAssessment Strategy{};
    CoachingTip Coach{};
    DeltaSample LapDelta{};
    TelemetryLap LastLap{};
    SectorSplitObservation Sector{};
    std::uint64_t SessionToken = 0;
    std::uint64_t ActorToken = 0;
    double SnapshotSeconds = 0.0;
    bool NativeObservation = false;
    bool ReplayMode = false;
};

inline DashboardDataQuality ClassifyDashboardSource(
    bool available, bool native,
    double confidence, double age,
    const DashboardSettings& settings) noexcept
{
    if (!available || !Finite(age) ||
        !Finite(confidence) || age < 0.0)
    {
        return DashboardDataQuality::Unavailable;
    }
    if (age > settings.MaximumStaleSeconds)
    {
        return DashboardDataQuality::Stale;
    }
    if (!native)
    {
        return DashboardDataQuality::OfflineEstimate;
    }
    if (age > settings.MaximumSnapshotAgeSeconds ||
        confidence < settings.MinimumSensorConfidence)
    {
        return DashboardDataQuality::IncompleteNative;
    }
    return DashboardDataQuality::QualifiedNative;
}

inline DashboardPanelStatus MakeDashboardPanelStatus(
    DashboardPanel panel, bool available, bool native,
    double confidence, double updatedAt,
    double now, std::uint64_t frames,
    const DashboardSettings& settings,
    bool critical = false) noexcept
{
    DashboardPanelStatus out;
    out.Panel = panel;
    out.UpdatedAtSeconds = updatedAt;
    out.AgeSeconds = std::max(0.0, now - updatedAt);
    out.Confidence = Clamp01(confidence);
    out.SampleCount = frames;
    out.Available = available;
    out.NativeObserved = native;
    out.Quality = ClassifyDashboardSource(
        available, native, confidence,
        out.AgeSeconds, settings);
    out.SafetyCritical = critical;
    out.MayDisplay = available &&
        out.Quality != DashboardDataQuality::Stale &&
        out.Quality != DashboardDataQuality::Unavailable &&
        (!settings.HideUnverifiedSensors || native);
    return out;
}

class RaceDashboardModel final
{
public:
    bool Configure(DashboardSettings settings) noexcept
    {
        if (!ValidDashboardSettings(settings))
        {
            return false;
        }
        Settings = settings;
        Reset();
        return true;
    }

    void Reset() noexcept
    {
        Last = {};
        SpeedFilter.Reset();
        PreviousSnapshotSeconds = 0.0;
        SourceSession = 0;
        SourceActor = 0;
        QualifiedFrames = 0;
        RejectedFrames = 0;
        ThreatHoldSeconds = 0.0;
        HasPrevious = false;
    }

    bool Update(
        const DashboardSources& sources) noexcept
    {
        if (!ValidDashboardSettings(Settings) ||
            !Finite(sources.SnapshotSeconds) ||
            sources.SnapshotSeconds < 0.0 ||
            !Finite(sources.Frame.MonotonicSeconds) ||
            sources.Frame.Identity.SessionToken == 0 ||
            sources.Frame.Identity.ActorToken == 0 ||
            (Settings.RequireSameNativeSession &&
             (sources.SessionToken !=
                  sources.Frame.Identity.SessionToken ||
              sources.ActorToken !=
                  sources.Frame.Identity.ActorToken)) ||
            (HasPrevious &&
             sources.SnapshotSeconds < PreviousSnapshotSeconds))
        {
            RejectedFrames = SaturatingIncrement(RejectedFrames);
            return false;
        }
        if (SourceSession != 0 &&
            (SourceSession != sources.SessionToken ||
             SourceActor != sources.ActorToken))
        {
            Reset();
        }
        SourceSession = sources.SessionToken;
        SourceActor = sources.ActorToken;
        const double frameAge = sources.SnapshotSeconds -
            sources.Frame.MonotonicSeconds;
        if (!Finite(frameAge) ||
            frameAge < 0.0 ||
            frameAge > Settings.MaximumStaleSeconds)
        {
            RejectedFrames = SaturatingIncrement(RejectedFrames);
            return false;
        }
        const bool native =
            sources.NativeObservation &&
            sources.Frame.Race.EnginePhysicsReady &&
            IsEngineObservation(sources.Frame.Identity.Trust);
        DashboardRaceView view;
        view.SessionToken = sources.SessionToken;
        view.ActorToken = sources.ActorToken;
        view.SourceFrameOrdinal =
            sources.Frame.Identity.FrameOrdinal;
        view.MonotonicSeconds = sources.SnapshotSeconds;
        view.RaceClockSeconds = sources.Frame.RaceClockSeconds;
        view.CurrentLapSeconds =
            sources.Frame.Race.CurrentLapSeconds;
        view.LastLapSeconds =
            sources.LastLap.DurationSeconds;
        view.BestLapSeconds =
            sources.LastLap.DurationSeconds;
        view.LapIndex = sources.Frame.Race.LapIndex;
        view.SectorIndex = sources.Frame.Race.SectorIndex;
        view.RacePosition = sources.Frame.Race.PositionOrdinal;
        view.Phase = sources.Frame.Race.Phase;
        view.Gear = sources.Frame.Controls.Gear;
        view.Throttle = sources.Frame.Controls.Throttle;
        view.Brake = sources.Frame.Controls.Brake;
        view.Steering = sources.Frame.Controls.Steering;
        view.EngineRpm = sources.Frame.Dynamics.EngineRpm;
        view.EngineTemperatureC =
            sources.Frame.Dynamics.EngineTemperatureCelsius;
        view.BrakeTemperatureC =
            sources.Frame.Dynamics.BrakeTemperatureCelsius;
        view.FuelFraction =
            sources.Frame.Dynamics.FuelFraction;
        view.IsPaused =
            sources.Frame.Race.Phase == TelemetryPhase::Paused;
        view.InPitLane = sources.Frame.Environment.InPitLane;
        view.OnRoad =
            sources.Frame.Environment.OnPhysicalPavement;
        view.NativeData = native;
        view.DrivenDistanceCm =
            sources.Kinematics.DistanceCm;
        view.TrackDistanceCm =
            sources.Track.Valid ?
                sources.Track.DistanceAlongCm : 0.0;
        view.CrossTrackErrorCm =
            sources.Track.Valid ?
                sources.Track.SignedLateralCm : 0.0;
        if (sources.Kinematics.Valid)
        {
            const double delta = HasPrevious ?
                std::max(0.001,
                    sources.SnapshotSeconds -
                    PreviousSnapshotSeconds) :
                1.0 / 60.0;
            view.SpeedKmh = SpeedFilter.Observe(
                sources.Kinematics.SpeedKmh, delta,
                Settings.SpeedFilterSeconds);
            view.ForwardSpeedKmh =
                sources.Kinematics.ForwardSpeedCmS *
                CmPerSecondToKmh;
            view.LateralSpeedKmh =
                sources.Kinematics.LateralSpeedCmS *
                CmPerSecondToKmh;
            view.SlipAngleDegrees =
                sources.Kinematics.SlipAngleDegrees;
            view.LongitudinalG =
                sources.Kinematics.LongitudinalAccelerationCmS2 /
                GravityCmS2;
            view.LateralG =
                sources.Kinematics.LateralAccelerationCmS2 /
                GravityCmS2;
        }
        view.EstimatedBrakeDistanceCm =
            sources.Brake.Valid ?
                sources.Brake.CurrentStopDistanceCm : 0.0;
        view.TireWearFraction = sources.Grip.Valid ?
            sources.Grip.MeanWearFraction : 0.0;
        view.GripUtilization = sources.Grip.Valid ?
            sources.Grip.TractionUtilizationFraction : 0.0;
        view.GripCondition = sources.Grip.State;
        view.EstimatedLapsOfFuel =
            sources.Fuel.EstimatedLapsRemaining;
        view.FuelQuality = sources.Fuel.Confidence;
        view.Health = sources.Health.Overall;
        view.AnyCriticalWarning =
            sources.Health.AnyCritical;
        view.NearbyRivals = sources.Opponents.Valid ?
            sources.Opponents.NearbyOpponents : 0;
        view.ThreateningRivals = sources.Opponents.Valid ?
            sources.Opponents.PredictedContacts : 0;
        view.NearestRivalDistanceCm =
            sources.Opponents.Valid &&
            Finite(sources.Opponents.NearestDistanceCm) ?
                sources.Opponents.NearestDistanceCm : 0.0;
        view.TimeToPotentialContactSeconds =
            sources.Opponents.Valid &&
            Finite(sources.Opponents.EarliestPredictedContactSeconds) ?
                sources.Opponents.EarliestPredictedContactSeconds : 0.0;
        if (view.ThreateningRivals > 0)
        {
            ThreatHoldSeconds = Settings.ThreatHoldSeconds;
        }
        else if (HasPrevious)
        {
            ThreatHoldSeconds = std::max(
                0.0, ThreatHoldSeconds -
                (sources.SnapshotSeconds -
                 PreviousSnapshotSeconds));
        }
        view.RiskEventHoldSeconds = ThreatHoldSeconds;
        view.SuggestedReview = sources.Strategy.Preferred;
        view.PrimaryCoachingTopic = sources.Coach.Visible ?
            sources.Coach.Evidence.Topic : CoachingTopic::None;
        view.EstimatedLapDeltaSeconds =
            sources.LapDelta.Qualified ?
                std::clamp(sources.LapDelta.DeltaSeconds,
                    -Settings.MaximumAbsoluteDeltaSeconds,
                     Settings.MaximumAbsoluteDeltaSeconds) : 0.0;
        const auto panel = [this, &view, &sources, native](
            DashboardPanel kind, bool available,
            double confidence, bool critical = false)
        {
            const std::size_t index =
                static_cast<std::size_t>(kind);
            view.Panels[index] = MakeDashboardPanelStatus(
                kind, available, native,
                confidence,
                sources.Frame.MonotonicSeconds,
                sources.SnapshotSeconds,
                QualifiedFrames + 1,
                Settings, critical);
        };
        panel(DashboardPanel::Session,
            true, sources.Reliability.OverallConfidence);
        panel(DashboardPanel::LapTiming,
            sources.LastLap.DurationSeconds > 0.0,
            sources.Reliability.TimingConfidence);
        panel(DashboardPanel::SectorTiming,
            sources.Sector.Accepted,
            sources.Reliability.TimingConfidence);
        panel(DashboardPanel::Chassis,
            sources.Kinematics.Valid,
            sources.Reliability.PhysicalSensorConfidence);
        panel(DashboardPanel::Brake,
            sources.Brake.Valid,
            sources.Reliability.DriverInputConfidence);
        panel(DashboardPanel::Corner,
            sources.Corner.Valid,
            sources.Reliability.PhysicalSensorConfidence);
        panel(DashboardPanel::Grip,
            sources.Grip.Valid,
            sources.Grip.Confidence);
        panel(DashboardPanel::Fuel,
            sources.Fuel.Qualified,
            sources.Reliability.PhysicalSensorConfidence);
        panel(DashboardPanel::VehicleHealth,
            sources.Health.Qualified,
            sources.Health.CombinedConfidence, true);
        panel(DashboardPanel::Opponents,
            sources.Opponents.Valid,
            sources.Reliability.PhysicalSensorConfidence, true);
        panel(DashboardPanel::Strategy,
            sources.Strategy.ForecastQualified,
            sources.Strategy.Confidence);
        panel(DashboardPanel::SensorStatus,
            true, sources.Reliability.OverallConfidence, true);
        panel(DashboardPanel::Coaching,
            sources.Coach.Active,
            sources.Coach.Evidence.Confidence);
        panel(DashboardPanel::Comparison,
            sources.LapDelta.Qualified,
            sources.Reliability.TimingConfidence);
        panel(DashboardPanel::Replay,
            sources.ReplayMode,
            sources.Reliability.PhysicalSensorConfidence);
        panel(DashboardPanel::PitReview,
            sources.Frame.Environment.InPitLane,
            sources.Reliability.PhysicalSensorConfidence);
        view.OverallData = ClassifyDashboardSource(
            sources.Kinematics.Valid,
            native, sources.Reliability.OverallConfidence,
            frameAge, Settings);
        view.ReadyForDisplay =
            view.OverallData == DashboardDataQuality::QualifiedNative ||
            (!Settings.HideUnverifiedSensors &&
             view.OverallData != DashboardDataQuality::Unavailable &&
             view.OverallData != DashboardDataQuality::Stale);
        view.AdvisoryOnly = true;
        Last = view;
        QualifiedFrames =
            SaturatingIncrement(QualifiedFrames);
        PreviousSnapshotSeconds = sources.SnapshotSeconds;
        HasPrevious = true;
        return true;
    }

    DashboardRaceView Snapshot() const noexcept { return Last; }
    DashboardPanelStatus Panel(DashboardPanel panel) const noexcept
    {
        const std::size_t index = static_cast<std::size_t>(panel);
        return index < Last.Panels.size() ?
            Last.Panels[index] : DashboardPanelStatus{};
    }
    std::uint64_t AcceptedFrames() const noexcept
    {
        return QualifiedFrames;
    }
    std::uint64_t RejectedFrames() const noexcept
    {
        return RejectedFrames;
    }
    bool CanDisplayNativeRaceView() const noexcept
    {
        return Last.ReadyForDisplay && Last.NativeData;
    }

private:
    DashboardSettings Settings{};
    DashboardRaceView Last{};
    ExponentialFilter SpeedFilter;
    std::uint64_t SourceSession = 0;
    std::uint64_t SourceActor = 0;
    std::uint64_t QualifiedFrames = 0;
    std::uint64_t RejectedFrames = 0;
    double PreviousSnapshotSeconds = 0.0;
    double ThreatHoldSeconds = 0.0;
    bool HasPrevious = false;
};
} // namespace latebrake::engineering

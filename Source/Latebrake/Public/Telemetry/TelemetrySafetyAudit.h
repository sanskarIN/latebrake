#pragma once

#include "Telemetry/TelemetryDashboard.h"
#include "Telemetry/TelemetrySessionTimeline.h"
#include "Telemetry/TelemetryEventJournal.h"
#include "Telemetry/TelemetryReplayBuffer.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
enum class EvidenceGate : std::uint8_t
{
    NativePhysics,
    ActorIdentity,
    SessionIdentity,
    MonotonicClock,
    RaceClock,
    FiniteWorldPose,
    FiniteWorldSpeed,
    DriverControls,
    NativeWheelContact,
    NativeWheelSlip,
    AuthoredTrackProjection,
    StableFrameSegment,
    VerifiedForwardGate,
    QualifiedLap,
    QualifiedSector,
    ObservedFuel,
    ObservedVehicleHeat,
    CompleteOpponentRoster,
    SensorReliability,
    NoCriticalMechanicalWarning,
    NoPredictedTrafficContact,
    SourceProvenance
};

inline constexpr int EvidenceGateCount = 22;

enum class EvidenceDisposition : std::uint8_t
{
    Missing,
    Rejected,
    Provisional,
    Qualified,
    NotApplicable
};

enum class ReadinessScope : std::uint8_t
{
    Untrusted,
    OfflineReplay,
    RaceTelemetry,
    OnTrackDrivingReview,
    SectorComparison,
    LapComparison,
    VehicleHealth,
    OpponentSafety,
    ShippingValidation
};

struct EvidenceSettings final
{
    double MinimumNativeConfidence = 0.75;
    double MinimumTimeContinuitySeconds = 1.0;
    double MaximumFrameAgeSeconds = 0.25;
    double MaximumComparisonTimeDriftSeconds = 0.12;
    double MinimumPhysicalContactFraction = 0.50;
    double MaximumAllowedSensorFailureFraction = 0.15;
    double MinimumTrackProjectionConfidence = 0.50;
    double MinimumVehicleHealthConfidence = 0.65;
    double MinimumOpponentEvidenceConfidence = 0.80;
    double MaximumAcceptedWorldMagnitudeCm = MaximumWorldMagnitudeCm;
    double MaximumAcceptedVehicleSpeedCmS = MaximumPlanarSpeedCmS;
    int MinimumFramesBeforeReady = 10;
    bool FailClosedOnMissingProvenance = true;
    bool RequireAllFourWheelsForGrip = false;
    bool ForbidOfficialResultClaims = true;
    bool RequireCompleteSixCarRoster = true;
};

inline bool ValidEvidenceSettings(
    const EvidenceSettings& s) noexcept
{
    return Finite(s.MinimumNativeConfidence) &&
           s.MinimumNativeConfidence >= 0.0 &&
           s.MinimumNativeConfidence <= 1.0 &&
           Finite(s.MinimumTimeContinuitySeconds) &&
           s.MinimumTimeContinuitySeconds > 0.0 &&
           Finite(s.MaximumFrameAgeSeconds) &&
           s.MaximumFrameAgeSeconds > 0.0 &&
           Finite(s.MaximumComparisonTimeDriftSeconds) &&
           s.MaximumComparisonTimeDriftSeconds >= 0.0 &&
           Finite(s.MinimumPhysicalContactFraction) &&
           s.MinimumPhysicalContactFraction >= 0.0 &&
           s.MinimumPhysicalContactFraction <= 1.0 &&
           Finite(s.MaximumAllowedSensorFailureFraction) &&
           s.MaximumAllowedSensorFailureFraction >= 0.0 &&
           Finite(s.MinimumTrackProjectionConfidence) &&
           s.MinimumTrackProjectionConfidence >= 0.0 &&
           Finite(s.MinimumVehicleHealthConfidence) &&
           s.MinimumVehicleHealthConfidence >= 0.0 &&
           Finite(s.MinimumOpponentEvidenceConfidence) &&
           s.MinimumOpponentEvidenceConfidence >= 0.0 &&
           Finite(s.MaximumAcceptedWorldMagnitudeCm) &&
           s.MaximumAcceptedWorldMagnitudeCm > 0.0 &&
           Finite(s.MaximumAcceptedVehicleSpeedCmS) &&
           s.MaximumAcceptedVehicleSpeedCmS > 0.0 &&
           s.MinimumFramesBeforeReady > 0 &&
           s.MinimumFramesBeforeReady <= 1000000;
}

struct EvidenceGateResult final
{
    EvidenceGate Gate = EvidenceGate::NativePhysics;
    EvidenceDisposition Disposition =
        EvidenceDisposition::Missing;
    double Confidence = 0.0;
    double Value = 0.0;
    double Threshold = 0.0;
    double LastChangedSeconds = 0.0;
    std::uint64_t Observations = 0;
    bool Required = true;
    bool Qualified = false;
};

struct EvidenceInputs final
{
    TelemetryFrame Frame{};
    TelemetryFrameDecision NativeGate{};
    TelemetryKinematics Motion{};
    TrackProjection Track{};
    GripTelemetry Grip{};
    VehicleHealthSnapshot Health{};
    FleetTelemetrySnapshot Opponents{};
    ReliabilitySnapshot Reliability{};
    TimelineObservation Timeline{};
    LapObservation Lap{};
    SectorSplitObservation Sector{};
    ReplaySample Replay{};
    double MonotonicSeconds = 0.0;
    double ContinuousNativeSeconds = 0.0;
    double VerifiedClockDisagreementSeconds = 0.0;
    bool NativeInputChannelsComplete = false;
    bool NativeTrackCollisionObserved = false;
    bool NativeWheelSlipObserved = false;
    bool NativeFuelSensorObserved = false;
    bool NativeThermalSensorObserved = false;
    bool SameOriginSession = false;
    bool SameOriginActor = false;
    bool GameplayEngineValidated = false;
    bool InstalledGameValidated = false;
    bool OriginalCookedMapValidated = false;
    bool GenuineHardwareFrameRatesValidated = false;
};

struct SafetyEvidenceReceipt final
{
    std::array<EvidenceGateResult, EvidenceGateCount> Gates{};
    std::uint64_t PassedBits = 0;
    std::uint64_t FailedBits = 0;
    std::uint64_t MissingBits = 0;
    std::uint64_t SessionToken = 0;
    std::uint64_t ActorToken = 0;
    std::uint64_t FrameOrdinal = 0;
    double ProducedAtSeconds = 0.0;
    double OverallConfidence = 0.0;
    double ReliabilityFraction = 0.0;
    double AvailableWheelFraction = 0.0;
    double MaximumObservedClockDriftSeconds = 0.0;
    int RequiredGates = 0;
    int PassedGates = 0;
    int RejectedGates = 0;
    int MissingGates = 0;
    ReadinessScope HighestValidatedScope =
        ReadinessScope::Untrusted;
    bool ProvenanceBound = false;
    bool RaceTelemetryQualified = false;
    bool LapComparisonQualified = false;
    bool OpponentSafetyQualified = false;
    bool EngineBuildVerified = false;
    bool InstalledGameVerified = false;
    bool ShippingEvidenceComplete = false;
    bool AdvisoryOnly = true;
};

inline bool IsGateQualified(
    const SafetyEvidenceReceipt& receipt,
    EvidenceGate gate) noexcept
{
    const std::size_t index =
        static_cast<std::size_t>(gate);
    return index < receipt.Gates.size() &&
           receipt.Gates[index].Qualified;
}

inline int CountPassingEvidenceGates(
    std::uint64_t bits) noexcept
{
    return static_cast<int>(std::popcount(bits));
}

inline bool HasNativeRaceClock(
    const EvidenceInputs& input,
    double maximumDrift) noexcept
{
    return input.Frame.Race.LocalRaceClockRunning &&
           input.Frame.Race.EnginePhysicsReady &&
           input.NativeGate.Accepted &&
           Finite(input.VerifiedClockDisagreementSeconds) &&
           std::abs(input.VerifiedClockDisagreementSeconds) <=
               maximumDrift;
}

class TelemetryEvidenceReview final
{
public:
    bool Configure(EvidenceSettings settings) noexcept
    {
        if (!ValidEvidenceSettings(settings))
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
        History.fill({});
        HistoryCount = 0;
        HistoryNext = 0;
        Accepted = 0;
        Rejected = 0;
        Session = 0;
        Actor = 0;
        LastTime = 0.0;
        HasPrevious = false;
    }

    SafetyEvidenceReceipt Review(
        const EvidenceInputs& input) noexcept
    {
        SafetyEvidenceReceipt receipt;
        receipt.ProducedAtSeconds = input.MonotonicSeconds;
        receipt.SessionToken =
            input.Frame.Identity.SessionToken;
        receipt.ActorToken =
            input.Frame.Identity.ActorToken;
        receipt.FrameOrdinal =
            input.Frame.Identity.FrameOrdinal;
        receipt.AvailableWheelFraction =
            WheelContactFraction(input.Frame.Wheels);
        receipt.ReliabilityFraction =
            input.Reliability.OverallConfidence;
        receipt.MaximumObservedClockDriftSeconds =
            std::abs(input.VerifiedClockDisagreementSeconds);
        if (!ValidEvidenceSettings(Settings) ||
            !Finite(input.MonotonicSeconds) ||
            input.MonotonicSeconds < 0.0 ||
            (HasPrevious &&
             input.MonotonicSeconds < LastTime))
        {
            Rejected = SaturatingIncrement(Rejected);
            return receipt;
        }
        if (Session != 0 &&
            (Session != receipt.SessionToken ||
             Actor != receipt.ActorToken))
        {
            Reset();
        }
        const bool native =
            IsEngineObservation(input.Frame.Identity.Trust) &&
            input.Frame.Race.EnginePhysicsReady &&
            input.NativeGate.Accepted;
        const bool identity =
            input.Frame.Identity.ActorToken != 0 &&
            input.Frame.Identity.SessionToken != 0 &&
            input.Frame.Identity.DriverSlot >= 0 &&
            input.Frame.Identity.DriverSlot <
                MaximumTelemetryDrivers;
        const bool finitePose =
            Finite(input.Frame.Dynamics.WorldPositionCm) &&
            Length(input.Frame.Dynamics.WorldPositionCm) <=
                Settings.MaximumAcceptedWorldMagnitudeCm;
        const bool finiteVelocity =
            Finite(input.Frame.Dynamics.WorldVelocityCmS) &&
            Length(input.Frame.Dynamics.WorldVelocityCmS) <=
                Settings.MaximumAcceptedVehicleSpeedCmS;
        const bool controls =
            ValidTelemetryControls(input.Frame.Controls);
        const bool clock = HasNativeRaceClock(
            input, Settings.MaximumComparisonTimeDriftSeconds);
        const bool wheelContact =
            input.NativeTrackCollisionObserved &&
            receipt.AvailableWheelFraction >=
                Settings.MinimumPhysicalContactFraction;
        const bool wheelSlip =
            input.NativeWheelSlipObserved &&
            ValidTelemetryWheels(input.Frame.Wheels);
        const bool track =
            input.Track.Valid &&
            Finite(input.Track.SignedLateralCm);
        const bool timeline =
            input.Timeline.Accepted &&
            input.ContinuousNativeSeconds >=
                Settings.MinimumTimeContinuitySeconds;
        const bool lap =
            input.Lap.HasVerifiedLap &&
            input.Lap.Completed.Quality ==
                TelemetryQuality::Qualified;
        const bool sector =
            input.Sector.SplitQualified &&
            input.Sector.Passage.VerifiedForwardGate;
        const bool reliable =
            input.Reliability.OverallConfidence >=
                Settings.MinimumNativeConfidence &&
            input.Reliability.Qualified;
        const bool health =
            input.Health.Qualified &&
            !input.Health.AnyCritical;
        const bool opponent =
            input.Opponents.Valid &&
            input.Opponents.CompletePhysicalRoster &&
            (!Settings.RequireCompleteSixCarRoster ||
             input.Opponents.QualifiedOpponents ==
                MaximumTelemetryDrivers - 1);
        const auto set = [
            &receipt, &input](EvidenceGate gate, bool good,
                            bool available, double confidence,
                            double value, double threshold)
        {
            const std::size_t index =
                static_cast<std::size_t>(gate);
            EvidenceGateResult& item = receipt.Gates[index];
            item.Gate = gate;
            item.Disposition = !available ?
                EvidenceDisposition::Missing :
                good ? EvidenceDisposition::Qualified :
                       EvidenceDisposition::Rejected;
            item.Confidence = Clamp01(confidence);
            item.Value = value;
            item.Threshold = threshold;
            item.LastChangedSeconds = input.MonotonicSeconds;
            item.Observations = 1;
            item.Required = true;
            item.Qualified = good && available;
            const std::uint64_t bit = 1ULL << index;
            if (!available)
            {
                receipt.MissingBits |= bit;
                ++receipt.MissingGates;
            }
            else if (!good)
            {
                receipt.FailedBits |= bit;
                ++receipt.RejectedGates;
            }
            else
            {
                receipt.PassedBits |= bit;
                ++receipt.PassedGates;
            }
            ++receipt.RequiredGates;
        };
        set(EvidenceGate::NativePhysics,
            native, true, native ? 1.0 : 0.0,
            native ? 1.0 : 0.0, 1.0);
        set(EvidenceGate::ActorIdentity,
            identity, true, 1.0,
            static_cast<double>(
                input.Frame.Identity.DriverSlot), 0.0);
        set(EvidenceGate::SessionIdentity,
            input.SameOriginSession,
            identity, 1.0,
            static_cast<double>(
                input.Frame.Identity.SessionRevision), 0.0);
        set(EvidenceGate::MonotonicClock,
            input.NativeGate.Accepted &&
            input.Timeline.Accepted, native, 1.0,
            input.Frame.MonotonicSeconds, 0.0);
        set(EvidenceGate::RaceClock,
            clock, native, 1.0,
            input.VerifiedClockDisagreementSeconds,
            Settings.MaximumComparisonTimeDriftSeconds);
        set(EvidenceGate::FiniteWorldPose,
            finitePose, native, 1.0,
            Length(input.Frame.Dynamics.WorldPositionCm),
            Settings.MaximumAcceptedWorldMagnitudeCm);
        set(EvidenceGate::FiniteWorldSpeed,
            finiteVelocity, native, 1.0,
            Length(input.Frame.Dynamics.WorldVelocityCmS),
            Settings.MaximumAcceptedVehicleSpeedCmS);
        set(EvidenceGate::DriverControls,
            controls, native, 1.0,
            input.Frame.Controls.Throttle, 1.0);
        set(EvidenceGate::NativeWheelContact,
            wheelContact, input.NativeTrackCollisionObserved,
            receipt.AvailableWheelFraction,
            receipt.AvailableWheelFraction,
            Settings.MinimumPhysicalContactFraction);
        set(EvidenceGate::NativeWheelSlip,
            wheelSlip, input.NativeWheelSlipObserved, 1.0,
            HighestWheelSlip(input.Frame.Wheels), 12.0);
        set(EvidenceGate::AuthoredTrackProjection,
            track, native, track ? 1.0 : 0.0,
            input.Track.SignedLateralCm, 0.0);
        set(EvidenceGate::StableFrameSegment,
            timeline, native, timeline ? 1.0 : 0.0,
            input.ContinuousNativeSeconds,
            Settings.MinimumTimeContinuitySeconds);
        set(EvidenceGate::VerifiedForwardGate,
            input.Frame.Race.VerifiedForwardGate,
            native, 1.0, input.Frame.Race.VerifiedForwardGate ? 1.0 : 0.0,
            1.0);
        set(EvidenceGate::QualifiedLap,
            lap, input.Lap.CompletedLap, 1.0,
            input.Lap.Completed.DurationSeconds, 0.0);
        set(EvidenceGate::QualifiedSector,
            sector, input.Sector.CompletedSplit, 1.0,
            input.Sector.Passage.DurationSeconds, 0.0);
        set(EvidenceGate::ObservedFuel,
            input.NativeFuelSensorObserved &&
            Finite(input.Frame.Dynamics.FuelFraction),
            input.NativeFuelSensorObserved, 1.0,
            input.Frame.Dynamics.FuelFraction, 0.0);
        set(EvidenceGate::ObservedVehicleHeat,
            input.NativeThermalSensorObserved &&
            Finite(input.Frame.Dynamics.EngineTemperatureCelsius),
            input.NativeThermalSensorObserved, 1.0,
            input.Frame.Dynamics.EngineTemperatureCelsius, 0.0);
        set(EvidenceGate::CompleteOpponentRoster,
            opponent, input.Opponents.Valid, 1.0,
            static_cast<double>(
                input.Opponents.QualifiedOpponents),
            static_cast<double>(
                MaximumTelemetryDrivers - 1));
        set(EvidenceGate::SensorReliability,
            reliable, input.Reliability.Qualified,
            input.Reliability.OverallConfidence,
            input.Reliability.OverallConfidence,
            Settings.MinimumNativeConfidence);
        set(EvidenceGate::NoCriticalMechanicalWarning,
            health, input.Health.Qualified,
            input.Health.CombinedConfidence,
            input.Health.AnyCritical ? 1.0 : 0.0, 0.0);
        set(EvidenceGate::NoPredictedTrafficContact,
            input.Opponents.PredictedContacts == 0,
            input.Opponents.Valid, 1.0,
            static_cast<double>(
                input.Opponents.PredictedContacts), 0.0);
        set(EvidenceGate::SourceProvenance,
            input.SameOriginSession &&
            input.SameOriginActor && native,
            identity, 1.0,
            native ? 1.0 : 0.0, 1.0);
        receipt.ProvenanceBound =
            IsGateQualified(receipt,
                EvidenceGate::SourceProvenance);
        receipt.OverallConfidence =
            SafeRatio(receipt.PassedGates,
                      receipt.RequiredGates);
        receipt.RaceTelemetryQualified =
            native && identity && controls &&
            finitePose && finiteVelocity &&
            reliable && timeline &&
            receipt.ProvenanceBound;
        receipt.LapComparisonQualified =
            receipt.RaceTelemetryQualified &&
            clock && lap && sector && track;
        receipt.OpponentSafetyQualified =
            receipt.RaceTelemetryQualified &&
            opponent && wheelContact &&
            input.Opponents.PredictedContacts == 0;
        receipt.EngineBuildVerified =
            input.GameplayEngineValidated;
        receipt.InstalledGameVerified =
            input.InstalledGameValidated &&
            input.GameplayEngineValidated &&
            input.OriginalCookedMapValidated;
        receipt.ShippingEvidenceComplete =
            receipt.InstalledGameVerified &&
            input.GenuineHardwareFrameRatesValidated &&
            receipt.OpponentSafetyQualified &&
            receipt.LapComparisonQualified;
        receipt.HighestValidatedScope =
            receipt.ShippingEvidenceComplete ?
                ReadinessScope::ShippingValidation :
            receipt.OpponentSafetyQualified ?
                ReadinessScope::OpponentSafety :
            receipt.LapComparisonQualified ?
                ReadinessScope::LapComparison :
            receipt.RaceTelemetryQualified ?
                ReadinessScope::RaceTelemetry :
            native ? ReadinessScope::OfflineReplay :
                     ReadinessScope::Untrusted;
        receipt.AdvisoryOnly = true;
        Last = receipt;
        Session = receipt.SessionToken;
        Actor = receipt.ActorToken;
        LastTime = input.MonotonicSeconds;
        HasPrevious = true;
        Accepted = SaturatingIncrement(Accepted);
        History[static_cast<std::size_t>(HistoryNext)] = receipt;
        HistoryNext = (HistoryNext + 1) %
            static_cast<int>(History.size());
        HistoryCount = std::min(
            HistoryCount + 1,
            static_cast<int>(History.size()));
        return receipt;
    }

    SafetyEvidenceReceipt Snapshot() const noexcept { return Last; }
    SafetyEvidenceReceipt HistoryAt(int fromNewest) const noexcept
    {
        if (fromNewest < 0 || fromNewest >= HistoryCount)
        {
            return {};
        }
        const int index = (
            HistoryNext + static_cast<int>(History.size()) -
            1 - fromNewest) %
            static_cast<int>(History.size());
        return History[static_cast<std::size_t>(index)];
    }
    int StoredReceipts() const noexcept { return HistoryCount; }
    std::uint64_t AcceptedReviews() const noexcept { return Accepted; }
    std::uint64_t RejectedReviews() const noexcept { return Rejected; }

private:
    EvidenceSettings Settings{};
    SafetyEvidenceReceipt Last{};
    std::array<SafetyEvidenceReceipt, 32> History{};
    std::uint64_t Session = 0;
    std::uint64_t Actor = 0;
    std::uint64_t Accepted = 0;
    std::uint64_t Rejected = 0;
    double LastTime = 0.0;
    int HistoryCount = 0;
    int HistoryNext = 0;
    bool HasPrevious = false;
};
} // namespace latebrake::engineering

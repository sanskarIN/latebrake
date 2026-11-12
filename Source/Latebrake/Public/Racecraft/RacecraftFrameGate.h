#pragma once
#include "Racecraft/CircuitGeometry.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace latebrake::racecraft
{
struct FrameGateSettings final
{
    double MaximumGapSeconds = MaxSimulationFrameSeconds;
    double MaximumRaceClockDeviationSeconds = 0.12;
    double MaximumPlanarDisplacementCm = 2000.0;
    double MaximumVerticalDisplacementCm = 350.0;
    double MaximumSpeedDeltaCmS = 2500.0;
    double MaximumAngularStepRadians = 0.75;
    double MaximumRoadDeviationCm = 2500.0;
    double MaximumInputLatencySeconds = 0.25;
    int MinimumPhysicalContacts = 2;
    bool RequireForwardOriginalRoute = false;
    bool RequireSixCarRoster = true;
    bool RejectAnyDisplacementAcrossGate = false;
    bool ResetOnActorSwap = true;
    bool RequireWheelSensors = false;
};
struct FrameGateDecision final
{
    SnapshotFault Fault = SnapshotFault::Uninitialized;
    double DeltaSeconds = 0.0;
    double ActorTravelCm = 0.0;
    double SpeedChangeCmS = 0.0;
    double HeadingDeltaRadians = 0.0;
    double SignedTrackProgressCm = 0.0;
    double RaceClockDeltaSeconds = 0.0;
    bool NewEpoch = false;
    bool ResetRequired = false;
    bool PhysicalCarOnRoad = false;
    bool EngineTrust = false;
    bool Qualified = false;
};
struct FrameGateStatistics final
{
    std::uint64_t AcceptedFrames = 0;
    std::uint64_t RejectedFrames = 0;
    std::uint64_t NewEpochs = 0;
    std::uint64_t ActorSwaps = 0;
    std::uint64_t ClockDiscontinuities = 0;
    std::uint64_t RoadDiscontinuities = 0;
    std::uint64_t ContactFailures = 0;
    double MaximumAcceptedGapSeconds = 0.0;
    double MaximumSpeedDeltaCmS = 0.0;
    double TotalAcceptedDistanceCm = 0.0;
    double TotalAcceptedTimeSeconds = 0.0;
    double QualifiedFraction = 0.0;
    SnapshotFault MostRecentFault = SnapshotFault::Uninitialized;
    bool LastFrameAuthorized = false;
};
inline bool ValidFrameGateSettings(FrameGateSettings s) noexcept
{
    return Finite(s.MaximumGapSeconds) &&
        s.MaximumGapSeconds > 0.0 &&
        s.MaximumGapSeconds <= 2.0 &&
        Finite(s.MaximumRaceClockDeviationSeconds) &&
        s.MaximumRaceClockDeviationSeconds >= 0.0 &&
        Finite(s.MaximumPlanarDisplacementCm) &&
        s.MaximumPlanarDisplacementCm > 0.0 &&
        Finite(s.MaximumVerticalDisplacementCm) &&
        s.MaximumVerticalDisplacementCm > 0.0 &&
        Finite(s.MaximumSpeedDeltaCmS) &&
        s.MaximumSpeedDeltaCmS > 0.0 &&
        Finite(s.MaximumAngularStepRadians) &&
        s.MaximumAngularStepRadians > 0.0 &&
        Finite(s.MaximumRoadDeviationCm) &&
        s.MaximumRoadDeviationCm > 0.0 &&
        Finite(s.MaximumInputLatencySeconds) &&
        s.MaximumInputLatencySeconds >= 0.0 &&
        s.MinimumPhysicalContacts >= 0 &&
        s.MinimumPhysicalContacts <= PhysicalWheelCount;
}
inline FrameGateDecision ReviewFrame(
    const PhysicalVehicleFrame& current,
    const PhysicalVehicleFrame* prior,
    FrameGateSettings settings = {}) noexcept
{
    FrameGateDecision out;
    if (!ValidFrameGateSettings(settings)) return out;
    out.Fault = ValidateVehicleFrame(
        current, settings.RequireWheelSensors);
    if (out.Fault != SnapshotFault::None)
    {
        out.ResetRequired = true;
        return out;
    }
    out.EngineTrust = true;
    out.PhysicalCarOnRoad = CenterOnOriginalRoad(
        current.Chassis.PlanarPose.Position);
    if (!out.PhysicalCarOnRoad &&
        settings.RequireForwardOriginalRoute)
    {
        out.Fault = SnapshotFault::UnsupportedGeometry;
        out.ResetRequired = true;
        return out;
    }
    if (settings.RequireWheelSensors)
    {
        int contacts = 0;
        for (WheelObservation wheel : current.Wheels)
        {
            contacts += wheel.PhysicalContact &&
                wheel.ContactVerified ? 1 : 0;
        }
        if (contacts < settings.MinimumPhysicalContacts)
        {
            out.Fault = SnapshotFault::InvalidContact;
            out.ResetRequired = true;
            return out;
        }
    }
    if (!prior)
    {
        out.Fault = SnapshotFault::None;
        out.NewEpoch = true;
        out.Qualified = true;
        return out;
    }
    if (prior->Identity.Session != current.Identity.Session ||
        prior->Identity.SessionRevision !=
            current.Identity.SessionRevision)
    {
        out.Fault = SnapshotFault::MissingSession;
        out.NewEpoch = true;
        out.ResetRequired = true;
        return out;
    }
    if (settings.ResetOnActorSwap &&
        (prior->Identity.Actor != current.Identity.Actor ||
         prior->Identity.Slot != current.Identity.Slot))
    {
        out.Fault = SnapshotFault::DuplicateActor;
        out.ResetRequired = true;
        return out;
    }
    if (current.Identity.Frame <= prior->Identity.Frame)
    {
        out.Fault = SnapshotFault::StaleTime;
        return out;
    }
    out.DeltaSeconds =
        current.TimeSeconds - prior->TimeSeconds;
    out.RaceClockDeltaSeconds =
        current.Race.ClockSeconds - prior->Race.ClockSeconds;
    if (!Finite(out.DeltaSeconds) ||
        out.DeltaSeconds <= 0.0 ||
        out.DeltaSeconds > settings.MaximumGapSeconds)
    {
        out.Fault = SnapshotFault::StaleTime;
        out.ResetRequired = true;
        return out;
    }
    if (out.RaceClockDeltaSeconds <
            -settings.MaximumRaceClockDeviationSeconds ||
        out.RaceClockDeltaSeconds >
            out.DeltaSeconds +
            settings.MaximumRaceClockDeviationSeconds)
    {
        out.Fault = SnapshotFault::RaceClockMismatch;
        out.ResetRequired = true;
        return out;
    }
    const Vec3 from = prior->Chassis.WorldPositionCm;
    const Vec3 to = current.Chassis.WorldPositionCm;
    out.ActorTravelCm = Distance(
        {from.X, from.Y}, {to.X, to.Y});
    out.SpeedChangeCmS = std::abs(
        Length(current.Chassis.VelocityCmS) -
        Length(prior->Chassis.VelocityCmS));
    out.HeadingDeltaRadians = AngleDifference(
        prior->Chassis.PlanarPose.HeadingRadians,
        current.Chassis.PlanarPose.HeadingRadians);
    if (!Finite(out.ActorTravelCm) ||
        out.ActorTravelCm >
            settings.MaximumPlanarDisplacementCm ||
        std::abs(to.Z - from.Z) >
            settings.MaximumVerticalDisplacementCm ||
        out.SpeedChangeCmS >
            settings.MaximumSpeedDeltaCmS ||
        std::abs(out.HeadingDeltaRadians) >
            settings.MaximumAngularStepRadians)
    {
        out.Fault = SnapshotFault::InvalidSpeed;
        out.ResetRequired = true;
        return out;
    }
    const double previousSpeed =
        Length(prior->Chassis.VelocityCmS);
    const double measuredSpeed =
        Length(current.Chassis.VelocityCmS);
    const double inferredSpeed =
        SafeDivision(out.ActorTravelCm,
                     out.DeltaSeconds);
    if (inferredSpeed > MaximumPhysicalSpeedCmS ||
        inferredSpeed > std::max(previousSpeed,
                                measuredSpeed) + 2000.0)
    {
        out.Fault = SnapshotFault::InvalidSpeed;
        out.ResetRequired = true;
        return out;
    }
    const auto projection = ProjectOriginalCircuit(
        current.Chassis.PlanarPose.Position,
        current.Chassis.PlanarPose.HeadingRadians);
    const auto previousProjection = ProjectOriginalCircuit(
        prior->Chassis.PlanarPose.Position,
        prior->Chassis.PlanarPose.HeadingRadians);
    if (!projection.Valid || !previousProjection.Valid ||
        projection.CenterlineDistanceCm >
            settings.MaximumRoadDeviationCm)
    {
        out.Fault = SnapshotFault::UnsupportedGeometry;
        out.ResetRequired = true;
        return out;
    }
    out.SignedTrackProgressCm = SignedShortestLapDistance(
        previousProjection.DistanceAlongLapCm,
        projection.DistanceAlongLapCm,
        OriginalCircuitLapCm);
    if (settings.RequireForwardOriginalRoute &&
        out.SignedTrackProgressCm < -75.0)
    {
        out.Fault = SnapshotFault::UnsupportedGeometry;
        return out;
    }
    if (settings.RejectAnyDisplacementAcrossGate &&
        current.Track.NextGate != prior->Track.NextGate)
    {
        out.Fault = SnapshotFault::UnsupportedGeometry;
        return out;
    }
    out.Fault = SnapshotFault::None;
    out.Qualified = true;
    return out;
}
class RacecraftFrameGate final
{
public:
    bool Configure(FrameGateSettings settings) noexcept
    {
        if (!ValidFrameGateSettings(settings)) return false;
        Settings = settings;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Last = {};
        HasLast = false;
        Receipt = {};
    }
    FrameGateDecision Observe(const PhysicalVehicleFrame& frame) noexcept
    {
        const FrameGateDecision decision = ReviewFrame(
            frame, HasLast ? &Last : nullptr, Settings);
        Receipt.MostRecentFault = decision.Fault;
        Receipt.LastFrameAuthorized = decision.Qualified;
        if (decision.Qualified)
        {
            if (decision.NewEpoch) ++Receipt.NewEpochs;
            ++Receipt.AcceptedFrames;
            Receipt.TotalAcceptedTimeSeconds +=
                decision.DeltaSeconds;
            Receipt.TotalAcceptedDistanceCm +=
                decision.ActorTravelCm;
            Receipt.MaximumAcceptedGapSeconds = std::max(
                Receipt.MaximumAcceptedGapSeconds,
                decision.DeltaSeconds);
            Receipt.MaximumSpeedDeltaCmS = std::max(
                Receipt.MaximumSpeedDeltaCmS,
                decision.SpeedChangeCmS);
            Last = frame;
            HasLast = true;
        }
        else
        {
            ++Receipt.RejectedFrames;
            if (decision.Fault == SnapshotFault::StaleTime ||
                decision.Fault == SnapshotFault::RaceClockMismatch)
                ++Receipt.ClockDiscontinuities;
            if (decision.Fault == SnapshotFault::UnsupportedGeometry)
                ++Receipt.RoadDiscontinuities;
            if (decision.Fault == SnapshotFault::InvalidContact)
                ++Receipt.ContactFailures;
            if (decision.Fault == SnapshotFault::DuplicateActor)
                ++Receipt.ActorSwaps;
            if (decision.ResetRequired) HasLast = false;
        }
        const double total = static_cast<double>(
            Receipt.AcceptedFrames + Receipt.RejectedFrames);
        Receipt.QualifiedFraction = SafeDivision(
            static_cast<double>(Receipt.AcceptedFrames), total);
        return decision;
    }
    FrameGateStatistics Snapshot() const noexcept { return Receipt; }
    bool HasBaseline() const noexcept { return HasLast; }
    PhysicalVehicleFrame LastFrame() const noexcept
    {
        return HasLast ? Last : PhysicalVehicleFrame{};
    }
    void Suspend() noexcept
    {
        HasLast = false;
        Receipt.LastFrameAuthorized = false;
    }
private:
    FrameGateSettings Settings{};
    PhysicalVehicleFrame Last{};
    FrameGateStatistics Receipt{};
    bool HasLast = false;
};
struct FleetFrameGateReceipt final
{
    std::array<FrameGateDecision, PhysicalFleetCapacity> Decisions{};
    SnapshotFault FirstFault = SnapshotFault::Uninitialized;
    int FaultSlot = -1;
    bool CompleteSixCarRoster = false;
    bool AllQualified = false;
    bool ResetForChangedEpoch = false;
    int PhysicalContacts = 0;
};
class FleetFrameGate final
{
public:
    bool Configure(FrameGateSettings settings) noexcept
    {
        if (!ValidFrameGateSettings(settings)) return false;
        Settings = settings;
        Reset();
        for (RacecraftFrameGate& gate : Gates)
            gate.Configure(settings);
        return true;
    }
    void Reset() noexcept
    {
        for (RacecraftFrameGate& gate : Gates) gate.Reset();
        Session = 0;
        Revision = 0;
        HasEpoch = false;
        Last = {};
    }
    FleetFrameGateReceipt Observe(const FleetFrame& fleet) noexcept
    {
        FleetFrameGateReceipt out;
        const SnapshotFault valid = ValidateFleetFrame(
            fleet, Settings.RequireSixCarRoster);
        if (valid != SnapshotFault::None)
        {
            out.FirstFault = valid;
            Reset();
            return out;
        }
        out.CompleteSixCarRoster =
            fleet.Count == PhysicalFleetCapacity;
        if (HasEpoch && (fleet.Session != Session ||
                         fleet.Revision != Revision))
        {
            Reset();
            out.ResetForChangedEpoch = true;
        }
        Session = fleet.Session;
        Revision = fleet.Revision;
        HasEpoch = true;
        for (int i = 0; i < fleet.Count; ++i)
        {
            FrameGateDecision decision =
                Gates[static_cast<std::size_t>(i)].Observe(
                    fleet.Members[static_cast<std::size_t>(i)]);
            out.Decisions[static_cast<std::size_t>(i)] = decision;
            if (!decision.Qualified && out.FaultSlot < 0)
            {
                out.FaultSlot = i;
                out.FirstFault = decision.Fault;
            }
            for (WheelObservation wheel :
                 fleet.Members[static_cast<std::size_t>(i)].Wheels)
                out.PhysicalContacts += wheel.ContactVerified &&
                    wheel.PhysicalContact ? 1 : 0;
        }
        out.AllQualified = out.FaultSlot < 0;
        if (out.AllQualified) out.FirstFault = SnapshotFault::None;
        Last = out;
        return out;
    }
    FleetFrameGateReceipt Snapshot() const noexcept { return Last; }
private:
    std::array<RacecraftFrameGate, PhysicalFleetCapacity> Gates{};
    FrameGateSettings Settings{};
    FleetFrameGateReceipt Last{};
    std::uint64_t Session = 0;
    std::uint32_t Revision = 0;
    bool HasEpoch = false;
};
} // namespace latebrake::racecraft

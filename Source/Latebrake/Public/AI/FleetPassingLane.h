#pragma once

#include "AI/FleetLineAim.h"
#include "AI/RivalFleetCollisionForecast.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numbers>

namespace latebrake
{
// Virtual race steering proposal only. Existing route, brakes, physical
// traffic and spin constraints remain downstream and authoritative.
inline constexpr double FleetPassLaneCm = 440.0;
inline constexpr double FleetPassMinimumStraightCm = 5000.0;
inline constexpr double FleetPassEntryRunwayCm = 4000.0;
inline constexpr double FleetPassExitRunwayCm = 2000.0;
inline constexpr double FleetPassStartGapMinCm = 1600.0;
inline constexpr double FleetPassStartGapMaxCm = 2400.0;
inline constexpr double FleetPassMinClosingCmS = 220.0;
inline constexpr double FleetPassMaxClosingCmS = 700.0;
inline constexpr double FleetPassMinForwardCmS = 850.0;
inline constexpr double FleetPassMaxForwardCmS = 2400.0;
inline constexpr double FleetPassMaxSideSpeedCmS = 350.0;
inline constexpr double FleetPassMinimumLeaderForwardCmS = 250.0;
inline constexpr double FleetPassMaxYawError = 18.0;
inline constexpr double FleetPassLookaheadCm = 1200.0;
inline constexpr double FleetPassForecastSeconds = 1.8;
inline constexpr double FleetPassMaxDurationSeconds = 4.0;
inline constexpr double FleetPassMarginCm = 60.0;
inline constexpr double FleetPassBehindClearanceCm = 650.0;
inline constexpr double FleetPassMaxSteer = 0.45;
inline constexpr double FleetPassConfirmSeconds = 0.15;
inline constexpr double FleetPassEpsilon = 1e-9;

enum class FleetPassPhase
{
    Idle,
    Confirming,
    Shifting,
    Holding,
    Retired
};

enum class FleetPassReason
{
    None,
    InvalidObservation,
    PriorBrake,
    UnsupportedStraight,
    NoLeader,
    NoClearLane,
    ChangedLeader,
    CorridorOccupied,
    RunwayEnded,
    PassedLeader,
    Timeout,
    TrafficBrake,
    Interrupted
};

// GameMode owns attempt and frame authority; a rival observes one actual
// game-thread physics frame. Teleport and session stamps break continuity
// even if a recovered vehicle happens to remain in its previous lane.
struct FleetPassStamp final
{
    std::uint64_t Attempt = 0;
    std::uint64_t Frame = 0;
    std::uint64_t EngineFrame = 0;
    std::uint64_t SessionEpoch = 0;
    std::uint64_t TeleportRevision = 0;

    bool Valid() const noexcept
    {
        return Attempt != 0 && Frame != 0 && EngineFrame != 0;
    }
};

inline bool FleetPassFresh(const FleetPassStamp& previous,
    const FleetPassStamp& current) noexcept
{
    return previous.Valid() && current.Valid() &&
        current.Attempt == previous.Attempt &&
        current.Frame > previous.Frame &&
        current.EngineFrame > previous.EngineFrame;
}

inline bool FleetPassContinuous(const FleetPassStamp& previous,
    const FleetPassStamp& current) noexcept
{
    return FleetPassFresh(previous,current) &&
        current.Frame-previous.Frame == 1 &&
        current.EngineFrame-previous.EngineFrame == 1 &&
        current.SessionEpoch == previous.SessionEpoch &&
        current.TeleportRevision == previous.TeleportRevision;
}

struct FleetPassSample final
{
    int GateOrdinal = -1;
    int NextGate = -1;
    double DeltaSeconds = 0.0;
    bool QualifiedRoad = false;
    RivalFleetTrafficSample Traffic{};
    FleetPassStamp Stamp{};
};

struct FleetPassLocalBody final
{
    int Slot = -1;
    std::uint64_t Token = 0;
    double Along = 0.0;
    double Side = 0.0;
    double Forward = 0.0;
    double Lateral = 0.0;
    double HalfAlong = 0.0;
    double HalfSide = 0.0;
    double YawError = 0.0;
};

struct FleetPassGeometry final
{
    bool Valid = false;
    FleetIncomingStraight Straight{};
    FleetPassLocalBody Self{};
    std::array<FleetPassLocalBody, MaxRecoveryObstacles> Others{};
    std::array<std::uint64_t, StartGridCapacity> Roster{};
};

struct FleetPassLaneEvidence final
{
    bool Valid = false;
    bool Clear = false;
    int BlockerSlot = -1;
    double LaneCm = 0.0;
    double FirstConflictSeconds = 0.0;
};

struct FleetPassReadout final
{
    FleetPassPhase Phase = FleetPassPhase::Idle;
    FleetPassReason Reason = FleetPassReason::None;
    int GateOrdinal = -1;
    int LeaderSlot = -1;
    std::uint64_t LeaderToken = 0;
    double LaneCm = 0.0;
    double ConfirmedSeconds = 0.0;
    double ActiveSeconds = 0.0;
    FleetPassLaneEvidence LaneEvidence{};
    PilotCommand Command{};
    bool SteeringChanged = false;
};

inline bool ValidFleetPassCommand(const PilotCommand& command) noexcept
{
    return std::isfinite(command.Throttle) && command.Throttle >= 0.0 &&
        command.Throttle <= 1.0 && std::isfinite(command.Steering) &&
        std::abs(command.Steering) <= 1.0;
}

inline FleetPassLocalBody FleetPassProjectBody(const RecoveryObstacle& body,
    const FleetIncomingStraight& straight) noexcept
{
    const auto rect = MakeStartGridRect(body.Pose);
    return {body.Slot, body.ActorToken,
        FleetLaneAlong(straight, body.Pose.Position),
        FleetLaneLateral(straight, body.Pose.Position),
        body.VelocityX * straight.Forward.X + body.VelocityY * straight.Forward.Y,
        body.VelocityX * straight.Right.X + body.VelocityY * straight.Right.Y,
        StartGridProjectionRadius(rect, straight.Forward.X, straight.Forward.Y),
        StartGridProjectionRadius(rect, straight.Right.X, straight.Right.Y),
        std::abs(std::remainder(body.Pose.HeadingDegrees - straight.Heading, 360.0))};
}

inline FleetPassGeometry InspectFleetPassGeometry(const FleetPassSample& sample) noexcept
{
    FleetPassGeometry out;
    if (!sample.QualifiedRoad || !IsCircuitGate(sample.NextGate) ||
        sample.GateOrdinal < 0 || sample.GateOrdinal >= 25600 ||
        sample.GateOrdinal % CircuitGateCount != sample.NextGate ||
        !std::isfinite(sample.DeltaSeconds) || sample.DeltaSeconds <= 0.0 ||
        sample.DeltaSeconds > 0.10)
        return out;
    // All actual blockers must be present; degraded off-road context can
    // still use ordinary traffic braking but cannot begin a lane change.
    const auto context = InspectFleetRoadContext(sample.Traffic,
        FleetForecastRoadPolicy::StrictAllBodies);
    if (!context.Valid)
        return out;
    out.Straight = DescribeFleetIncomingStraight(sample.NextGate);
    out.Roster = context.Roster;
    out.Self = FleetPassProjectBody(sample.Traffic.Self, out.Straight);
    for (int i = 0; i < MaxRecoveryObstacles; ++i)
        out.Others[static_cast<std::size_t>(i)] =
            FleetPassProjectBody(sample.Traffic.Blockers[static_cast<std::size_t>(i)],
                out.Straight);
    out.Valid = true;
    return out;
}

inline bool FleetPassStraightQualified(const FleetPassGeometry& geometry,
    const RecoveryObstacle& actual, bool entry) noexcept
{
    if (!geometry.Valid)
        return false;
    const auto& straight = geometry.Straight;
    const auto& self = geometry.Self;
    const double runway = entry ? FleetPassEntryRunwayCm : FleetPassExitRunwayCm;
    return straight.LengthCm >= FleetPassMinimumStraightCm &&
        self.Along >= -straight.LengthCm + 100.0 &&
        self.Along <= -runway &&
        std::abs(self.Side) <= FleetPassLaneCm + 70.0 &&
        self.YawError <= FleetPassMaxYawError &&
        self.Forward >= FleetPassMinForwardCmS &&
        self.Forward <= FleetPassMaxForwardCmS &&
        std::abs(self.Lateral) <= FleetPassMaxSideSpeedCmS &&
        FleetLanePoseFitsSlab(actual.Pose, straight.Slab) &&
        FleetLanePoseFitsSlab({actual.Pose.Position, straight.Heading}, straight.Slab);
}

inline const FleetPassLocalBody* FleetPassFindSlot(const FleetPassGeometry& geometry,
    int slot, std::uint64_t token) noexcept
{
    for (const auto& other : geometry.Others)
        if (other.Slot == slot && other.Token == token)
            return &other;
    return nullptr;
}

inline bool FleetPassLeaderAligned(const FleetPassLocalBody& leader) noexcept
{
    return leader.YawError <= FleetPassMaxYawError &&
        leader.Forward >= FleetPassMinimumLeaderForwardCmS &&
        std::abs(leader.Lateral) <= FleetPassMaxSideSpeedCmS;
}

inline const FleetPassLocalBody* FindFleetPassLeader(const FleetPassGeometry& geometry) noexcept
{
    if (!geometry.Valid)
        return nullptr;
    const FleetPassLocalBody* selected = nullptr;
    const auto& self = geometry.Self;
    for (const auto& other : geometry.Others)
    {
        const double gap = other.Along - self.Along;
        const double closing = self.Forward - other.Forward;
        if (!FleetPassLeaderAligned(other) ||
            gap < FleetPassStartGapMinCm || gap > FleetPassStartGapMaxCm ||
            other.Along > -500.0 ||
            closing < FleetPassMinClosingCmS || closing > FleetPassMaxClosingCmS ||
            std::abs(other.Side - self.Side) > self.HalfSide + other.HalfSide + 30.0 ||
            (gap - self.HalfAlong - other.HalfAlong - FleetPassMarginCm) /
                closing < FleetPassForecastSeconds)
            continue;
        if (!selected || other.Along < selected->Along ||
            (other.Along == selected->Along && other.Slot < selected->Slot))
            selected = &other;
    }
    return selected;
}

struct FleetPassWindow final
{
    bool Intersects = false;
    double FirstSeconds = 0.0;
};

// Exact intersection of two constant-velocity axis time intervals. A
// body crossing the lateral strip between endpoints is not missed.
inline bool ClipFleetPassAxis(double offset, double velocity, double extent,
    double& first, double& last) noexcept
{
    if (std::abs(velocity) < FleetPassEpsilon)
        return std::abs(offset) <= extent;
    const double a = (-extent - offset) / velocity;
    const double b = (extent - offset) / velocity;
    first = std::max(first, std::min(a,b));
    last = std::min(last, std::max(a,b));
    return first <= last;
}

inline FleetPassWindow FleetPassSweptBox(double along, double side,
    double forward, double lateral, double halfAlong, double halfSide,
    double horizon) noexcept
{
    if (!std::isfinite(along) || !std::isfinite(side) ||
        !std::isfinite(forward) || !std::isfinite(lateral) ||
        !std::isfinite(halfAlong) || !std::isfinite(halfSide) ||
        !std::isfinite(horizon) || halfAlong < 0.0 || halfSide < 0.0 ||
        horizon <= 0.0)
        return {true,0.0};
    double first = 0.0, last = horizon;
    if (!ClipFleetPassAxis(along,forward,halfAlong,first,last) ||
        !ClipFleetPassAxis(side,lateral,halfSide,first,last))
        return {};
    return {true,first};
}

// The whole lateral sweep is reserved; a longitudinal forecast cannot
// certify future yaw or acceleration. This is only a conservative veto.
inline FleetPassLaneEvidence InspectFleetPassLane(const FleetPassGeometry& geometry,
    const RecoveryObstacle& actual, double lane) noexcept
{
    FleetPassLaneEvidence out;
    if (!geometry.Valid || !std::isfinite(lane) ||
        std::abs(lane) != FleetPassLaneCm)
        return out;
    out.LaneCm = lane;
    const auto& self = geometry.Self;
    const auto& straight = geometry.Straight;
    const double travel = self.Forward * FleetPassForecastSeconds;
    const double endAlong = self.Along + travel;
    if (endAlong > -300.0 || travel <= 0.0)
        return out;
    // Include the measured lateral drift as well as the proposed lane.
    // A steering command cannot instantaneously cancel existing side
    // velocity. Reserve its full constant-velocity lateral envelope.
    const double driftEnd = self.Side + self.Lateral*FleetPassForecastSeconds;
    const double lower = std::min({self.Side,lane,driftEnd});
    const double upper = std::max({self.Side,lane,driftEnd});
    const double middle = (lower+upper)*0.5;
    const double sweepHalf = (upper-lower)*0.5;
    if (!FleetLanePoseFitsSlab(actual.Pose, straight.Slab))
        return out;
    // All corners of the bounded translation envelope must fit the
    // same convex authored slab, with both relevant fixed headings.
    for (const double along:{self.Along,endAlong})
    {
        for (const double side:{lower,upper})
        {
            const auto point = FleetLaneWorld(straight,along,side);
            if (!FleetLanePoseFitsSlab({point,actual.Pose.HeadingDegrees},straight.Slab) ||
                !FleetLanePoseFitsSlab({point,straight.Heading},straight.Slab))
                return out;
        }
    }
    out.Valid = true;
    double firstThreat = std::numeric_limits<double>::infinity();
    for (const auto& other : geometry.Others)
    {
        const double along = other.Along - self.Along;
        const double side = other.Side - middle;
        const double alongExtent = self.HalfAlong + other.HalfAlong + FleetPassMarginCm;
        const double sideExtent = self.HalfSide + other.HalfSide + sweepHalf + FleetPassMarginCm;
        const auto hit = FleetPassSweptBox(along, side,
            other.Forward-self.Forward, other.Lateral,
            alongExtent, sideExtent, FleetPassForecastSeconds);
        if (hit.Intersects && (hit.FirstSeconds < firstThreat ||
            (hit.FirstSeconds == firstThreat && other.Slot < out.BlockerSlot)))
        {
            firstThreat = hit.FirstSeconds;
            out.BlockerSlot = other.Slot;
        }
    }
    out.Clear = out.BlockerSlot < 0;
    out.FirstConflictSeconds = out.Clear ? 0.0 : firstThreat;
    return out;
}

inline FleetPassLaneEvidence SelectFleetPassLane(const FleetPassGeometry& geometry,
    const RecoveryObstacle& actual, const FleetPassLocalBody& leader) noexcept
{
    const auto left = InspectFleetPassLane(geometry, actual, -FleetPassLaneCm);
    const auto right = InspectFleetPassLane(geometry, actual, FleetPassLaneCm);
    const auto usable = [&](const FleetPassLaneEvidence& e)
    {
        return e.Valid && e.Clear &&
            std::abs(e.LaneCm-leader.Side) >=
                geometry.Self.HalfSide+leader.HalfSide+FleetPassMarginCm;
    };
    const bool l = usable(left), r = usable(right);
    if (!l && !r)
        return {};
    if (l && r)
    {
        if (std::abs(left.LaneCm-leader.Side) != std::abs(right.LaneCm-leader.Side))
            return std::abs(left.LaneCm-leader.Side) > std::abs(right.LaneCm-leader.Side)
                ? left : right;
        return FleetLanePreference(geometry.Self.Slot) < 0.0 ? left : right;
    }
    return l ? left : right;
}

class FleetPassingLane final
{
public:
    void Reset() noexcept
    {
        State = {};
        Identity = {};
        HaveIdentity = false;
        LastStamp = {};
    }

    void Suspend() noexcept
    {
        if (InProgress())
            Retire(FleetPassReason::Interrupted);
        State.SteeringChanged = false;
        State.Command = {};
    }

    void AbortTraffic() noexcept
    {
        if (InProgress())
            Retire(FleetPassReason::TrafficBrake);
        State.Command = {};
    }

    FleetPassReadout Readout() const noexcept { return State; }

    FleetPassReadout Apply(PilotCommand original, const FleetPassSample& sample) noexcept
    {
        State.Command = original;
        State.SteeringChanged = false;
        State.LaneEvidence = {};
        if (!ValidFleetPassCommand(original))
        {
            Suspend();
            State.Reason = FleetPassReason::InvalidObservation;
            return State;
        }
        if (!sample.Stamp.Valid() ||
            (LastStamp.Valid() && !FleetPassFresh(LastStamp,sample.Stamp)))
        {
            Suspend();
            State.Command = original;
            State.Reason = FleetPassReason::InvalidObservation;
            return State;
        }
        if (LastStamp.Valid() && !FleetPassContinuous(LastStamp,sample.Stamp))
        {
            Suspend();
            State.Command = original;
        }
        // Consume each fresh stamp once even when the geometry below
        // is refused. A repeated frame cannot accrue confirmation time.
        LastStamp = sample.Stamp;
        const auto geometry = InspectFleetPassGeometry(sample);
        if (!geometry.Valid || sample.GateOrdinal < State.GateOrdinal ||
            (HaveIdentity && Identity != geometry.Roster))
        {
            Suspend();
            State.Command = original;
            State.Reason = FleetPassReason::InvalidObservation;
            return State;
        }
        Identity = geometry.Roster;
        HaveIdentity = true;
        if (sample.GateOrdinal > State.GateOrdinal)
        {
            State = {};
            State.GateOrdinal = sample.GateOrdinal;
            State.Command = original;
        }
        if (State.Phase == FleetPassPhase::Retired)
            return State;
        if (original.Brake || original.Arrived)
        {
            if (InProgress()) Retire(FleetPassReason::PriorBrake);
            else State.Reason = FleetPassReason::PriorBrake;
            return State;
        }
        const bool entry = State.Phase == FleetPassPhase::Idle ||
            State.Phase == FleetPassPhase::Confirming;
        if (!FleetPassStraightQualified(geometry, sample.Traffic.Self, entry))
        {
            if (InProgress()) Retire(FleetPassReason::RunwayEnded);
            else State.Reason = FleetPassReason::UnsupportedStraight;
            return State;
        }
        if (State.Phase == FleetPassPhase::Idle)
        {
            const auto* leader = FindFleetPassLeader(geometry);
            if (!leader)
            {
                State.Reason = FleetPassReason::NoLeader;
                return State;
            }
            const auto lane = SelectFleetPassLane(geometry,sample.Traffic.Self,*leader);
            if (!lane.Clear)
            {
                State.Reason = FleetPassReason::NoClearLane;
                return State;
            }
            State.Phase = FleetPassPhase::Confirming;
            State.Reason = FleetPassReason::None;
            State.LeaderSlot = leader->Slot;
            State.LeaderToken = leader->Token;
            State.LaneCm = lane.LaneCm;
        }

        const auto* leader = FleetPassFindSlot(geometry,State.LeaderSlot,State.LeaderToken);
        if (!leader || !FleetPassLeaderAligned(*leader))
        {
            Retire(FleetPassReason::ChangedLeader);
            return State;
        }
        const double gap = leader->Along - geometry.Self.Along;
        if (gap <= -FleetPassBehindClearanceCm)
        {
            Retire(FleetPassReason::PassedLeader);
            return State;
        }
        if (gap > FleetPassStartGapMaxCm + 400.0)
        {
            Retire(FleetPassReason::ChangedLeader);
            return State;
        }
        if (State.Phase == FleetPassPhase::Confirming)
        {
            const auto* fresh = FindFleetPassLeader(geometry);
            if (!fresh || fresh->Token != State.LeaderToken)
            {
                Retire(FleetPassReason::ChangedLeader);
                return State;
            }
        }
        State.LaneEvidence = InspectFleetPassLane(geometry,sample.Traffic.Self,State.LaneCm);
        if (!State.LaneEvidence.Valid || !State.LaneEvidence.Clear)
        {
            Retire(FleetPassReason::CorridorOccupied);
            return State;
        }
        if (State.Phase == FleetPassPhase::Confirming)
        {
            State.ConfirmedSeconds = std::min(FleetPassConfirmSeconds,
                State.ConfirmedSeconds + sample.DeltaSeconds);
            if (State.ConfirmedSeconds + FleetPassEpsilon < FleetPassConfirmSeconds)
                return State;
            State.Phase = FleetPassPhase::Shifting;
        }
        State.ActiveSeconds += sample.DeltaSeconds;
        if (State.ActiveSeconds >= FleetPassMaxDurationSeconds)
        {
            Retire(FleetPassReason::Timeout);
            return State;
        }
        const auto& self = geometry.Self;
        const auto& straight = geometry.Straight;
        const auto aim = FleetLaneWorld(straight,
            self.Along + FleetPassLookaheadCm, State.LaneCm);
        if (!FleetLanePoseFitsSlab({aim,straight.Heading},straight.Slab))
        {
            Retire(FleetPassReason::RunwayEnded);
            return State;
        }
        const double heading = sample.Traffic.Self.Pose.HeadingDegrees *
            std::numbers::pi / 180.0;
        const auto position = sample.Traffic.Self.Pose.Position;
        const double desired = std::atan2(aim.Y-position.Y,aim.X-position.X);
        const double error = std::remainder(desired-heading,2.0*std::numbers::pi);
        State.Command.Steering = std::clamp(error/0.82,-FleetPassMaxSteer,FleetPassMaxSteer);
        State.SteeringChanged = true;
        State.Phase = std::abs(self.Side-State.LaneCm) <= 60.0
            ? FleetPassPhase::Holding : FleetPassPhase::Shifting;
        return State;
    }

private:
    bool InProgress() const noexcept
    {
        return State.Phase == FleetPassPhase::Confirming ||
            State.Phase == FleetPassPhase::Shifting ||
            State.Phase == FleetPassPhase::Holding;
    }

    void Retire(FleetPassReason reason) noexcept
    {
        State.Phase = FleetPassPhase::Retired;
        State.Reason = reason;
        State.SteeringChanged = false;
    }

    FleetPassReadout State{};
    std::array<std::uint64_t,StartGridCapacity> Identity{};
    bool HaveIdentity = false;
    FleetPassStamp LastStamp{};
};
} // namespace latebrake

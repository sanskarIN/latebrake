#pragma once

#include "AI/ExperimentalFleetLiveRecovery.h"
#include "AI/GridRivalLedger.h"
#include "AI/FleetGateTimingJournal.h"
#include "Racing/RivalFinishClockReview.h"
#include "Track/RaceMiniMap.h"

#include <array>
#include <cmath>
#include <cstdint>

namespace latebrake
{
static_assert(StartGridCapacity == FleetTimingCarCount);
static_assert(GridRivalLedger::MaxGateCount == FleetTimingMaxGates);
static_assert(GridRivalLedger::MaxRaceLaps == FleetTimingMaxLaps);
static_assert(RivalFinishClockReview::NearFinishToleranceSeconds == FleetTimingUncertainSeconds);
// Advisory evidence only. Neither discrete gate order, sampled callback
// times nor map coordinates establish an official classification.
struct FleetRouteObservation final
{
    bool Verified=false;
    int CompletedLaps=0;
    int NextGate=0;
    int RecoveryAttempts=0;
    int SuccessfulRecoveries=0;
    int FailedRecoveries=0;
    bool Finished=false;
};

enum class FleetGateRelation
{
    Unavailable,
    FewerAcceptedGates,
    SameAcceptedGates,
    MoreAcceptedGates
};

enum class FleetFinishRelation
{
    Unavailable,
    RouteIncompleteAtPlayerFinish,
    EarlierObserved,
    SameClockWindow
};

struct FleetRivalReadout final
{
    int Slot=-1;
    bool BodyObserved=false;
    bool RouteEvidenceValid=false;
    RadarMarker Marker{};
    int CompletedLaps=0;
    int NextGate=-1;
    int AcceptedGates=0;
    int RecoveryAttempts=0;
    int SuccessfulRecoveries=0;
    int FailedRecoveries=0;
    bool RouteFinished=false;
    double LastGateActiveSeconds=0.0;
    FleetGateRelation GateRelation=FleetGateRelation::Unavailable;
    FleetFinishRelation FinishRelation=FleetFinishRelation::Unavailable;
    double EarlierObservedSeconds=0.0;
};

struct FleetRaceReview final
{
    bool ValidRoster=false;
    bool PlayerRouteValid=false;
    bool PlayerFinished=false;
    int RequiredLaps=0;
    int GateCount=0;
    int PlayerAcceptedGates=0;
    int ComparedRivals=0;
    int MoreGates=0;
    int SameGates=0;
    int FewerGates=0;
    int UnavailableRivals=0;
    int ObservedCompletedRoutes=0;
    double PlayerActiveSeconds=0.0;
    std::array<FleetRivalReadout,StartGridCapacity-1> Rivals{};
    FleetTimingReadout Timing{};
};

inline bool FleetRouteOrdinal(int laps,int nextGate,bool finished,
    int gateCount,int requiredLaps,int& out) noexcept
{
    out=0;
    if (gateCount<1 || gateCount>GridRivalLedger::MaxGateCount ||
        requiredLaps<1 || requiredLaps>GridRivalLedger::MaxRaceLaps ||
        laps<0 || laps>requiredLaps || nextGate<0 || nextGate>=gateCount ||
        finished!=(laps==requiredLaps) || (finished && nextGate!=0))
        return false;
    out=laps*gateCount+nextGate;
    return true;
}

// Unlike the original single-rival radar, a vehicle beyond the visible
// course bounds is omitted rather than falsely placed at a screen edge.
inline RadarMarker MakeFleetRadarMarker(CircuitPoint p) noexcept
{
    if (!std::isfinite(p.X) || !std::isfinite(p.Y) ||
        std::abs(p.X)>4200.0 || std::abs(p.Y)>3000.0)
        return {};
    const auto map=NormalizeCircuitMap(p);
    return {true,map.U,map.V};
}

inline bool FleetEvidenceMatchesRoute(const GridRivalStatus& ledger,
    const FleetRouteObservation& route,std::uint64_t token,
    int slot,int gateCount,int requiredLaps,double activeSeconds,int& ordinal) noexcept
{
    ordinal=0;
    if (!ledger.Bound || ledger.Slot!=slot || ledger.ActorToken!=token ||
        !route.Verified ||
        !FleetRouteOrdinal(route.CompletedLaps,route.NextGate,route.Finished,
            gateCount,requiredLaps,ordinal) ||
        ledger.LastGateOrdinal!=ordinal || ledger.Finished!=route.Finished ||
        route.RecoveryAttempts<0 || route.RecoveryAttempts>GridRivalLedger::MaxRecoveryAttempts ||
        route.SuccessfulRecoveries<0 || route.SuccessfulRecoveries>route.RecoveryAttempts ||
        route.FailedRecoveries!=route.RecoveryAttempts-route.SuccessfulRecoveries ||
        route.RecoveryAttempts!=ledger.RecoveryAttempts ||
        route.SuccessfulRecoveries!=ledger.SuccessfulRecoveries ||
        route.FailedRecoveries!=ledger.FailedRecoveries)
        return false;
    if (ordinal==0) return ledger.LastGateSeconds==0.0;
    return std::isfinite(ledger.LastGateSeconds) &&
        ledger.LastGateSeconds>0.0 && ledger.LastGateSeconds<=activeSeconds;
}

// Bind every actual AI or bind none. No old ledger entry can leak across
// an accepted restart, partially created fleet or new actor generation.
inline bool BindFleetReviewLedger(GridRivalLedger& ledger,
    const SixCarIdentity& identity,int gates,int laps) noexcept
{
    GridRivalLedger candidate;
    if (identity[0]==0 || !candidate.Configure(gates,laps))
    {
        ledger.Reset();
        return false;
    }
    for(int slot=1;slot<StartGridCapacity;++slot)
    {
        const auto token=identity[static_cast<std::size_t>(slot)];
        if (token==identity[0] || !candidate.Bind(slot,token))
        {
            ledger.Reset();
            return false;
        }
    }
    ledger=candidate;
    return true;
}

inline FleetRaceReview BuildFleetRaceReview(const FleetPhysicalFrame& live,
    const SixCarIdentity& bound,const GridRivalLedger& ledger,
    const std::array<FleetRouteObservation,StartGridCapacity>& routes,
    int playerLaps,int playerNext,bool playerFinished,int gateCount,
    int requiredLaps,double activeSeconds) noexcept
{
    FleetRaceReview out;
    if (live.Inactive || !MatchLiveFleetIdentity(live,bound) ||
        !ledger.IsConfigured() || ledger.GateCount()!=gateCount ||
        gateCount<1 || gateCount>GridRivalLedger::MaxGateCount ||
        requiredLaps<1 || requiredLaps>GridRivalLedger::MaxRaceLaps ||
        ledger.TotalGateCount()!=gateCount*requiredLaps ||
        !std::isfinite(activeSeconds) || activeSeconds<0.0 ||
        activeSeconds>GridRivalLedger::MaxObservedSeconds)
        return out;

    out.ValidRoster=true;
    out.GateCount=gateCount;
    out.RequiredLaps=requiredLaps;
    out.PlayerFinished=playerFinished;
    out.PlayerActiveSeconds=activeSeconds;
    out.PlayerRouteValid=FleetRouteOrdinal(playerLaps,playerNext,playerFinished,
        gateCount,requiredLaps,out.PlayerAcceptedGates) &&
        (out.PlayerAcceptedGates==0 || activeSeconds>0.0);

    for(int slot=1;slot<StartGridCapacity;++slot)
    {
        const auto index=static_cast<std::size_t>(slot);
        auto& row=out.Rivals[index-1];
        const auto& observation=routes[index];
        const auto status=ledger.Snapshot(slot);
        row.Slot=slot;
        row.BodyObserved=true;
        row.Marker=MakeFleetRadarMarker(live.Cars[index].Pose.Position);
        int ordinal=0;
        if (!FleetEvidenceMatchesRoute(status,observation,bound[index],
                slot,gateCount,requiredLaps,activeSeconds,ordinal))
        {
            ++out.UnavailableRivals;
            continue;
        }
        row.RouteEvidenceValid=true;
        row.CompletedLaps=observation.CompletedLaps;
        row.NextGate=observation.Finished?-1:observation.NextGate;
        row.AcceptedGates=ordinal;
        row.RouteFinished=observation.Finished;
        row.RecoveryAttempts=status.RecoveryAttempts;
        row.SuccessfulRecoveries=status.SuccessfulRecoveries;
        row.FailedRecoveries=status.FailedRecoveries;
        row.LastGateActiveSeconds=status.LastGateSeconds;
        if (row.RouteFinished) ++out.ObservedCompletedRoutes;

        if (!out.PlayerRouteValid) continue;
        ++out.ComparedRivals;
        if (ordinal>out.PlayerAcceptedGates)
        {
            row.GateRelation=FleetGateRelation::MoreAcceptedGates;
            ++out.MoreGates;
        }
        else if (ordinal<out.PlayerAcceptedGates)
        {
            row.GateRelation=FleetGateRelation::FewerAcceptedGates;
            ++out.FewerGates;
        }
        else
        {
            row.GateRelation=FleetGateRelation::SameAcceptedGates;
            ++out.SameGates;
        }

        if (!playerFinished) continue;
        if (!row.RouteFinished)
        {
            row.FinishRelation=FleetFinishRelation::RouteIncompleteAtPlayerFinish;
            continue;
        }
        const double difference=activeSeconds-status.LastGateSeconds;
        if (difference<=RivalFinishClockReview::NearFinishToleranceSeconds)
            row.FinishRelation=FleetFinishRelation::SameClockWindow;
        else
        {
            row.FinishRelation=FleetFinishRelation::EarlierObserved;
            row.EarlierObservedSeconds=difference;
        }
    }
    return out;
}

// Enrich an already validated physical route review with independently
// recorded timing. No timing information can repair invalid route evidence.
inline FleetTimingReadout ReviewFleetGateTiming(const FleetRaceReview& review,
    const FleetGateTimingJournal& journal, std::uint64_t attempt,
    const SixCarIdentity& identity) noexcept
{
    if (!review.ValidRoster || !review.PlayerRouteValid)
        return {};
    std::array<FleetTimingEvidence, FleetTimingCarCount> evidence{};
    evidence[0] = {true, review.PlayerAcceptedGates, 0.0};
    for (int slot = 1; slot < StartGridCapacity; ++slot)
    {
        const auto& row = review.Rivals[static_cast<std::size_t>(slot - 1)];
        evidence[static_cast<std::size_t>(slot)] = {
            row.Slot == slot && row.BodyObserved && row.RouteEvidenceValid,
            row.AcceptedGates, row.LastGateActiveSeconds};
    }
    return journal.Review(attempt, identity, review.GateCount,
        review.RequiredLaps, review.PlayerActiveSeconds, evidence);
}

// Keep overlapping rival markers legible on the compact course radar.
// Clusters are a visual grouping of measured positions, NOT a position
// estimate for a missing actor and NOT a ranking. Every represented slot
// belongs to exactly one cluster; one cluster can contain up to five.
struct FleetRadarCluster final
{
    RadarMarker Marker{};
    std::uint32_t SlotMask=0;
    int Count=0;
};

struct FleetRadarClusters final
{
    std::array<FleetRadarCluster,StartGridCapacity-1> Groups{};
    int Count=0;
};

inline FleetRadarClusters GroupFleetRadar(const FleetRaceReview& review,
    double mergeDistance) noexcept
{
    FleetRadarClusters result;
    if (!review.ValidRoster || !std::isfinite(mergeDistance) ||
        mergeDistance<0.0 || mergeDistance>0.25)
        return result;
    // Connected components of a five-point graph are deterministic even
    // when A overlaps B and B overlaps C but A does not overlap C.
    std::array<int,StartGridCapacity-1> parent{};
    std::array<bool,StartGridCapacity-1> valid{};
    for(int i=0;i<StartGridCapacity-1;++i)
    {
        parent[static_cast<std::size_t>(i)]=i;
        const auto& row=review.Rivals[static_cast<std::size_t>(i)];
        valid[static_cast<std::size_t>(i)]=row.BodyObserved &&
            row.Slot==i+1 && row.Marker.Valid &&
            std::isfinite(row.Marker.U) && std::isfinite(row.Marker.V) &&
            row.Marker.U>=0.0 && row.Marker.U<=1.0 &&
            row.Marker.V>=0.0 && row.Marker.V<=1.0;
    }
    const auto root=[&](int index) noexcept
    {
        while(parent[static_cast<std::size_t>(index)]!=index)
            index=parent[static_cast<std::size_t>(index)];
        return index;
    };
    for(int i=0;i<StartGridCapacity-1;++i)
    {
        if (!valid[static_cast<std::size_t>(i)]) continue;
        const auto a=review.Rivals[static_cast<std::size_t>(i)].Marker;
        for(int j=0;j<i;++j)
        {
            if (!valid[static_cast<std::size_t>(j)]) continue;
            const auto b=review.Rivals[static_cast<std::size_t>(j)].Marker;
            if (std::hypot(a.U-b.U,a.V-b.V)<=mergeDistance)
            {
                const int ar=root(i),br=root(j);
                if (ar!=br) parent[static_cast<std::size_t>(ar>br?ar:br)]=ar>br?br:ar;
            }
        }
    }
    std::array<int,StartGridCapacity-1> groupByRoot{};
    groupByRoot.fill(-1);
    for(int i=0;i<StartGridCapacity-1;++i)
    {
        if (!valid[static_cast<std::size_t>(i)]) continue;
        const int r=root(i);
        int& group=groupByRoot[static_cast<std::size_t>(r)];
        if (group<0) group=result.Count++;
        auto& cluster=result.Groups[static_cast<std::size_t>(group)];
        const auto marker=review.Rivals[static_cast<std::size_t>(i)].Marker;
        cluster.Marker.Valid=true;
        cluster.Marker.U+=marker.U;
        cluster.Marker.V+=marker.V;
        cluster.SlotMask|=std::uint32_t{1}<<static_cast<unsigned>(i+1);
        ++cluster.Count;
    }
    for(int i=0;i<result.Count;++i)
    {
        auto& cluster=result.Groups[static_cast<std::size_t>(i)];
        cluster.Marker.U/=static_cast<double>(cluster.Count);
        cluster.Marker.V/=static_cast<double>(cluster.Count);
    }
    return result;
}
} // namespace latebrake

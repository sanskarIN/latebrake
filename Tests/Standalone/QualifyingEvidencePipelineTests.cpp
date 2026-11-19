#include "../../Source/Latebrake/Public/Competition/QualifyingEvidencePipeline.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <limits>

#define VERIFY(condition) do { if (!(condition)) { \
    std::fprintf(stderr, "Pipeline check: %s at %d\n", #condition, __LINE__); \
    std::abort(); \
} } while (false)

using namespace latebrake::competition;

namespace {

std::array<std::uint64_t, MaximumQualifyingCars> Actors() {
    return {101, 102, 103, 104, 105, 106};
}

QualifyingPipelineSettings Configuration() {
    QualifyingPipelineSettings p{};
    p.gates.session = 901;
    p.gates.circuit = 750;
    p.gates.gatesPerLap = 3;
    p.gates.minimumGateIntervalSeconds = 0.01;
    p.gates.maximumGateIntervalSeconds = 100.0;
    p.gates.maximumObservedSpeedCmS = 15000.0;
    p.gates.maximumPositionNoiseCm = 30.0;

    p.laps.session = 901;
    p.laps.circuit = 750;
    p.laps.gatesPerLap = 3;
    p.laps.maximumRecordedLaps = 3;
    p.laps.minimumLapSeconds = 10.0;
    p.laps.maximumLapSeconds = 120.0;
    p.laps.maximumClockSkewSeconds = 0.05;

    p.sectors.session = 901;
    p.sectors.circuit = 750;
    p.sectors.gatesPerLap = 3;
    p.sectors.sectorsPerLap = 2;
    p.sectors.minimumSectorSeconds = 1.0;
    p.sectors.maximumSectorSeconds = 100.0;
    p.sectors.maximumClockSkewSeconds = 0.05;
    p.sectorGateBoundaries[0] = 0;
    p.sectorGateBoundaries[1] = 2;
    p.sectorGateBoundaries[2] = 3;
    return p;
}

VerifiedGateObservation Gate(std::uint64_t actor, int lap,
                             int gate, double interval = 10.0) {
    VerifiedGateObservation source{};
    source.session = 901;
    source.circuit = 750;
    source.actor = actor;
    source.lapOrdinal = lap;
    source.gateOrdinal = gate;
    source.engineFrame =
        static_cast<std::uint64_t>(lap * 20000 + gate * 1200 + 1);
    source.clockSeconds = (lap - 1) * 50.0 +
                          gate * interval;
    source.position = {
        static_cast<double>(gate) * 10000.0, 0.0, 100.0
    };
    source.measuredSpeedCmS = 1000.0;
    source.fromOriginalEngine = true;
    source.actualPhysicalCar = true;
    source.physicalGateCrossing = true;
    source.onAuthoredCircuit = true;
    source.uninterruptedRace = true;
    source.legalTrackLimits = true;
    source.raceControlAuthorized = true;
    return source;
}

QualifyingPipelineReceipt CompleteLap(
    QualifyingEvidencePipeline& pipeline,
    std::uint64_t actor, int lap, double interval = 10.0) {
    QualifyingPipelineReceipt final{};
    for (int gate = 0; gate <= 3; ++gate) {
        final = pipeline.Observe(Gate(actor, lap, gate, interval));
        if (gate < 3) {
            VERIFY(final.status == QualifyingPipelineStatus::GateAccepted);
        }
    }
    return final;
}

void ValidateConfiguration() {
    const auto valid = Configuration();
    VERIFY(ValidQualifyingPipelineSettings(valid));
    auto modified = valid;
    modified.laps.session = 902;
    VERIFY(!ValidQualifyingPipelineSettings(modified));
    modified = valid;
    modified.sectors.circuit = 800;
    VERIFY(!ValidQualifyingPipelineSettings(modified));
    modified = valid;
    modified.sectorGateBoundaries[1] = 3;
    VERIFY(!ValidQualifyingPipelineSettings(modified));
    modified = valid;
    modified.sectorGateBoundaries[2] = 2;
    VERIFY(!ValidQualifyingPipelineSettings(modified));
    modified = valid;
    modified.sectorGateBoundaries[4] = 1;
    VERIFY(!ValidQualifyingPipelineSettings(modified));
    modified = valid;
    modified.gates.gatesPerLap = 4;
    VERIFY(!ValidQualifyingPipelineSettings(modified));

    QualifyingEvidencePipeline pipeline;
    VERIFY(!pipeline.Grid().valid);
    VERIFY(!pipeline.SectorLeaderboard().valid);
    auto empty = pipeline.Observe(Gate(101, 1, 0));
    VERIFY(empty.status == QualifyingPipelineStatus::NoSession);
    VERIFY(!empty.accepted);
    auto roster = Actors();
    roster[2] = roster[0];
    VERIFY(!pipeline.Begin(valid, roster, 6));
    roster = Actors();
    VERIFY(pipeline.Begin(valid, roster, 6));
    VERIFY(!pipeline.Begin(valid, roster, 6));
    VERIFY(pipeline.Active() && pipeline.Count() == 6);
    VERIFY(pipeline.Grid().valid);
    VERIFY(pipeline.Grid().qualifiedCount == 0);
}

void InterleavedSixCarEvidence() {
    QualifyingEvidencePipeline pipeline;
    VERIFY(pipeline.Begin(Configuration(), Actors(), 6));

    // Actual source actor identities, not standings indices, determine
    // the independent in-progress gate and split histories.
    for (int gate = 0; gate < 3; ++gate) {
        auto a = pipeline.Observe(Gate(101, 1, gate));
        auto b = pipeline.Observe(Gate(102, 1, gate));
        VERIFY(a.status == QualifyingPipelineStatus::GateAccepted);
        VERIFY(b.status == QualifyingPipelineStatus::GateAccepted);
        VERIFY(!a.accepted && !b.accepted);
    }
    const auto one = pipeline.Observe(Gate(101, 1, 3));
    VERIFY(one.status == QualifyingPipelineStatus::QualifyingLapAccepted);
    VERIFY(one.accepted);
    VERIFY(one.lapStatus == LapAdmission::Accepted);
    VERIFY(one.sectorStatus == SectorAdmission::Accepted);
    VERIFY(one.acceptedLap.actor == 101);
    VERIFY(one.acceptedLap.lapOrdinal == 1);
    VERIFY(one.sectorReceipt.valid);
    VERIFY(one.sectorReceipt.measuredLapSeconds == 30.0);
    VERIFY(one.sectorReceipt.cumulativeSectorSeconds == 30.0);
    VERIFY(one.sectorReceipt.theoreticalBestSeconds == 30.0);
    VERIFY(pipeline.Grid().qualifiedCount == 1);
    VERIFY(pipeline.SectorLeaderboard().timedDrivers == 1);

    const auto two = pipeline.Observe(Gate(102, 1, 3));
    VERIFY(two.status == QualifyingPipelineStatus::QualifyingLapAccepted);
    VERIFY(two.accepted);
    VERIFY(pipeline.Grid().qualifiedCount == 2);
    VERIFY(pipeline.Grid().actors[0] == 101);
    VERIFY(pipeline.Grid().actors[1] == 102);
    VERIFY(pipeline.SectorLeaderboard().timedDrivers == 2);

    // Invalid packets for other sessions or actors cannot erase an
    // unrelated, already-accepted genuine result.
    auto foreign = Gate(101, 2, 0);
    foreign.session = 999;
    auto r = pipeline.Observe(foreign);
    VERIFY(r.status == QualifyingPipelineStatus::GateRejected);
    VERIFY(r.gateStatus == GateTraceAdmission::WrongSession);
    VERIFY(pipeline.Grid().qualifiedCount == 2);
    foreign = Gate(999, 2, 0);
    r = pipeline.Observe(foreign);
    VERIFY(r.status == QualifyingPipelineStatus::GateRejected);
    VERIFY(r.gateStatus == GateTraceAdmission::InvalidRoster);
    VERIFY(pipeline.Grid().qualifiedCount == 2);

    const auto third = CompleteLap(pipeline, 101, 2, 9.0);
    VERIFY(third.status == QualifyingPipelineStatus::QualifyingLapAccepted);
    VERIFY(third.acceptedLap.reportedLapSeconds == 27.0);
    VERIFY(third.sectorReceipt.theoreticalBestSeconds == 27.0);
    VERIFY(pipeline.Grid().qualifiedCount == 2);
    VERIFY(pipeline.Grid().bestLapSeconds[0] == 27.0);
    VERIFY(pipeline.Qualifying().Driver(101)->admittedLaps == 2);
    VERIFY(pipeline.Sectors().Driver(101)->reviewedLaps == 2);
    VERIFY(pipeline.Gates().Car(101)->completedLapOrdinal == 2);

    // An unauthorised intermediate gate terminates this *one actor* lap.
    VERIFY(pipeline.Observe(Gate(103, 1, 0)).status ==
           QualifyingPipelineStatus::GateAccepted);
    auto illegal = Gate(103, 1, 1);
    illegal.legalTrackLimits = false;
    r = pipeline.Observe(illegal);
    VERIFY(r.status == QualifyingPipelineStatus::GateRejected);
    VERIFY(r.gateStatus == GateTraceAdmission::InvalidSource);
    VERIFY(pipeline.Qualifying().Driver(103)->admittedLaps == 0);
    VERIFY(pipeline.Sectors().Driver(103)->reviewedLaps == 0);
    VERIFY(pipeline.Gates().Car(103)->completedLapOrdinal == 1);
    auto forgery = Gate(103, 1, 0);
    forgery.engineFrame += 1;
    r = pipeline.Observe(forgery);
    VERIFY(r.gateStatus == GateTraceAdmission::StaleOrMisordered);
    VERIFY(!r.accepted);
    VERIFY(pipeline.Grid().qualifiedCount == 2);

    // The next *new* physical lap ordinal is accepted independently.
    const auto fourth = CompleteLap(pipeline, 103, 2);
    VERIFY(fourth.status == QualifyingPipelineStatus::QualifyingLapAccepted);
    VERIFY(pipeline.Grid().qualifiedCount == 3);
    VERIFY(pipeline.Qualifying().Driver(103)->admittedLaps == 1);
}

void AtomicCrossDomainRejection() {
    // The gate recorder can reconstruct a shorter lap, but this stricter
    // competition policy must reject it without modifying sector history.
    auto settings = Configuration();
    settings.laps.minimumLapSeconds = 35.0;
    QualifyingEvidencePipeline pipeline;
    VERIFY(pipeline.Begin(settings, Actors(), 6));
    auto rejected = CompleteLap(pipeline, 101, 1);
    VERIFY(rejected.status ==
           QualifyingPipelineStatus::QualifyingLapRejected);
    VERIFY(rejected.gateStatus == GateTraceAdmission::LapReconstructed);
    VERIFY(rejected.lapStatus == LapAdmission::InvalidTiming);
    VERIFY(!rejected.accepted);
    VERIFY(pipeline.Qualifying().Driver(101)->admittedLaps == 0);
    VERIFY(pipeline.Sectors().Driver(101)->reviewedLaps == 0);
    VERIFY(pipeline.Grid().qualifiedCount == 0);
    const auto accepted = CompleteLap(pipeline, 101, 2, 12.0);
    VERIFY(accepted.status == QualifyingPipelineStatus::QualifyingLapAccepted);
    VERIFY(pipeline.Qualifying().Driver(101)->admittedLaps == 1);
    VERIFY(pipeline.Sectors().Driver(101)->reviewedLaps == 1);

    // A stricter split policy may reject an otherwise valid full lap.
    settings = Configuration();
    settings.sectors.maximumSectorSeconds = 15.0;
    QualifyingEvidencePipeline strictSectors;
    VERIFY(strictSectors.Begin(settings, Actors(), 6));
    rejected = CompleteLap(strictSectors, 101, 1);
    VERIFY(rejected.status ==
           QualifyingPipelineStatus::SectorReviewRejected);
    VERIFY(rejected.lapStatus == LapAdmission::Accepted);
    VERIFY(rejected.sectorStatus == SectorAdmission::InconsistentTiming);
    VERIFY(!rejected.accepted);
    VERIFY(strictSectors.Grid().qualifiedCount == 0);
    VERIFY(strictSectors.Sectors().Driver(101)->reviewedLaps == 0);
    VERIFY(strictSectors.Qualifying().Driver(101)->admittedLaps == 0);

    // Nonfinite or impossible sources never mutate points or best splits.
    QualifyingEvidencePipeline malformed;
    VERIFY(malformed.Begin(Configuration(), Actors(), 6));
    auto badStart = Gate(102, 1, 0);
    badStart.clockSeconds = std::numeric_limits<double>::quiet_NaN();
    rejected = malformed.Observe(badStart);
    VERIFY(rejected.status == QualifyingPipelineStatus::GateRejected);
    VERIFY(malformed.Grid().qualifiedCount == 0);
    VERIFY(malformed.SectorLeaderboard().timedDrivers == 0);

    VERIFY(malformed.Observe(Gate(102, 2, 0)).status ==
           QualifyingPipelineStatus::GateAccepted);
    auto teleport = Gate(102, 2, 1);
    teleport.position.x = 1000000.0;
    rejected = malformed.Observe(teleport);
    VERIFY(rejected.gateStatus == GateTraceAdmission::ImplausibleMotion);
    VERIFY(!rejected.accepted);
    VERIFY(malformed.Qualifying().Driver(102)->admittedLaps == 0);
    VERIFY(malformed.Sectors().Driver(102)->reviewedLaps == 0);
}

} // namespace

int main() {
    ValidateConfiguration();
    InterleavedSixCarEvidence();
    AtomicCrossDomainRejection();
    return 0;
}

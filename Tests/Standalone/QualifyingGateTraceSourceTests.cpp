#include "../../Source/Latebrake/Public/Competition/QualifyingGateTrace.h"

#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>

#define VERIFY(x) do { if (!(x)) { \
    std::fprintf(stderr, "gate trace assertion: %s at %d\n", #x, __LINE__); \
    std::abort(); \
} } while (false)

using namespace latebrake::competition;

static GateTraceSettings GateSettings() {
    GateTraceSettings s{};
    s.session = 501;
    s.circuit = 72;
    s.gatesPerLap = 3;
    s.minimumGateIntervalSeconds = 0.01;
    s.maximumGateIntervalSeconds = 60.0;
    s.maximumObservedSpeedCmS = 15000.0;
    s.maximumPositionNoiseCm = 20.0;
    return s;
}

static VerifiedGateObservation Sample(
    std::uint64_t actor, int lap, int gate,
    double offsetSeconds = 0.0) {
    VerifiedGateObservation s{};
    s.session = 501;
    s.circuit = 72;
    s.actor = actor;
    s.lapOrdinal = lap;
    s.gateOrdinal = gate;
    s.engineFrame = static_cast<std::uint64_t>(
        lap * 10000 + gate * 720 + 1);
    s.clockSeconds = (lap - 1) * 40.0 +
                     gate * 12.0 + offsetSeconds;
    s.measuredSpeedCmS = 1000.0;
    s.position = {10000.0 * gate, 0.0, 100.0};
    s.fromOriginalEngine = true;
    s.actualPhysicalCar = true;
    s.physicalGateCrossing = true;
    s.onAuthoredCircuit = true;
    s.uninterruptedRace = true;
    s.legalTrackLimits = true;
    s.raceControlAuthorized = true;
    return s;
}

int main() {
    QualifyingGateTrace trace;
    const std::array<std::uint64_t, MaximumQualifyingCars> actors{
        101, 102, 103, 104, 105, 106
    };
    VERIFY(trace.Observe(Sample(101, 1, 0)).status ==
           GateTraceAdmission::NoSession);

    auto settings = GateSettings();
    settings.gatesPerLap = 1;
    VERIFY(!trace.Begin(settings, actors, 6));
    settings = GateSettings();
    auto duplicate = actors;
    duplicate[4] = duplicate[0];
    VERIFY(!trace.Begin(settings, duplicate, 6));
    VERIFY(trace.Begin(settings, actors, 6));
    VERIFY(!trace.Begin(settings, actors, 6));

    auto start = trace.Observe(Sample(101, 1, 0));
    VERIFY(start.status == GateTraceAdmission::Started);
    VERIFY(!start.lapProduced);
    VERIFY(trace.Car(101)->inLap);
    // Independent rivals may qualify at interleaved engine frames.
    VERIFY(trace.Observe(Sample(102, 1, 0)).status ==
           GateTraceAdmission::Started);
    VERIFY(trace.Observe(Sample(102, 1, 1)).status ==
           GateTraceAdmission::Advanced);
    VERIFY(trace.Observe(Sample(101, 1, 1)).status ==
           GateTraceAdmission::Advanced);
    VERIFY(trace.Observe(Sample(102, 1, 2)).status ==
           GateTraceAdmission::Advanced);
    VERIFY(trace.Observe(Sample(101, 1, 2)).status ==
           GateTraceAdmission::Advanced);

    auto finish = trace.Observe(Sample(101, 1, 3));
    VERIFY(finish.status == GateTraceAdmission::LapReconstructed);
    VERIFY(finish.lapProduced);
    VERIFY(finish.lap.actor == 101 && finish.lap.session == 501);
    VERIFY(finish.lap.circuit == 72);
    VERIFY(finish.lap.lapOrdinal == 1);
    VERIFY(finish.lap.verifiedGates == 3);
    VERIFY(finish.lap.startSeconds == 0.0);
    VERIFY(finish.lap.finishSeconds == 36.0);
    VERIFY(finish.lap.reportedLapSeconds == 36.0);
    VERIFY(finish.lap.measuredFromEngine);
    VERIFY(finish.lap.raceControlConfirmed);
    VERIFY(!trace.Car(101)->inLap);
    VERIFY(trace.Car(101)->completedLapOrdinal == 1);

    QualifyingSession provisional;
    QualifyingSettings q{};
    q.session = 501;
    q.circuit = 72;
    q.gatesPerLap = 3;
    q.minimumLapSeconds = 30.0;
    q.maximumLapSeconds = 180.0;
    VERIFY(provisional.Begin(q, actors, 6));
    VERIFY(provisional.Admit(finish.lap) == LapAdmission::Accepted);
    VERIFY(provisional.ProvisionalGrid().actors[0] == 101);
    VERIFY(provisional.ProvisionalGrid().qualifiedCount == 1);

    finish = trace.Observe(Sample(102, 1, 3));
    VERIFY(finish.status == GateTraceAdmission::LapReconstructed);
    VERIFY(provisional.Admit(finish.lap) == LapAdmission::Accepted);
    VERIFY(provisional.ProvisionalGrid().qualifiedCount == 2);
    VERIFY(provisional.ProvisionalGrid().actors[0] == 101);

    // A completed first lap cannot be replayed as another valid lap.
    VERIFY(trace.Observe(Sample(101, 1, 0)).status ==
           GateTraceAdmission::StaleOrMisordered);
    auto otherSession = Sample(101, 2, 0);
    otherSession.circuit = 999;
    VERIFY(trace.Observe(otherSession).status ==
           GateTraceAdmission::WrongSession);
    VERIFY(trace.Observe(Sample(999, 1, 0)).status ==
           GateTraceAdmission::InvalidRoster);

    // A failed gate sequence cancels the partial observation, so no late
    // finish may reconstitute a truncated race lap.
    VERIFY(trace.Observe(Sample(103, 1, 0)).status ==
           GateTraceAdmission::Started);
    auto skipped = Sample(103, 1, 2);
    VERIFY(trace.Observe(skipped).status ==
           GateTraceAdmission::StaleOrMisordered);
    VERIFY(!trace.Car(103)->inLap);
    VERIFY(trace.Observe(Sample(103, 1, 3)).status ==
           GateTraceAdmission::StaleOrMisordered);
    // An interrupted attempt cannot be restarted with a second gate zero
    // inside the same physical lap ordinal, even at a newer source frame.
    auto forgedRestart = Sample(103, 1, 0);
    forgedRestart.engineFrame = 8500;
    forgedRestart.clockSeconds = 39.0;
    VERIFY(trace.Observe(forgedRestart).status ==
           GateTraceAdmission::StaleOrMisordered);
    VERIFY(trace.Car(103)->completedLapOrdinal == 1);

    VERIFY(trace.Observe(Sample(104, 1, 0)).status ==
           GateTraceAdmission::Started);
    auto teleported = Sample(104, 1, 1);
    teleported.position.x = 1000000.0;
    VERIFY(trace.Observe(teleported).status ==
           GateTraceAdmission::ImplausibleMotion);
    VERIFY(!trace.Car(104)->inLap);

    VERIFY(trace.Observe(Sample(105, 1, 0)).status ==
           GateTraceAdmission::Started);
    auto stale = Sample(105, 1, 1);
    stale.clockSeconds = 0.0;
    VERIFY(trace.Observe(stale).status ==
           GateTraceAdmission::StaleOrMisordered);
    VERIFY(!trace.Car(105)->inLap);

    VERIFY(trace.Observe(Sample(106, 1, 0)).status ==
           GateTraceAdmission::Started);
    auto bad = Sample(106, 1, 1);
    bad.fromOriginalEngine = false;
    VERIFY(trace.Observe(bad).status ==
           GateTraceAdmission::InvalidSource);
    VERIFY(!trace.Car(106)->inLap);

    // A clean second lap must use a newer engine frame/clock and original
    // official lap ordinal; independent logical sessions stay separated.
    VERIFY(trace.Observe(Sample(101, 2, 0)).status ==
           GateTraceAdmission::Started);
    VERIFY(trace.Observe(Sample(101, 2, 1)).status ==
           GateTraceAdmission::Advanced);
    VERIFY(trace.Observe(Sample(101, 2, 2)).status ==
           GateTraceAdmission::Advanced);
    finish = trace.Observe(Sample(101, 2, 3));
    VERIFY(finish.status == GateTraceAdmission::LapReconstructed);
    VERIFY(finish.lap.lapOrdinal == 2);
    VERIFY(finish.lap.finishSeconds == 76.0);
    VERIFY(provisional.Admit(finish.lap) == LapAdmission::Accepted);
    VERIFY(provisional.Driver(101)->admittedLaps == 2);

    // Nonfinite physical data may not produce a valid lap.
    auto corrupt = Sample(103, 2, 0);
    corrupt.position.y = std::numeric_limits<double>::quiet_NaN();
    VERIFY(trace.Observe(corrupt).status ==
           GateTraceAdmission::InvalidSource);
    corrupt = Sample(103, 2, 0);
    corrupt.measuredSpeedCmS = std::numeric_limits<double>::infinity();
    VERIFY(trace.Observe(corrupt).status ==
           GateTraceAdmission::InvalidSource);
    return 0;
}

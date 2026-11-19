#include "../../Source/Latebrake/Public/Competition/QualifyingSectorReview.h"

#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>

#define CHECK(x) do { if (!(x)) { \
    std::fprintf(stderr, "Sector review failed: %s at %d\n", #x, __LINE__); \
    std::abort(); \
} } while (false)

using namespace latebrake::competition;

namespace {
std::array<std::uint64_t, MaximumQualifyingCars> Roster() {
    return {101, 102, 103, 104, 105, 106};
}

SectorSettings Settings() {
    SectorSettings s{};
    s.session = 500;
    s.circuit = 71;
    s.gatesPerLap = 4;
    s.sectorsPerLap = 2;
    s.minimumSectorSeconds = 1.0;
    s.maximumSectorSeconds = 200.0;
    s.maximumClockSkewSeconds = 0.05;
    return s;
}

QualifyingLap Lap(std::uint64_t actor, int ordinal,
                  std::uint64_t startFrame, std::uint64_t finishFrame,
                  double start, double finish) {
    QualifyingLap lap{};
    lap.session = 500;
    lap.circuit = 71;
    lap.actor = actor;
    lap.lapOrdinal = ordinal;
    lap.beginningFrame = startFrame;
    lap.endingFrame = finishFrame;
    lap.verifiedGates = 4;
    lap.startSeconds = start;
    lap.finishSeconds = finish;
    lap.reportedLapSeconds = finish - start;
    lap.measuredFromEngine = true;
    lap.originalCircuitVerified = true;
    lap.raceControlConfirmed = true;
    lap.crossedFinishPhysically = true;
    lap.uninterrupted = true;
    lap.legalTrackLimits = true;
    return lap;
}

PhysicalSectorEvidence Sector(const QualifyingLap& lap,
                              int startGate, int endGate,
                              std::uint64_t startFrame,
                              std::uint64_t endFrame,
                              double start, double finish) {
    PhysicalSectorEvidence s{};
    s.session = lap.session;
    s.circuit = lap.circuit;
    s.actor = lap.actor;
    s.lapOrdinal = lap.lapOrdinal;
    s.fromGate = startGate;
    s.toGate = endGate;
    s.gateEntryFrame = startFrame;
    s.gateExitFrame = endFrame;
    s.entryClockSeconds = start;
    s.exitClockSeconds = finish;
    s.reportedSectorSeconds = finish - start;
    s.engineClockVerified = true;
    s.verifiedPhysicalGatePair = true;
    s.originalCircuitVerified = true;
    s.continuousPhysicalMotion = true;
    s.validTrackLimits = true;
    s.raceControlAuthorized = true;
    return s;
}

std::array<PhysicalSectorEvidence, MaximumQualifyingSectors> FirstLapEvidence(
    const QualifyingLap& lap) {
    std::array<PhysicalSectorEvidence, MaximumQualifyingSectors> evidence{};
    evidence[0] = Sector(lap, 0, 2, lap.beginningFrame, 320,
                         lap.startSeconds, 17.0);
    evidence[1] = Sector(lap, 2, 4, 320, lap.endingFrame,
                         17.0, lap.finishSeconds);
    return evidence;
}

void TestPhysicalSectorReviews() {
    const auto roster = Roster();
    auto settings = Settings();
    QualifyingSectorReview review;
    SectorReviewReceipt receipt{};
    QualifyingLap first = Lap(101, 1, 200, 440, 0.0, 34.0);
    auto data = FirstLapEvidence(first);
    CHECK(review.ReviewLap(first, data, 2, &receipt) ==
          SectorAdmission::NoSession);
    CHECK(!receipt.valid);
    settings.session = 0;
    CHECK(!review.Begin(settings, roster, 6));
    settings = Settings();
    auto duplicates = roster;
    duplicates[1] = duplicates[0];
    CHECK(!review.Begin(settings, duplicates, 6));
    CHECK(review.Begin(settings, roster, 6));
    CHECK(!review.Begin(settings, roster, 6));
    CHECK(review.Active() && review.Count() == 6);
    CHECK(review.Driver(101) != nullptr);
    CHECK(review.Driver(999) == nullptr);
    CHECK(review.Driver(101)->reviewedLaps == 0);

    CHECK(review.ReviewLap(first, data, 2, &receipt) ==
          SectorAdmission::Accepted);
    CHECK(receipt.valid);
    CHECK(receipt.actor == 101 && receipt.lapOrdinal == 1);
    CHECK(receipt.measuredLapSeconds == 34.0);
    CHECK(receipt.cumulativeSectorSeconds == 34.0);
    CHECK(receipt.theoreticalBestSeconds == 34.0);
    CHECK(receipt.theoreticalGainSeconds == 0.0);
    CHECK(receipt.improvedActualLap && receipt.improvedTheoreticalLap);
    CHECK(review.Driver(101)->reviewedLaps == 1);
    CHECK(review.Driver(101)->bestLapSeconds == 34.0);
    CHECK(review.Driver(101)->bestSectorSeconds[0] == 17.0);
    CHECK(review.Driver(101)->bestSectorSeconds[1] == 17.0);
    CHECK(review.Driver(101)->bestSectorLapOrdinals[0] == 1);

    CHECK(review.ReviewLap(first, data, 2, &receipt) ==
          SectorAdmission::StaleLap);
    CHECK(!receipt.valid);
    CHECK(review.Driver(101)->reviewedLaps == 1);

    QualifyingLap second = Lap(101, 2, 441, 680, 34.0, 67.0);
    std::array<PhysicalSectorEvidence, MaximumQualifyingSectors> split{};
    split[0] = Sector(second, 0, 2, 441, 560, 34.0, 52.0);
    split[1] = Sector(second, 2, 4, 560, 680, 52.0, 67.0);
    CHECK(review.ReviewLap(second, split, 2, &receipt) ==
          SectorAdmission::Accepted);
    CHECK(receipt.measuredLapSeconds == 33.0);
    CHECK(receipt.theoreticalBestSeconds == 32.0);
    CHECK(receipt.theoreticalGainSeconds == 1.0);
    CHECK(receipt.improvedActualLap && receipt.improvedTheoreticalLap);
    CHECK(review.Driver(101)->bestSectorSeconds[0] == 17.0);
    CHECK(review.Driver(101)->bestSectorSeconds[1] == 15.0);
    CHECK(review.Driver(101)->bestSectorLapOrdinals[0] == 1);
    CHECK(review.Driver(101)->bestSectorLapOrdinals[1] == 2);
    CHECK(review.Driver(101)->reviewedLaps == 2);

    const auto before = *review.Driver(101);
    auto badLap = second;
    badLap.lapOrdinal = 3;
    badLap.beginningFrame = 681;
    badLap.endingFrame = 920;
    badLap.startSeconds = 67.0;
    badLap.finishSeconds = 100.0;
    badLap.reportedLapSeconds = 33.0;
    auto bogus = split;
    CHECK(review.ReviewLap(badLap, bogus, 2, &receipt) ==
          SectorAdmission::InvalidSource);
    CHECK(review.Driver(101)->reviewedLaps == before.reviewedLaps);

    badLap = second;
    badLap.actor = 999;
    CHECK(review.ReviewLap(badLap, split, 2) ==
          SectorAdmission::UnknownActor);
    badLap.actor = 101;
    badLap.session = 9;
    CHECK(review.ReviewLap(badLap, split, 2) ==
          SectorAdmission::WrongSession);

    auto other = Lap(102, 1, 200, 440, 0.0, 34.0);
    auto otherEvidence = FirstLapEvidence(other);
    CHECK(review.ReviewLap(other, otherEvidence, 1) ==
          SectorAdmission::IncorrectSectorCount);
    CHECK(review.Driver(102)->reviewedLaps == 0);

    auto corrupted = otherEvidence;
    corrupted[1].toGate = 3;
    CHECK(review.ReviewLap(other, corrupted, 2) ==
          SectorAdmission::MisorderedGates);
    corrupted = otherEvidence;
    corrupted[1].fromGate = 1;
    CHECK(review.ReviewLap(other, corrupted, 2) ==
          SectorAdmission::MisorderedGates);
    corrupted = otherEvidence;
    corrupted[0].gateExitFrame = 200;
    CHECK(review.ReviewLap(other, corrupted, 2) ==
          SectorAdmission::InvalidChronology);
    corrupted = otherEvidence;
    corrupted[0].gateExitFrame = 500;
    CHECK(review.ReviewLap(other, corrupted, 2) ==
          SectorAdmission::InvalidChronology);
    corrupted = otherEvidence;
    corrupted[0].continuousPhysicalMotion = false;
    CHECK(review.ReviewLap(other, corrupted, 2) ==
          SectorAdmission::InvalidSource);
    corrupted = otherEvidence;
    corrupted[0].reportedSectorSeconds = 16.0;
    CHECK(review.ReviewLap(other, corrupted, 2) ==
          SectorAdmission::InconsistentTiming);
    corrupted = otherEvidence;
    corrupted[1].entryClockSeconds = 18.0;
    CHECK(review.ReviewLap(other, corrupted, 2) ==
          SectorAdmission::InconsistentTiming);
    corrupted = otherEvidence;
    corrupted[0].exitClockSeconds =
        std::numeric_limits<double>::quiet_NaN();
    CHECK(review.ReviewLap(other, corrupted, 2) ==
          SectorAdmission::InvalidChronology);

    other.reportedLapSeconds = 20.0;
    CHECK(review.ReviewLap(other, otherEvidence, 2) ==
          SectorAdmission::InvalidLap);
    other.reportedLapSeconds = 34.0;
    other.uninterrupted = false;
    CHECK(review.ReviewLap(other, otherEvidence, 2) ==
          SectorAdmission::InvalidLap);
    other.uninterrupted = true;
    CHECK(review.ReviewLap(other, otherEvidence, 2, &receipt) ==
          SectorAdmission::Accepted);
    CHECK(receipt.valid);
    CHECK(review.Driver(102)->reviewedLaps == 1);
    CHECK(review.Driver(102)->bestLapSeconds == 34.0);
    CHECK(review.Driver(101)->bestLapSeconds == 33.0);
}

} // namespace

int main() {
    TestPhysicalSectorReviews();
    return 0;
}

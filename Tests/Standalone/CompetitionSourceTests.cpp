#include "../../Source/Latebrake/Public/Competition/ChampionshipSeason.h"
#include "../../Source/Latebrake/Public/Competition/QualifyingSession.h"

#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>

#define VERIFY(expression) do { \
    if (!(expression)) { \
        std::fprintf(stderr, "Failed: %s at line %d\n", #expression, __LINE__); \
        std::abort(); \
    } \
} while (false)

namespace {
using namespace latebrake::competition;

std::array<std::uint64_t, MaximumSeasonDrivers> Roster() {
    return {101, 102, 103, 104, 105, 106};
}

QualifyingSettings TimedSettings() {
    QualifyingSettings settings{};
    settings.session = 500;
    settings.circuit = 71;
    settings.gatesPerLap = 12;
    settings.maximumRecordedLaps = 5;
    settings.minimumLapSeconds = 30.0;
    settings.maximumLapSeconds = 180.0;
    settings.maximumClockSkewSeconds = 0.05;
    return settings;
}

QualifyingLap Lap(std::uint64_t actor, int ordinal,
                  double start, double finish) {
    QualifyingLap lap{};
    lap.session = 500;
    lap.circuit = 71;
    lap.actor = actor;
    lap.lapOrdinal = ordinal;
    lap.beginningFrame =
        static_cast<std::uint64_t>(start * 60.0) + 1;
    lap.endingFrame =
        static_cast<std::uint64_t>(finish * 60.0) + 1;
    lap.verifiedGates = 12;
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

void QualifyingValidation() {
    QualifyingSession session;
    const auto actors = Roster();
    auto settings = TimedSettings();
    auto grid = session.ProvisionalGrid();
    VERIFY(!grid.valid);
    VERIFY(session.Admit(Lap(101, 1, 0.0, 50.0)) ==
           LapAdmission::NoSession);
    settings.session = 0;
    VERIFY(!session.Begin(settings, actors, 6));
    settings = TimedSettings();
    auto duplicates = actors;
    duplicates[2] = duplicates[1];
    VERIFY(!session.Begin(settings, duplicates, 6));
    VERIFY(!session.Active());
    VERIFY(session.Begin(settings, actors, 6));
    VERIFY(!session.Begin(settings, actors, 6));
    VERIFY(session.Count() == 6);

    auto first = Lap(101, 1, 0.0, 50.0);
    VERIFY(session.Admit(first) == LapAdmission::Accepted);
    VERIFY(session.Driver(101)->admittedLaps == 1);
    VERIFY(session.Admit(first) == LapAdmission::StaleObservation);
    auto secondCar = Lap(102, 1, 0.0, 50.0);
    VERIFY(session.Admit(secondCar) == LapAdmission::Accepted);
    grid = session.ProvisionalGrid();
    VERIFY(grid.valid && grid.count == 6 && grid.qualifiedCount == 2);
    VERIFY(grid.actors[0] == 101 && grid.actors[1] == 102);
    VERIFY(grid.qualified[0] && !grid.qualified[2]);

    auto improved = Lap(101, 2, 50.0, 99.0);
    VERIFY(session.Admit(improved) == LapAdmission::Accepted);
    VERIFY(session.Driver(101)->bestLapOrdinal == 2);
    VERIFY(session.Driver(101)->bestLapSeconds == 49.0);
    VERIFY(session.Driver(101)->secondLapSeconds == 50.0);
    grid = session.ProvisionalGrid();
    VERIFY(grid.actors[0] == 101);

    auto faster = Lap(102, 2, 50.0, 98.0);
    VERIFY(session.Admit(faster) == LapAdmission::Accepted);
    grid = session.ProvisionalGrid();
    VERIFY(grid.actors[0] == 102 && grid.actors[1] == 101);
    VERIFY(grid.bestLapOrdinals[0] == 2);
    VERIFY(grid.bestLapSeconds[0] == 48.0);

    auto malformed = Lap(103, 1, 0.0, 60.0);
    malformed.session = 600;
    VERIFY(session.Admit(malformed) == LapAdmission::WrongSession);
    malformed = Lap(999, 1, 0.0, 60.0);
    VERIFY(session.Admit(malformed) == LapAdmission::UnknownCar);
    malformed = Lap(103, 1, 0.0, 60.0);
    malformed.verifiedGates = 11;
    VERIFY(session.Admit(malformed) == LapAdmission::InvalidSource);
    malformed = Lap(103, 1, 0.0, 60.0);
    malformed.measuredFromEngine = false;
    VERIFY(session.Admit(malformed) == LapAdmission::InvalidSource);
    malformed = Lap(103, 1, 0.0, 60.0);
    malformed.uninterrupted = false;
    VERIFY(session.Admit(malformed) == LapAdmission::InvalidSource);
    malformed = Lap(103, 1, 0.0, 60.0);
    malformed.penaltySeconds = 0.01;
    VERIFY(session.Admit(malformed) == LapAdmission::InvalidSource);
    malformed = Lap(103, 1, 0.0, 60.0);
    malformed.reportedLapSeconds = 70.0;
    VERIFY(session.Admit(malformed) == LapAdmission::InvalidTiming);
    malformed = Lap(103, 1, 0.0, 60.0);
    malformed.finishSeconds = std::numeric_limits<double>::infinity();
    VERIFY(session.Admit(malformed) == LapAdmission::InvalidTiming);
    malformed = Lap(103, 1, 0.0, 60.0);
    malformed.reportedLapSeconds =
        std::numeric_limits<double>::quiet_NaN();
    VERIFY(session.Admit(malformed) == LapAdmission::InvalidTiming);
    VERIFY(session.Driver(103)->admittedLaps == 0);

    malformed = Lap(103, 1, 0.0, 60.0);
    VERIFY(session.Admit(malformed) == LapAdmission::Accepted);
    auto overlap = Lap(103, 2, 40.0, 100.0);
    VERIFY(session.Admit(overlap) == LapAdmission::StaleObservation ||
           session.Admit(overlap) == LapAdmission::InvalidTiming);
    VERIFY(session.Driver(103)->admittedLaps == 1);

    auto noContact = Lap(104, 1, 0.0, 70.0);
    noContact.crossedFinishPhysically = false;
    VERIFY(session.Admit(noContact) == LapAdmission::InvalidSource);
    VERIFY(session.Driver(104)->bestLapOrdinal == 0);
}

SeasonSettings SeasonConfiguration() {
    SeasonSettings settings{};
    settings.seasonIdentity = 891;
    settings.scheduledRounds = 3;
    settings.requiredRaceLaps = 3;
    settings.minimumLegalRaceSeconds = 10.0;
    settings.maximumLegalRaceSeconds = 500.0;
    settings.minimumLegalLapSeconds = 5.0;
    return settings;
}

SeasonRoundEvidence Round(int number, std::uint64_t session) {
    SeasonRoundEvidence event{};
    event.seasonIdentity = 891;
    event.raceSessionIdentity = session;
    event.circuitIdentity = 700 + static_cast<std::uint64_t>(number);
    event.roundNumber = number;
    event.resultCount = 6;
    event.originalCircuitVerified = true;
    event.gameModeResultsVerified = true;
    event.raceClockClosed = true;
    event.controlStateSafe = true;
    event.officialClockSeconds = 200.0;
    for (std::size_t i = 0; i < 6; ++i) {
        auto& p = event.participants[i];
        p.actor = static_cast<std::uint64_t>(101 + i);
        p.state = FinishState::Classified;
        p.officialPosition = static_cast<int>(i + 1);
        p.completedLaps = 3;
        p.officialRaceSeconds = 180.0 + static_cast<double>(i);
        p.lastVerifiedFrame = 10000 + i;
        p.realPhysicalActor = true;
        p.officialClassificationVerified = true;
        p.originalRaceSessionVerified = true;
        p.physicalFinishGateVerified = true;
    }
    event.participants[0].fastestLapVerified = true;
    event.participants[0].fastestLegalLapSeconds = 40.0;
    return event;
}

void SeasonValidation() {
    ChampionshipSeason season;
    const auto roster = Roster();
    const auto settings = SeasonConfiguration();
    auto race = Round(1, 3001);
    VERIFY(season.Accept(race) == SeasonAdmission::NotConfigured);
    auto badRoster = roster;
    badRoster[5] = badRoster[0];
    VERIFY(!season.Begin(settings, badRoster, 6));
    auto badSettings = settings;
    badSettings.minimumLegalRaceSeconds =
        std::numeric_limits<double>::quiet_NaN();
    VERIFY(!season.Begin(badSettings, roster, 6));
    VERIFY(season.Begin(settings, roster, 6));
    VERIFY(!season.Begin(settings, roster, 6));
    VERIFY(season.Ranking().valid);
    VERIFY(season.Ranking().remainingRounds == 3);

    auto bad = race;
    bad.roundNumber = 2;
    VERIFY(season.Accept(bad) == SeasonAdmission::WrongRound);
    bad = race;
    bad.seasonIdentity = 999;
    VERIFY(season.Accept(bad) == SeasonAdmission::WrongSeason);
    bad = race;
    bad.participants[2].actor = 101;
    VERIFY(season.Accept(bad) == SeasonAdmission::InvalidRoster);
    bad = race;
    bad.participants[2].officialPosition = 2;
    VERIFY(season.Accept(bad) == SeasonAdmission::InvalidClassification);
    bad = race;
    bad.participants[1].lastVerifiedFrame = 0;
    VERIFY(season.Accept(bad) == SeasonAdmission::UntrustedResult);
    bad = race;
    bad.gameModeResultsVerified = false;
    VERIFY(season.Accept(bad) == SeasonAdmission::UntrustedResult);
    bad = race;
    bad.participants[0].fastestLegalLapSeconds = 0.0;
    VERIFY(season.Accept(bad) == SeasonAdmission::InvalidField);
    bad = race;
    bad.participants[0].officialRaceSeconds =
        std::numeric_limits<double>::infinity();
    VERIFY(season.Accept(bad) == SeasonAdmission::InvalidField);
    bad = race;
    bad.participants[5].state = static_cast<FinishState>(255);
    bad.participants[5].officialPosition = 0;
    bad.participants[5].physicalFinishGateVerified = false;
    VERIFY(season.Accept(bad) == SeasonAdmission::InvalidClassification);
    bad = race;
    bad.participants[5].state = FinishState::DidNotStart;
    bad.participants[5].officialPosition = 0;
    bad.participants[5].physicalFinishGateVerified = false;
    VERIFY(season.Accept(bad) == SeasonAdmission::InvalidClassification);
    VERIFY(season.CompletedRounds() == 0);
    VERIFY(season.Driver(101)->points == 0);

    VERIFY(season.Accept(race) == SeasonAdmission::Accepted);
    VERIFY(season.CompletedRounds() == 1);
    VERIFY(season.Driver(101)->points == 26);
    VERIFY(season.Driver(101)->wins == 1);
    VERIFY(season.Driver(101)->fastestLapBonuses == 1);
    VERIFY(season.Driver(102)->points == 18);
    VERIFY(season.Driver(106)->points == 8);
    VERIFY(season.Round(1)->fastestBonusActor == 101);
    VERIFY(season.Round(1)->classifiedFinishers == 6);
    VERIFY(season.Round(0) == nullptr);
    VERIFY(season.Round(2) == nullptr);
    VERIFY(season.Accept(race) == SeasonAdmission::WrongRound);

    auto second = Round(2, 3001);
    VERIFY(season.Accept(second) == SeasonAdmission::WrongRound);
    second.raceSessionIdentity = 3002;
    for (auto& p : second.participants) {
        p.officialPosition = 7 - p.officialPosition;
    }
    second.participants[0].fastestLapVerified = false;
    second.participants[0].fastestLegalLapSeconds = 0.0;
    second.participants[5].fastestLapVerified = true;
    second.participants[5].fastestLegalLapSeconds = 38.0;
    VERIFY(season.Accept(second) == SeasonAdmission::Accepted);
    VERIFY(season.Driver(106)->points == 34);
    VERIFY(season.Driver(106)->wins == 1);
    VERIFY(season.Driver(101)->points == 34);
    VERIFY(season.Ranking().count == 6);
    // Ties on points and wins are broken using podium counts.
    const auto ranking = season.Ranking();
    VERIFY(ranking.sorted[0].points == 34);
    VERIFY(ranking.sorted[0].actor == 101 ||
           ranking.sorted[0].actor == 106);

    auto third = Round(3, 3003);
    third.participants[4].state = FinishState::Retired;
    third.participants[4].officialPosition = 0;
    third.participants[4].physicalFinishGateVerified = false;
    third.participants[4].completedLaps = 2;
    third.participants[5].state = FinishState::Disqualified;
    third.participants[5].officialPosition = 0;
    third.participants[5].physicalFinishGateVerified = false;
    third.participants[5].completedLaps = 3;
    VERIFY(season.Accept(third) == SeasonAdmission::Accepted);
    VERIFY(season.Round(3)->classifiedFinishers == 4);
    VERIFY(season.Driver(105)->points ==
           SeasonPositionPoints[4] + SeasonPositionPoints[1]);
    VERIFY(season.Driver(105)->pointsPerRound[2] == 0);
    VERIFY(season.Driver(106)->pointsPerRound[2] == 0);
    VERIFY(season.CompletedRounds() == 3);
    VERIFY(season.Ranking().remainingRounds == 0);
    VERIFY(season.Accept(third) == SeasonAdmission::Complete);
}

void SparseGridAndAudit() {
    ChampionshipSeason season;
    auto settings = SeasonConfiguration();
    settings.requireFullGrid = false;
    settings.scheduledRounds = 1;
    settings.awardFastestLapBonus = false;
    VERIFY(season.Begin(settings, Roster(), 2));
    auto event = Round(1, 4001);
    event.resultCount = 2;
    event.participants[0].fastestLapVerified = false;
    event.participants[0].fastestLegalLapSeconds = 0.0;
    VERIFY(season.Accept(event) == SeasonAdmission::Accepted);
    VERIFY(season.Driver(101)->points == 25);
    VERIFY(season.Driver(102)->points == 18);
    VERIFY(season.Round(1)->fastestBonusActor == 0);
    VERIFY(season.Ranking().count == 2);
}

} // namespace

int main() {
    QualifyingValidation();
    SeasonValidation();
    SparseGridAndAudit();
    return 0;
}

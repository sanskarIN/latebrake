#include "../../Source/Latebrake/Public/Competition/SeasonTitleBounds.h"

#include <array>
#include <cstdio>
#include <cstdlib>

#define CHECK(x) do { if (!(x)) { \
    std::fprintf(stderr, "Title bounds failed: %s at %d\n", #x, __LINE__); \
    std::abort(); \
} } while (false)

using namespace latebrake::competition;

namespace {
SeasonSettings Settings(int rounds) {
    SeasonSettings settings{};
    settings.seasonIdentity = 900;
    settings.scheduledRounds = rounds;
    settings.requiredRaceLaps = 3;
    settings.requireFullGrid = false;
    settings.minimumLegalRaceSeconds = 10.0;
    settings.maximumLegalRaceSeconds = 500.0;
    settings.minimumLegalLapSeconds = 5.0;
    return settings;
}

SeasonRoundEvidence Round(int ordinal, std::uint64_t session,
                          bool reversed) {
    SeasonRoundEvidence evidence{};
    evidence.seasonIdentity = 900;
    evidence.raceSessionIdentity = session;
    evidence.circuitIdentity = 800 + static_cast<std::uint64_t>(ordinal);
    evidence.roundNumber = ordinal;
    evidence.resultCount = 2;
    evidence.originalCircuitVerified = true;
    evidence.gameModeResultsVerified = true;
    evidence.raceClockClosed = true;
    evidence.controlStateSafe = true;
    evidence.officialClockSeconds = 200.0;
    for (int i = 0; i < 2; ++i) {
        auto& result = evidence.participants[static_cast<std::size_t>(i)];
        result.actor = static_cast<std::uint64_t>(100 + i);
        result.state = FinishState::Classified;
        result.officialPosition = reversed ? 2 - i : 1 + i;
        result.completedLaps = 3;
        result.officialRaceSeconds = 180.0 + i;
        result.lastVerifiedFrame = 10000 + static_cast<std::uint64_t>(i);
        result.realPhysicalActor = true;
        result.officialClassificationVerified = true;
        result.originalRaceSessionVerified = true;
        result.physicalFinishGateVerified = true;
        result.fastestLapVerified =
            (reversed ? i == 1 : i == 0);
        result.fastestLegalLapSeconds =
            result.fastestLapVerified ? 42.0 : 0.0;
    }
    return evidence;
}

void TestPointBounds() {
    const std::array<std::uint64_t, MaximumSeasonDrivers> actors{
        100, 101, 0, 0, 0, 0
    };
    ChampionshipSeason season;
    CHECK(season.Begin(Settings(2), actors, 2));
    const auto preEvent = season.Ranking();
    auto pre = ProjectSeasonTitleOnPoints(preEvent);
    CHECK(pre.valid && pre.count == 2);
    CHECK(pre.completedRounds == 0 && pre.remainingRounds == 2);
    CHECK(pre.maximumPointsPerRemainingRound == 26);
    CHECK(pre.guaranteedChampionOnPoints == 0);
    CHECK(pre.drivers[0].maximumFinalPoints == 52);
    CHECK(pre.drivers[1].maximumFinalPoints == 52);
    CHECK(pre.drivers[0].alreadyClinchedTitleOnPoints == false);

    CHECK(season.Accept(Round(1, 4401, false)) ==
          SeasonAdmission::Accepted);
    const auto firstRanking = season.Ranking();
    CHECK(firstRanking.sorted[0].actor == 100);
    CHECK(firstRanking.sorted[0].points == 26);
    CHECK(firstRanking.sorted[1].points == 18);
    const auto first = ProjectSeasonTitleOnPoints(firstRanking);
    CHECK(first.valid);
    CHECK(first.completedRounds == 1);
    CHECK(first.remainingRounds == 1);
    CHECK(first.currentLeaderPoints == 26);
    CHECK(first.drivers[0].actor == 100);
    CHECK(first.drivers[0].maximumFinalPoints == 52);
    CHECK(first.drivers[0].additionalPointsToGuaranteeTitleOnPoints == 19);
    CHECK(first.drivers[0].alreadyClinchedTitleOnPoints == false);
    CHECK(first.drivers[1].actor == 101);
    CHECK(first.drivers[1].currentPoints == 18);
    CHECK(first.drivers[1].pointsBehindCurrentLeader == 8);
    CHECK(first.drivers[1].pointsNeededToExceedCurrentLeader == 9);
    CHECK(first.drivers[1].maximumFinalPoints == 44);
    CHECK(first.drivers[1].canOutscoreCurrentLeaderOnPoints);
    CHECK(first.drivers[1].additionalPointsToGuaranteeTitleOnPoints == -1);
    CHECK(!first.drivers[1].definitivelyEliminatedOnPoints);

    const auto noBonus = ProjectSeasonTitleOnPoints(
        firstRanking, SeasonTitleOptions{false});
    CHECK(noBonus.valid);
    CHECK(noBonus.maximumPointsPerRemainingRound == 25);
    CHECK(noBonus.drivers[0].maximumFinalPoints == 51);
    CHECK(noBonus.drivers[1].maximumFinalPoints == 43);
    CHECK(noBonus.drivers[0].additionalPointsToGuaranteeTitleOnPoints == 18);

    CHECK(season.Accept(Round(2, 4402, true)) ==
          SeasonAdmission::Accepted);
    auto tied = season.Ranking();
    CHECK(tied.completedRounds == 2 && tied.remainingRounds == 0);
    CHECK(tied.sorted[0].points == 44);
    CHECK(tied.sorted[1].points == 44);
    const auto afterTie = ProjectSeasonTitleOnPoints(tied);
    CHECK(afterTie.valid);
    CHECK(afterTie.guaranteedChampionOnPoints == 0);
    CHECK(!afterTie.drivers[0].alreadyClinchedTitleOnPoints);
    CHECK(!afterTie.drivers[1].alreadyClinchedTitleOnPoints);
    CHECK(afterTie.drivers[0].remainingPointCapacity == 0);
    CHECK(afterTie.drivers[1].remainingPointCapacity == 0);
    CHECK(afterTie.drivers[0].additionalPointsToGuaranteeTitleOnPoints == -1);
    CHECK(afterTie.drivers[1].additionalPointsToGuaranteeTitleOnPoints == -1);

    tied.sorted[0].points += 1;
    CHECK(!ProjectSeasonTitleOnPoints(tied).valid);
    tied = season.Ranking();
    tied.sorted[1].actor = tied.sorted[0].actor;
    CHECK(!ProjectSeasonTitleOnPoints(tied).valid);
}

void TestGuaranteedTitle() {
    const std::array<std::uint64_t, MaximumSeasonDrivers> actors{
        100, 101, 0, 0, 0, 0
    };
    ChampionshipSeason season;
    CHECK(season.Begin(Settings(1), actors, 2));
    CHECK(season.Accept(Round(1, 9901, false)) ==
          SeasonAdmission::Accepted);
    const auto forecast = ProjectSeasonTitleOnPoints(season.Ranking());
    CHECK(forecast.valid);
    CHECK(forecast.remainingRounds == 0);
    CHECK(forecast.currentLeaderPoints == 26);
    CHECK(forecast.guaranteedChampionOnPoints == 100);
    CHECK(forecast.drivers[0].alreadyClinchedTitleOnPoints);
    CHECK(forecast.drivers[0].additionalPointsToGuaranteeTitleOnPoints == 0);
    CHECK(forecast.drivers[1].definitivelyEliminatedOnPoints);
    CHECK(!forecast.drivers[1].canReachCurrentLeaderOnPoints);
    CHECK(!forecast.drivers[1].canOutscoreCurrentLeaderOnPoints);
}

} // namespace

int main() {
    TestPointBounds();
    TestGuaranteedTitle();
    return 0;
}

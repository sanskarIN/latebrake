#include "Career/CareerProgress.h"
#include <cstdlib>
#include <iostream>
#include <limits>

static void Check(bool ok, const char* msg)
{
    if (!ok) { std::cerr << "Career progress FAIL: " << msg << '\n'; std::exit(1); }
}
static latebrake::CareerFinish Finish(int laps, double seconds = 90.0,
                                     int recoveries = 0, int vehicle = 0,
                                     double fastest = 25.0, double slowest = 30.0)
{
    return {seconds, fastest, slowest, laps, recoveries, vehicle, true};
}
int main()
{
    using namespace latebrake;
    CareerProgress career;
    Check(career.CompletedCount() == 0, "fresh zero completions");
    Check(career.Status(0) == CareerStatus::Available, "rookie available");
    Check(career.Status(1) == CareerStatus::Locked, "second round locked");
    Check(!career.Select(1), "cannot skip round");
    Check(career.NextAvailable() == 0, "first available round");

    Check(career.Select(0), "select first round");
    const auto first = career.CommitFinish(0, Finish(1));
    Check(first.ValidFinish && first.FirstCompletion && first.Medal == CareerMedal::Gold,
          "first win gives a medal");
    Check(first.Credits == 100 && first.NewlyAvailable == 1, "one-time round reward unlocks round 2");
    Check(career.CompletedCount() == 1, "completion count updates");
    Check(career.Status(0) == CareerStatus::Mastered, "gold is mastered");
    Check(career.ChampionshipPoints(0) == 10, "gold gives 10 points");
    Check(!career.CommitFinish(0, Finish(1)).FirstCompletion &&
          career.CommitFinish(0, Finish(1)).Credits == 0,
          "repeat call never duplicates credits");

    Check(career.Select(1), "select unlocked round 2");
    Check(career.CommitFinish(1, Finish(2, 180.0)).Credits == 150, "second round credits");
    Check(career.Select(2), "select final round");
    auto champion = career.CommitFinish(2, Finish(3, 270.0));
    Check(champion.FirstCompletion && champion.ChampionshipBonus == 350 &&
          champion.Credits == 575, "championship first completion bonus");
    Check(career.ChampionshipComplete(0), "series finishes");
    Check(career.ChampionshipPoints(0) == 30, "three gold rounds award 30 points");
    Check(career.Status(3) == CareerStatus::Available, "second championship unlock");

    const CareerSnapshot saved = career.Snapshot();
    CareerProgress reloaded;
    reloaded.Load(saved);
    Check(reloaded.CompletedCount() == 3 && reloaded.ChampionshipComplete(0),
          "save/restore retains finished series");
    Check(reloaded.Snapshot().Medals == saved.Medals, "save reload idempotent");
    Check(reloaded.Select(2), "can replay completed round");
    Check(reloaded.CommitFinish(2, Finish(3, 270.0)).Credits == 0,
          "reload does not duplicate championship reward");

    // Improvement changes medals and points, but never gives extra credit.
    Check(reloaded.Select(6), "independent challenge unlocked");
    const auto bronze = reloaded.CommitFinish(6, Finish(1, 155.0));
    Check(bronze.Medal == CareerMedal::Bronze && bronze.Credits == 100,
          "first bronze gives first-completion credits");
    const auto silver = reloaded.CommitFinish(6, Finish(1, 120.0));
    Check(silver.MedalImproved && !silver.FirstCompletion && silver.Credits == 0,
          "upgrade medal without farming");
    Check(reloaded.Medal(6) == CareerMedal::Silver, "medal upgrade persisted");
    Check(reloaded.CommitFinish(6, Finish(1, 159.0)).Credits == 0,
          "weaker replay cannot downgrade best");

    // Tampered impossible progression and future schema are rejected.
    CareerSnapshot corrupt{};
    corrupt.Medals[2] = 3; // Missing prerequisite rounds.
    corrupt.Medals[7] = 255;
    corrupt.SelectedEvent = 2;
    CareerProgress repaired;
    repaired.Load(corrupt);
    Check(repaired.CompletedCount() == 0 && repaired.SelectedEvent() == -1,
          "invalid/orphan medals cannot grant unlocks");
    corrupt.Schema = std::numeric_limits<int>::max();
    corrupt.Medals[0] = 3;
    repaired.Load(corrupt);
    Check(repaired.CompletedCount() == 0 && repaired.SelectedEvent() == -1,
          "future schema not interpreted as valid present schema");
    corrupt.Schema = 0;
    repaired.Load(corrupt);
    Check(repaired.CompletedCount() == 0, "legacy profile loads with empty career");
    Check(repaired.CommitFinish(-1, Finish(1)).Credits == 0,
          "unknown IDs not rewarded");
    Check(repaired.ChampionshipPoints(99) == 0, "bad series ID does not score");
    std::cout << "Career progress passed.\n";
    return 0;
}

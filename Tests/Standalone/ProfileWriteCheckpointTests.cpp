#include "Progression/ProfileWriteCheckpoint.h"
#include <cstdlib>
#include <iostream>

static void Check(bool ok, const char* why)
{
    if (!ok) { std::cerr << "Profile write checkpoint FAIL: " << why << '\n'; std::exit(1); }
}

static latebrake::CareerFinish ValidRookieFinish()
{
    return {90.0, 25.0, 30.0, 1, 0, 0, true};
}

int main()
{
    using namespace latebrake;
    GarageState garage;
    CareerProgress career;
    int races = 2;
    float best = 120.f;
    float lap = 35.f;
    float attack = 115.f;

    Check(career.Select(0), "rookie available for transaction fixture");
    // Failed disk write must restore medals/credits and the stored race records.
    {
        ProfileWriteCheckpoint checkpoint(garage, career, races, best, lap, attack);
        const auto award = career.CommitFinish(0, ValidRookieFinish());
        Check(award.FirstCompletion && award.Credits == 100, "valid first award");
        garage.EarnCredits(award.Credits);
        races = 3;
        best = 90.f;
        lap = 25.f;
        attack = 90.f;
        checkpoint.Rollback();
        checkpoint.Rollback(); // idempotent: never revert a later change twice.
        Check(garage.Credits() == 0 && career.CompletedCount() == 0, "failed save rolls back bank and medal");
        Check(races == 2 && best == 120.f && lap == 35.f && attack == 115.f,
              "failed save restores all records");
        Check(career.SelectedEvent() == 0, "failed save retains prior event selection");
    }

    // Retry after a failed save is still a first completion, never a replay.
    {
        ProfileWriteCheckpoint checkpoint(garage, career, races, best, lap, attack);
        const auto again = career.CommitFinish(0, ValidRookieFinish());
        garage.EarnCredits(again.Credits);
        Check(again.FirstCompletion && again.Credits == 100, "retry earns first completion");
        races = 3;
        best = 90.f;
        lap = 25.f;
        checkpoint.Commit(); // an actual successful SaveGameToSlot is required.
    }
    Check(garage.Credits() == 100 && career.CompletedCount() == 1 && races == 3,
          "successful write leaves profile changes intact");
    Check(best == 90.f && lap == 25.f && attack == 115.f, "success keeps only changed records");

    // An abandoned/early-return scope automatically restores changes.
    {
        ProfileWriteCheckpoint checkpoint(garage, career, races, best, lap, attack);
        garage.EarnCredits(600);
        Check(garage.Unlock(1), "fixture can unlock purchased car");
        Check(garage.Select(1), "fixture can select purchased car");
        Check(garage.Upgrade(UpgradeKind::Engine), "fixture can buy engine upgrade");
    }
    Check(garage.Credits() == 100 && !garage.Owns(1) &&
          garage.SelectedVehicle() == 0 && garage.EngineLevel() == 0,
          "scope fallback restores purchase, tuning and selection");

    // Failed write after earning more credits must restore even an
    // existing persisted medal and all later championship prerequisites.
    {
        ProfileWriteCheckpoint checkpoint(garage, career, races, best, lap, attack);
        Check(career.Select(1), "round 2 unlocked by previously saved medal");
        const auto award = career.CommitFinish(1, {180., 60., 64., 2, 0, 0, true});
        Check(award.FirstCompletion && award.Credits == 150, "round 2 award");
        garage.EarnCredits(award.Credits);
    }
    Check(career.CompletedCount() == 1 && career.Status(2) == CareerStatus::Locked,
          "failed sequential round cannot unlock subsequent rounds");
    Check(garage.Credits() == 100, "failed sequential round cannot bank credits");

    // Quick-race credits and timing records are protected by the same
    // save transaction; explicit rollback is safe before UI refresh.
    {
        ProfileWriteCheckpoint checkpoint(garage, career, races, best, lap, attack);
        garage.EarnCredits(45);
        races = 4;
        best = 80.f;
        lap = 22.f;
        checkpoint.Rollback();
        Check(garage.Credits() == 100 && races == 3 &&
              best == 90.f && lap == 25.f, "quick race save failure rolled back");
    }

    // Once committed, destruction must never erase the successfully saved
    // progression even if subsequent gameplay changes the in-memory state.
    {
        ProfileWriteCheckpoint checkpoint(garage, career, races, best, lap, attack);
        garage.EarnCredits(10);
        checkpoint.Commit();
        garage.EarnCredits(5);
    }
    Check(garage.Credits() == 115, "committed changes survive guard destruction");
    std::cout << "Profile write checkpoint passed.\n";
    return 0;
}

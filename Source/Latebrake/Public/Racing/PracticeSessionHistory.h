#pragma once

#include "Garage/GarageTuningKey.h"
#include "Racing/FinishTimingEligibility.h"
#include "Racing/PracticeAttemptSeries.h"
#include "Racing/QuickRaceModePicker.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake
{
// Each practice configuration has its own session-only record. Garage
// appearance, current credits and ownership of other cars are irrelevant.
// Recorded clocks include all charged race penalties.
struct PracticeConfiguration final
{
    QuickPracticePreset Preset=QuickPracticePreset::None;
    GarageTuningKey Tuning{};
    int Handling=-1; // 0 arcade, 1 simulation-inspired.

    bool operator==(const PracticeConfiguration& other) const noexcept
    {
        return Preset==other.Preset && Tuning==other.Tuning &&
            Handling==other.Handling;
    }
};

inline constexpr std::size_t PracticeConfigurationCount=4u*VehicleCount*4u*4u*2u;

inline int PracticeConfigurationIndex(const PracticeConfiguration& key) noexcept
{
    if(!IsQuickPractice(key.Preset) || !GarageState::IsVehicle(key.Tuning.Selected) ||
        key.Tuning.Engine<0 || key.Tuning.Engine>3 ||
        key.Tuning.Grip<0 || key.Tuning.Grip>3 ||
        key.Handling<0 || key.Handling>1)
        return -1;
    int preset=-1;
    switch(key.Preset)
    {
        case QuickPracticePreset::SoloThree:preset=0;break;
        case QuickPracticePreset::SoloFive:preset=1;break;
        case QuickPracticePreset::RivalFive:preset=2;break;
        case QuickPracticePreset::RivalTen:preset=3;break;
        default:return -1;
    }
    return ((((preset*VehicleCount+key.Tuning.Selected)*4+key.Tuning.Engine)*4+
        key.Tuning.Grip)*2+key.Handling);
}

struct PracticeSessionRecord final
{
    std::uint32_t FinishedAttempts=0;
    double BestFinishSeconds=0.0;
    double BestLapSeconds=0.0;
    double LastFinishSeconds=0.0;
    double LastPenaltySeconds=0.0;
};

enum class PracticeComparisonStatus : std::uint8_t
{
    Unavailable,
    ConfigurationChanged,
    First,
    Improved,
    Matched,
    Slower
};

struct PracticeComparison final
{
    PracticeComparisonStatus Status=PracticeComparisonStatus::Unavailable;
    PracticeConfiguration Configuration{};
    PracticeSessionRecord Record{};
    double PreviousBestFinishSeconds=0.0;
    double PreviousFinishSeconds=0.0;
    double DeltaToPreviousBestSeconds=0.0;
    double DeltaToPreviousFinishSeconds=0.0;
};

// Fixed-size in-memory store, exactly one slot per supported configuration.
// Begin/Observe/Finish are driven by the actual player attempt. A changed
// configuration or pawn irreversibly invalidates that attempt's comparison;
// switching back cannot launder a mixed-tuning time into a reference.
class PracticeSessionHistory final
{
public:
    void CancelAttempt() noexcept
    {
        Active=false;
        Changed=false;
        Key={};
        Actor=0;
        Last={};
    }

    void ResetSession() noexcept
    {
        Records.fill({});
        for(auto& series:Series)series.Reset();
        BumpRevision();
        CancelAttempt();
    }

    bool Begin(PracticeConfiguration key,std::uint64_t actor) noexcept
    {
        CancelAttempt();
        if(PracticeConfigurationIndex(key)<0 || actor==0) return false;
        Active=true;
        Key=key;
        Actor=actor;
        return true;
    }

    void ObserveConfiguration(PracticeConfiguration key,std::uint64_t actor) noexcept
    {
        if(Active && (!(Key==key) || actor!=Actor || actor==0))
            Changed=true;
    }

    void InvalidateConfiguration() noexcept
    {
        if(Active) Changed=true;
    }

    PracticeSessionRecord Read(PracticeConfiguration key) const noexcept
    {
        const int index=PracticeConfigurationIndex(key);
        return index>=0?Records[static_cast<std::size_t>(index)]:PracticeSessionRecord{};
    }

    PracticeAttemptSeriesReview ReadSeries(PracticeConfiguration key) const noexcept
    {
        const int index=PracticeConfigurationIndex(key);
        return index>=0?Series[static_cast<std::size_t>(index)].Read():PracticeAttemptSeriesReview{};
    }

    PracticeComparison Finish(PracticeConfiguration key,std::uint64_t actor,
        const RaceProgress& race) noexcept
    {
        if(!Active) return {};
        ObserveConfiguration(key,actor);
        Active=false;
        Last={};
        if(Changed)
        {
            Last.Status=PracticeComparisonStatus::ConfigurationChanged;
            return Last;
        }
        const int index=PracticeConfigurationIndex(Key);
        const auto definition=GetQuickPracticeDefinition(Key.Preset);
        if(index<0 || !definition.Valid ||
            race.GetCheckpointCount()!=definition.CheckpointCount ||
            race.GetRequiredLaps()!=definition.RequiredLaps ||
            !CanAwardFinishedRace(race))
            return Last;
        const auto full=race.GetWholeRaceLapSummary();
        if(!full.Valid || full.All.Count!=definition.RequiredLaps)
            return Last;
        // Keep series and comparison records transactionally aligned.
        const auto sample=MakePracticeAttemptSample(race);
        if(!Series[static_cast<std::size_t>(index)].Observe(sample))return Last;
        const double seconds=race.GetElapsedSeconds();
        const double lap=race.GetBestLapSeconds();
        auto& record=Records[static_cast<std::size_t>(index)];
        Last.Configuration=Key;
        Last.PreviousBestFinishSeconds=record.BestFinishSeconds;
        Last.PreviousFinishSeconds=record.LastFinishSeconds;
        if(record.FinishedAttempts==0)
        {
            Last.Status=PracticeComparisonStatus::First;
            record.BestFinishSeconds=seconds;
            record.BestLapSeconds=lap;
        }
        else
        {
            Last.DeltaToPreviousBestSeconds=seconds-record.BestFinishSeconds;
            Last.DeltaToPreviousFinishSeconds=seconds-record.LastFinishSeconds;
            if(seconds<record.BestFinishSeconds)
            {
                record.BestFinishSeconds=seconds;
                Last.Status=PracticeComparisonStatus::Improved;
            }
            else if(seconds==record.BestFinishSeconds)
                Last.Status=PracticeComparisonStatus::Matched;
            else Last.Status=PracticeComparisonStatus::Slower;
            if(lap<record.BestLapSeconds) record.BestLapSeconds=lap;
        }
        if(record.FinishedAttempts!=std::numeric_limits<std::uint32_t>::max())
            ++record.FinishedAttempts;
        record.LastFinishSeconds=seconds;
        record.LastPenaltySeconds=race.GetTotalPenaltySeconds();
        Last.Record=record;
        BumpRevision();
        return Last;
    }

    PracticeComparison LastComparison() const noexcept {return Last;}
    // Only durable session observations change this revision. Beginning,
    // canceling or rejecting an attempt does not invalidate analysis.
    std::uint64_t AnalysisRevision() const noexcept {return Revision;}

private:
    void BumpRevision() noexcept
    {
        ++Revision;
        if(Revision==0)++Revision;
    }
    std::uint64_t Revision=1;
    std::array<PracticeSessionRecord,PracticeConfigurationCount> Records{};
    std::array<PracticeAttemptSeries,PracticeConfigurationCount> Series{};
    bool Active=false;
    bool Changed=false;
    PracticeConfiguration Key{};
    std::uint64_t Actor=0;
    PracticeComparison Last{};
};
} // namespace latebrake

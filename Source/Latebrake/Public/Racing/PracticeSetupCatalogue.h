#pragma once

#include "Racing/PracticeSessionHistory.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake
{
// Read-only comparison of actually completed setups, within one preset
// and one handling mode. Solo/rival and different lap-count runs are never
// compared. Different tuning is a correlation, not an isolated causal test.
inline constexpr int PracticeSetupsPerCar=16;
inline constexpr int PracticeSetupsPerGroup=VehicleCount*PracticeSetupsPerCar;

enum class PracticeSetupMetric : std::uint8_t
{
    BestFinish, BestFeeFreeFinish, BestLap, BestFeeFreeLap, MeanFinish
};

struct PracticeSetupEntry final
{
    bool Valid=false;
    PracticeConfiguration Key{};
    std::uint32_t Finishes=0;
    std::uint32_t FeeFreeFinishes=0;
    double BestFinish=0;
    double BestFeeFreeFinish=0;
    double BestLap=0;
    double BestFeeFreeLap=0;
    double MeanFinish=0;
    double FinishDeviation=0;

    double Value(PracticeSetupMetric metric) const noexcept
    {
        if(!Valid)return 0;
        switch(metric)
        {
        case PracticeSetupMetric::BestFinish:return BestFinish;
        case PracticeSetupMetric::BestFeeFreeFinish:return BestFeeFreeFinish;
        case PracticeSetupMetric::BestLap:return BestLap;
        case PracticeSetupMetric::BestFeeFreeLap:return BestFeeFreeLap;
        case PracticeSetupMetric::MeanFinish:return MeanFinish;
        default:return 0;
        }
    }
};

inline PracticeSetupEntry ReadPracticeSetupEntry(const PracticeSessionHistory& history,
    PracticeConfiguration key) noexcept
{
    const auto record=history.Read(key);
    const auto series=history.ReadSeries(key);
    if(PracticeConfigurationIndex(key)<0 || !series.Valid ||
        record.FinishedAttempts==0 || series.Finishes!=record.FinishedAttempts ||
        series.Finish.Minimum!=record.BestFinishSeconds ||
        series.BestLap.Minimum!=record.BestLapSeconds)
        return {};
    return {true,key,record.FinishedAttempts,series.FeeFreeFinishes,
        record.BestFinishSeconds,series.FeeFreeFinish.Minimum,
        record.BestLapSeconds,series.BestFeeFreeLap,
        series.Finish.Mean,series.Finish.PopulationDeviation};
}

// Deterministic ties by exact configuration slot; a tie is not claimed
// as an improvement over another setup.
inline bool BetterPracticeSetup(const PracticeSetupEntry& candidate,
    const PracticeSetupEntry& incumbent,PracticeSetupMetric metric) noexcept
{
    const double value=candidate.Value(metric),other=incumbent.Value(metric);
    if(!IsValidFinishedRaceTime(value))return false;
    if(!IsValidFinishedRaceTime(other))return true;
    if(value!=other)return value<other;
    return PracticeConfigurationIndex(candidate.Key)<PracticeConfigurationIndex(incumbent.Key);
}

struct PracticeSetupCoverage final
{
    int Tested=0;
    int FeeFreeTested=0;
    std::uint64_t Finishes=0;
    std::uint64_t FeeFreeFinishes=0;
    PracticeSetupEntry FastestFinish{};
    PracticeSetupEntry FastestFeeFreeFinish{};
    PracticeSetupEntry FastestLap{};
    PracticeSetupEntry FastestFeeFreeLap{};
    PracticeSetupEntry LowestMean{};

    void Include(const PracticeSetupEntry& entry) noexcept
    {
        if(!entry.Valid)return;
        ++Tested;
        if(entry.FeeFreeFinishes)++FeeFreeTested;
        Finishes+=entry.Finishes;
        FeeFreeFinishes+=entry.FeeFreeFinishes;
        if(BetterPracticeSetup(entry,FastestFinish,PracticeSetupMetric::BestFinish))FastestFinish=entry;
        if(BetterPracticeSetup(entry,FastestFeeFreeFinish,PracticeSetupMetric::BestFeeFreeFinish))FastestFeeFreeFinish=entry;
        if(BetterPracticeSetup(entry,FastestLap,PracticeSetupMetric::BestLap))FastestLap=entry;
        if(BetterPracticeSetup(entry,FastestFeeFreeLap,PracticeSetupMetric::BestFeeFreeLap))FastestFeeFreeLap=entry;
        if(BetterPracticeSetup(entry,LowestMean,PracticeSetupMetric::MeanFinish))LowestMean=entry;
    }
};

struct PracticeSetupCatalogue final
{
    bool Valid=false;
    QuickPracticePreset Preset=QuickPracticePreset::None;
    int Handling=-1;
    PracticeSetupEntry Selected{};
    PracticeSetupCoverage All{};
    std::array<PracticeSetupCoverage,VehicleCount> Cars{};
    std::array<PracticeSetupCoverage,4> Engines{};
    std::array<PracticeSetupCoverage,4> Grips{};
    std::array<PracticeSetupEntry,PracticeSetupsPerGroup> Entries{};

    PracticeSetupEntry At(int car,int engine,int grip) const noexcept
    {
        if(!Valid || !ValidCoordinates(car,engine,grip))return {};
        return Entries[static_cast<std::size_t>(Slot(car,engine,grip))];
    }

    static constexpr bool ValidCoordinates(int car,int engine,int grip) noexcept
    {
        return GarageState::IsVehicle(car) && engine>=0 && engine<4 && grip>=0 && grip<4;
    }

    static constexpr int Slot(int car,int engine,int grip) noexcept
    {
        return (car*4+engine)*4+grip;
    }
};

inline PracticeSetupCatalogue BuildPracticeSetupCatalogue(const PracticeSessionHistory& history,
    PracticeConfiguration selected) noexcept
{
    if(PracticeConfigurationIndex(selected)<0)return {};
    PracticeSetupCatalogue out;
    out.Valid=true;out.Preset=selected.Preset;out.Handling=selected.Handling;
    for(int car=0;car<VehicleCount;++car)
        for(int engine=0;engine<4;++engine)
            for(int grip=0;grip<4;++grip)
            {
                const PracticeConfiguration key{selected.Preset,{car,engine,grip},selected.Handling};
                const auto row=ReadPracticeSetupEntry(history,key);
                out.Entries[static_cast<std::size_t>(PracticeSetupCatalogue::Slot(car,engine,grip))]=row;
                if(!row.Valid)continue;
                out.All.Include(row);
                out.Cars[static_cast<std::size_t>(car)].Include(row);
                out.Engines[static_cast<std::size_t>(engine)].Include(row);
                out.Grips[static_cast<std::size_t>(grip)].Include(row);
                if(key==selected)out.Selected=row;
            }
    return out;
}

struct PracticeSetupPair final
{
    bool Valid=false;
    PracticeSetupEntry Reference{};
    PracticeSetupEntry Candidate{};
    PracticeSetupMetric Metric=PracticeSetupMetric::BestFinish;
    double DeltaSeconds=0; // positive candidate slower

    double DeltaPercent() const noexcept
    {
        return Valid?100.0*DeltaSeconds/Reference.Value(Metric):0;
    }
};

inline PracticeSetupPair ComparePracticeSetups(const PracticeSetupEntry& reference,
    const PracticeSetupEntry& candidate,PracticeSetupMetric metric) noexcept
{
    if(!reference.Valid || !candidate.Valid ||
        PracticeConfigurationIndex(reference.Key)<0 ||
        PracticeConfigurationIndex(candidate.Key)<0 ||
        reference.Key.Preset!=candidate.Key.Preset ||
        reference.Key.Handling!=candidate.Key.Handling ||
        reference.Key==candidate.Key)return {};
    const double a=reference.Value(metric),b=candidate.Value(metric);
    if(!IsValidFinishedRaceTime(a) || !IsValidFinishedRaceTime(b))return {};
    return {true,reference,candidate,metric,b-a};
}

enum class PracticeSetupAxis : std::uint8_t
{
    Vehicle, Engine, Grip
};

inline bool DiffersOnlyOnPracticeAxis(PracticeConfiguration a,
    PracticeConfiguration b,PracticeSetupAxis axis) noexcept
{
    if(PracticeConfigurationIndex(a)<0 || PracticeConfigurationIndex(b)<0 ||
        a.Preset!=b.Preset || a.Handling!=b.Handling)return false;
    const bool car=a.Tuning.Selected!=b.Tuning.Selected;
    const bool engine=a.Tuning.Engine!=b.Tuning.Engine;
    const bool grip=a.Tuning.Grip!=b.Tuning.Grip;
    switch(axis)
    {
    case PracticeSetupAxis::Vehicle:return car && !engine && !grip;
    case PracticeSetupAxis::Engine:return !car && engine && !grip;
    case PracticeSetupAxis::Grip:return !car && !engine && grip;
    default:return false;
    }
}

struct PracticeAxisComparison final
{
    bool Valid=false;
    PracticeSetupAxis Axis=PracticeSetupAxis::Vehicle;
    int TestedAlternatives=0;
    int ComparableAlternatives=0;
    PracticeSetupPair FastestAlternative{};
    PracticeSetupPair SlowestAlternative{};
};

inline PracticeAxisComparison ReviewPracticeAxis(const PracticeSetupCatalogue& catalogue,
    PracticeSetupAxis axis,PracticeSetupMetric metric) noexcept
{
    if(!catalogue.Valid || !catalogue.Selected.Valid ||
        (axis!=PracticeSetupAxis::Vehicle && axis!=PracticeSetupAxis::Engine &&
            axis!=PracticeSetupAxis::Grip))return {};
    PracticeAxisComparison out;
    out.Valid=true;out.Axis=axis;
    for(const auto& entry:catalogue.Entries)
    {
        if(!entry.Valid || !DiffersOnlyOnPracticeAxis(catalogue.Selected.Key,entry.Key,axis))continue;
        ++out.TestedAlternatives;
        const auto pair=ComparePracticeSetups(catalogue.Selected,entry,metric);
        if(!pair.Valid)continue;
        ++out.ComparableAlternatives;
        if(!out.FastestAlternative.Valid ||
            BetterPracticeSetup(entry,out.FastestAlternative.Candidate,metric))
            out.FastestAlternative=pair;
        if(!out.SlowestAlternative.Valid || pair.DeltaSeconds>out.SlowestAlternative.DeltaSeconds ||
            (pair.DeltaSeconds==out.SlowestAlternative.DeltaSeconds &&
                PracticeConfigurationIndex(entry.Key)<PracticeConfigurationIndex(out.SlowestAlternative.Candidate.Key)))
            out.SlowestAlternative=pair;
    }
    return out;
}

struct PracticeSetupRanking final
{
    bool Valid=false;
    PracticeSetupMetric Metric=PracticeSetupMetric::BestFinish;
    int Count=0;
    int SelectedRank=0; // 1-based deterministic listing, not competitive placement.
    std::array<PracticeSetupEntry,PracticeSetupsPerGroup> Rows{};
};

inline PracticeSetupRanking RankPracticeSetups(const PracticeSetupCatalogue& catalogue,
    PracticeSetupMetric metric,int car=-1) noexcept
{
    if(!catalogue.Valid || (car!=-1 && !GarageState::IsVehicle(car)))return {};
    PracticeSetupRanking out;
    out.Valid=true;out.Metric=metric;
    for(const auto& row:catalogue.Entries)
    {
        if(!row.Valid || (car!=-1 && row.Key.Tuning.Selected!=car) ||
            !IsValidFinishedRaceTime(row.Value(metric)))continue;
        // Fixed bounded insertion; no heap allocations or comparator side effects.
        int at=out.Count;
        while(at>0 && BetterPracticeSetup(row,out.Rows[static_cast<std::size_t>(at-1)],metric))
        {
            out.Rows[static_cast<std::size_t>(at)]=out.Rows[static_cast<std::size_t>(at-1)];
            --at;
        }
        out.Rows[static_cast<std::size_t>(at)]=row;
        ++out.Count;
    }
    if(catalogue.Selected.Valid)
        for(int i=0;i<out.Count;++i)
            if(out.Rows[static_cast<std::size_t>(i)].Key==catalogue.Selected.Key)
                out.SelectedRank=i+1;
    return out;
}
} // namespace latebrake

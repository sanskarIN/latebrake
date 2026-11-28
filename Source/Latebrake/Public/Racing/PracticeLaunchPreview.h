#pragma once

#include "Racing/PracticeGateReference.h"
#include "Racing/PracticeTrialSetup.h"

namespace latebrake
{
// Pre-launch advisory snapshot for the EXACT staged vehicle, event and
// handling. A complete gate reference can legitimately be slower than the
// best finish if the faster attempt lacked complete sector evidence.
struct PracticeLaunchPreview final
{
    bool Valid=false;
    PracticeConfiguration Key{};
    int Laps=0;
    int Gates=0;
    int Rivals=0;
    bool TemporaryTrial=false;
    bool Owned=false;
    VehiclePerformance Performance{};
    bool HasMeasuredFinish=false;
    bool HasFeeFreeFinish=false;
    bool HasExactGateReference=false;
    PracticeSessionRecord Record{};
    PracticeAttemptSeriesReview Series{};
    double GateReferenceSeconds=0;
    double GateReferenceGapSeconds=0;
};

inline PracticeLaunchPreview BuildPracticeLaunchPreview(PracticeConfiguration key,
    PracticeTrialReadout setup,const PracticeSessionHistory& history,
    const PracticeGateReferenceHistory& gates) noexcept
{
    if(PracticeConfigurationIndex(key)<0 || !(key.Tuning==setup.Key) ||
        !ValidPracticeTrialKey(setup.Key) || (!setup.Enabled && !setup.Owned))
        return {};
    const auto event=GetQuickPracticeDefinition(key.Preset);
    const auto performance=PracticeTrialPerformance(key.Tuning);
    if(!event.Valid || event.RequiredLaps<1 || event.CheckpointCount<1 ||
        event.RequiredLaps*event.CheckpointCount>PracticeMaximumGates)
        return {};
    PracticeLaunchPreview out;
    out.Valid=true;
    out.Key=key;
    out.Laps=event.RequiredLaps;
    out.Gates=event.RequiredLaps*event.CheckpointCount;
    out.Rivals=event.SpawnRival?1:0;
    out.TemporaryTrial=setup.Enabled;
    out.Owned=setup.Owned;
    out.Performance=performance;
    const auto record=history.Read(key);
    const auto series=history.ReadSeries(key);
    if(record.FinishedAttempts==0 || !series.Valid ||
        series.Finishes!=record.FinishedAttempts ||
        series.Finish.Minimum!=record.BestFinishSeconds ||
        series.BestLap.Minimum!=record.BestLapSeconds ||
        !IsValidFinishedRaceTime(record.BestFinishSeconds) ||
        !IsValidFinishedRaceTime(record.BestLapSeconds))
        return out;
    out.HasMeasuredFinish=true;
    out.Record=record;
    out.Series=series;
    out.HasFeeFreeFinish=series.FeeFreeFinishes>0 &&
        IsValidFinishedRaceTime(series.FeeFreeFinish.Minimum);
    const double reference=gates.ReferenceFinish(key);
    // A gate history alone is not accepted as a finish comparison. Both
    // stores must support the same exact staged setup.
    if(IsValidFinishedRaceTime(reference) &&
        reference+PracticeGateClockTolerance>=record.BestFinishSeconds)
    {
        out.HasExactGateReference=true;
        out.GateReferenceSeconds=reference;
        out.GateReferenceGapSeconds=std::max(0.0,reference-record.BestFinishSeconds);
    }
    return out;
}
} // namespace latebrake

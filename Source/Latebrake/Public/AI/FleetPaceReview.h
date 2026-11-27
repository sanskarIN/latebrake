#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>

namespace latebrake
{
// Attempt-local, observed active-clock statistics. The first lap includes
// the standing start, and all laps can include time spent recovering.
// No official lap record, clean-lap claim, rank, or reward is derived here.
inline constexpr int FleetPaceMaxGates = 256;
inline constexpr int FleetPaceMaxLaps = 100;
inline constexpr int FleetPaceCars = 6;
inline constexpr double FleetPaceMaxSeconds = 604800.0;
inline constexpr double FleetPaceClockWindow = 0.25;

struct FleetPaceMoments final
{
    int Count = 0;
    double Minimum = 0.0;
    double Maximum = 0.0;
    double Mean = 0.0;
    double M2 = 0.0;
    double Sum = 0.0;

    // A signed sample is allowed for a matched difference; durations are
    // validated before reaching this accumulator. Welford's recurrence
    // avoids subtracting nearly equal squares on stable lap sequences.
    bool Add(double value) noexcept
    {
        if (!std::isfinite(value) || std::abs(value) > FleetPaceMaxSeconds ||
            Count < 0 || Count >= FleetPaceMaxLaps)
            return false;
        if (Count == 0)
            Minimum = Maximum = value;
        else
        {
            Minimum = std::min(Minimum,value);
            Maximum = std::max(Maximum,value);
        }
        ++Count;
        Sum += value;
        const double delta = value-Mean;
        Mean += delta/static_cast<double>(Count);
        M2 += delta*(value-Mean);
        return true;
    }

    double PopulationDeviation() const noexcept
    {
        return Count > 0 ? std::sqrt(std::max(0.0,M2/static_cast<double>(Count))) : 0.0;
    }

    bool HasVariationEvidence() const noexcept
    {
        return Count >= 2;
    }
};

struct FleetPaceLap final
{
    int Number = 0;
    double StartSeconds = 0.0;
    double EndSeconds = 0.0;
    double DurationSeconds = 0.0;
    int SlowestSector = 0;
    double SlowestSectorSeconds = 0.0;
};

struct FleetPaceSector final
{
    int Gate = 0;
    FleetPaceMoments Duration{};
    int BestLap = 0;
    int WorstLap = 0;
    double LatestSeconds = 0.0;
};

struct FleetPaceLapSummary final
{
    bool Valid = false;
    int FirstLap = 0;
    int LastLap = 0;
    int Count = 0;
    int BestLap = 0;
    int WorstLap = 0;
    double TotalSeconds = 0.0;
    double BestSeconds = 0.0;
    double WorstSeconds = 0.0;
    double MeanSeconds = 0.0;
    double MedianSeconds = 0.0;
    bool VariationValid = false;
    double DeviationSeconds = 0.0;
    double LatestSeconds = 0.0;
    bool TrendValid = false;
    double LastMinusFirstSeconds = 0.0;
};

struct FleetPaceCarSummary final
{
    bool Valid = false;
    int Slot = -1;
    int AcceptedGates = 0;
    int CompleteLaps = 0;
    int PendingSectors = 0;
    double PendingElapsedSeconds = 0.0;
    FleetPaceLapSummary AllLaps{};
    FleetPaceLapSummary AfterLaunch{};
    bool SectorEvidenceValid = false;
    double IdealObservedSectorSum = 0.0;
    int SlowestMeanSector = 0;
    double SlowestMeanSectorSeconds = 0.0;
    bool SectorVariationValid = false;
    int MostVariableSector = 0;
    double MostVariableSectorDeviationSeconds = 0.0;
};

// Only completed laps update sector statistics. A fast sector in an
// incomplete lap is not mixed with a set of completed-lap statistics.
class FleetPaceStream final
{
public:
    void Reset() noexcept
    {
        Gates = 0;
        Laps = 0;
        LastOrdinal = 0;
        LastClock = 0.0;
        LapStart = 0.0;
        Completed = 0;
        Pending = {};
        History = {};
        Sectors = {};
    }

    bool Configure(int gates,int laps) noexcept
    {
        if (Gates != 0 || gates < 1 || gates > FleetPaceMaxGates ||
            laps < 1 || laps > FleetPaceMaxLaps)
            return false;
        Gates = gates;
        Laps = laps;
        for (int gate=0;gate<Gates;++gate)
            Sectors[static_cast<std::size_t>(gate)].Gate = gate+1;
        return true;
    }

    bool Record(int ordinal,double seconds) noexcept
    {
        if (!Gates || ordinal != LastOrdinal+1 || ordinal > Gates*Laps ||
            !std::isfinite(seconds) || seconds <= LastClock || seconds > FleetPaceMaxSeconds)
            return false;
        const int sector = (ordinal-1)%Gates;
        const double duration = seconds-LastClock;
        if (!std::isfinite(duration) || duration <= 0.0)
            return false;
        if (sector == Gates-1 && Completed >= Laps)
            return false;
        Pending[static_cast<std::size_t>(sector)] = duration;
        if (sector == Gates-1)
        {
            // Every prior sector has been accepted sequentially. Check the
            // whole completed lap before publishing its summary.
            const double lapDuration = seconds-LapStart;
            int slowest = 0;
            double slowestSeconds = 0.0;
            for (int i=0;i<Gates;++i)
            {
                const double value = Pending[static_cast<std::size_t>(i)];
                if (!std::isfinite(value) || value <= 0.0)
                    return false;
                if (value > slowestSeconds)
                {
                    slowest = i+1;
                    slowestSeconds = value;
                }
            }
            const int lap = Completed+1;
            for (int i=0;i<Gates;++i)
            {
                auto& profile = Sectors[static_cast<std::size_t>(i)];
                const double value = Pending[static_cast<std::size_t>(i)];
                if (profile.Duration.Count == 0 || value < profile.Duration.Minimum)
                    profile.BestLap = lap;
                if (profile.Duration.Count == 0 || value > profile.Duration.Maximum)
                    profile.WorstLap = lap;
                profile.Duration.Add(value);
                profile.LatestSeconds = value;
            }
            History[static_cast<std::size_t>(Completed)] =
                {lap,LapStart,seconds,lapDuration,slowest,slowestSeconds};
            ++Completed;
            LapStart = seconds;
            Pending.fill(0.0);
        }
        LastOrdinal = ordinal;
        LastClock = seconds;
        return true;
    }

    int GateCount() const noexcept { return Gates; }
    int RequiredLaps() const noexcept { return Laps; }
    int AcceptedGates() const noexcept { return LastOrdinal; }
    int CompleteLaps() const noexcept { return Completed; }
    double LastGateClock() const noexcept { return LastClock; }

    bool TryLap(int number,FleetPaceLap& out) const noexcept
    {
        out = {};
        if (number < 1 || number > Completed)
            return false;
        out = History[static_cast<std::size_t>(number-1)];
        return out.Number == number && out.DurationSeconds > 0.0;
    }

    bool TrySector(int gate,FleetPaceSector& out) const noexcept
    {
        out = {};
        if (gate < 1 || gate > Gates || Completed == 0)
            return false;
        out = Sectors[static_cast<std::size_t>(gate-1)];
        return out.Gate == gate && out.Duration.Count == Completed;
    }

    FleetPaceLapSummary Summarize(int first,int last) const noexcept
    {
        FleetPaceLapSummary out;
        if (first < 1 || last < first || last > Completed)
            return out;
        std::array<double,FleetPaceMaxLaps> sorted{};
        FleetPaceMoments moments;
        FleetPaceLap begin, end;
        if (!TryLap(first,begin) || !TryLap(last,end))
            return out;
        for (int lap=first;lap<=last;++lap)
        {
            FleetPaceLap observed;
            if (!TryLap(lap,observed))
                return {};
            const double time = observed.DurationSeconds;
            if (moments.Count == 0 || time < moments.Minimum)
                out.BestLap = lap;
            if (moments.Count == 0 || time > moments.Maximum)
                out.WorstLap = lap;
            sorted[static_cast<std::size_t>(moments.Count)] = time;
            if (!moments.Add(time))
                return {};
        }
        std::sort(sorted.begin(),sorted.begin()+moments.Count);
        const int middle = moments.Count/2;
        out.Valid = true;
        out.FirstLap = first;
        out.LastLap = last;
        out.Count = moments.Count;
        out.TotalSeconds = moments.Sum;
        out.BestSeconds = moments.Minimum;
        out.WorstSeconds = moments.Maximum;
        out.MeanSeconds = moments.Mean;
        out.MedianSeconds = moments.Count%2
            ? sorted[static_cast<std::size_t>(middle)]
            : (sorted[static_cast<std::size_t>(middle-1)] +
                sorted[static_cast<std::size_t>(middle)])*0.5;
        out.VariationValid = moments.HasVariationEvidence();
        out.DeviationSeconds = moments.PopulationDeviation();
        out.LatestSeconds = end.DurationSeconds;
        out.TrendValid = moments.Count >= 2;
        out.LastMinusFirstSeconds = out.TrendValid
            ? end.DurationSeconds-begin.DurationSeconds : 0.0;
        return out;
    }

    FleetPaceCarSummary Summary(int slot) const noexcept
    {
        FleetPaceCarSummary out;
        if (Gates == 0 || slot < 0 || slot >= FleetPaceCars)
            return out;
        out.Valid = true;
        out.Slot = slot;
        out.AcceptedGates = LastOrdinal;
        out.CompleteLaps = Completed;
        out.PendingSectors = LastOrdinal%Gates;
        out.PendingElapsedSeconds = LastClock-LapStart;
        out.AllLaps = Summarize(1,Completed);
        // Offer a separate sample excluding the standing-start lap.
        // It remains an observation, not a clean/flying lap certification.
        out.AfterLaunch = Summarize(2,Completed);
        if (!out.AllLaps.Valid)
            return out;
        double sum = 0.0;
        double slowestMean = -1.0;
        double mostVariable = -1.0;
        for (int gate=1;gate<=Gates;++gate)
        {
            FleetPaceSector profile;
            if (!TrySector(gate,profile))
                return out;
            sum += profile.Duration.Minimum;
            if (profile.Duration.Mean > slowestMean)
            {
                slowestMean = profile.Duration.Mean;
                out.SlowestMeanSector = gate;
            }
            if (profile.Duration.HasVariationEvidence())
            {
                const double deviation = profile.Duration.PopulationDeviation();
                if (deviation > mostVariable)
                {
                    mostVariable = deviation;
                    out.MostVariableSector = gate;
                }
            }
        }
        out.SectorEvidenceValid = true;
        // Sum of independently observed best sectors. This is not a
        // completed lap and is never stored as a lap record.
        out.IdealObservedSectorSum = sum;
        out.SlowestMeanSectorSeconds = slowestMean;
        out.SectorVariationValid = mostVariable >= 0.0;
        out.MostVariableSectorDeviationSeconds = out.SectorVariationValid ? mostVariable : 0.0;
        return out;
    }

private:
    int Gates = 0;
    int Laps = 0;
    int LastOrdinal = 0;
    int Completed = 0;
    double LastClock = 0.0;
    double LapStart = 0.0;
    std::array<double,FleetPaceMaxGates> Pending{};
    std::array<FleetPaceLap,FleetPaceMaxLaps> History{};
    std::array<FleetPaceSector,FleetPaceMaxGates> Sectors{};
};

enum class FleetPaceRelation
{
    Unavailable,
    PlayerShorter,
    WithinWindow,
    RivalShorter
};

inline FleetPaceRelation FleetPaceCompareDuration(double rivalMinusPlayer) noexcept
{
    if (!std::isfinite(rivalMinusPlayer))
        return FleetPaceRelation::Unavailable;
    if (std::abs(rivalMinusPlayer) <= FleetPaceClockWindow)
        return FleetPaceRelation::WithinWindow;
    return rivalMinusPlayer > 0.0
        ? FleetPaceRelation::PlayerShorter : FleetPaceRelation::RivalShorter;
}

struct FleetPacePairedLap final
{
    int Number = 0;
    double PlayerSeconds = 0.0;
    double RivalSeconds = 0.0;
    double RivalMinusPlayerSeconds = 0.0;
    FleetPaceRelation Relation = FleetPaceRelation::Unavailable;
};

struct FleetPacePairSummary final
{
    bool Valid = false;
    int FirstLap = 0;
    int LastLap = 0;
    int Count = 0;
    int PlayerShorter = 0;
    int RivalShorter = 0;
    int Close = 0;
    FleetPaceLapSummary Player{};
    FleetPaceLapSummary Rival{};
    FleetPaceMoments Difference{};
    double MeanAbsoluteDifference = 0.0;
    double TotalRivalMinusPlayerSeconds = 0.0;
    FleetPacePairedLap Latest{};
    FleetPacePairedLap BiggestPlayerGain{};
    FleetPacePairedLap BiggestPlayerLoss{};
};

inline bool SameFleetPaceShape(const FleetPaceStream& player,
    const FleetPaceStream& rival) noexcept
{
    return player.GateCount() > 0 &&
        player.GateCount() == rival.GateCount() &&
        player.RequiredLaps() == rival.RequiredLaps();
}

inline FleetPacePairSummary CompareFleetPaceLaps(const FleetPaceStream& player,
    const FleetPaceStream& rival,int first,int last) noexcept
{
    FleetPacePairSummary out;
    if (!SameFleetPaceShape(player,rival) || first < 1 || last < first ||
        last > std::min(player.CompleteLaps(),rival.CompleteLaps()))
        return out;
    out.Player = player.Summarize(first,last);
    out.Rival = rival.Summarize(first,last);
    if (!out.Player.Valid || !out.Rival.Valid)
        return {};
    for (int lap=first;lap<=last;++lap)
    {
        FleetPaceLap p,r;
        if (!player.TryLap(lap,p) || !rival.TryLap(lap,r))
            return {};
        const double delta = r.DurationSeconds-p.DurationSeconds;
        const auto relation = FleetPaceCompareDuration(delta);
        if (relation == FleetPaceRelation::Unavailable || !out.Difference.Add(delta))
            return {};
        const FleetPacePairedLap row{lap,p.DurationSeconds,r.DurationSeconds,delta,relation};
        if (relation == FleetPaceRelation::PlayerShorter)
        {
            ++out.PlayerShorter;
            if (!out.BiggestPlayerGain.Number ||
                delta > out.BiggestPlayerGain.RivalMinusPlayerSeconds)
                out.BiggestPlayerGain = row;
        }
        else if (relation == FleetPaceRelation::RivalShorter)
        {
            ++out.RivalShorter;
            if (!out.BiggestPlayerLoss.Number ||
                delta < out.BiggestPlayerLoss.RivalMinusPlayerSeconds)
                out.BiggestPlayerLoss = row;
        }
        else
            ++out.Close;
        out.MeanAbsoluteDifference += std::abs(delta);
        out.Latest = row;
    }
    out.Valid = true;
    out.FirstLap = first;
    out.LastLap = last;
    out.Count = out.Difference.Count;
    out.MeanAbsoluteDifference /= static_cast<double>(out.Count);
    out.TotalRivalMinusPlayerSeconds = out.Difference.Sum;
    return out;
}

struct FleetPaceSectorDelta final
{
    bool Valid = false;
    int Gate = 0;
    int Laps = 0;
    double PlayerMeanSeconds = 0.0;
    double RivalMeanSeconds = 0.0;
    double RivalMinusPlayerSeconds = 0.0;
    double PlayerBestSeconds = 0.0;
    double RivalBestSeconds = 0.0;
    FleetPaceRelation Relation = FleetPaceRelation::Unavailable;
};

struct FleetPaceSectorComparison final
{
    bool Valid = false;
    int MatchedLaps = 0;
    int PlayerShorter = 0;
    int RivalShorter = 0;
    int Close = 0;
    double TotalMeanDifferenceSeconds = 0.0;
    FleetPaceSectorDelta LargestPlayerGain{};
    FleetPaceSectorDelta LargestPlayerLoss{};
};

// Sector accumulators contain all completed laps in their stream. Only
// compare the means when they cover the identical first N completed laps.
// Do not compare a three-lap mean with a rival's first two laps.
inline FleetPaceSectorComparison CompareFleetPaceSectors(const FleetPaceStream& player,
    const FleetPaceStream& rival) noexcept
{
    FleetPaceSectorComparison out;
    if (!SameFleetPaceShape(player,rival) || player.CompleteLaps() == 0 ||
        player.CompleteLaps() != rival.CompleteLaps())
        return out;
    const int count = player.CompleteLaps();
    for (int gate=1;gate<=player.GateCount();++gate)
    {
        FleetPaceSector p,r;
        if (!player.TrySector(gate,p) || !rival.TrySector(gate,r))
            return {};
        const double delta = r.Duration.Mean-p.Duration.Mean;
        const auto relation = FleetPaceCompareDuration(delta);
        if (relation == FleetPaceRelation::Unavailable)
            return {};
        const FleetPaceSectorDelta row{true,gate,count,p.Duration.Mean,r.Duration.Mean,
            delta,p.Duration.Minimum,r.Duration.Minimum,relation};
        out.TotalMeanDifferenceSeconds += delta;
        if (relation == FleetPaceRelation::PlayerShorter)
        {
            ++out.PlayerShorter;
            if (!out.LargestPlayerGain.Valid ||
                delta > out.LargestPlayerGain.RivalMinusPlayerSeconds)
                out.LargestPlayerGain = row;
        }
        else if (relation == FleetPaceRelation::RivalShorter)
        {
            ++out.RivalShorter;
            if (!out.LargestPlayerLoss.Valid ||
                delta < out.LargestPlayerLoss.RivalMinusPlayerSeconds)
                out.LargestPlayerLoss = row;
        }
        else
            ++out.Close;
    }
    out.Valid = true;
    out.MatchedLaps = count;
    return out;
}

struct FleetPaceRivalSummary final
{
    int Slot = -1;
    bool EvidenceValid = false;
    FleetPaceCarSummary Car{};
    FleetPacePairSummary CommonLaps{};
    FleetPacePairSummary CommonAfterLaunch{};
    FleetPaceSectorComparison Sectors{};
};

struct FleetPaceCohortSummary final
{
    bool Valid = false;
    int CommonLaps = 0;
    int ComparedRivals = 0;
    int PlayerShorterMeans = 0;
    int RivalShorterMeans = 0;
    int CloseMeans = 0;
    double PlayerMeanSeconds = 0.0;
    double RivalMeanMinimumSeconds = 0.0;
    double RivalMeanMaximumSeconds = 0.0;
};

struct FleetPaceReadout final
{
    bool ValidPlayer = false;
    FleetPaceCarSummary Player{};
    int VerifiedRivals = 0;
    int ComparableRivals = 0;
    int UnavailableRivals = 0;
    std::array<FleetPaceRivalSummary,FleetPaceCars-1> Rivals{};
    FleetPaceCohortSummary Cohort{};
};

inline FleetPaceRivalSummary BuildFleetPaceRival(int slot,const FleetPaceStream& player,
    const FleetPaceStream& rival) noexcept
{
    FleetPaceRivalSummary out;
    out.Slot = slot;
    if (slot < 1 || slot >= FleetPaceCars || !SameFleetPaceShape(player,rival))
        return out;
    out.EvidenceValid = true;
    out.Car = rival.Summary(slot);
    const int common = std::min(player.CompleteLaps(),rival.CompleteLaps());
    out.CommonLaps = CompareFleetPaceLaps(player,rival,1,common);
    out.CommonAfterLaunch = CompareFleetPaceLaps(player,rival,2,common);
    out.Sectors = CompareFleetPaceSectors(player,rival);
    return out;
}

// A full-fleet comparison is drawn only from a shared prefix of laps.
// Pairwise comparisons with different sample sizes are not aggregated.
inline FleetPaceCohortSummary BuildFleetPaceCohort(const FleetPaceStream& player,
    const std::array<const FleetPaceStream*,FleetPaceCars-1>& rivals) noexcept
{
    FleetPaceCohortSummary out;
    int common = player.CompleteLaps();
    for (const auto* rival:rivals)
    {
        if (!rival || !SameFleetPaceShape(player,*rival))
            return out;
        common = std::min(common,rival->CompleteLaps());
    }
    if (common == 0)
        return out;
    const auto human = player.Summarize(1,common);
    if (!human.Valid)
        return out;
    out.PlayerMeanSeconds = human.MeanSeconds;
    out.RivalMeanMinimumSeconds = std::numeric_limits<double>::infinity();
    for (const auto* rival:rivals)
    {
        const auto summary = rival->Summarize(1,common);
        if (!summary.Valid)
            return {};
        ++out.ComparedRivals;
        out.RivalMeanMinimumSeconds = std::min(out.RivalMeanMinimumSeconds,summary.MeanSeconds);
        out.RivalMeanMaximumSeconds = std::max(out.RivalMeanMaximumSeconds,summary.MeanSeconds);
        const auto relation = FleetPaceCompareDuration(summary.MeanSeconds-human.MeanSeconds);
        if (relation == FleetPaceRelation::PlayerShorter) ++out.PlayerShorterMeans;
        else if (relation == FleetPaceRelation::RivalShorter) ++out.RivalShorterMeans;
        else if (relation == FleetPaceRelation::WithinWindow) ++out.CloseMeans;
        else return {};
    }
    out.Valid = true;
    out.CommonLaps = common;
    return out;
}
} // namespace latebrake

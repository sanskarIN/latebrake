#pragma once

#include "Track/TrackLimitsSteward.h"
#include "Racing/RaceProgress.h"
#include "Progression/RecordValidation.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <limits>

namespace latebrake
{
// Observational receipts for the original starter circuit only. Projected
// decorative wheel overlap is not measured tire-road contact. No receipt
// grants a checkpoint, award, or saved record.
struct TrackLimitsReceipt final
{
    TrackLimitsEvent Event=TrackLimitsEvent::None;
    int Lap=0;
    int NextGate=0;
    double RaceSeconds=0.0;
    double MovingOffPavementSeconds=0.0;
    double ActualChargedSeconds=0.0;
    int Violations=0;
    int Penalties=0;
};

struct TrackLimitsReview final
{
    bool Valid=false;
    int Violations=0;
    int Penalties=0;
    int RefusedCharges=0;
    int Warnings=0;
    int CleanRejoins=0;
    double TrackLimitsSeconds=0.0;
    int RecoveryCount=0;
    double RecoverySeconds=0.0;
    int RetainedReceipts=0;
};

// The only mutating path is a real RaceProgress::AddTimePenalty call
// admitted by the existing steward. The wrapper captures actual clock
// attribution (including representable rounding) rather than multiplying
// a displayed count by a nominal five seconds.
class TrackLimitsSession final
{
public:
    static constexpr int Capacity=16;
    static constexpr double MaximumSpeedKmH=360.0;
    static constexpr double NoticeSeconds=3.0;

    void Reset() noexcept
    {
        Steward.Reset();
        Entries.fill({});
        Next=0;
        Retained=0;
        Warnings=0;
        RefusedCharges=0;
        CleanRejoins=0;
        TrackSeconds=0.0;
        Observed=false;
        Complete=true;
    }

    void Suspend() noexcept { Steward.Suspend(); }

    TrackLimitsEvent Observe(RaceProgress& race,CircuitPoint position,
        double headingDegrees,double speedKmH,double activeDelta,
        bool actualClockAdvanced,bool continuousDriver) noexcept
    {
        if (!actualClockAdvanced || !continuousDriver || race.IsFinished() ||
            !IsValidFinishedRaceTime(race.GetElapsedSeconds()) ||
            !std::isfinite(speedKmH) || speedKmH<0.0 ||
            speedKmH>MaximumSpeedKmH ||
            race.GetCheckpointCount()!=CircuitGateCount ||
            !IsCircuitGate(race.GetNextCheckpoint()))
        {
            Suspend();
            return TrackLimitsEvent::Suspended;
        }
        const double beforeTime=race.GetElapsedSeconds();
        const double beforeFees=race.GetTotalPenaltySeconds();
        const int beforeCount=race.GetTimePenaltyCount();
        const auto before=Steward.Snapshot();
        double charged=0.0;
        const auto event=Steward.Update(position,headingDegrees,speedKmH,
            activeDelta,true,[&](double seconds) noexcept {
                // The existing RaceProgress transaction is the sole
                // source of truth. Never calculate or apply a second fee.
                if (!race.AddTimePenalty(seconds)) return false;
                charged=race.GetTotalPenaltySeconds()-beforeFees;
                return true;
            });
        if (event==TrackLimitsEvent::Suspended) return event;
        Observed=true;
        const auto after=Steward.Snapshot();
        const int countDelta=race.GetTimePenaltyCount()-beforeCount;
        const double elapsedDelta=race.GetElapsedSeconds()-beforeTime;
        if (event==TrackLimitsEvent::PenaltyApplied)
        {
            if (countDelta!=1 || after.Penalties!=before.Penalties+1 ||
                after.Violations!=before.Violations+1 ||
                !std::isfinite(charged) || charged<TrackLimitsRules::PenaltySeconds ||
                !std::isfinite(elapsedDelta) ||
                std::abs(elapsedDelta-charged)>1.0e-8 ||
                !std::isfinite(TrackSeconds+charged))
                Complete=false;
            else
                TrackSeconds+=charged;
        }
        else if (countDelta!=0 || charged!=0.0 || elapsedDelta!=0.0 ||
                 race.GetTotalPenaltySeconds()!=beforeFees ||
                 after.Penalties!=before.Penalties)
        {
            Complete=false;
        }

        if (event==TrackLimitsEvent::None) return event;
        if (event==TrackLimitsEvent::WarningStarted)
            Increment(Warnings);
        else if (event==TrackLimitsEvent::PenaltyRefused)
            Increment(RefusedCharges);
        else if (event==TrackLimitsEvent::CleanRejoin)
            Increment(CleanRejoins);

        if (Complete)
        {
            Entries[static_cast<std::size_t>(Next)]={event,
                race.GetCompletedLaps()+1,race.GetNextCheckpoint(),
                race.GetElapsedSeconds(),after.OffPavementMovingSeconds,
                charged,after.Violations,after.Penalties};
            Next=(Next+1)%Capacity;
            if (Retained<Capacity) ++Retained;
        }
        return event;
    }

    TrackLimitsSnapshot Snapshot() const noexcept { return Steward.Snapshot(); }

    TrackLimitsReceipt Recent(int offset) const noexcept
    {
        if (!Complete || offset<0 || offset>=Retained) return {};
        return Entries[static_cast<std::size_t>(
            (Next+Capacity-1-offset)%Capacity)];
    }

    TrackLimitsReceipt Notice(double raceSeconds) const noexcept
    {
        if (!IsValidFinishedRaceTime(raceSeconds)) return {};
        const auto last=Recent(0);
        if (last.Event==TrackLimitsEvent::None ||
            raceSeconds<last.RaceSeconds ||
            raceSeconds-last.RaceSeconds>NoticeSeconds)
            return {};
        return last;
    }

    TrackLimitsReview Review(const RaceProgress& race,int recoveryCount) const noexcept
    {
        const auto snapshot=Steward.Snapshot();
        const int actualCount=race.GetTimePenaltyCount();
        const double actualSeconds=race.GetTotalPenaltySeconds();
        if (!Complete || !Observed || recoveryCount<0 ||
            actualCount<snapshot.Penalties ||
            actualCount-snapshot.Penalties!=recoveryCount ||
            !std::isfinite(actualSeconds) || actualSeconds<TrackSeconds ||
            !std::isfinite(TrackSeconds) || TrackSeconds<0.0)
            return {};
        const double recoverySeconds=actualSeconds-TrackSeconds;
        // Only the two known sources can participate in this breakdown.
        // A foreign/unattributed charge hides it rather than mislabeling
        // the difference as a recovery. Existing official totals remain.
        if ((recoveryCount==0 && recoverySeconds>1e-8) ||
            (recoveryCount>0 && recoverySeconds+1e-8<
                static_cast<double>(recoveryCount)*5.0))
            return {};
        return {true,snapshot.Violations,snapshot.Penalties,
            RefusedCharges,Warnings,CleanRejoins,TrackSeconds,
            recoveryCount,recoverySeconds,Retained};
    }

private:
    void Increment(int& value) noexcept
    {
        if (value==std::numeric_limits<int>::max()) Complete=false;
        else ++value;
    }

    TrackLimitsSteward Steward;
    std::array<TrackLimitsReceipt,Capacity> Entries{};
    int Next=0;
    int Retained=0;
    int Warnings=0;
    int RefusedCharges=0;
    int CleanRejoins=0;
    double TrackSeconds=0.0;
    bool Observed=false;
    bool Complete=true;
};
} // namespace latebrake

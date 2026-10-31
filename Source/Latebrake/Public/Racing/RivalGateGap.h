#pragma once

#include <cstdint>

namespace latebrake
{
// Advisory, *discrete* comparison of accepted directional gate counts for the
// same circuit event. Not physical race order, time difference, overtaking,
// standings, finish classification or a reward. Equal gate counts do not
// mean that two actual cars are side-by-side.
enum class RivalGateGapKind
{
    Unavailable,
    SameCreditedGateCount,
    PlayerMoreCreditedGates,
    RivalMoreCreditedGates
};

struct RivalGateGapSample final
{
    int GateCount = 0;
    int RequiredLaps = 0;
    int PlayerCompletedLaps = 0;
    int PlayerNextGate = 0;
    int RivalCompletedLaps = 0;
    int RivalNextGate = 0;
    bool CircuitRacingEnabled = false;
    bool PlayerPhysicsReady = false;
    bool RivalPhysicsReady = false;
};

struct RivalGateGap final
{
    bool Valid = false;
    RivalGateGapKind Kind = RivalGateGapKind::Unavailable;
    // Positive means the player has accepted more forward gate crossings.
    // Zero means only equal *discrete gate count*, never equal physical pace.
    int CreditedGateDelta = 0;
    int PlayerCreditedGates = 0;
    int RivalCreditedGates = 0;
};

inline RivalGateGap CompareRivalCreditedGates(
    const RivalGateGapSample& sample) noexcept
{
    constexpr int MaxGates = 256;
    constexpr int MaxLaps = 100;
    if (!sample.CircuitRacingEnabled || !sample.PlayerPhysicsReady ||
        !sample.RivalPhysicsReady ||
        sample.GateCount < 1 || sample.GateCount > MaxGates ||
        sample.RequiredLaps < 1 || sample.RequiredLaps > MaxLaps ||
        sample.PlayerCompletedLaps < 0 ||
        sample.PlayerCompletedLaps > sample.RequiredLaps ||
        sample.RivalCompletedLaps < 0 ||
        sample.RivalCompletedLaps > sample.RequiredLaps ||
        sample.PlayerNextGate < 0 ||
        sample.PlayerNextGate >= sample.GateCount ||
        sample.RivalNextGate < 0 ||
        sample.RivalNextGate >= sample.GateCount ||
        (sample.PlayerCompletedLaps == sample.RequiredLaps &&
         sample.PlayerNextGate != 0) ||
        (sample.RivalCompletedLaps == sample.RequiredLaps &&
         sample.RivalNextGate != 0))
    {
        return {};
    }

    // In the finished state the next-gate field is zero; no phantom gate
    // is awarded to the other driver merely because a lap rolled over.
    const std::int64_t player =
        static_cast<std::int64_t>(sample.PlayerCompletedLaps) *
            sample.GateCount + sample.PlayerNextGate;
    const std::int64_t rival =
        static_cast<std::int64_t>(sample.RivalCompletedLaps) *
            sample.GateCount + sample.RivalNextGate;
    const std::int64_t delta = player - rival;
    return {true,
        delta > 0 ? RivalGateGapKind::PlayerMoreCreditedGates
        : delta < 0 ? RivalGateGapKind::RivalMoreCreditedGates
                    : RivalGateGapKind::SameCreditedGateCount,
        static_cast<int>(delta),
        static_cast<int>(player),
        static_cast<int>(rival)};
}
} // namespace latebrake

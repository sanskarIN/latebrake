#pragma once

#include <cstdint>

namespace latebrake
{
// A checkpoint's previous position is valid only within one uninterrupted
// racing session. Advancing the epoch before a restart, pause/resume or
// menu-to-race transition forces CheckpointSessionTrace to re-baseline.
// UInt64 rollover is well-defined and the observed revision still changes.
class RaceSessionEpoch final
{
public:
    using Revision = std::uint64_t;

    Revision Current() const noexcept { return Value; }

    void OnPauseTransition() noexcept { ++Value; }
    void OnMenuReturnToRace() noexcept { ++Value; }
    void OnRaceRestart() noexcept { ++Value; }

private:
    Revision Value = 0;
};
} // namespace latebrake

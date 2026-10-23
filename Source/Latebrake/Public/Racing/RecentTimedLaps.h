#pragma once

#include <array>
#include <cmath>
#include <cstddef>

namespace latebrake
{
// Most recent finite positive lap durations, never player identifiers,
// position samples, network telemetry or unlimited dynamic history.
struct TimedLapRecord final
{
    int CompletedLap = 0;
    double Seconds = 0.0;
    bool Valid() const noexcept { return CompletedLap > 0 && Seconds > 0.0; }
};

class RecentTimedLaps final
{
public:
    static constexpr int Capacity = 8;

    void Reset() noexcept
    {
        Entries.fill({});
        Next = 0;
        Count = 0;
    }

    bool Record(int completedLap, double seconds) noexcept
    {
        if (completedLap <= 0 || !std::isfinite(seconds) || seconds <= 0.0)
        {
            return false;
        }
        // This buffer models completed laps in one uninterrupted race, not
        // an editable leaderboard. Reject duplicate/backdated numbers so
        // recent-lap comparisons and pace estimates cannot silently pair
        // durations with fabricated or out-of-order lap identifiers.
        if (Count > 0 && completedLap <= GetRecent(0).CompletedLap)
        {
            return false;
        }
        Entries[static_cast<std::size_t>(Next)] = {completedLap, seconds};
        Next = (Next + 1) % Capacity;
        if (Count < Capacity) ++Count;
        return true;
    }

    int Size() const noexcept { return Count; }

    // offset 0 = newest, 1 = prior timed lap, etc.
    // A missing/invalid offset returns {0,0}, never out-of-range memory.
    TimedLapRecord GetRecent(int offset) const noexcept
    {
        if (offset < 0 || offset >= Count) return {};
        const int index = (Next + Capacity - 1 - offset) % Capacity;
        return Entries[static_cast<std::size_t>(index)];
    }

private:
    std::array<TimedLapRecord, Capacity> Entries{};
    int Next = 0;
    int Count = 0;
};
} // namespace latebrake

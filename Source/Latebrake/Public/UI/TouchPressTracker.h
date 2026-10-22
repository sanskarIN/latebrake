#pragma once

#include <array>
#include <cstddef>

namespace latebrake
{
// Tracks physical touch-down ownership independently from driving pads.
// Every touch index can produce at most one action until its release, even
// when an action clears the driving state or the finger changes screen area.
class TouchPressTracker final
{
public:
    static constexpr int MaxFingerCount = 16;

    bool Press(int finger) noexcept
    {
        if (!ValidFinger(finger)) return false;
        bool& held = Down[static_cast<std::size_t>(finger)];
        if (held) return false;
        held = true;
        return true;
    }

    void Release(int finger) noexcept
    {
        if (ValidFinger(finger)) Down[static_cast<std::size_t>(finger)] = false;
    }

    bool IsDown(int finger) const noexcept
    {
        return ValidFinger(finger) && Down[static_cast<std::size_t>(finger)];
    }

    // Only use on a genuine OS focus/lifecycle boundary, where the platform
    // may have discarded release notifications. Never reset as a side effect
    // of a PAUSE, RESTART, RECOVER, or menu action by a still-held finger.
    void Reset() noexcept { Down.fill(false); }

private:
    static constexpr bool ValidFinger(int finger) noexcept
    {
        return finger >= 0 && finger < MaxFingerCount;
    }

    std::array<bool, MaxFingerCount> Down{};
};
} // namespace latebrake

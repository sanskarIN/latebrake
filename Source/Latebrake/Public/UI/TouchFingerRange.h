#pragma once

namespace latebrake
{
// Unreal touch indices are forwarded to the same fixed-capacity portable
// tracker and driving state. Keep one bound so neither subsystem can accept
// a finger which the other cannot represent.
inline constexpr int MaxTrackedTouchFingers = 16;

inline constexpr bool IsTrackableTouchFinger(int finger) noexcept
{
    return finger >= 0 && finger < MaxTrackedTouchFingers;
}
} // namespace latebrake

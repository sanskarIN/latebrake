#pragma once

#include "UI/TouchControlLayout.h"

namespace latebrake
{
// If Unreal fails to pause its world after app deactivation, the focus-loss
// latch still owns input. Only a NEW press on the visible PAUSE pad can
// explicitly resume racing; drags, held-finger duplicate presses, movement,
// recovery, restart and career actions remain blocked.
inline constexpr bool CanResumeFocusSuspendedRaceFromTouch(
    bool isNewPress, TouchControl control) noexcept
{
    return isNewPress && control == TouchControl::Pause;
}
} // namespace latebrake

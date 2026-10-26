#pragma once

#include "UI/TouchControlLayout.h"

namespace latebrake
{
// Explicit one-shot touch-camera command admission. Camera view does not
// mutate race results, but must remain isolated from menu, pause, focus loss,
// unqualified circuits and held steering/pedal fingers.
struct CameraTouchRequest final
{
    TouchControl Region = TouchControl::None;
    bool FreshPress = false;
    bool HasRaceMode = false;
    bool CircuitReady = false;
    bool PlayerReady = false;
    bool MenuOpen = false;
    bool Paused = false;
    bool FocusSuspended = false;
};

constexpr bool CanCycleCameraFromTouch(CameraTouchRequest request) noexcept
{
    return request.Region == TouchControl::MenuBrowse &&
        request.FreshPress && request.HasRaceMode &&
        request.CircuitReady && request.PlayerReady &&
        !request.MenuOpen && !request.Paused && !request.FocusSuspended;
}
} // namespace latebrake

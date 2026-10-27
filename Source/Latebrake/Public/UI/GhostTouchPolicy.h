#pragma once

#include "UI/TouchControlLayout.h"

namespace latebrake
{
// Visual-only control admission. It cannot grant race eligibility or
// modify BestLapGhost recordings, penalties, records or car physics.
struct GhostTouchRequest final
{
    TouchControl Region = TouchControl::None;
    bool FreshPress = false;
    bool HasRaceMode = false;
    bool CircuitReady = false;
    bool PlayerReady = false;
    bool RaceGreen = false;
    bool MenuOpen = false;
    bool Paused = false;
    bool FocusSuspended = false;
    bool RaceFinished = false;
};

constexpr bool CanToggleGhostFromTouch(GhostTouchRequest request) noexcept
{
    return request.Region == TouchControl::GhostView &&
        request.FreshPress && request.HasRaceMode &&
        request.CircuitReady && request.PlayerReady && request.RaceGreen &&
        !request.MenuOpen && !request.Paused &&
        !request.FocusSuspended && !request.RaceFinished;
}
} // namespace latebrake

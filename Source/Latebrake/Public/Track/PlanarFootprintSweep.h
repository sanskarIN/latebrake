#pragma once

#include "Track/CircuitStartGrid.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace latebrake
{
// Translational prediction for the existing padded starter-grid chassis.
// These fixed-heading envelopes are conservative observations, not a
// contact manifold, tire model, angular sweep or Chaos collision result.
inline constexpr double FootprintSweepMaximumBodySpeed=15000.0;
inline constexpr double FootprintSweepMaximumRelativeSpeed=2.0*FootprintSweepMaximumBodySpeed;
inline constexpr double FootprintSweepMaximumHorizon=2.0;
inline constexpr double FootprintSweepMaximumPadding=1000.0;
inline constexpr double FootprintSweepNoContact=std::numeric_limits<double>::infinity();

struct PlanarFootprintSweepRequest final
{
    CircuitPose Self{};
    CircuitPose Other{};
    double RelativeVelocityX=0.0;
    double RelativeVelocityY=0.0;
    double Horizon=0.0;
    double Padding=0.0;
};

struct PlanarFootprintSweepResult final
{
    bool Valid=false;
    bool Intersects=false;
    bool StartsOverlapping=false;
    double EntrySeconds=FootprintSweepNoContact;
    double ExitSeconds=FootprintSweepNoContact;
    // First and last times are clipped to the requested interval. They
    // cannot reveal actual contact beyond the modelled constant velocity.
};

inline bool ValidFootprintSweepRequest(const PlanarFootprintSweepRequest& request) noexcept
{
    return IsValidStartGridPose(request.Self) && IsValidStartGridPose(request.Other) &&
        std::isfinite(request.RelativeVelocityX) && std::isfinite(request.RelativeVelocityY) &&
        std::hypot(request.RelativeVelocityX,request.RelativeVelocityY)<=
            FootprintSweepMaximumRelativeSpeed+1.0e-8 &&
        std::isfinite(request.Horizon) && request.Horizon>=0.0 &&
        request.Horizon<=FootprintSweepMaximumHorizon &&
        std::isfinite(request.Padding) && request.Padding>=0.0 &&
        request.Padding<=FootprintSweepMaximumPadding;
}

struct FootprintAxisWindow final
{
    bool Valid=false;
    bool Intersects=false;
    bool StartsOverlapping=false;
    double EntrySeconds=FootprintSweepNoContact;
    double ExitSeconds=FootprintSweepNoContact;
};

// Continuous interval where the projected centers lie inside the
// summed support radii. Check endpoints before division: a near-zero
// speed must not overflow a time calculation or invent swept contact.
inline FootprintAxisWindow SolveFootprintAxisWindow(double separation,double velocity,
    double radius,double horizon) noexcept
{
    if(!std::isfinite(separation) || !std::isfinite(velocity) ||
        !std::isfinite(radius) || radius<0.0 || !std::isfinite(horizon) ||
        horizon<0.0 || horizon>FootprintSweepMaximumHorizon)
        return {};
    const bool starts=std::abs(separation)<=radius;
    const double end=separation+velocity*horizon;
    if(!std::isfinite(end))return {};
    if(velocity==0.0 || end==separation || horizon==0.0)
        return starts?FootprintAxisWindow{true,true,true,0.0,horizon}:
            FootprintAxisWindow{true,false,false};
    const double low=std::min(separation,end);
    const double high=std::max(separation,end);
    if(low>radius || high< -radius)return {true,false,starts};

    // Compute times as fractions of an already finite segment. Clipping
    // inside the segment avoids unbounded times from tiny velocities.
    const double travel=end-separation;
    double enter=0.0;
    double leave=horizon;
    if(velocity>0.0)
    {
        if(separation< -radius)enter=(-radius-separation)/travel*horizon;
        if(end>radius)leave=(radius-separation)/travel*horizon;
    }
    else
    {
        if(separation>radius)enter=(radius-separation)/travel*horizon;
        if(end< -radius)leave=(-radius-separation)/travel*horizon;
    }
    if(!std::isfinite(enter) || !std::isfinite(leave) || enter>leave)return {};
    return {true,true,starts,std::clamp(enter,0.0,horizon),std::clamp(leave,0.0,horizon)};
}

inline PlanarFootprintSweepResult SweepPlanarFootprints(
    const PlanarFootprintSweepRequest& request) noexcept
{
    if(!ValidFootprintSweepRequest(request))return {};
    const auto a=MakeStartGridRect(request.Self);
    const auto b=MakeStartGridRect(request.Other);
    const double dx=b.X-a.X;
    const double dy=b.Y-a.Y;
    const std::array<CircuitPoint,4> axes{{
        {a.ForwardX,a.ForwardY},{a.RightX,a.RightY},
        {b.ForwardX,b.ForwardY},{b.RightX,b.RightY}
    }};
    double enter=0.0;
    double leave=request.Horizon;
    bool starts=true;
    bool intersects=true;
    for(const auto& axis:axes)
    {
        const double separation=dx*axis.X+dy*axis.Y;
        const double speed=request.RelativeVelocityX*axis.X+request.RelativeVelocityY*axis.Y;
        const double radius=StartGridProjectionRadius(a,axis.X,axis.Y)+
            StartGridProjectionRadius(b,axis.X,axis.Y)+request.Padding;
        const auto window=SolveFootprintAxisWindow(separation,speed,radius,request.Horizon);
        if(!window.Valid)return {};
        starts=starts && window.StartsOverlapping;
        if(!window.Intersects)
        {
            intersects=false;
            continue;
        }
        enter=std::max(enter,window.EntrySeconds);
        leave=std::min(leave,window.ExitSeconds);
    }
    if(!intersects || enter>leave)return {true,false,false};
    return {true,true,starts,enter,leave};
}
} // namespace latebrake

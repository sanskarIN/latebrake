#pragma once

// Freestanding, deterministic driver guidance for the optional 2D Win64
// playtest. This is not Unreal vehicle physics and never controls rewards,
// checkpoint order, ownership, timers, collision or the rival simulation.
// Geometry is supplied by the SAME procedural track evaluated by the game.
namespace latebrake_preview
{
inline constexpr float CoachAbs(float x) noexcept { return x < 0.0f ? -x : x; }
inline constexpr float CoachClamp(float x,float lo,float hi) noexcept
{
    return x < lo ? lo : x > hi ? hi : x;
}
inline constexpr bool CoachFinite(float v) noexcept
{
    // Intentional no-CRT finite test: both NaNs and infinities fail.
    return v >= -1000000.0f && v <= 1000000.0f;
}
inline float CoachSqrt(float x) noexcept
{
    if (x <= 0.0f) return 0.0f;
    float g = x >= 1.0f ? x : 1.0f;
    for (int i=0;i<12;++i) g=0.5f*(g+x/g);
    return g;
}

// World-space centerline samples come from shapePos() and trackOffset().
// Aim is a point ON the forward road centerline, not the next checkpoint:
// a direct arrow to a gate would reward cutting across the infield.
struct PreviewCoachInput final
{
    float CarX=0.0f, CarY=0.0f;
    float ForwardX=0.0f, ForwardY=0.0f;
    float TangentX=0.0f, TangentY=0.0f;
    float FutureTangentX=0.0f, FutureTangentY=0.0f;
    float AimX=0.0f, AimY=0.0f;
    float Speed=0.0f;
    float ManualSteer=0.0f;
    float TrackOffset=0.0f;
    float TrackHalfWidth=0.22f;
    float Grip=1.0f;
    bool OnRoad=false;
    bool AssistEnabled=false;
};

struct PreviewCoachResult final
{
    bool Valid=false;
    bool WrongWay=false;
    bool BrakeForCorner=false;
    bool AssistApplied=false;
    float OutputSteer=0.0f;
    float Alignment=1.0f;
    float Bend=0.0f;
    float TargetSpeed=0.0f;
    float AimTurn=0.0f;
    // -1 for left bend, +1 for right bend, 0 for nearly straight.
    int RoadTurn=0;
};

inline PreviewCoachResult EvaluatePreviewCoach(
    const PreviewCoachInput& in) noexcept
{
    PreviewCoachResult out{};
    out.OutputSteer=CoachFinite(in.ManualSteer)
        ? CoachClamp(in.ManualSteer,-1.0f,1.0f) : 0.0f;
    if (!CoachFinite(in.CarX) || !CoachFinite(in.CarY) ||
        !CoachFinite(in.ForwardX) || !CoachFinite(in.ForwardY) ||
        !CoachFinite(in.TangentX) || !CoachFinite(in.TangentY) ||
        !CoachFinite(in.FutureTangentX) ||
        !CoachFinite(in.FutureTangentY) ||
        !CoachFinite(in.AimX) || !CoachFinite(in.AimY) ||
        !CoachFinite(in.Speed) || !CoachFinite(in.TrackOffset) ||
        !CoachFinite(in.TrackHalfWidth) || !CoachFinite(in.Grip) ||
        in.TrackHalfWidth <= 0.01f || in.TrackHalfWidth > 2.0f ||
        in.Grip < 0.25f || in.Grip > 3.0f)
        return out;

    const float flen=CoachSqrt(in.ForwardX*in.ForwardX+
                                in.ForwardY*in.ForwardY);
    const float tlen=CoachSqrt(in.TangentX*in.TangentX+
                                in.TangentY*in.TangentY);
    const float futureLen=CoachSqrt(
        in.FutureTangentX*in.FutureTangentX+
        in.FutureTangentY*in.FutureTangentY);
    const float dx=in.AimX-in.CarX,dy=in.AimY-in.CarY;
    const float aimLen=CoachSqrt(dx*dx+dy*dy);
    if (flen<0.1f || tlen<0.1f || futureLen<0.1f || aimLen<0.1f)
        return out;
    const float fx=in.ForwardX/flen,fy=in.ForwardY/flen;
    const float tx=in.TangentX/tlen,ty=in.TangentY/tlen;
    const float nx=in.FutureTangentX/futureLen,
                ny=in.FutureTangentY/futureLen;
    const float pathCross=tx*ny-ty*nx;
    out.Valid=true;
    out.Alignment=CoachClamp(fx*tx+fy*ty,-1.0f,1.0f);
    out.Bend=CoachAbs(pathCross);
    out.RoadTurn=pathCross>0.1f ? 1 : pathCross< -0.1f ? -1 : 0;
    out.AimTurn=CoachClamp((fx*dy-fy*dx)/aimLen,-1.0f,1.0f);
    // Illustrative preview advice calibrated to the playable 2D speeds;
    // not a real stopping-distance/tyre-force or collision prediction.
    out.TargetSpeed=CoachClamp(
        290.0f-245.0f*out.Bend+36.0f*(in.Grip-1.0f),
        150.0f,320.0f);
    out.WrongWay=in.OnRoad && in.Speed>50.0f &&
                 out.Alignment< -0.45f;
    out.BrakeForCorner=in.OnRoad && in.Speed>0.0f &&
        out.Bend>0.12f && in.Speed>out.TargetSpeed+18.0f &&
        !out.WrongWay;
    // A small bounded course-following hint, not autopilot.
    // At a strong manual steer the player has complete authority.
    // No automatic steer in reverse, with the menu paused, offroad,
    // or when disabled; caller owns the pause/focus conditions.
    if (in.AssistEnabled && in.OnRoad && in.Speed>45.0f &&
        CoachAbs(out.OutputSteer)<0.35f && !out.WrongWay)
    {
        const float edge=CoachClamp(
            CoachAbs(in.TrackOffset)/in.TrackHalfWidth,0.0f,1.0f);
        const float misalign=CoachClamp(
            (1.0f-out.Alignment)*0.5f,0.0f,1.0f);
        const float gain=CoachClamp(
            0.07f+0.13f*edge+0.15f*misalign,0.07f,0.27f);
        const float correction=CoachClamp(
            out.AimTurn*gain*(1.0f-CoachAbs(out.OutputSteer)),
            -0.26f,0.26f);
        out.OutputSteer=CoachClamp(
            out.OutputSteer+correction,-1.0f,1.0f);
        out.AssistApplied=CoachAbs(correction)>0.0001f;
    }
    return out;
}
} // namespace latebrake_preview

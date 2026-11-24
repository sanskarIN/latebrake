#pragma once
#include <cstdint>
#include <limits>

namespace latebrake
{
// Source-only accounting: these fields count attempts to build the true
// UE five-part original car render surface and successful source commits.
// They do NOT prove Unreal UHT compilation, shader output, visible GPU
// pixels, collision accuracy, smooth framerate or hands-on gameplay.
struct OriginalMeshBuildTelemetry final
{
    std::uint32_t Attempts=0;
    std::uint32_t Committed=0;
};

constexpr bool ValidOriginalMeshBuildTelemetry(
    OriginalMeshBuildTelemetry value) noexcept
{
    return value.Committed<=value.Attempts;
}

constexpr OriginalMeshBuildTelemetry OriginalMeshBuildAttempt(
    OriginalMeshBuildTelemetry value) noexcept
{
    if (!ValidOriginalMeshBuildTelemetry(value)) return {};
    if(value.Attempts<std::numeric_limits<std::uint32_t>::max())
        ++value.Attempts;
    return value;
}

constexpr OriginalMeshBuildTelemetry OriginalMeshBuildCommit(
    OriginalMeshBuildTelemetry value) noexcept
{
    if (!ValidOriginalMeshBuildTelemetry(value)) return {};
    if(value.Committed<value.Attempts)
        ++value.Committed;
    return value;
}

static_assert(OriginalMeshBuildAttempt({}).Attempts==1u);
static_assert(OriginalMeshBuildCommit({}).Committed==0u);
static_assert(OriginalMeshBuildCommit(OriginalMeshBuildAttempt({})).Committed==1u);
static_assert(!ValidOriginalMeshBuildTelemetry({1u,2u}));
} // namespace latebrake

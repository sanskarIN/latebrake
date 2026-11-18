#include "Track/WorldScaleSweptEnvelopeAudit.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "WorldScaleSweptEnvelope FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace latebrake;
    const auto source = BuildWorldScalePacePlan();
    Require(source.Valid, "authored pace plan must be available");
    WorldScaleLineRequest request{};
    request.ProposalIdentity = 17;
    request.AuthoredReferenceConfirmed = true;
    request.SourceOnlyAdvisory = true;
    const auto line = BuildWorldScaleCandidateRacingLine(source, request);
    Require(line.Valid, "candidate must pass existing road-center audit");

    WorldScaleSweptEnvelopeSettings settings{};
    Require(ValidWorldScaleSweptEnvelopeSettings(settings),
            "default real vehicle footprint settings accepted");
    Require(!AuditWorldScaleSweptEnvelope({}, settings).Valid,
            "untrusted plan rejected");
    settings.SamplesPerSegment = 0;
    Require(!AuditWorldScaleSweptEnvelope(line, settings).Valid,
            "zero sample density rejected");
    settings = {};
    settings.HalfLengthCm = std::numeric_limits<double>::quiet_NaN();
    Require(!AuditWorldScaleSweptEnvelope(line, settings).Valid,
            "NaN wheelbase envelope rejected");
    settings = {};
    Require(!WorldScaleChassisCornersOnAsphalt(
                {std::numeric_limits<double>::infinity(), 0.0}, 0.0, settings),
            "nonfinite vehicle center rejected");
    Require(!WorldScaleChassisCornersOnAsphalt(
                {0.0, 0.0}, std::numeric_limits<double>::quiet_NaN(), settings),
            "nonfinite yaw rejected");

    // The independent footprint audit can legitimately reject a candidate
    // whose centerline is on asphalt but whose swept corners are not.
    // Do not assume a passing result without real chassis clearance.
    const auto first = AuditWorldScaleSweptEnvelope(line);
    const auto second = AuditWorldScaleSweptEnvelope(line);
    Require(first.Valid == second.Valid &&
            first.SegmentsChecked == second.SegmentsChecked &&
            first.ChassisPosesChecked == second.ChassisPosesChecked &&
            first.CornerChecks == second.CornerChecks,
            "identical source geometry produces deterministic audit");
    if (first.Valid)
    {
        Require(first.SegmentsChecked == WorldScaleLineStationCount &&
                first.ChassisPosesChecked ==
                    WorldScaleLineStationCount * 13u &&
                first.CornerChecks == first.ChassisPosesChecked * 4u &&
                first.MinimumVerifiedCornerDistanceCm > 1.0,
                "accepted plan verifies all 512 source segments");
    }
    else
    {
        Require(first.SegmentsChecked < WorldScaleLineStationCount,
                "rejected plan never reports a complete valid audit");
    }
    std::cout << "WorldScaleSweptEnvelope PASS: independent chassis "
                 "footprint validation and deterministic fail-closed "
                 "source-only audit (not Unreal/Chaos qualification)\n";
    return EXIT_SUCCESS;
}

#include "Track/WorldScaleApproachBoardPlan.h"
#include "Track/WorldScaleSurfaceScenePlan.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool condition, const char* why)
{
    if (!condition)
    {
        std::cerr << "WorldScaleSourceScene FAIL: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void ValidatePaintedRoad()
{
    using namespace latebrake;
    const auto original = BuildWorldScaleSceneGuidePlan();
    Require(original.Valid &&
            original.PhysicalSourceSectionsVerified ==
                WorldScaleSceneSections &&
            original.LayerQuads[0] == WorldScaleSceneSections &&
            original.LayerQuads[1] == WorldScaleSceneSections &&
            original.LayerQuads[2] > 0u &&
            original.LayerQuads[3] == original.LayerQuads[4] &&
            original.LayerQuads[3] > 20u &&
            original.MaximumQuadLengthCm > 0.0,
            "real 1.7-km source route provides complete safe painted boundaries");
    Require(AuditWorldScaleSceneGuidePlan(original),
            "all four corners of every original 3D guide quad are road-backed");

    std::array<unsigned, WorldScaleSceneSections> perSection{};
    unsigned redKerb = 0;
    unsigned whiteKerb = 0;
    unsigned center = 0;
    unsigned indexedSupport = 0;
    for (const auto& quad : original.Quads)
    {
        Require(WorldScaleSceneQuadSupported(quad) &&
                WorldScaleSceneSignedDoubleArea(quad) > 1e-5,
                "all 3D projected quads have positive CCW top winding");
        Require(quad.Corners[0].Z == quad.Corners[1].Z &&
                quad.Corners[1].Z == quad.Corners[2].Z &&
                quad.Corners[2].Z == quad.Corners[3].Z,
                "one quad is planar and never a fabricated drivable ramp");
        Require(quad.SourceSection < WorldScaleSceneSections &&
                quad.StartDistanceCm >= 0.0 &&
                quad.EndDistanceCm <= WorldScaleCircuitLapCm,
                "all paint belongs to an original course station");
        ++perSection[quad.SourceSection];
        if (quad.Layer == WorldScaleSceneLayer::LeftKerbPaint ||
            quad.Layer == WorldScaleSceneLayer::RightKerbPaint)
        {
            if (quad.RedKerb) ++redKerb;
            else ++whiteKerb;
        }
        if (quad.Layer == WorldScaleSceneLayer::CenterDashPaint)
            ++center;
        for (const auto& vertex : quad.Corners)
        {
            Require(IsOnWorldScaleRoad({vertex.X,vertex.Y}),
                    "cosmetic markings must never add unsupported pavement");
            ++indexedSupport;
        }
    }
    Require(center > 100u && redKerb > 0u && whiteKerb > 0u &&
                indexedSupport >= WorldScaleSceneSections * 8u,
            "visible guide includes edges, center dashes and alternating kerbs");
    for (unsigned count : perSection)
    {
        Require(count >= 2u && count <= 5u,
                "every source segment has both painted safety boundaries");
    }
    const auto identical = BuildWorldScaleSceneGuidePlan();
    Require(identical.Valid &&
            identical.Quads.size() == original.Quads.size() &&
            identical.LayerQuads == original.LayerQuads &&
            identical.TotalVisibleAreaSquareCm ==
                original.TotalVisibleAreaSquareCm,
            "independent rebuild uses deterministic source geometry");

    auto invalid = WorldScaleSceneSettings{};
    invalid.KerbDistanceCm = 443.0;
    Require(!BuildWorldScaleSceneGuidePlan(invalid).Valid,
            "never place kerb paint beyond visually backed asphalt");
    invalid = {};
    invalid.EdgePaintDistanceCm = 416.0;
    Require(!BuildWorldScaleSceneGuidePlan(invalid).Valid,
            "safety bounds forbid moving edge paint outside paved core");
    invalid = {};
    invalid.DashedLengthSections = invalid.DashedPeriodSections;
    Require(!BuildWorldScaleSceneGuidePlan(invalid).Valid,
            "solid center stripes cannot masquerade as a passing lane");
    invalid = {};
    invalid.DisableIndependentCollision = false;
    Require(!BuildWorldScaleSceneGuidePlan(invalid).Valid,
            "nonphysical paint cannot gain independent collision authority");
    invalid = {};
    invalid.LeaveCheckpointAuthorityUnchanged = false;
    Require(!BuildWorldScaleSceneGuidePlan(invalid).Valid,
            "paint plan cannot award checkpoint or lap events");
    invalid = {};
    invalid.MarkingLiftAboveRoadCm =
        std::numeric_limits<double>::infinity();
    Require(!BuildWorldScaleSceneGuidePlan(invalid).Valid,
            "nonfinite painted geometry fails before vector allocation");
}

void ValidateApproachBoards()
{
    using namespace latebrake;
    const auto reference = BuildWorldScalePacePlan();
    Require(reference.Valid, "real world-scale authored curvature exists");
    const auto plan = BuildWorldScaleApproachBoardPlan(reference);
    Require(plan.Valid && plan.TurnCount == 4u &&
            plan.BoardCount == 24u &&
            plan.Fault == WorldScaleApproachFault::None &&
            plan.NoPhysicalCollision && plan.NoRaceControlAuthority,
            "four authored corners require exactly six visual boards each");
    Require(AuditWorldScaleApproachBoardPlan(plan),
            "visual-only 150/100/50m boards are outside physical asphalt");

    std::array<std::array<unsigned, 3>, 4> right{};
    std::array<std::array<unsigned, 3>, 4> left{};
    for (unsigned i = 0; i < plan.BoardCount; ++i)
    {
        const auto& board = plan.Boards[i];
        const double distance =
            static_cast<double>(board.DistanceToCornerMeters) * 100.0;
        const double expected = WrapWorldScaleApproachStation(
            board.UpcomingCornerStationCm - distance);
        Require(std::abs(expected - board.StationCm) <= 1.0e-6 &&
                !IsOnWorldScaleRoad(board.GroundLocation) &&
                board.BoardTopCm > board.GroundTopCm &&
                std::isfinite(board.FacingDegrees),
                "board must be located before turn and outside source collision road");
        const unsigned marker =
            board.DistanceToCornerMeters == 150 ? 0u :
            board.DistanceToCornerMeters == 100 ? 1u : 2u;
        auto& counter = board.RoadSide > 0 ?
            left[board.TurnIndex][marker] :
            right[board.TurnIndex][marker];
        ++counter;
        for (unsigned j = 0; j < i; ++j)
        {
            const auto& other = plan.Boards[j];
            const double dx =
                board.GroundLocation.X - other.GroundLocation.X;
            const double dy =
                board.GroundLocation.Y - other.GroundLocation.Y;
            Require(std::hypot(dx,dy) >= 400.0 &&
                    board.UniqueBoardToken != other.UniqueBoardToken,
                    "unique physical placement proposals never overlap each other");
        }
    }
    for (unsigned i = 0; i < plan.TurnCount; ++i)
    {
        for (unsigned j = 0; j < 3u; ++j)
        {
            Require(right[i][j] == 1u && left[i][j] == 1u,
                    "all six approach markers are present at every turn");
        }
    }

    const auto malformed = BuildWorldScaleApproachBoardPlan({});
    Require(!malformed.Valid &&
            malformed.Fault == WorldScaleApproachFault::MissingReference,
            "invalid authored reference never produces physical fixtures");
    auto settings = WorldScaleApproachSettings{};
    settings.BoardLateralOffsetCm = 300.0;
    Require(!BuildWorldScaleApproachBoardPlan(reference,settings).Valid,
            "roadway is not a legal standalone 3D brake sign location");
    settings = {};
    settings.DoNotCreatePhysicalCollision = false;
    Require(!BuildWorldScaleApproachBoardPlan(reference,settings).Valid,
            "render-only markers cannot change WorldStatic collision");
    settings = {};
    settings.NeverChangeActualBrakingAuthority = false;
    Require(!BuildWorldScaleApproachBoardPlan(reference,settings).Valid,
            "visual brake signage cannot issue driver/brake commands");
    settings = {};
    settings.BoardHeightCm =
        std::numeric_limits<double>::quiet_NaN();
    Require(!BuildWorldScaleApproachBoardPlan(reference,settings).Valid,
            "invalid height fails closed with no false completed plan");
}

void ValidateStartFinishContinuity()
{
    using namespace latebrake;
    const auto scene = BuildWorldScaleSceneGuidePlan();
    Require(scene.Valid, "complete source scene required at lap seam");
    unsigned finalSection = 0;
    unsigned firstSection = 0;
    for (const auto& quad : scene.Quads)
    {
        if (quad.Layer != WorldScaleSceneLayer::LeftEdgePaint) continue;
        if (quad.SourceSection == 0u)
        {
            ++firstSection;
            Require(quad.StartDistanceCm == 0.0,
                    "first paint starts at genuine authored race station zero");
        }
        if (quad.SourceSection == WorldScaleSceneSections - 1u)
        {
            ++finalSection;
            Require(quad.EndDistanceCm == WorldScaleCircuitLapCm,
                    "last paint joins start station without fake lap extension");
        }
    }
    Require(firstSection == 1u && finalSection == 1u,
            "exactly one left-edge quad spans each side of the lap seam");
}
} // namespace

int main()
{
    ValidatePaintedRoad();
    ValidateApproachBoards();
    ValidateStartFinishContinuity();
    std::cout << "WorldScaleSourceScene PASS: original 1.7km "
                 "road-backed paint quads, 4 turns/24 brake markers, "
                 "seam support and fail-closed visual-only policy "
                 "(not Unreal render/collision QA)\n";
    return EXIT_SUCCESS;
}

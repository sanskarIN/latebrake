#include "Track/CollisionBlockSetup.h"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool condition, const char* reason)
{
    if (!condition)
    {
        std::cerr << "Collision block configuration FAIL: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}
} // namespace

int main()
{
    using namespace latebrake;
    using Decision = CollisionBlockSetupDecision;
    for (int i = 0; i < CircuitCollisionBlockCount; ++i)
    {
        const auto block = PlannedCollisionBlock(i);
        Require(EvaluateCollisionBlockSetup(block, true, true, true, true, true) ==
                Decision::Ready, "each original block accepts complete configuration");
        Require(EvaluateCollisionBlockSetup(block, false, true, true, true, true) ==
                Decision::ActorMissing, "missing actor is never counted");
        Require(EvaluateCollisionBlockSetup(block, true, false, true, true, true) ==
                Decision::MeshComponentMissing,
                "a spawned actor without mesh component is not ready");
        Require(EvaluateCollisionBlockSetup(block, true, true, false, true, true) ==
                Decision::UnexpectedMesh, "incorrect mesh cannot certify collision");
        Require(EvaluateCollisionBlockSetup(block, true, true, true, false, true) ==
                Decision::CollisionNotConfigured,
                "no blocking QueryAndPhysics is not an acceptable track actor");
        Require(EvaluateCollisionBlockSetup(block, true, true, true, true, false) ==
                Decision::TransformNotApplied,
                "spawned mesh with rejected transform is not ready");
    }
    auto invalid = PlannedCollisionBlock(0);
    invalid.ScaleX = 0.0;
    Require(EvaluateCollisionBlockSetup(invalid, true, true, true, true, true) ==
            Decision::InvalidPlan, "nonpositive scale fails before actor validation");
    invalid = PlannedCollisionBlock(0);
    invalid.Center.X = std::numeric_limits<double>::infinity();
    Require(EvaluateCollisionBlockSetup(invalid, true, true, true, true, true) ==
            Decision::InvalidPlan, "nonfinite location fails before actor validation");
    invalid = PlannedCollisionBlock(-1);
    Require(EvaluateCollisionBlockSetup(invalid, false, false, false, false, false) ==
            Decision::InvalidPlan, "invalid index never reports normal spawn failure");

    Require(CollisionBlockSetupDecisionName(Decision::CollisionNotConfigured)[0] == 'c',
            "classified failures have stable diagnostic names");
    Require(CollisionBlockSetupDecisionName(static_cast<Decision>(100))[0] == 'u',
            "unknown decision fails to safe diagnostic label");

    std::cout << "Collision-block setup policy passed.\n";
    return EXIT_SUCCESS;
}

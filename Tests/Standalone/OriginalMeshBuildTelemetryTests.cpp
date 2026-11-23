#include "Vehicles/OriginalMeshBuildTelemetry.h"
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
namespace
{
void Require(bool b, const char* message)
{
    if(!b){std::cerr<<"Unreal source-mesh build telemetry: "<<message<<'\n';
        std::exit(EXIT_FAILURE);}
}
}
int main()
{
    using namespace latebrake;
    OriginalMeshBuildTelemetry s{};
    Require(ValidOriginalMeshBuildTelemetry(s), "initial counters must be valid");
    s=OriginalMeshBuildCommit(s);
    Require(s.Attempts==0u && s.Committed==0u,
        "commit without an attempted source build cannot invent render success");
    s=OriginalMeshBuildAttempt(s);
    s=OriginalMeshBuildCommit(s);
    Require(s.Attempts==1u && s.Committed==1u,
        "a fully allocated renderer yields exactly one committed source mesh");
    s=OriginalMeshBuildAttempt(s);
    Require(s.Attempts==2u && s.Committed==1u,
        "missing material/pivot failure counts as attempt without success");
    s=OriginalMeshBuildCommit(s);
    Require(s.Attempts==2u && s.Committed==2u,
        "successful recovery consumes exactly one outstanding attempt");
    for(unsigned i=0;i<300000u;++i)
    {
        s=OriginalMeshBuildAttempt(s);
        if(i%3u!=0u) s=OriginalMeshBuildCommit(s);
        Require(ValidOriginalMeshBuildTelemetry(s),
            "300k attempts/commits never exceed honest source provenance");
    }
    const auto before=s;
    for(int i=0;i<200000;++i)
        s=OriginalMeshBuildCommit(s);
    Require(s.Attempts==before.Attempts && s.Committed<=s.Attempts,
        "repeated commit without attempts may never overclaim build count");
    s={std::numeric_limits<std::uint32_t>::max(),
       std::numeric_limits<std::uint32_t>::max()};
    const auto saturated=OriginalMeshBuildCommit(OriginalMeshBuildAttempt(s));
    Require(saturated.Attempts==s.Attempts && saturated.Committed==s.Committed,
        "source telemetry counters must saturate, never wrap over long sessions");
    const auto damaged=OriginalMeshBuildAttempt({1u,2u});
    Require(damaged.Attempts==0u && damaged.Committed==0u,
        "corrupted/inconsistent telemetry resets conservatively");
    std::cout<<"Original UE mesh build accounting PASS.\n";
    return EXIT_SUCCESS;
}

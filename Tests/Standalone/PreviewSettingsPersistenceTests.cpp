#include "PreviewSettingsPersistence.h"
#include <cstdlib>
#include <iostream>

namespace
{
void Check(bool ok,const char* reason)
{
    if(!ok){std::cerr<<"Preview settings persistence failure: "<<reason<<'\n';
        std::exit(EXIT_FAILURE);}
}
bool Same(const latebrake_preview::PreviewSettingsState& a,
          const latebrake_preview::PreviewSettingsState& b)
{
    return a.Car==b.Car && a.Track==b.Track && a.Event==b.Event &&
        a.Contrast==b.Contrast && a.Assist==b.Assist;
}
void Versions()
{
    using namespace latebrake_preview;
    Check(!IsNewerPreviewSettingsSchema(PreviewSettingsMagicV3,3),
        "legacy V3 is readable");
    Check(!IsNewerPreviewSettingsSchema(PreviewSettingsMagicV4,4),
        "current V4 may be saved");
    Check(IsNewerPreviewSettingsSchema(0x35424955u,5),
        "future V5 must not be downgraded");
    Check(IsNewerPreviewSettingsSchema(PreviewSettingsMagicV4,6),
        "current magic with future version must be guarded");
    Check(IsNewerPreviewSettingsSchema(0x41424955u,10),
        "the UIB family also supports two-digit future versions");
    Check(!IsNewerPreviewSettingsSchema(0x35425955u,5),
        "unrelated file families are not misidentified");
    Check(!IsNewerPreviewSettingsSchema(0xDEADBEEFu,500),
        "corruption is not a recognized future schema");
    for(unsigned int i=0;i<=4u;++i)
        Check(!IsNewerPreviewSettingsSchema(PreviewSettingsMagicV4,i),
            "older/current version numbers may not be treated as future");
}
void Transactions()
{
    using namespace latebrake_preview;
    const PreviewSettingsState previous{0,1,2,false,true};
    const PreviewSettingsState desired{7,3,0,true,false};
    Check(Same(ResolvePreviewSettingsSave(previous,desired,true),desired),
        "successful write applies equipped car, track, event and toggles");
    Check(Same(ResolvePreviewSettingsSave(previous,desired,false),previous),
        "failed write atomically reverts all five preferences");
    unsigned int seed=0x9E3779B9u;
    for(int i=0;i<200000;i++){
        seed=seed*1664525u+1013904223u;
        const PreviewSettingsState before{
            static_cast<int>((seed>>8)%12u),
            static_cast<int>((seed>>16)%4u),
            static_cast<int>((seed>>21)%4u),
            (seed&1u)!=0u,(seed&2u)!=0u};
        seed=seed*1664525u+1013904223u;
        const PreviewSettingsState next{
            static_cast<int>((seed>>7)%12u),
            static_cast<int>((seed>>13)%4u),
            static_cast<int>((seed>>23)%4u),
            (seed&4u)!=0u,(seed&8u)!=0u};
        const bool saved=(seed&16u)!=0u;
        const auto actual=ResolvePreviewSettingsSave(before,next,saved);
        Check(Same(actual,saved?next:before),
            "200k deterministic success/failure recovery decisions");
        Check(Same(before,PreviewSettingsState{
            before.Car,before.Track,before.Event,before.Contrast,before.Assist}),
            "input snapshots are never mutated");
    }
}
} // namespace
int main()
{
    Versions();
    Transactions();
    std::cout<<"Portable 2D settings version and rollback QA passed.\n";
    return EXIT_SUCCESS;
}

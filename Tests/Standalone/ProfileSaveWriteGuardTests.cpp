#include "Progression/ProfileSaveWriteGuard.h"
#include <cstdlib>
#include <iostream>

namespace
{
void Check(bool condition,const char* reason)
{
    if (!condition){
        std::cerr<<"Profile SaveGame pre-write failure: "<<reason<<'\n';
        std::exit(EXIT_FAILURE);
    }
}

void Table()
{
    using namespace latebrake;
    const int career=CareerSaveSchema;
    const int appearance=static_cast<int>(GarageAppearanceState::SaveVersion);
    Check(AssessProfileSaveWrite(false,false,99,99)==
        ProfileSaveWriteState::NewProfile,"missing first-ever slot may be created");
    Check(AssessProfileSaveWrite(true,false,0,0)==
        ProfileSaveWriteState::UnreadableProfile,
        "existing unreadable/corrupt file must be preserved");
    Check(AssessProfileSaveWrite(true,true,-1,0)==
        ProfileSaveWriteState::CorruptSchema,"invalid negative career schema");
    Check(AssessProfileSaveWrite(true,true,0,-1)==
        ProfileSaveWriteState::CorruptSchema,"invalid negative cosmetic schema");
    Check(AssessProfileSaveWrite(true,true,career+1,appearance)==
        ProfileSaveWriteState::NewerCareer,"newer career format must be retained");
    Check(AssessProfileSaveWrite(true,true,career,appearance+1)==
        ProfileSaveWriteState::NewerAppearance,
        "newer appearance format must be retained independently");
    Check(AssessProfileSaveWrite(true,true,career,appearance)==
        ProfileSaveWriteState::CompatibleProfile,
        "current exact format is still writable");
    Check(AssessProfileSaveWrite(true,true,0,0)==
        ProfileSaveWriteState::CompatibleProfile,
        "legacy older format is still writable for migration");
    Check(!CanWriteProfileSave(ProfileSaveWriteState::UnreadableProfile)&&
        !CanWriteProfileSave(ProfileSaveWriteState::CorruptSchema)&&
        !CanWriteProfileSave(ProfileSaveWriteState::NewerAppearance)&&
        !CanWriteProfileSave(ProfileSaveWriteState::NewerCareer),
        "unknown data cannot gain write permission");
}

void DeterministicStress()
{
    using namespace latebrake;
    unsigned seed=0x7f4a7c15u;
    constexpr int cases=250000;
    for(int i=0;i<cases;i++){
        seed=seed*1664525u+1013904223u;
        const bool exists=(seed&1u)!=0,loaded=(seed&2u)!=0;
        const int career=static_cast<int>((seed>>8)%8u)-2;
        const int appearance=static_cast<int>((seed>>16)%7u)-2;
        const auto state=AssessProfileSaveWrite(
            exists,loaded,career,appearance);
        const bool expected=!exists ||
            (loaded&&career>=0&&appearance>=0&&
             career<=CareerSaveSchema &&
             appearance<=static_cast<int>(GarageAppearanceState::SaveVersion));
        Check(CanWriteProfileSave(state)==expected,
            "250k combinations match independent reference write gate");
        if(exists&&!loaded)
            Check(state==ProfileSaveWriteState::UnreadableProfile,
                "unreadable file always fails closed");
        if(!exists)
            Check(state==ProfileSaveWriteState::NewProfile,
                "nonexistent slot is the only permitted first-write bypass");
    }
}
} // namespace
int main()
{
    Table();
    DeterministicStress();
    std::cout<<"Unreal on-disk SaveGame schema recheck passed.\n";
    return EXIT_SUCCESS;
}

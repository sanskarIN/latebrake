#include "Garage/GarageLookShareCode.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <random>

namespace
{
using namespace latebrake;

void Check(bool truth,const char* label)
{
    if(!truth)
    {
        std::cerr<<"GarageLookShareCode: "<<label<<'\n';
        std::exit(EXIT_FAILURE);
    }
}

bool SameText(const GarageLookShareCode& a,const GarageLookShareCode& b)
{
    return a.Text==b.Text;
}

void RoundTripEveryCosmeticCombination()
{
    std::array<std::array<std::array<std::array<char,GarageLookCodeTextBytes>,
        GarageWheelOptions>,GarageStripeOptions>,GaragePaintOptions> unique{};
    for(int car=0;car<6;++car)
    for(int paint=0;paint<GaragePaintOptions;++paint)
    for(int stripe=0;stripe<GarageStripeOptions;++stripe)
    for(int wheel=0;wheel<GarageWheelOptions;++wheel)
    {
        const GarageLook look{
            static_cast<std::uint8_t>(paint),
            static_cast<std::uint8_t>(stripe),
            static_cast<std::uint8_t>(wheel)};
        const auto coded=EncodeGarageLookShareCode(car,look);
        Check(coded.Valid && coded.Status==GarageLookCodeStatus::Valid &&
            coded.VehicleId==car && SameGarageLook(coded.Look,look),
            "all allowed vehicle and cosmetic settings encode");
        Check(coded.Text[0]=='L'&&coded.Text[1]=='B'&&
            coded.Text[2]=='1'&&coded.Text[3]=='-'&&
            coded.Text[9]=='\0',"canonical fixed-width text structure");
        const auto decoded=DecodeGarageLookShareCode(coded.Text);
        Check(decoded.Valid && SameText(decoded,coded) &&
            MatchesGarageLookShareCode(decoded,car,look),
            "all 432 appearance combinations decode without approximation");
        unique[paint][stripe][wheel]=coded.Text;
        Check(!decoded.ContainsPurchaseAuthority && !decoded.ContainsSaveData &&
            !decoded.ContainsPersonalIdentifier,
            "code never carries economy or account state");
    }
    (void)unique;
}

void InvalidInputsAndVersioning()
{
    const auto valid=EncodeGarageLookShareCode(2,{7,2,1});
    Check(valid.Valid,"reference appearance encoded");
    for(int id:{-999,-1,6,8,999})
        Check(!EncodeGarageLookShareCode(id,{0,0,0}).Valid,
            "out-of-domain vehicle ID cannot encode");
    for(GarageLook look:std::array<GarageLook,4>{{{8,0,0},{0,3,0},
        {0,0,3},{255,255,255}}})
        Check(!EncodeGarageLookShareCode(0,look).Valid,
            "out-of-domain look cannot encode");
    Check(!DecodeGarageLookShareCode(nullptr,9).Valid,
        "null pointer rejected before access");
    for(std::size_t length:{0u,1u,3u,4u,8u,10u,999u})
        Check(!DecodeGarageLookShareCode(valid.Text.data(),length).Valid,
            "wrong length cannot alias a valid code");
    auto text=valid.Text;
    text[0]='X';
    Check(DecodeGarageLookShareCode(text).Status==
        GarageLookCodeStatus::InvalidPrefix,"prefix mismatch rejected");
    text=valid.Text;
    text[2]='2';
    Check(DecodeGarageLookShareCode(text).Status==
        GarageLookCodeStatus::UnsupportedVersion,"future text version is explicit");
    text=valid.Text;
    text[5]='i';
    Check(DecodeGarageLookShareCode(text).Status==
        GarageLookCodeStatus::InvalidCharacter,"lowercase ambiguity rejected");
    text=valid.Text;
    text[7]='O';
    Check(DecodeGarageLookShareCode(text).Status==
        GarageLookCodeStatus::InvalidCharacter,"ambiguous O rejected");
    text=valid.Text;
    text[8]='U';
    Check(DecodeGarageLookShareCode(text).Status==
        GarageLookCodeStatus::InvalidCharacter,"ambiguous U rejected");
    text=valid.Text;
    text.back()='Z';
    Check(!DecodeGarageLookShareCode(text).Valid,"missing NUL terminator rejected");
    Check(!MatchesGarageLookShareCode(valid,5,{7,2,1}) &&
        !MatchesGarageLookShareCode(valid,2,{7,1,1}),
        "share code identity and look must both match");
}

void ManualChecksumAndReservedBits()
{
    auto code=EncodeGarageLookShareCode(4,{6,2,2});
    const auto original=code;
    code.Text[8]=code.Text[8]=='0'?'1':'0';
    Check(DecodeGarageLookShareCode(code.Text).Status==
        GarageLookCodeStatus::InvalidChecksum,
        "changed checksum-only least significant digit rejected");
    code=original;
    code.Text[4]='Z';
    Check(DecodeGarageLookShareCode(code.Text).Status==
        GarageLookCodeStatus::Noncanonical,
        "unused top bits cannot create alternate representations");
    const auto payload=PackGarageLookCodePayload(4,{6,2,2});
    Check(payload<8192u && GarageLookCodeChecksum(payload)<1024u,
        "payload and checksum stay inside versioned bit allocation");
    int car=-1;
    GarageLook parsed{};
    Check(UnpackGarageLookCodePayload(payload,car,parsed) &&
        car==4 && SameGarageLook(parsed,{6,2,2}),
        "packed metadata restores exactly the original display preferences");
    const auto wrongVersion=(payload&1023u)|(2u<<10u);
    Check(!UnpackGarageLookCodePayload(wrongVersion,car,parsed),
        "future payload layout never decoded as current version");
    const auto outOfRangeCar=(payload&~(7u<<7u))|(7u<<7u);
    Check(!UnpackGarageLookCodePayload(outOfRangeCar,car,parsed),
        "reserved vehicle IDs do not grant fictional ownership");
}

void DeterministicFuzzAndNoStateMutation()
{
    std::mt19937_64 random(0xBACE1F01ULL);
    std::uniform_int_distribution<int> id(0,5);
    std::uniform_int_distribution<int> paint(0,7);
    std::uniform_int_distribution<int> stripe(0,2);
    std::uniform_int_distribution<int> wheel(0,2);
    std::uniform_int_distribution<int> digit(4,8);
    std::uniform_int_distribution<int> alternative(0,31);
    int rejected=0;
    int mutations=0;
    for(int step=0;step<8000;++step)
    {
        const int car=id(random);
        GarageLook look{static_cast<std::uint8_t>(paint(random)),
            static_cast<std::uint8_t>(stripe(random)),
            static_cast<std::uint8_t>(wheel(random))};
        const auto before=look;
        const auto encoded=EncodeGarageLookShareCode(car,look);
        Check(encoded.Valid && SameGarageLook(before,look),
            "encoding cannot mutate or authorize a look transaction");
        auto parsed=DecodeGarageLookShareCode(encoded.Text);
        Check(parsed.Valid && parsed.VehicleId==car &&
            SameGarageLook(parsed.Look,look),
            "seeded round trip matches selected vehicle and cosmetic layers");
        auto corrupted=encoded.Text;
        const auto slot=digit(random);
        const char change=GarageLookCodeAlphabet[alternative(random)];
        if(corrupted[slot]==change)continue;
        corrupted[slot]=change;
        ++mutations;
        const auto inspected=DecodeGarageLookShareCode(corrupted);
        if(!inspected.Valid)++rejected;
        Check(!inspected.Valid ||
            (inspected.VehicleId!=encoded.VehicleId ||
                !SameGarageLook(inspected.Look,encoded.Look)),
            "mutated code cannot silently create an exact duplicate canonical value");
    }
    Check(mutations>6000 && rejected>mutations*99/100,
        "checksum detects overwhelming majority of random single-character typos");
}

void CompileTimeExamples()
{
    constexpr auto original=EncodeGarageLookShareCode(0,{0,0,0});
    static_assert(original.Valid);
    static_assert(original.Text[0]=='L');
    constexpr auto recovered=DecodeGarageLookShareCode(original.Text);
    static_assert(recovered.Valid);
    static_assert(recovered.VehicleId==0);
    static_assert(recovered.Look.Paint==0);
    static_assert(recovered.Look.Stripe==0);
    static_assert(recovered.Look.Wheels==0);
    Check(recovered.Text==original.Text,
        "runtime and constexpr encoders share the same immutable representation");
}
}

int main()
{
    RoundTripEveryCosmeticCombination();
    InvalidInputsAndVersioning();
    ManualChecksumAndReservedBits();
    DeterministicFuzzAndNoStateMutation();
    CompileTimeExamples();
    std::cout<<"GarageLookShareCode: exhaustive value-domain and typo tests passed.\n";
    return EXIT_SUCCESS;
}

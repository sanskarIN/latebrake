#pragma once

#include "Garage/GarageAppearance.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace latebrake
{
// A reproducible *cosmetic preference* string for the six original Unreal
// garage cars. It neither unlocks ownership nor applies styling, persists
// a save, verifies authorship, or changes driving characteristics.
//
// Wire format: LB1-XXXXX (Crockford Base32, uppercase only), five digits
// pack version, immutable car ID, paint, stripe, wheels and 10-bit checksum.
// No user ID, device data, saved money, license, online request or timestamp.
inline constexpr std::size_t GarageLookCodeDigits=5;
inline constexpr std::size_t GarageLookCodeTextBytes=10;
inline constexpr std::uint32_t GarageLookCodeVersion=1;
inline constexpr char GarageLookCodeAlphabet[]=
    "0123456789ABCDEFGHJKMNPQRSTVWXYZ";
static_assert(sizeof(GarageLookCodeAlphabet)==33);

enum class GarageLookCodeStatus : std::uint8_t
{
    Valid,
    InvalidLength,
    InvalidPrefix,
    InvalidCharacter,
    UnsupportedVersion,
    InvalidChecksum,
    InvalidVehicle,
    InvalidAppearance,
    Noncanonical
};

struct GarageLookShareCode final
{
    std::array<char,GarageLookCodeTextBytes> Text{};
    bool Valid=false;
    int VehicleId=-1;
    GarageLook Look{};
    GarageLookCodeStatus Status=GarageLookCodeStatus::InvalidAppearance;
    bool ContainsPurchaseAuthority=false;
    bool ContainsSaveData=false;
    bool ContainsPersonalIdentifier=false;
};

inline constexpr unsigned GarageLookCodeChecksum(
    std::uint32_t payload) noexcept
{
    // A small error-detection check, NOT a MAC, digital signature, or
    // evidence of a trustworthy sender. All inputs are user editable.
    std::uint32_t hash=2166136261u;
    for(unsigned i=0;i<4;++i)
    {
        hash^=(payload>>(i*8u))&255u;
        hash*=16777619u;
    }
    hash^=0xA65DA1B7u;
    hash*=16777619u;
    return static_cast<unsigned>((hash^(hash>>13u)^(hash>>23u))&1023u);
}

inline constexpr std::uint32_t PackGarageLookCodePayload(
    int vehicleId,GarageLook look) noexcept
{
    return (GarageLookCodeVersion&7u)<<10u |
        (static_cast<std::uint32_t>(vehicleId)&7u)<<7u |
        (static_cast<std::uint32_t>(look.Paint)&7u)<<4u |
        (static_cast<std::uint32_t>(look.Stripe)&3u)<<2u |
        (static_cast<std::uint32_t>(look.Wheels)&3u);
}

inline constexpr bool UnpackGarageLookCodePayload(
    std::uint32_t payload,int& vehicleId,GarageLook& look) noexcept
{
    const std::uint32_t version=(payload>>10u)&7u;
    if(version!=GarageLookCodeVersion)return false;
    vehicleId=static_cast<int>((payload>>7u)&7u);
    look={static_cast<std::uint8_t>((payload>>4u)&7u),
          static_cast<std::uint8_t>((payload>>2u)&3u),
          static_cast<std::uint8_t>(payload&3u)};
    return GarageState::IsVehicle(vehicleId) && ValidGarageLook(look);
}

inline constexpr int GarageLookCodeDigitValue(char ch) noexcept
{
    // Strict canonical alphabet makes sharing unambiguous and avoids
    // silently guessing a different car when a character is mistyped.
    for(int index=0;index<32;++index)
        if(ch==GarageLookCodeAlphabet[index])return index;
    return -1;
}

inline constexpr GarageLookShareCode EncodeGarageLookShareCode(
    int vehicleId,GarageLook appearance) noexcept
{
    GarageLookShareCode out;
    if(!GarageState::IsVehicle(vehicleId))
    {
        out.Status=GarageLookCodeStatus::InvalidVehicle;
        return out;
    }
    if(!ValidGarageLook(appearance))
    {
        out.Status=GarageLookCodeStatus::InvalidAppearance;
        return out;
    }
    const std::uint32_t payload=
        PackGarageLookCodePayload(vehicleId,appearance);
    const std::uint32_t packed=(payload<<10u)|
        GarageLookCodeChecksum(payload);
    out.Text[0]='L';
    out.Text[1]='B';
    out.Text[2]='1';
    out.Text[3]='-';
    for(unsigned digit=0;digit<GarageLookCodeDigits;++digit)
    {
        const unsigned shift=5u*static_cast<unsigned>(GarageLookCodeDigits-1-digit);
        const unsigned index=(packed>>shift)&31u;
        out.Text[4u+digit]=GarageLookCodeAlphabet[index];
    }
    out.Text[GarageLookCodeTextBytes-1]='\0';
    out.Valid=true;
    out.VehicleId=vehicleId;
    out.Look=appearance;
    out.Status=GarageLookCodeStatus::Valid;
    return out;
}

inline constexpr GarageLookShareCode DecodeGarageLookShareCode(
    const char* text,std::size_t length) noexcept
{
    GarageLookShareCode out;
    if(!text || length!=GarageLookCodeTextBytes-1)
    {
        out.Status=GarageLookCodeStatus::InvalidLength;
        return out;
    }
    if(text[0]!='L' || text[1]!='B' || text[3]!='-')
    {
        out.Status=GarageLookCodeStatus::InvalidPrefix;
        return out;
    }
    if(text[2]!='1')
    {
        out.Status=GarageLookCodeStatus::UnsupportedVersion;
        return out;
    }
    std::uint32_t bits=0u;
    for(std::size_t i=4;i<4+GarageLookCodeDigits;++i)
    {
        const int value=GarageLookCodeDigitValue(text[i]);
        if(value<0)
        {
            out.Status=GarageLookCodeStatus::InvalidCharacter;
            return out;
        }
        bits=(bits<<5u)|static_cast<unsigned>(value);
    }
    // The highest two bits of this five-digit representation are reserved.
    // Accepting their nonzero values would allow noncanonical aliases.
    if((bits>>23u)!=0)
    {
        out.Status=GarageLookCodeStatus::Noncanonical;
        return out;
    }
    const std::uint32_t payload=(bits>>10u)&8191u;
    const unsigned claimed=bits&1023u;
    if(GarageLookCodeChecksum(payload)!=claimed)
    {
        out.Status=GarageLookCodeStatus::InvalidChecksum;
        return out;
    }
    int id=-1;
    GarageLook look{};
    if(!UnpackGarageLookCodePayload(payload,id,look))
    {
        out.Status=((payload>>10u)&7u)!=GarageLookCodeVersion ?
            GarageLookCodeStatus::UnsupportedVersion :
            !GarageState::IsVehicle(static_cast<int>((payload>>7u)&7u)) ?
                GarageLookCodeStatus::InvalidVehicle :
                GarageLookCodeStatus::InvalidAppearance;
        return out;
    }
    out=EncodeGarageLookShareCode(id,look);
    // Check the original exactly, including the canonical prefix and digits.
    for(std::size_t i=0;i<length;++i)
        if(out.Text[i]!=text[i])
        {
            return GarageLookShareCode{};
        }
    return out;
}

inline constexpr GarageLookShareCode DecodeGarageLookShareCode(
    const std::array<char,GarageLookCodeTextBytes>& text) noexcept
{
    if(text.back()!='\0')return {};
    return DecodeGarageLookShareCode(text.data(),GarageLookCodeTextBytes-1);
}

inline constexpr bool MatchesGarageLookShareCode(
    const GarageLookShareCode& code,int vehicleId,GarageLook appearance) noexcept
{
    return code.Valid && code.Status==GarageLookCodeStatus::Valid &&
        code.VehicleId==vehicleId && SameGarageLook(code.Look,appearance) &&
        !code.ContainsPurchaseAuthority && !code.ContainsSaveData &&
        !code.ContainsPersonalIdentifier;
}
} // namespace latebrake

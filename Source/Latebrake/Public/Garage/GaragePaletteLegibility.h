#pragma once

#include "Vehicles/VehiclePaintRecipe.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake
{
// Exact authored sRGB paint recipes can be contrasted on a schematic.
// Real UE lighting, tone mapping, reflections, disability needs and HUD
// font rendering must be tested independently; this is NOT accessibility
// compliance certification or an automatic appearance mutation.
inline constexpr double GarageContrastIllustrativeLargeShape=3.0;
inline constexpr double GarageContrastIllustrativeSmallText=4.5;

enum class GaragePaletteVisibility : std::uint8_t
{
    Unavailable,
    NoStripes,
    LowDistinctiveness,
    ModerateDistinctiveness,
    StrongDistinctiveness
};

inline const char* GaragePaletteVisibilityLabel(GaragePaletteVisibility quality) noexcept
{
    switch(quality)
    {
    case GaragePaletteVisibility::Unavailable:return "LOOK VISIBILITY UNAVAILABLE";
    case GaragePaletteVisibility::NoStripes:return "NO STRIPES: BODY COLOR ONLY";
    case GaragePaletteVisibility::LowDistinctiveness:
        return "LOW STRIPE/BODY SCHEMATIC CONTRAST";
    case GaragePaletteVisibility::ModerateDistinctiveness:
        return "MODERATE STRIPE/BODY SCHEMATIC CONTRAST";
    case GaragePaletteVisibility::StrongDistinctiveness:
        return "STRONG STRIPE/BODY SCHEMATIC CONTRAST";
    default:return "LOOK CONTRAST UNAVAILABLE";
    }
}

enum class GaragePaletteInk : std::uint8_t
{
    Unavailable,
    Dark,
    Light
};

inline const char* GaragePaletteInkLabel(GaragePaletteInk ink) noexcept
{
    switch(ink)
    {
    case GaragePaletteInk::Dark:return "DARK SCHEMATIC LABEL INK";
    case GaragePaletteInk::Light:return "LIGHT SCHEMATIC LABEL INK";
    default:return "SCHEMATIC LABEL INK UNAVAILABLE";
    }
}

inline bool ValidGaragePaintColor(VehiclePaintRgb c) noexcept
{
    return std::isfinite(c.R) && std::isfinite(c.G) &&
        std::isfinite(c.B) && c.R>=0.0 && c.R<=1.0 &&
        c.G>=0.0 && c.G<=1.0 && c.B>=0.0 && c.B<=1.0;
}

inline double GarageSrgbLinearize(double channel) noexcept
{
    if(!std::isfinite(channel) || channel<0.0 || channel>1.0)
        return std::numeric_limits<double>::quiet_NaN();
    return channel<=0.04045 ? channel/12.92 :
        std::pow((channel+0.055)/1.055,2.4);
}

inline double GaragePaletteLuminance(VehiclePaintRgb color) noexcept
{
    if(!ValidGaragePaintColor(color))
        return std::numeric_limits<double>::quiet_NaN();
    return 0.2126*GarageSrgbLinearize(color.R)+
        0.7152*GarageSrgbLinearize(color.G)+
        0.0722*GarageSrgbLinearize(color.B);
}

inline double GaragePaletteContrastRatio(VehiclePaintRgb a,
    VehiclePaintRgb b) noexcept
{
    const double first=GaragePaletteLuminance(a);
    const double second=GaragePaletteLuminance(b);
    if(!std::isfinite(first) || !std::isfinite(second))return 0.0;
    const double bright=std::max(first,second);
    const double dark=std::min(first,second);
    return (bright+0.05)/(dark+0.05);
}

struct GaragePaletteLegibility final
{
    bool Valid=false;
    int VehicleId=-1;
    GarageLook Look{};
    VehiclePaintRgb Body{};
    VehiclePaintRgb Stripe{};
    VehiclePaintRgb Accent{};
    VehiclePaintRgb SuggestedInk{};
    GaragePaletteInk Ink=GaragePaletteInk::Unavailable;
    GaragePaletteVisibility Visibility=GaragePaletteVisibility::Unavailable;
    double BodyStripeContrast=0.0;
    double BodyAccentContrast=0.0;
    double BodyLabelContrast=0.0;
    double StripeLuminance=0.0;
    double BodyLuminance=0.0;
    bool StripeVisible=false;
    bool LabelMeetsIllustrativeContrast=false;
    bool StripesMeetIllustrativeContrast=false;
    bool UsesCalibratedDisplayEvidence=false;
    bool IsOfficialAccessibilityAudit=false;
    bool ChangesAppearance=false;
    bool ChangesGarageEconomy=false;
};

inline GaragePaletteVisibility GaragePaletteGradeContrast(
    double ratio,bool stripes) noexcept
{
    if(!std::isfinite(ratio) || ratio<1.0 || ratio>21.0)return
        GaragePaletteVisibility::Unavailable;
    if(!stripes)return GaragePaletteVisibility::NoStripes;
    if(ratio<2.0)return GaragePaletteVisibility::LowDistinctiveness;
    if(ratio<GarageContrastIllustrativeLargeShape)
        return GaragePaletteVisibility::ModerateDistinctiveness;
    return GaragePaletteVisibility::StrongDistinctiveness;
}

inline GaragePaletteLegibility EvaluateGaragePaletteLegibility(
    int vehicle,GarageLook look) noexcept
{
    if(!GarageState::IsVehicle(vehicle) || !ValidGarageLook(look))return {};
    const auto recipe=PaintForGarageLook(vehicle,look);
    const auto stripe=StripeTintForGarageLook(look);
    if(!ValidGaragePaintColor(recipe.Body) ||
       !ValidGaragePaintColor(recipe.Accent) ||
       !ValidGaragePaintColor(stripe))return {};
    GaragePaletteLegibility out;
    out.VehicleId=vehicle;
    out.Look=look;
    out.Body=recipe.Body;
    out.Stripe=stripe;
    out.Accent=recipe.Accent;
    out.BodyLuminance=GaragePaletteLuminance(out.Body);
    out.StripeLuminance=GaragePaletteLuminance(out.Stripe);
    out.BodyStripeContrast=GaragePaletteContrastRatio(out.Body,out.Stripe);
    out.BodyAccentContrast=GaragePaletteContrastRatio(out.Body,out.Accent);
    const VehiclePaintRgb light{1.0,1.0,1.0};
    const VehiclePaintRgb dark{0.0,0.0,0.0};
    const auto lightRatio=GaragePaletteContrastRatio(out.Body,light);
    const auto darkRatio=GaragePaletteContrastRatio(out.Body,dark);
    const bool useDark=darkRatio>=lightRatio;
    out.SuggestedInk=useDark?dark:light;
    out.Ink=useDark?GaragePaletteInk::Dark:GaragePaletteInk::Light;
    out.BodyLabelContrast=std::max(lightRatio,darkRatio);
    out.StripeVisible=look.Stripe!=0u;
    out.Visibility=GaragePaletteGradeContrast(out.BodyStripeContrast,
        out.StripeVisible);
    out.LabelMeetsIllustrativeContrast=
        out.BodyLabelContrast>=GarageContrastIllustrativeSmallText;
    out.StripesMeetIllustrativeContrast=out.StripeVisible &&
        out.BodyStripeContrast>=GarageContrastIllustrativeLargeShape;
    out.Valid=std::isfinite(out.BodyLuminance) &&
        std::isfinite(out.StripeLuminance) &&
        out.BodyStripeContrast>=1.0 && out.BodyStripeContrast<=21.0 &&
        out.BodyLabelContrast>=1.0 && out.BodyLabelContrast<=21.0;
    return out.Valid?out:GaragePaletteLegibility{};
}

inline bool ValidGaragePaletteLegibility(GaragePaletteLegibility result) noexcept
{
    if(!result.Valid || !GarageState::IsVehicle(result.VehicleId) ||
       !ValidGarageLook(result.Look) ||
       !ValidGaragePaintColor(result.Body) ||
       !ValidGaragePaintColor(result.Stripe) ||
       !ValidGaragePaintColor(result.Accent) ||
       !ValidGaragePaintColor(result.SuggestedInk) ||
       !std::isfinite(result.BodyStripeContrast) ||
       !std::isfinite(result.BodyLabelContrast) ||
       result.BodyStripeContrast<1.0 || result.BodyStripeContrast>21.0 ||
       result.BodyLabelContrast<1.0 || result.BodyLabelContrast>21.0 ||
       result.StripeVisible!=(result.Look.Stripe!=0u) ||
       result.Ink==GaragePaletteInk::Unavailable ||
       result.Visibility==GaragePaletteVisibility::Unavailable ||
       result.UsesCalibratedDisplayEvidence ||
       result.IsOfficialAccessibilityAudit ||
       result.ChangesAppearance || result.ChangesGarageEconomy)return false;
    const auto expected=EvaluateGaragePaletteLegibility(result.VehicleId,
        result.Look);
    return expected.Valid &&
        std::abs(expected.BodyStripeContrast-result.BodyStripeContrast)<1e-8 &&
        std::abs(expected.BodyLabelContrast-result.BodyLabelContrast)<1e-8 &&
        std::abs(expected.BodyLuminance-result.BodyLuminance)<1e-8 &&
        expected.Ink==result.Ink &&
        expected.Visibility==result.Visibility &&
        expected.StripesMeetIllustrativeContrast==
            result.StripesMeetIllustrativeContrast &&
        expected.LabelMeetsIllustrativeContrast==
            result.LabelMeetsIllustrativeContrast;
}

struct GaragePalettePaintOption final
{
    int PaintId=-1;
    GaragePaletteLegibility Legibility{};
};

struct GaragePaletteOptionReview final
{
    bool Valid=false;
    int VehicleId=-1;
    GarageLook Baseline{};
    int StrongestStripeOption=-1;
    int FirstIllustrativelyReadableOption=-1;
    unsigned StrongStripeOptionCount=0;
    std::array<GaragePalettePaintOption,GaragePaintOptions> Paints{};
    bool RequiresPlayerConsent=true;
    bool ReadOnly=true;
};

inline GaragePaletteOptionReview CompareGaragePalettePaintOptions(
    int vehicle,GarageLook baseline) noexcept
{
    GaragePaletteOptionReview out;
    if(!GarageState::IsVehicle(vehicle) || !ValidGarageLook(baseline))return out;
    out.VehicleId=vehicle;
    out.Baseline=baseline;
    double strongest=-1.0;
    for(int i=0;i<GaragePaintOptions;++i)
    {
        auto candidate=baseline;
        candidate.Paint=static_cast<std::uint8_t>(i);
        const auto evaluation=EvaluateGaragePaletteLegibility(vehicle,candidate);
        if(!ValidGaragePaletteLegibility(evaluation))return {};
        out.Paints[static_cast<std::size_t>(i)]={i,evaluation};
        if(baseline.Stripe!=0u &&
           evaluation.BodyStripeContrast>=GarageContrastIllustrativeLargeShape)
        {
            ++out.StrongStripeOptionCount;
            if(out.FirstIllustrativelyReadableOption<0)
                out.FirstIllustrativelyReadableOption=i;
        }
        if(evaluation.BodyStripeContrast>strongest)
        {
            strongest=evaluation.BodyStripeContrast;
            out.StrongestStripeOption=i;
        }
    }
    out.Valid=true;
    return out;
}
}

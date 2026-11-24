#pragma once
#include "Vehicles/OriginalVehicleAnimatedWheels.h"
#include "Vehicles/VehiclePaintRecipe.h"

#include <array>
#include <cctype>
#include <iomanip>
#include <ostream>
#include <string>
#include <string_view>

namespace latebrake::vehiclemesh
{
// An inspectable Y-up OBJ export of the SAME generated six-car 3D geometry
// that the native UE Pawn consumes. NO claimed cooked UE assets, car handling,
// GPU material equivalence, wheel animation or licensed engine validation.
inline constexpr std::array<const char*,OriginalVehicleSectionCount>
OriginalPreviewMaterials{{
    "BodyPaint","Accent","ClearGlass","Rubber","Alloy",
    "BrakeRotor","Headlamp","Taillamp","Intake","CarbonTrim"
}};
inline constexpr std::array<const char*,OriginalVehicleAnimatedPartCount>
OriginalPreviewParts{{
    "Body","FrontLeftWheel","FrontRightWheel",
    "RearLeftWheel","RearRightWheel"
}};

inline bool ValidObjMtlBasename(std::string_view name) noexcept
{
    if(name.empty() || name.size()>80 || name=="." || name=="..")
        return false;
    for(const unsigned char ch:name)
        if(!((ch>='a'&&ch<='z') || (ch>='A'&&ch<='Z') ||
             (ch>='0'&&ch<='9') || ch=='_' || ch=='-' || ch=='.'))
            return false;
    return name.find("..")==std::string_view::npos;
}

inline bool WriteOriginalVehicleObj(
    std::ostream& out,const OriginalVehicleMesh& mesh,
    std::string_view mtlBasename)
{
    if(!ValidObjMtlBasename(mtlBasename) ||
       !MakeOriginalVehicleAnimatedParts(mesh).Valid)
        return false;
    out<<std::fixed<<std::setprecision(6);
    out<<"# LateBrake source-generated 3D geometry - NOT a cooked Unreal game\n"
       <<"# Y-up, metres; mapping from source (X,Y,Z) cm to (X,Z,-Y) m\n"
       <<"# Original wheel components tagged but NOT physically animated in OBJ\n"
       <<"mtllib "<<mtlBasename<<"\n"
       <<"o LateBrakeOriginalCar_"<<mesh.VehicleId
       <<"_Rim_"<<mesh.RimStyle<<"\n";
    for(const auto& v:mesh.Vertices)
        out<<"v "<<v.Position.X/100.0<<" "<<v.Position.Z/100.0
           <<" "<<-v.Position.Y/100.0<<"\n";
    for(const auto& v:mesh.Vertices)
        out<<"vt "<<v.TexCoord.U<<" "<<1.0-v.TexCoord.V<<"\n";
    for(const auto& v:mesh.Vertices)
        out<<"vn "<<v.Normal.X<<" "<<v.Normal.Z<<" "
           <<-v.Normal.Y<<"\n";

    int currentPart=-2;
    int currentMaterial=-1;
    for(const auto& tri:mesh.Triangles)
    {
        const int part=static_cast<int>(tri.WheelSlot)+1;
        const int material=static_cast<int>(tri.Material);
        if(part!=currentPart)
        {
            out<<"g "<<OriginalPreviewParts[static_cast<std::size_t>(part)]
               <<"\n";
            currentPart=part;
        }
        if(material!=currentMaterial)
        {
            out<<"usemtl "<<OriginalPreviewMaterials[
                static_cast<std::size_t>(material)]<<"\n";
            currentMaterial=material;
        }
        out<<"f";
        for(const auto i:tri.Index)
        {
            const auto index=static_cast<std::size_t>(i)+1u;
            out<<" "<<index<<"/"<<index<<"/"<<index;
        }
        out<<"\n";
    }
    return static_cast<bool>(out);
}

inline bool WriteOriginalVehicleMtl(
    std::ostream& out,int vehicleId,latebrake::GarageLook look)
{
    if(!latebrake::GarageState::IsVehicle(vehicleId) ||
       !latebrake::ValidGarageLook(look))
        return false;
    const auto paint=latebrake::PaintForGarageLook(vehicleId,look);
    const auto rim=look.Wheels==2u
        ? latebrake::VehiclePaintRgb{0.10,0.82,0.91}
        : look.Wheels==1u
            ? latebrake::VehiclePaintRgb{0.87,0.89,0.92}
            : latebrake::VehiclePaintRgb{0.30,0.33,0.38};
    const std::array<latebrake::VehiclePaintRgb,OriginalVehicleSectionCount>
        colors{{
            paint.Body,paint.Accent,paint.Canopy,
            {0.030,0.033,0.038},rim,{0.30,0.32,0.36},
            {0.95,0.95,0.81},{0.90,0.06,0.045},
            {0.04,0.06,0.07},{0.12,0.14,0.17}
        }};
    out<<std::fixed<<std::setprecision(6);
    out<<"# Source preview diffuse swatches only. NOT Unreal PBR shaders.\n";
    for(std::size_t i=0;i<colors.size();++i)
    {
        const auto c=colors[i];
        out<<"newmtl "<<OriginalPreviewMaterials[i]<<"\n"
           <<"Ka 0.000000 0.000000 0.000000\n"
           <<"Kd "<<c.R<<" "<<c.G<<" "<<c.B<<"\n"
           <<"Ks 0.120000 0.120000 0.120000\n"
           <<"Ns 20.000000\n"
           <<"d 1.000000\n\n";
    }
    return static_cast<bool>(out);
}
static_assert(OriginalPreviewMaterials.size()==OriginalVehicleSectionCount);
static_assert(OriginalPreviewParts.size()==5u);
} // namespace latebrake::vehiclemesh

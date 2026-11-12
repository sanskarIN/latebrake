#pragma once
#include "Vehicles/VehicleVisualRecipe.h"
#include "Vehicles/VehicleBodyKitRecipe.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace latebrake::vehiclemesh
{
inline constexpr double OriginalVehiclePi=3.14159265358979323846;
inline constexpr int OriginalVehicleWheelSides=24;
inline constexpr std::size_t OriginalVehicleMaxVertices=12000;
inline constexpr std::size_t OriginalVehicleMaxTriangles=8000;
enum class OriginalVehicleMaterial : std::uint8_t
{
    BodyPaint, Accent, ClearGlass, Rubber, Alloy, BrakeRotor,
    Headlamp, Taillamp, Intake, CarbonTrim, Count
};
struct OriginalVehiclePoint final
{
    double X=0.0;
    double Y=0.0;
    double Z=0.0;
};
struct OriginalVehicleUv final
{
    double U=0.0;
    double V=0.0;
};
struct OriginalVehicleVertex final
{
    OriginalVehiclePoint Position{};
    OriginalVehiclePoint Normal{};
    OriginalVehicleUv TexCoord{};
};
struct OriginalVehicleTriangle final
{
    std::array<std::uint32_t,3> Index{};
    OriginalVehicleMaterial Material=OriginalVehicleMaterial::BodyPaint;
};
struct OriginalVehicleBounds final
{
    OriginalVehiclePoint Minimum{
        std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::infinity()};
    OriginalVehiclePoint Maximum{
        -std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity()};
};
struct OriginalVehicleMesh final
{
    std::vector<OriginalVehicleVertex> Vertices{};
    std::vector<OriginalVehicleTriangle> Triangles{};
    OriginalVehicleBounds Bounds{};
    int VehicleId=-1;
    int WheelTireQuads=0;
    int WindowQuads=0;
    int BodyQuads=0;
    int DetailQuads=0;
    int HeadlampQuads=0;
    int TaillampQuads=0;
    bool OriginalGeometry=true;
    bool VisualOnly=true;
    bool ChangesChaosContact=false;
    bool Ready=false;
};
struct OriginalVehicleMeshSettings final
{
    double LengthCm=444.0;
    double HalfWidthCm=99.0;
    double WheelbaseCm=275.0;
    double WheelCenterHeightCm=40.0;
    double WheelRadiusCm=39.0;
    double RimRadiusCm=23.0;
    double TireWidthCm=30.0;
    double BodyShoulderHeightCm=105.0;
    double CabinRoofHeightCm=174.0;
    double WheelOpeningClearanceCm=5.0;
    int LongitudinalSegments=32;
    int WheelRadialSegments=OriginalVehicleWheelSides;
    bool NeverChangeCollision=true;
    bool OriginalFictionalDesign=true;
};
enum class OriginalVehicleMeshFault : std::uint8_t
{
    None, InvalidConfiguration, InvalidVehicle, DegenerateSurface,
    CapacityExceeded, InvalidMesh, IncompleteVisuals
};
struct OriginalVehicleMeshResult final
{
    OriginalVehicleMeshFault Fault=OriginalVehicleMeshFault::InvalidConfiguration;
    OriginalVehicleMesh Mesh{};
    bool Valid=false;
};
inline OriginalVehiclePoint MeshSubtract(
    OriginalVehiclePoint a,OriginalVehiclePoint b) noexcept
{
    return {a.X-b.X,a.Y-b.Y,a.Z-b.Z};
}
inline OriginalVehiclePoint MeshCross(
    OriginalVehiclePoint a,OriginalVehiclePoint b) noexcept
{
    return {
        a.Y*b.Z-a.Z*b.Y,
        a.Z*b.X-a.X*b.Z,
        a.X*b.Y-a.Y*b.X
    };
}
inline double MeshLength(OriginalVehiclePoint p) noexcept
{
    return std::hypot(p.X,p.Y,p.Z);
}
inline bool MeshFinite(double x) noexcept
{
    return std::isfinite(x);
}
inline bool MeshFinitePoint(OriginalVehiclePoint p) noexcept
{
    return MeshFinite(p.X) && MeshFinite(p.Y) && MeshFinite(p.Z);
}
inline double MeshSmoothStep(double x) noexcept
{
    const double u=std::clamp(x,0.0,1.0);
    return u*u*(3.0-2.0*u);
}
inline double MeshRaisedCosine(double x) noexcept
{
    const double t=std::abs(x);
    if(t>=1.0)
        return 0.0;
    return 0.5+0.5*std::cos(OriginalVehiclePi*t);
}
inline bool ValidOriginalVehicleSettings(
    const OriginalVehicleMeshSettings& p) noexcept
{
    return MeshFinite(p.LengthCm) &&
        p.LengthCm>=370.0 && p.LengthCm<=520.0 &&
        MeshFinite(p.HalfWidthCm) &&
        p.HalfWidthCm>=80.0 && p.HalfWidthCm<=115.0 &&
        MeshFinite(p.WheelbaseCm) &&
        p.WheelbaseCm>200.0 &&
        p.WheelbaseCm<p.LengthCm-70.0 &&
        MeshFinite(p.WheelCenterHeightCm) &&
        p.WheelCenterHeightCm>=30.0 &&
        p.WheelCenterHeightCm<=55.0 &&
        MeshFinite(p.WheelRadiusCm) &&
        p.WheelRadiusCm>=28.0 &&
        p.WheelRadiusCm<=p.WheelCenterHeightCm+5.0 &&
        MeshFinite(p.RimRadiusCm) &&
        p.RimRadiusCm>8.0 &&
        p.RimRadiusCm<p.WheelRadiusCm-7.0 &&
        MeshFinite(p.TireWidthCm) &&
        p.TireWidthCm>=20.0 &&
        p.TireWidthCm<=45.0 &&
        MeshFinite(p.BodyShoulderHeightCm) &&
        p.BodyShoulderHeightCm>=90.0 &&
        p.BodyShoulderHeightCm<=135.0 &&
        MeshFinite(p.CabinRoofHeightCm) &&
        p.CabinRoofHeightCm>p.BodyShoulderHeightCm+35.0 &&
        p.CabinRoofHeightCm<=230.0 &&
        MeshFinite(p.WheelOpeningClearanceCm) &&
        p.WheelOpeningClearanceCm>=2.0 &&
        p.WheelOpeningClearanceCm<=12.0 &&
        p.LongitudinalSegments>=20 &&
        p.LongitudinalSegments<=64 &&
        p.WheelRadialSegments>=16 &&
        p.WheelRadialSegments<=48 &&
        p.NeverChangeCollision &&
        p.OriginalFictionalDesign;
}
class OriginalVehicleSurfaceComposer final
{
public:
    explicit OriginalVehicleSurfaceComposer(
        OriginalVehicleMeshSettings settings={}) noexcept
        : Settings(settings)
    {
    }
    OriginalVehicleMeshResult Build(int vehicleId) const
    {
        OriginalVehicleMeshResult result;
        if(!ValidOriginalVehicleSettings(Settings))
        {
            result.Fault=OriginalVehicleMeshFault::InvalidConfiguration;
            return result;
        }
        if(vehicleId<0 || vehicleId>=6)
        {
            result.Fault=OriginalVehicleMeshFault::InvalidVehicle;
            return result;
        }
        auto& mesh=result.Mesh;
        mesh.VehicleId=vehicleId;
        mesh.Vertices.reserve(5000);
        mesh.Triangles.reserve(2400);
        const auto visual=latebrake::VisualRecipeForVehicle(vehicleId);
        const auto aero=latebrake::BodyKitForVehicle(vehicleId);
        MeshWriter writer(mesh);
        BuildMainBody(writer,visual,vehicleId);
        BuildFourWheels(writer);
        BuildLights(writer);
        BuildWindows(writer,visual);
        BuildAero(writer,visual,aero);
        BuildRoadDetails(writer,aero);
        if(writer.Fault!=OriginalVehicleMeshFault::None)
        {
            result.Fault=writer.Fault;
            result.Mesh={};
            return result;
        }
        if(!ValidateMesh(mesh))
        {
            result.Fault=OriginalVehicleMeshFault::InvalidMesh;
            result.Mesh={};
            return result;
        }
        if(mesh.WheelTireQuads<
                Settings.WheelRadialSegments*4 ||
           mesh.WindowQuads<6 ||
           mesh.BodyQuads<20 ||
           mesh.HeadlampQuads<2 ||
           mesh.TaillampQuads<2)
        {
            result.Fault=OriginalVehicleMeshFault::IncompleteVisuals;
            result.Mesh={};
            return result;
        }
        mesh.Ready=true;
        result.Fault=OriginalVehicleMeshFault::None;
        result.Valid=true;
        return result;
    }
    bool ValidateMesh(const OriginalVehicleMesh& mesh) const noexcept
    {
        if(mesh.Vertices.empty() || mesh.Triangles.empty() ||
           mesh.Vertices.size()>OriginalVehicleMaxVertices ||
           mesh.Triangles.size()>OriginalVehicleMaxTriangles ||
           !mesh.OriginalGeometry || !mesh.VisualOnly ||
           mesh.ChangesChaosContact ||
           mesh.VehicleId<0 || mesh.VehicleId>=6)
            return false;
        for(const auto& vertex:mesh.Vertices)
        {
            if(!MeshFinitePoint(vertex.Position) ||
               !MeshFinitePoint(vertex.Normal) ||
               !MeshFinite(vertex.TexCoord.U) ||
               !MeshFinite(vertex.TexCoord.V) ||
               std::abs(MeshLength(vertex.Normal)-1.0)>1.0e-6)
                return false;
            if(std::abs(vertex.Position.X)>Settings.LengthCm ||
               std::abs(vertex.Position.Y)>Settings.HalfWidthCm*2.0 ||
               vertex.Position.Z< -15.0 ||
               vertex.Position.Z>Settings.CabinRoofHeightCm+100.0)
                return false;
        }
        for(const auto& triangle:mesh.Triangles)
        {
            if(static_cast<std::size_t>(triangle.Material)>=
                   static_cast<std::size_t>(OriginalVehicleMaterial::Count))
                return false;
            const auto a=triangle.Index[0];
            const auto b=triangle.Index[1];
            const auto c=triangle.Index[2];
            if(a>=mesh.Vertices.size() ||
               b>=mesh.Vertices.size() ||
               c>=mesh.Vertices.size() ||
               a==b || b==c || a==c)
                return false;
            const auto side=MeshCross(
                MeshSubtract(mesh.Vertices[b].Position,
                             mesh.Vertices[a].Position),
                MeshSubtract(mesh.Vertices[c].Position,
                             mesh.Vertices[a].Position));
            if(MeshLength(side)<1.0e-6)
                return false;
        }
        return MeshFinitePoint(mesh.Bounds.Minimum) &&
            MeshFinitePoint(mesh.Bounds.Maximum) &&
            mesh.Bounds.Maximum.X>mesh.Bounds.Minimum.X &&
            mesh.Bounds.Maximum.Y>mesh.Bounds.Minimum.Y &&
            mesh.Bounds.Maximum.Z>mesh.Bounds.Minimum.Z;
    }
private:
    struct MeshWriter final
    {
        explicit MeshWriter(OriginalVehicleMesh& output) noexcept
            : Mesh(output)
        {
        }
        void Track(OriginalVehiclePoint p) noexcept
        {
            Mesh.Bounds.Minimum.X=std::min(Mesh.Bounds.Minimum.X,p.X);
            Mesh.Bounds.Minimum.Y=std::min(Mesh.Bounds.Minimum.Y,p.Y);
            Mesh.Bounds.Minimum.Z=std::min(Mesh.Bounds.Minimum.Z,p.Z);
            Mesh.Bounds.Maximum.X=std::max(Mesh.Bounds.Maximum.X,p.X);
            Mesh.Bounds.Maximum.Y=std::max(Mesh.Bounds.Maximum.Y,p.Y);
            Mesh.Bounds.Maximum.Z=std::max(Mesh.Bounds.Maximum.Z,p.Z);
        }
        bool Face(
            OriginalVehiclePoint a,OriginalVehiclePoint b,
            OriginalVehiclePoint c,
            OriginalVehicleMaterial material,
            OriginalVehicleUv uvA={0.0,0.0},
            OriginalVehicleUv uvB={1.0,0.0},
            OriginalVehicleUv uvC={0.0,1.0})
        {
            if(Fault!=OriginalVehicleMeshFault::None)
                return false;
            if(!MeshFinitePoint(a) || !MeshFinitePoint(b) ||
               !MeshFinitePoint(c) ||
               Mesh.Vertices.size()+3>OriginalVehicleMaxVertices ||
               Mesh.Triangles.size()+1>OriginalVehicleMaxTriangles)
            {
                Fault=OriginalVehicleMeshFault::CapacityExceeded;
                return false;
            }
            auto normal=MeshCross(MeshSubtract(b,a),MeshSubtract(c,a));
            const double length=MeshLength(normal);
            if(!MeshFinite(length) || length<1.0e-9)
            {
                Fault=OriginalVehicleMeshFault::DegenerateSurface;
                return false;
            }
            normal.X/=length;
            normal.Y/=length;
            normal.Z/=length;
            const auto base=static_cast<std::uint32_t>(Mesh.Vertices.size());
            Mesh.Vertices.push_back({a,normal,uvA});
            Mesh.Vertices.push_back({b,normal,uvB});
            Mesh.Vertices.push_back({c,normal,uvC});
            Mesh.Triangles.push_back({{base,base+1,base+2},material});
            Track(a);
            Track(b);
            Track(c);
            return true;
        }
        bool Quad(
            OriginalVehiclePoint a,OriginalVehiclePoint b,
            OriginalVehiclePoint c,OriginalVehiclePoint d,
            OriginalVehicleMaterial material)
        {
            if(!Face(a,b,c,material,
                     {0.0,0.0},{1.0,0.0},{1.0,1.0}))
                return false;
            return Face(a,c,d,material,
                        {0.0,0.0},{1.0,1.0},{0.0,1.0});
        }
        void Box(double left,double right,
                 double nearY,double farY,
                 double lowZ,double highZ,
                 OriginalVehicleMaterial material)
        {
            if(right<=left || farY<=nearY || highZ<=lowZ)
            {
                Fault=OriginalVehicleMeshFault::DegenerateSurface;
                return;
            }
            const OriginalVehiclePoint a{left,nearY,lowZ};
            const OriginalVehiclePoint b{right,nearY,lowZ};
            const OriginalVehiclePoint c{right,farY,lowZ};
            const OriginalVehiclePoint d{left,farY,lowZ};
            const OriginalVehiclePoint e{left,nearY,highZ};
            const OriginalVehiclePoint f{right,nearY,highZ};
            const OriginalVehiclePoint g{right,farY,highZ};
            const OriginalVehiclePoint h{left,farY,highZ};
            Quad(a,d,c,b,material);
            Quad(e,f,g,h,material);
            Quad(a,b,f,e,material);
            Quad(b,c,g,f,material);
            Quad(c,d,h,g,material);
            Quad(d,a,e,h,material);
            Mesh.DetailQuads+=6;
        }
        OriginalVehicleMesh& Mesh;
        OriginalVehicleMeshFault Fault=OriginalVehicleMeshFault::None;
    };
    struct BodyStation final
    {
        double X=0.0;
        double HalfWidth=0.0;
        double RoofHalfWidth=0.0;
        double BottomZ=0.0;
        double ShoulderZ=0.0;
        double RoofZ=0.0;
        bool Cab=false;
    };
    BodyStation Station(
        double x,const latebrake::VehicleVisualRecipe& visual) const noexcept
    {
        const double halfLength=Settings.LengthCm*0.5;
        const double absX=std::abs(x)/halfLength;
        const double taper=MeshSmoothStep((absX-0.60)/0.40);
        const double width=Settings.HalfWidthCm*(1.0-0.21*taper);
        const double roofBegin=-90.0*visual.RearDeckLengthScale;
        const double roofEnd=96.0*visual.HoodLengthScale;
        const double rearRamp=MeshSmoothStep((x-roofBegin+42.0)/50.0);
        const double frontRamp=1.0-MeshSmoothStep((x-roofEnd+20.0)/55.0);
        const double crown=rearRamp*frontRamp;
        const double archWheelX=Settings.WheelbaseCm*0.5;
        const double archRadius=Settings.WheelRadiusCm+
            Settings.WheelOpeningClearanceCm;
        const double opening=std::max(
            MeshRaisedCosine((x+archWheelX)/archRadius),
            MeshRaisedCosine((x-archWheelX)/archRadius));
        const double bottom=16.0+opening*68.0;
        const double shoulder=Settings.BodyShoulderHeightCm-
            taper*10.0;
        const double roof=shoulder+10.0+
            crown*(Settings.CabinRoofHeightCm-shoulder-10.0);
        return {
            x,width,
            width*std::clamp(
                0.72*visual.RoofWidthScale,0.58,0.85),
            bottom,shoulder,roof,crown>0.25
        };
    }
    static std::array<OriginalVehiclePoint,8> Ring(
        const BodyStation& p) noexcept
    {
        const double shoulderY=p.HalfWidth*0.94;
        const double bottomY=p.HalfWidth*0.77;
        return {{
            {p.X,-bottomY,p.BottomZ},
            {p.X,-p.HalfWidth,p.BottomZ+13.0},
            {p.X,-shoulderY,p.ShoulderZ},
            {p.X,-p.RoofHalfWidth,p.RoofZ},
            {p.X,+p.RoofHalfWidth,p.RoofZ},
            {p.X,+shoulderY,p.ShoulderZ},
            {p.X,+p.HalfWidth,p.BottomZ+13.0},
            {p.X,+bottomY,p.BottomZ}
        }};
    }
    void BuildMainBody(
        MeshWriter& writer,
        const latebrake::VehicleVisualRecipe& visual,
        int vehicleId) const
    {
        const double halfLength=Settings.LengthCm*0.5;
        auto old=Ring(Station(-halfLength,visual));
        for(int index=1;index<=Settings.LongitudinalSegments;++index)
        {
            const double x=-halfLength+
                Settings.LengthCm*
                static_cast<double>(index)/
                static_cast<double>(Settings.LongitudinalSegments);
            const auto station=Station(x,visual);
            const auto now=Ring(station);
            const double middle=(x+old[0].X)*0.5;
            const bool windshield=
                middle>70.0 && middle<128.0;
            const bool sideGlass=
                middle>-82.0 && middle<92.0 &&
                station.Cab;
            for(std::size_t strip=0;strip+1<old.size();++strip)
            {
                auto material=OriginalVehicleMaterial::BodyPaint;
                if((strip==2 || strip==4) &&
                   (sideGlass || windshield))
                    material=OriginalVehicleMaterial::ClearGlass;
                if(strip==0 || strip==6)
                    material=OriginalVehicleMaterial::Accent;
                if(strip==3 && middle>-85.0 && middle<80.0)
                    material=OriginalVehicleMaterial::BodyPaint;
                if(writer.Quad(old[strip],now[strip],
                               now[strip+1],old[strip+1],
                               material))
                {
                    ++writer.Mesh.BodyQuads;
                    if(material==OriginalVehicleMaterial::ClearGlass)
                        ++writer.Mesh.WindowQuads;
                }
            }
            old=now;
        }
        const auto rear=Ring(Station(-halfLength,visual));
        const auto front=Ring(Station(halfLength,visual));
        const OriginalVehiclePoint rearCenter{
            -halfLength,0.0,Settings.BodyShoulderHeightCm*0.55};
        const OriginalVehiclePoint frontCenter{
            halfLength,0.0,Settings.BodyShoulderHeightCm*0.55};
        for(std::size_t side=0;side+1<front.size();++side)
        {
            writer.Face(rearCenter,rear[side+1],rear[side],
                        OriginalVehicleMaterial::BodyPaint);
            writer.Face(frontCenter,front[side],front[side+1],
                        OriginalVehicleMaterial::BodyPaint);
        }
        writer.Face(rearCenter,rear[0],rear[7],
                    OriginalVehicleMaterial::BodyPaint);
        writer.Face(frontCenter,front[7],front[0],
                    OriginalVehicleMaterial::BodyPaint);
        if(vehicleId==3 || vehicleId==5)
        {
            const double halfWidth=Settings.HalfWidthCm*0.64;
            writer.Box(55.0,78.0,-halfWidth,halfWidth,
                       Settings.BodyShoulderHeightCm+4.0,
                       Settings.BodyShoulderHeightCm+8.0,
                       OriginalVehicleMaterial::CarbonTrim);
        }
    }
    void BuildWheel(
        MeshWriter& writer,double wheelX,int side) const
    {
        const double centerY=static_cast<double>(side)*
            Settings.HalfWidthCm;
        const double halfTire=Settings.TireWidthCm*0.5;
        const double outerY=centerY+static_cast<double>(side)*halfTire;
        const double innerY=centerY-static_cast<double>(side)*halfTire;
        const double rimY=outerY+
            static_cast<double>(side)*0.8;
        const double z=Settings.WheelCenterHeightCm;
        const double radius=Settings.WheelRadiusCm;
        const double inner=Settings.RimRadiusCm;
        for(int index=0;index<Settings.WheelRadialSegments;++index)
        {
            const double theta=2.0*OriginalVehiclePi*
                static_cast<double>(index)/
                static_cast<double>(Settings.WheelRadialSegments);
            const double next=2.0*OriginalVehiclePi*
                static_cast<double>(index+1)/
                static_cast<double>(Settings.WheelRadialSegments);
            const auto polar=[wheelX,z](double r,double y,double angle)
            {
                return OriginalVehiclePoint{
                    wheelX+r*std::cos(angle),
                    y,
                    z+r*std::sin(angle)};
            };
            writer.Quad(
                polar(radius,innerY,theta),
                polar(radius,outerY,theta),
                polar(radius,outerY,next),
                polar(radius,innerY,next),
                OriginalVehicleMaterial::Rubber);
            writer.Quad(
                polar(radius,outerY,theta),
                polar(inner,outerY,theta),
                polar(inner,outerY,next),
                polar(radius,outerY,next),
                OriginalVehicleMaterial::Rubber);
            writer.Quad(
                polar(radius,innerY,next),
                polar(inner,innerY,next),
                polar(inner,innerY,theta),
                polar(radius,innerY,theta),
                OriginalVehicleMaterial::Rubber);
            const double insetR=inner*0.83;
            writer.Quad(
                polar(inner,rimY,theta),
                polar(insetR,rimY,theta),
                polar(insetR,rimY,next),
                polar(inner,rimY,next),
                OriginalVehicleMaterial::Alloy);
            const double rotorR=inner*0.61;
            writer.Face(
                polar(0.0,rimY,0.0),
                polar(rotorR,rimY,theta),
                polar(rotorR,rimY,next),
                OriginalVehicleMaterial::BrakeRotor);
            writer.Mesh.WheelTireQuads+=3;
        }
        const int spokeCount=7;
        for(int spoke=0;spoke<spokeCount;++spoke)
        {
            const double angle=2.0*OriginalVehiclePi*
                static_cast<double>(spoke)/
                static_cast<double>(spokeCount);
            const double width=0.09;
            const auto spokePoint=[wheelX,z,rimY](
                double radius,double radians)
            {
                return OriginalVehiclePoint{
                    wheelX+radius*std::cos(radians),
                    rimY+0.3,
                    z+radius*std::sin(radians)};
            };
            writer.Quad(
                spokePoint(inner*0.16,angle-width),
                spokePoint(inner*0.91,angle-width*0.25),
                spokePoint(inner*0.91,angle+width*0.25),
                spokePoint(inner*0.16,angle+width),
                OriginalVehicleMaterial::Alloy);
        }
    }
    void BuildFourWheels(MeshWriter& writer) const
    {
        const double axle=Settings.WheelbaseCm*0.5;
        for(int side:{-1,1})
        {
            BuildWheel(writer,-axle,side);
            BuildWheel(writer,+axle,side);
        }
    }
    void BuildLights(MeshWriter& writer) const
    {
        const double half=Settings.LengthCm*0.5;
        for(int side:{-1,1})
        {
            const double sideY=static_cast<double>(side);
            const double yA=sideY*Settings.HalfWidthCm*0.49;
            const double yB=sideY*Settings.HalfWidthCm*0.83;
            const double front=half+0.6;
            if(writer.Quad(
                {front,yA,72.0},{front,yB,76.0},
                {front,yB,94.0},{front,yA,98.0},
                OriginalVehicleMaterial::Headlamp))
                ++writer.Mesh.HeadlampQuads;
            const double rear=-half-0.6;
            if(writer.Quad(
                {rear,yB,72.0},{rear,yA,72.0},
                {rear,yA,99.0},{rear,yB,99.0},
                OriginalVehicleMaterial::Taillamp))
                ++writer.Mesh.TaillampQuads;
            writer.Box(
                half-8.0,half-2.0,
                std::min(yA,yB),std::max(yA,yB),
                67.0,71.0,OriginalVehicleMaterial::Accent);
            writer.Box(
                -half+2.0,-half+9.0,
                std::min(yA,yB),std::max(yA,yB),
                65.0,70.0,OriginalVehicleMaterial::Accent);
        }
    }
    void BuildWindows(
        MeshWriter& writer,
        const latebrake::VehicleVisualRecipe& visual) const
    {
        const auto atBack=Station(-72.0,visual);
        const auto atFront=Station(88.0,visual);
        const double topBack=atBack.RoofZ+1.2;
        const double topFront=atFront.RoofZ+1.2;
        // The separately glazed roof strip and angled rear/forward
        // windshields are visually distinct from side windows.
        if(writer.Quad(
            {-72.0,-atBack.RoofHalfWidth,topBack},
            {88.0,-atFront.RoofHalfWidth,topFront},
            {88.0,atFront.RoofHalfWidth,topFront},
            {-72.0,atBack.RoofHalfWidth,topBack},
            OriginalVehicleMaterial::ClearGlass))
            ++writer.Mesh.WindowQuads;
        for(int side:{-1,1})
        {
            const double s=static_cast<double>(side);
            if(writer.Quad(
                {-98.0,s*Settings.HalfWidthCm*0.84,
                         Settings.BodyShoulderHeightCm+2.0},
                {-72.0,s*atBack.RoofHalfWidth,topBack},
                {-53.0,s*atBack.RoofHalfWidth,topBack},
                {-68.0,s*Settings.HalfWidthCm*0.85,
                         Settings.BodyShoulderHeightCm+2.0},
                OriginalVehicleMaterial::ClearGlass))
                ++writer.Mesh.WindowQuads;
            if(writer.Quad(
                {82.0,s*atFront.RoofHalfWidth,topFront},
                {102.0,s*Settings.HalfWidthCm*0.87,
                        Settings.BodyShoulderHeightCm+1.0},
                {118.0,s*Settings.HalfWidthCm*0.80,
                         Settings.BodyShoulderHeightCm+2.0},
                {103.0,s*atFront.RoofHalfWidth,topFront},
                OriginalVehicleMaterial::ClearGlass))
                ++writer.Mesh.WindowQuads;
        }
        writer.Box(-9.0,12.0,
                   -Settings.HalfWidthCm*0.86,
                   Settings.HalfWidthCm*0.86,
                   Settings.BodyShoulderHeightCm+0.8,
                   Settings.BodyShoulderHeightCm+1.8,
                   OriginalVehicleMaterial::CarbonTrim);
    }
    void BuildAero(
        MeshWriter& writer,
        const latebrake::VehicleVisualRecipe& visual,
        const latebrake::VehicleBodyKitRecipe& aero) const
    {
        const double halfLength=Settings.LengthCm*0.5;
        const double halfWidth=Settings.HalfWidthCm;
        writer.Box(
            halfLength-27.0,halfLength+10.0,
            -halfWidth*0.85*aero.SplitterWidthScale,
            halfWidth*0.85*aero.SplitterWidthScale,
            18.0,21.0,OriginalVehicleMaterial::CarbonTrim);
        writer.Box(
            -halfLength-4.0,-halfLength+35.0,
            -halfWidth*0.76*aero.DiffuserWidthScale,
            halfWidth*0.76*aero.DiffuserWidthScale,
            18.0,24.0,OriginalVehicleMaterial::CarbonTrim);
        const double skirtSpan=Settings.WheelbaseCm*
            0.43*aero.SideSkirtLengthScale;
        for(int side:{-1,1})
        {
            const double y=static_cast<double>(side)*
                (halfWidth-0.5);
            const double outer=static_cast<double>(side)*
                (halfWidth+7.5);
            writer.Box(
                -skirtSpan,+skirtSpan,
                std::min(y,outer),std::max(y,outer),
                19.0,30.0,OriginalVehicleMaterial::CarbonTrim);
        }
        if(visual.HasRearWing)
        {
            const double spread=std::clamp(
                80.0*visual.WingSpanScale,50.0,112.0);
            const double wingHeight=visual.WingHeightCm+17.0;
            const double rearX=-halfLength+36.0;
            writer.Box(rearX-11.0,rearX+18.0,
                       -spread,spread,
                       wingHeight,wingHeight+7.0,
                       OriginalVehicleMaterial::CarbonTrim);
            for(int side:{-1,1})
            {
                const double y=static_cast<double>(side)*spread*0.72;
                writer.Box(
                    rearX+1.0,rearX+8.0,
                    std::min(y-3.0,y+3.0),
                    std::max(y-3.0,y+3.0),
                    90.0,wingHeight,
                    OriginalVehicleMaterial::CarbonTrim);
            }
        }
        if(aero.HasHoodScoop)
        {
            writer.Box(44.0,108.0,-17.0,17.0,
                       Settings.BodyShoulderHeightCm+11.0,
                       Settings.BodyShoulderHeightCm+13.0+
                           aero.ScoopHeightCm,
                       OriginalVehicleMaterial::Intake);
        }
        if(aero.HasRearFins)
        {
            for(int side:{-1,1})
            {
                const double y=static_cast<double>(side)*
                    Settings.HalfWidthCm*0.78;
                writer.Box(
                    -halfLength+37.0,-halfLength+75.0,
                    y-2.0,y+2.0,
                    Settings.BodyShoulderHeightCm-5.0,
                    Settings.BodyShoulderHeightCm+
                        aero.RearFinHeightCm,
                    OriginalVehicleMaterial::CarbonTrim);
            }
        }
    }
    void BuildRoadDetails(
        MeshWriter& writer,
        const latebrake::VehicleBodyKitRecipe& aero) const
    {
        const double width=Settings.HalfWidthCm;
        const double rear=-Settings.LengthCm*0.5;
        for(int side:{-1,1})
        {
            const double s=static_cast<double>(side);
            const double mirrorY=s*(width+11.0);
            writer.Box(
                57.0,77.0,mirrorY-9.0,mirrorY+9.0,
                109.0,120.0,
                OriginalVehicleMaterial::CarbonTrim);
            writer.Box(
                76.0,79.0,mirrorY-7.0,mirrorY+7.0,
                111.0,117.0,
                OriginalVehicleMaterial::ClearGlass);
            const double tailY=s*width*0.51;
            writer.Box(
                rear-7.0,rear+6.0,
                tailY-12.0,tailY+12.0,
                25.0,37.0,
                OriginalVehicleMaterial::CarbonTrim);
            writer.Box(
                rear-8.0,rear+4.0,
                tailY-7.0,tailY+7.0,
                28.0,34.0,
                OriginalVehicleMaterial::Accent);
        }
        if(aero.HasHoodScoop)
        {
            writer.Box(107.0,116.0,-11.0,11.0,
                       Settings.BodyShoulderHeightCm+14.0,
                       Settings.BodyShoulderHeightCm+20.0,
                       OriginalVehicleMaterial::Intake);
        }
    }
    OriginalVehicleMeshSettings Settings{};
};
} // namespace latebrake::vehiclemesh

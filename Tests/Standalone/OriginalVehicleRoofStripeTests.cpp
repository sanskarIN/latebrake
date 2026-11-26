#include "Vehicles/OriginalVehicleRoofStripe.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
using namespace latebrake;
using namespace latebrake::vehiclemesh;
void Check(bool ok,const char* what)
{
    if(!ok)
    {
        std::cerr<<what<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a,double b,double tolerance=1e-8)
{return std::abs(a-b)<=tolerance;}

void ExactMeshRoofAgreement()
{
    for(int id=0;id<VehicleCount;++id)
        for(int segments:{20,32,48,64})
        {
            OriginalVehicleMeshSettings settings;
            settings.LongitudinalSegments=segments;
            const OriginalVehicleSurfaceComposer composer(settings);
            const auto body=composer.Build(id);
            Check(body.Valid,"body source remains valid");
            const auto stripe=BuildOriginalRoofStripePlan(id,settings);
            Check(stripe.Valid&&stripe.VehicleId==id,"valid three-band plan");
            for(int k=0;k<=segments;++k)
            {
                const double x=-settings.LengthCm*0.5+
                    settings.LengthCm*static_cast<double>(k)/segments;
                const auto sample=composer.SampleRoof(id,x);
                Check(sample.Valid&&Near(sample.X,x),"valid exact grid sample");
                // At each original station, the top-strip ring contains
                // both roof edges. Check against authored vertices, not a
                // second implementation of the same height formula.
                bool left=false,right=false;
                for(const auto& v:body.Mesh.Vertices)
                {
                    const auto p=v.Position;
                    if(Near(p.X,x)&&Near(p.Z,sample.Z))
                    {
                        if(Near(p.Y,-sample.HalfWidth))left=true;
                        if(Near(p.Y,sample.HalfWidth))right=true;
                    }
                }
                Check(left&&right,"sample matches actual emitted roof ring vertices");
            }
            for(std::size_t band=0;band<stripe.Bands.size();++band)
            {
                const auto& b=stripe.Bands[band];
                Check(b.Count>0&&b.Count<=OriginalRoofStripeMaxQuads,
                    "bounded band geometry");
                Check(Near(b.Quads[0].Points[0].X,-70.0)&&
                    Near(b.Quads[b.Count-1].Points[1].X,70.0),"complete central roof coverage");
                for(std::size_t j=0;j<b.Count;++j)
                {
                    const auto& q=b.Quads[j];
                    const auto& p=q.Points;
                    const double y=OriginalRoofStripeOffsets[band];
                    Check(p[1].X>p[0].X&&Near(p[0].X,p[3].X)&&Near(p[1].X,p[2].X),
                        "positive stripe extent");
                    Check(Near(p[0].Y,y-OriginalRoofStripeHalfWidthCm)&&
                        Near(p[3].Y,y+OriginalRoofStripeHalfWidthCm),"exact stripe band width");
                    for(const auto& point:p)
                    {
                        const auto roof=composer.SampleRoof(id,point.X);
                        Check(roof.Valid&&Near(point.Z-roof.Z,OriginalRoofStripeClearanceCm)&&
                            std::abs(point.Y)<roof.HalfWidth,"corners follow actual surface at bounded clearance");
                    }
                    for(int t=1;t<=9;++t)
                    {
                        const double u=t/10.0;
                        const double x=p[0].X+(p[1].X-p[0].X)*u;
                        const double z=p[0].Z+(p[1].Z-p[0].Z)*u;
                        Check(Near(z-composer.SampleRoof(id,x).Z,OriginalRoofStripeClearanceCm),
                            "complete segment follows mesh, not just corners");
                    }
                    const auto cross=MeshCross(MeshSubtract(p[1],p[0]),MeshSubtract(p[2],p[0]));
                    const double length=MeshLength(cross);
                    Check(length>0.0&&Near(MeshLength(q.Normal),1.0)&&
                        Near(MeshLength(q.Tangent),1.0),"nondegenerate unit shading frame");
                    Check(Near(q.Normal.X,cross.X/length)&&Near(q.Normal.Y,cross.Y/length)&&
                        Near(q.Normal.Z,cross.Z/length)&&q.Normal.Z>0.0,"up-facing winding matches normal");
                    Check(Near(q.Normal.X*q.Tangent.X+q.Normal.Z*q.Tangent.Z,0.0),
                        "tangent is orthogonal to normal");
                    if(j>0)
                    {
                        const auto& previous=b.Quads[j-1].Points;
                        Check(Near(previous[1].X,p[0].X)&&Near(previous[1].Z,p[0].Z)&&
                            Near(previous[2].X,p[3].X)&&Near(previous[2].Z,p[3].Z),
                            "adjacent bands meet with no gap");
                    }
                }
            }
        }
}
void VisibilityAndInvalidInputs()
{
    for(int choice=-1;choice<=3;++choice)
    {
        int visible=0;
        for(std::size_t band=0;band<OriginalRoofStripeBandCount;++band)
            if(OriginalRoofStripeVisible(choice,band))++visible;
        Check(visible==(choice==1?1:choice==2?2:0),"clean center twin visibility contract");
        Check(!OriginalRoofStripeVisible(choice,OriginalRoofStripeBandCount),
            "invalid band never visible");
    }
    const OriginalVehicleSurfaceComposer composer;
    const double nan=std::numeric_limits<double>::quiet_NaN();
    const double inf=std::numeric_limits<double>::infinity();
    for(double x:{-223.0,223.0,nan,inf,-inf})
        Check(!composer.SampleRoof(0,x).Valid,"invalid x rejected");
    for(int id:{-1,VehicleCount,std::numeric_limits<int>::min(),std::numeric_limits<int>::max()})
    {
        Check(!composer.SampleRoof(id,0).Valid,"invalid sample vehicle");
        Check(!BuildOriginalRoofStripePlan(id).Valid,"invalid plan vehicle");
    }
    OriginalVehicleMeshSettings malformed;
    malformed.LongitudinalSegments=0;
    Check(!BuildOriginalRoofStripePlan(0,malformed).Valid,"invalid segment count rejected");
    malformed={};
    malformed.CabinRoofHeightCm=nan;
    Check(!BuildOriginalRoofStripePlan(0,malformed).Valid,"invalid roof height rejected");
    malformed={};
    malformed.NeverChangeCollision=false;
    Check(!BuildOriginalRoofStripePlan(0,malformed).Valid,"invalid collision policy rejected");
}
}
int main()
{
    ExactMeshRoofAgreement();
    VisibilityAndInvalidInputs();
    std::cout<<"Original vehicle conforming stripe geometry passed.\n";
    return EXIT_SUCCESS;
}

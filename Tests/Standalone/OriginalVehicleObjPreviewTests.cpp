#include "Vehicles/OriginalVehicleObjPreview.h"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace
{
void Check(bool yes,const char* why)
{
    if(!yes){std::cerr<<"OBJ source preview QA: "<<why<<'\n';
        std::exit(EXIT_FAILURE);}
}
std::size_t CountLines(std::string_view text,std::string_view prefix)
{
    std::size_t count=0;
    std::size_t start=0;
    while(start<text.size())
    {
        const auto end=text.find('\n',start);
        const auto line=text.substr(start,
            end==std::string_view::npos ? text.size()-start : end-start);
        if(line.substr(0,prefix.size())==prefix)++count;
        if(end==std::string_view::npos)break;
        start=end+1;
    }
    return count;
}
}
int main()
{
    using namespace latebrake;
    using namespace latebrake::vehiclemesh;
    Check(ValidObjMtlBasename("OriginalCar_3_Rim_2.mtl") &&
          !ValidObjMtlBasename("../materials.mtl") &&
          !ValidObjMtlBasename("models/fake.mtl") &&
          !ValidObjMtlBasename("file\nusemtl fake.mtl") &&
          !ValidObjMtlBasename("..") &&
          !ValidObjMtlBasename("") &&
          !ValidObjMtlBasename(std::string(81,'a')),
          "OBJ material filename is bounded, flat and safe");
    unsigned complete=0;
    for(int car=0;car<VehicleCount;++car)
        for(int rim=0;rim<GarageWheelOptions;++rim)
        {
            const auto built=OriginalVehicleSurfaceComposer{}.Build(car,rim);
            Check(built.Valid && built.Mesh.Ready,
                "use actual original six-car Unreal source geometry");
            std::ostringstream obj,mtl;
            const auto look=FactoryGarageLook(car);
            const GarageLook variant{look.Paint,0u,
                static_cast<std::uint8_t>(rim)};
            Check(WriteOriginalVehicleObj(obj,built.Mesh,"car.mtl") &&
                  WriteOriginalVehicleMtl(mtl,car,variant),
                "all original car/rim geometry and paints export");
            const auto o=obj.str();
            const auto m=mtl.str();
            Check(o.find("mtllib car.mtl\n")!=std::string::npos &&
                  o.find("Y-up, metres")!=std::string::npos &&
                  o.find("NOT a cooked Unreal game")!=std::string::npos,
                "file documents material and source-only limitations");
            Check(CountLines(o,"v ")==built.Mesh.Vertices.size() &&
                  CountLines(o,"vt ")==built.Mesh.Vertices.size() &&
                  CountLines(o,"vn ")==built.Mesh.Vertices.size() &&
                  CountLines(o,"f ")==built.Mesh.Triangles.size(),
                "real vertex UV normal and face counts survive OBJ conversion");
            for(const char* group:OriginalPreviewParts)
                Check(o.find(std::string("g ")+group+"\n")!=std::string::npos,
                    "OBJ retains body and four independently authored wheels");
            for(const char* material:OriginalPreviewMaterials)
            {
                Check(o.find(std::string("usemtl ")+material+"\n")!=
                        std::string::npos,
                    "actual source car uses each original material group");
                Check(m.find(std::string("newmtl ")+material+"\n")!=
                        std::string::npos,
                    "MTL provides each original color surface");
            }
            Check(CountLines(m,"newmtl ")==OriginalVehicleSectionCount,
                "10 material definitions, no duplicate or fake identity");
            std::ostringstream repeated;
            Check(WriteOriginalVehicleObj(repeated,built.Mesh,"car.mtl") &&
                  repeated.str()==o,
                "stable source geometry always generates deterministic OBJ");
            ++complete;
        }
    auto invalid=OriginalVehicleSurfaceComposer{}.Build(1).Mesh;
    invalid.Triangles[0].Index[0]=
        static_cast<std::uint32_t>(invalid.Vertices.size()+15u);
    std::ostringstream refused;
    Check(!WriteOriginalVehicleObj(refused,invalid,"good.mtl") &&
          refused.str().empty(),
        "invalid/unsafe geometry cannot write a partial misleading preview");
    Check(!WriteOriginalVehicleMtl(
            refused,4,GarageLook{255u,2u,0u}),
          "corrupt cosmetic choices cannot create material output");
    Check(complete==18u,"six cars x three real rim shapes exported");
    std::cout<<"18 original car/rim OBJ and MTL pairs validated.\n";
    return EXIT_SUCCESS;
}

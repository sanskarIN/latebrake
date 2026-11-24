#include "Vehicles/OriginalVehicleObjPreview.h"
#include "Garage/GarageAppearance.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

int main(int argc,char** argv)
{
    if(argc!=2)
    {
        std::cerr<<"Usage: latebrake-original-obj-preview OUTPUT_DIRECTORY\n"
                 <<"Exports 18 original car/rim meshes. NOT a runnable Unreal game.\n";
        return 2;
    }
    const std::filesystem::path output(argv[1]);
    std::error_code ec;
    std::filesystem::create_directories(output,ec);
    if(ec)
    {
        std::cerr<<"Cannot create requested output directory: "<<ec.message()<<'\n';
        return 2;
    }
    int written=0;
    for(int id=0;id<latebrake::VehicleCount;++id)
    {
        const auto look=latebrake::FactoryGarageLook(id);
        for(int rim=0;rim<latebrake::GarageWheelOptions;++rim)
        {
            const auto generated=
                latebrake::vehiclemesh::OriginalVehicleSurfaceComposer{}.Build(id,rim);
            if(!generated.Valid)
            {
                std::cerr<<"Source mesh rejected car "<<id<<" rim "<<rim<<'\n';
                return 3;
            }
            const auto stem="LateBrakeOriginalCar_"+std::to_string(id)+
                "_Rim_"+std::to_string(rim);
            const auto obj=output/(stem+".obj");
            const auto mtl=output/(stem+".mtl");
            std::ofstream meshFile(obj,std::ios::binary|std::ios::trunc);
            std::ofstream materialFile(mtl,std::ios::binary|std::ios::trunc);
            if(!meshFile || !materialFile ||
                !latebrake::vehiclemesh::WriteOriginalVehicleObj(
                    meshFile,generated.Mesh,stem+".mtl") ||
                !latebrake::vehiclemesh::WriteOriginalVehicleMtl(
                    materialFile,id,{look.Paint,look.Stripe,
                        static_cast<std::uint8_t>(rim)}))
            {
                std::cerr<<"Cannot export original source car "<<id
                         <<" with rim "<<rim<<'\n';
                return 4;
            }
            ++written;
        }
    }
    if(written!=18) return 5;
    std::cout<<"Exported "<<written
        <<" original car/rim pairs as OBJ + MTL. No game or UE shader claim.\n";
    return 0;
}

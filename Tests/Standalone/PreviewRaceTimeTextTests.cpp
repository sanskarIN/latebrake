#include "PreviewRaceTimeText.h"
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <limits>

namespace
{
void Check(bool x,const char* why)
{
    if(!x){std::cerr<<"2D timing UI failure: "<<why<<'\n';
           std::exit(EXIT_FAILURE);}
}
void CheckTime(float value,bool available,const char* expected)
{
    char text[16]={};
    const bool valid=latebrake_preview::FormatPreviewRaceSeconds(
        value,available,text);
    Check(valid==available && std::strcmp(text,expected)==0,
          "race centisecond presentation and missing samples");
}
void Examples()
{
    CheckTime(0.0f,true,"0.00");
    CheckTime(0.019f,true,"0.02");
    CheckTime(1.234f,true,"1.23");
    CheckTime(65.99f,true,"65.99");
    CheckTime(123.45f,true,"123.45");
    CheckTime(0.0f,false,"--.--");
    char out[16]={};
    Check(latebrake_preview::FormatPreviewRecordMilliseconds(42500,out) &&
          std::strcmp(out,"42.50")==0,
          "stored 42.500s track best shows hundredths");
    Check(latebrake_preview::FormatPreviewRecordMilliseconds(65555,out) &&
          std::strcmp(out,"65.56")==0,
          "record millisecond -> hundredth rounding is deterministic");
    Check(!latebrake_preview::FormatPreviewRecordMilliseconds(0,out) &&
          std::strcmp(out,"--.--")==0,"unset PB is not shown as zero");
    Check(!latebrake_preview::FormatPreviewRaceSeconds(
              std::numeric_limits<float>::quiet_NaN(),true,out) &&
          std::strcmp(out,"--.--")==0,"NaN cannot appear in HUD");
    Check(!latebrake_preview::FormatPreviewRaceSeconds(
              std::numeric_limits<float>::infinity(),true,out),
          "infinite telemetry rejected");
    Check(!latebrake_preview::FormatPreviewRaceSeconds(-1.0f,true,out),
          "negative telemetry rejected");
}
void Stress()
{
    unsigned int seed=0xf319287u;
    for(int n=0;n<100000;++n)
    {
        seed=1664525u*seed+1013904223u;
        const unsigned int cs=seed%99999999u;
        char out[16]={};
        Check(latebrake_preview::FormatPreviewCentiseconds(cs,out),
              "bounded timing string is always representable");
        int dotIndex=0;
        while(out[dotIndex] && out[dotIndex]!='.')dotIndex++;
        Check(dotIndex>=1 && dotIndex<=6 &&
              out[dotIndex]=='.' &&
              out[dotIndex+1]>='0' && out[dotIndex+1]<='9' &&
              out[dotIndex+2]>='0' && out[dotIndex+2]<='9' &&
              out[dotIndex+3]==0,"100k strings contain exact .SS");
    }
}
}
int main()
{
    Examples();Stress();
    std::cout<<"Preview race timing presentation passed.\n";
    return EXIT_SUCCESS;
}

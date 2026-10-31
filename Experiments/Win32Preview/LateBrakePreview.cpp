// LateBrake standalone Win64 top-down gameplay preview.
// This is deliberately NOT the Unreal Engine Latebrake target or cooked content.
// Self-contained Win32/GDI progression prototype: no network, CRT, plugins, or Unreal assets.
// Build and launch on Windows; --self-test runs an isolated deterministic logic check.

extern "C" { int _fltused = 0; } // MSVC floating-point linkage marker.
#if defined(_MSC_VER) && !defined(__clang__)
#pragma function(memset)
#pragma function(memcpy)
#endif

// Freestanding compiler-generated zero initialization support.
extern "C" void* memset(void* dest,int value,unsigned long long size) {
    unsigned char* p=(unsigned char*)dest;
    for(unsigned long long i=0;i<size;i++)p[i]=(unsigned char)value;
    return dest;
}
extern "C" void* memcpy(void* dest,const void* src,unsigned long long size) {
    unsigned char* d=(unsigned char*)dest;const unsigned char* p=(const unsigned char*)src;
    for(unsigned long long i=0;i<size;i++)d[i]=p[i];return dest;
}
typedef void* HWND;
typedef void* HINSTANCE;
typedef void* HICON;
typedef void* HCURSOR;
typedef void* HBRUSH;
typedef void* HDC;
typedef void* HANDLE;
typedef unsigned int UINT;
typedef unsigned long DWORD;
typedef unsigned long long WPARAM;
typedef long long LPARAM;
typedef long long LRESULT;
typedef int BOOL;
struct POINT { long x, y; };
struct RECT { long left,top,right,bottom; };
struct MSG { HWND hwnd; UINT message; WPARAM wParam; LPARAM lParam; DWORD time; POINT pt; DWORD privateData; };
typedef LRESULT (__stdcall *WNDPROC)(HWND,UINT,WPARAM,LPARAM);
struct WNDCLASSA { UINT style; WNDPROC proc; int clsExtra,wndExtra; HINSTANCE instance; HICON icon; HCURSOR cursor; HBRUSH background; const char* menu; const char* name; };
struct BITMAPINFOHEADER { DWORD size; long width,height; unsigned short planes,bits; DWORD compression,imageSize; long xppm,yppm; DWORD used,important; };
struct BITMAPINFO { BITMAPINFOHEADER header; DWORD colors[3]; };

extern "C" {
__declspec(dllimport) HINSTANCE __stdcall GetModuleHandleA(const char*);
__declspec(dllimport) void __stdcall ExitProcess(UINT);
__declspec(dllimport) BOOL __stdcall QueryPerformanceCounter(long long*);
__declspec(dllimport) BOOL __stdcall QueryPerformanceFrequency(long long*);
__declspec(dllimport) void __stdcall Sleep(DWORD);
__declspec(dllimport) char* __stdcall GetCommandLineA();
__declspec(dllimport) DWORD __stdcall GetEnvironmentVariableA(const char*,char*,DWORD);
__declspec(dllimport) BOOL __stdcall CreateDirectoryA(const char*,void*);
__declspec(dllimport) HANDLE __stdcall CreateFileA(const char*,DWORD,DWORD,void*,DWORD,DWORD,HANDLE);
__declspec(dllimport) BOOL __stdcall ReadFile(HANDLE,void*,DWORD,DWORD*,void*);
__declspec(dllimport) BOOL __stdcall WriteFile(HANDLE,const void*,DWORD,DWORD*,void*);
__declspec(dllimport) BOOL __stdcall CloseHandle(HANDLE);
__declspec(dllimport) BOOL __stdcall MoveFileExA(const char*,const char*,DWORD);
__declspec(dllimport) unsigned short __stdcall RegisterClassA(const WNDCLASSA*);
__declspec(dllimport) HWND __stdcall CreateWindowExA(DWORD,const char*,const char*,DWORD,int,int,int,int,HWND,void*,HINSTANCE,void*);
__declspec(dllimport) BOOL __stdcall ShowWindow(HWND,int);
__declspec(dllimport) BOOL __stdcall UpdateWindow(HWND);
__declspec(dllimport) BOOL __stdcall PeekMessageA(MSG*,HWND,UINT,UINT,UINT);
__declspec(dllimport) BOOL __stdcall TranslateMessage(const MSG*);
__declspec(dllimport) LRESULT __stdcall DispatchMessageA(const MSG*);
__declspec(dllimport) LRESULT __stdcall DefWindowProcA(HWND,UINT,WPARAM,LPARAM);
__declspec(dllimport) void __stdcall PostQuitMessage(int);
__declspec(dllimport) short __stdcall GetAsyncKeyState(int);
__declspec(dllimport) HDC __stdcall GetDC(HWND);
__declspec(dllimport) int __stdcall ReleaseDC(HWND,HDC);
__declspec(dllimport) BOOL __stdcall GetClientRect(HWND,RECT*);
__declspec(dllimport) int __stdcall StretchDIBits(HDC,int,int,int,int,int,int,int,int,const void*,const BITMAPINFO*,UINT,DWORD);
}

static const int WIDTH=960, HEIGHT=640;
static const float CX=480.0f, CY=331.0f;
static unsigned int backdrop[WIDTH*HEIGHT], frame[WIDTH*HEIGHT];
static bool running=true, paused=false, lastPause=false, lastRestart=false;
static HWND windowHandle=0;
static float gameTime=0.0f, gateFlash=0.0f, toastTime=0.0f;
static int totalLaps=3;
static int homeFocus=0, mouseX=-1,mouseY=-1;
static bool showHelp=false, highContrast=false, isHovering=false;
static float countdown=0, countdownGo=0;
static unsigned int seed=0x6a09e667u;

// Preview progression is intentionally local-only; online lobbies are planned,
// NOT available, and there are no socket imports or network dependencies.
enum Page { HOME, GARAGE, TRACKS, CHALLENGES, RACING, RESULTS, ONLINE_INFO };
static Page page=HOME;
static int selectedCar=0,selectedTrack=0,selectedChallenge=0,credits=0,completed=0;
static int ownedMask=1,careerStars=0,bonusCredits=0;
static float trackRx=291.0f,trackRy=161.0f,trackWidth=0.235f;
static bool lastEnter=false,lastBack=false,lastGarage=false,lastTracks=false,lastChallenge=false,lastOnline=false,lastMenu=false;
static bool lastLeft=false,lastRight=false,lastUp=false,lastDown=false,lastUnlock=false;
static bool lastHelp=false,lastContrast=false;
static bool testing=false;
static char toast[72]="";
static const char* carNames[6]={"COMET GT","DUNE BUGGY","NIGHTFALL","VOLT RALLY","APEX V12","PHANTOM R"};
static const int carPrices[6]={0,140,280,450,750,1100};
static const unsigned int carPaints[6]={0xF65745,0xF0C54D,0x44D8CE,0xB67FFF,0xF0F0F2,0xFD70B3};
static const float carAccel[6]={1.00f,0.89f,1.13f,0.99f,1.24f,1.32f};
static const float carSpeed[6]={1.00f,0.88f,1.05f,1.12f,1.18f,1.28f};
static const float carGrip[6]={1.00f,1.24f,1.01f,1.18f,0.91f,1.11f};
static const char* trackNames[4]={"GREEN LOOP","CANYON RUN","COASTAL CURVE","NEON NIGHT"};
static const char* challengeNames[4]={"CIRCUIT RACE","TIME ATTACK","RIVAL DUEL","CHECKPOINT RUSH"};
static const char* challengeSub[4]={"FINISH 3 LAPS","BEAT 65 SEC","BEAT RIVAL IN 2 LAPS","2 LAPS UNDER 95 SEC"};
static const int trackReward[4]={75,95,115,150};
static float sectorC[4],sectorS[4];
static float absf(float v){return v<0.0f?-v:v;}
static float minf(float a,float b){return a<b?a:b;}
static float maxf(float a,float b){return a>b?a:b;}
static float clamp(float a,float lo,float hi){return minf(maxf(a,lo),hi);}
static float fsqrt(float v){if(v<=0.0f)return 0.0f;float g=v>1.0f?v:1.0f;for(int i=0;i<11;++i)g=(g+v/g)*0.5f;return g;}
static unsigned int shade(unsigned int c,float m){int r=(int)(((c>>16)&255)*m),g=(int)(((c>>8)&255)*m),b=(int)((c&255)*m);return (unsigned)(clamp((float)r,0,255))<<16 | (unsigned)(clamp((float)g,0,255))<<8 | (unsigned)clamp((float)b,0,255);}
static void put(int x,int y,unsigned int color){if((unsigned)x<WIDTH && (unsigned)y<HEIGHT)frame[y*WIDTH+x]=color;}
static void rect(int x,int y,int w,int h,unsigned int color){int xx=maxf((float)x,0),yy=maxf((float)y,0),ex=minf((float)(x+w),WIDTH),ey=minf((float)(y+h),HEIGHT);for(int row=yy;row<ey;++row)for(int col=xx;col<ex;++col)frame[row*WIDTH+col]=color;}
static void dot(int cx,int cy,int r,unsigned int color){for(int y=-r;y<=r;++y)for(int x=-r;x<=r;++x)if(x*x+y*y<=r*r)put(cx+x,cy+y,color);}
static void line(int x0,int y0,int x1,int y1,int thickness,unsigned int color){int dx=x1-x0,dy=y1-y0,steps=(int)maxf(absf((float)dx),absf((float)dy));if(steps<1){dot(x0,y0,thickness,color);return;}for(int i=0;i<=steps;i++){int x=x0+dx*i/steps,y=y0+dy*i/steps;dot(x,y,thickness,color);}}
static unsigned int color(int r,int g,int b){return (unsigned)(r<<16|g<<8|b);}

// Simple 5x7 uppercase pixel font; no system fonts or external resources.
static void glyph(char ch,unsigned char b[7]){
    for(int i=0;i<7;i++)b[i]=0;
    switch(ch){
#define G(K,A,B,C,D,E,F,H) case K:{unsigned char t[7]={A,B,C,D,E,F,H};for(int i=0;i<7;i++)b[i]=t[i];break;}
    G('A',14,17,17,31,17,17,17) G('B',30,17,17,30,17,17,30)
    G('C',14,17,16,16,16,17,14) G('D',30,17,17,17,17,17,30)
    G('E',31,16,16,30,16,16,31) G('F',31,16,16,30,16,16,16)
    G('G',14,17,16,23,17,17,14) G('H',17,17,17,31,17,17,17)
    G('I',31,4,4,4,4,4,31) G('J',7,2,2,2,18,18,12)
    G('K',17,18,20,24,20,18,17) G('L',16,16,16,16,16,16,31)
    G('M',17,27,21,21,17,17,17) G('N',17,25,21,19,17,17,17)
    G('O',14,17,17,17,17,17,14) G('P',30,17,17,30,16,16,16)
    G('Q',14,17,17,17,21,18,13) G('R',30,17,17,30,20,18,17)
    G('S',15,16,16,14,1,1,30) G('T',31,4,4,4,4,4,4)
    G('U',17,17,17,17,17,17,14) G('V',17,17,17,17,17,10,4)
    G('W',17,17,17,21,21,21,10) G('X',17,17,10,4,10,17,17)
    G('Y',17,17,10,4,4,4,4) G('Z',31,1,2,4,8,16,31)
    G('0',14,17,19,21,25,17,14) G('1',4,12,4,4,4,4,14)
    G('2',14,17,1,2,4,8,31) G('3',30,1,1,14,1,1,30)
    G('4',2,6,10,18,31,2,2) G('5',31,16,30,1,1,17,14)
    G('6',6,8,16,30,17,17,14) G('7',31,1,2,4,8,8,8)
    G('8',14,17,17,14,17,17,14) G('9',14,17,17,15,1,2,12)
    G(':',0,4,4,0,4,4,0) G('/',1,1,2,4,8,16,16)
    G('-',0,0,0,31,0,0,0) G('.',0,0,0,0,0,4,4)
    G('%',17,18,2,4,8,9,17) G('!',4,4,4,4,4,0,4)
    G('+',0,4,4,31,4,4,0) G('=',0,0,31,0,31,0,0)
    G('?',14,17,1,2,4,0,4) G('>',16,8,4,2,4,8,16)
    G('<',1,2,4,8,4,2,1) G(' ',0,0,0,0,0,0,0)
#undef G
    }
}
static void text(int x,int y,const char* s,unsigned int c,int scale=2){int origin=x;for(;*s;s++){if(*s=='\n'){y+=9*scale;x=origin;continue;}unsigned char g[7];glyph(*s,g);for(int row=0;row<7;row++)for(int col=0;col<5;col++)if(g[row]&(1<<(4-col)))rect(x+col*scale,y+row*scale,scale,scale,c);x+=6*scale;}}
static void decimal(char* out,int v){char tmp[16];int len=0;if(v<0)v=0;do{tmp[len++]=(char)('0'+v%10);v/=10;}while(v>0&&len<14);int i=0;while(len)out[i++]=tmp[--len];out[i]=0;}
static void number(int x,int y,int v,unsigned int c,int scale=2){char buf[16];decimal(buf,v);text(x,y,buf,c,scale);}
static unsigned int rnd(){seed=1664525u*seed+1013904223u;return seed;}

// Save file is a strict little-endian/versioned local POD; no cloud/network.
struct ProgressFile { unsigned int magic, version, unlocked, money, wins, stars, checksum; };
static unsigned int saveChecksum(ProgressFile d){return d.magic ^ d.version ^ d.unlocked ^ d.money ^ d.wins ^ d.stars ^ 0x5A73C991u;}
static bool joinPath(char* out,int cap,const char* a,const char* b){
    int n=0;while(*a&&n<cap-1)out[n++]=*a++;
    if(n>=cap-2)return false;
    if(n&&out[n-1]!='\\')out[n++]='\\';
    while(*b&&n<cap-1)out[n++]=*b++;
    out[n]=0;return !*b;
}
static bool savePath(char* dir,char* file,char* temp){
    char home[320]={};
    DWORD len=GetEnvironmentVariableA("LOCALAPPDATA",home,319);
    if(len==0||len>=319)return false;
    if(!joinPath(dir,400,home,"LateBrakePreview"))return false;
    if(!joinPath(file,440,dir,"career.dat"))return false;
    if(!joinPath(temp,440,dir,"career.tmp"))return false;
    return true;
}
static void loadProgress(){
    char dir[400],file[440],tmp[440];if(!savePath(dir,file,tmp))return;
    HANDLE h=CreateFileA(file,0x80000000u,1,0,3,0,0);
    if(h==(HANDLE)-1||!h)return;
    ProgressFile d={};DWORD got=0;
    BOOL ok=ReadFile(h,&d,sizeof(d),&got,0);CloseHandle(h);
    if(!ok||got!=sizeof(d)||d.magic!=0x32424C4Cu||d.version!=2||d.checksum!=saveChecksum(d))return;
    if((d.unlocked&1u)==0||d.unlocked>63||d.money>500000||d.wins>100000||d.stars>100000)return;
    ownedMask=(int)d.unlocked;credits=(int)d.money;completed=(int)d.wins;careerStars=(int)d.stars;
}
static void saveProgress(){
    if(testing)return; // Synthetic --self-test must not mutate real local career.
    char dir[400],file[440],tmp[440];if(!savePath(dir,file,tmp))return;
    CreateDirectoryA(dir,0);
    ProgressFile d={0x32424C4Cu,2u,(unsigned)ownedMask,(unsigned)credits,(unsigned)completed,(unsigned)careerStars,0};
    d.checksum=saveChecksum(d);
    HANDLE h=CreateFileA(tmp,0x40000000u,0,0,2,0,0);
    if(h==(HANDLE)-1||!h)return;
    DWORD got=0;BOOL ok=WriteFile(h,&d,sizeof(d),&got,0);CloseHandle(h);
    if(ok&&got==sizeof(d))MoveFileExA(tmp,file,0x1u|0x8u);
}
// Separate versioned UI preferences keep the existing V2 career.dat
// format fully compatible with older personal-preview installations.
struct UISettings { unsigned int magic, version, car, track, event, contrast, checksum; };
static unsigned int uiChecksum(UISettings d){
    return d.magic ^ d.version ^ d.car ^ d.track ^ d.event ^ d.contrast ^ 0x823A9D5Bu;
}
static bool settingsPath(char* dir,char* file,char* temp){
    char home[320]={};
    DWORD n=GetEnvironmentVariableA("LOCALAPPDATA",home,319);
    if(n==0||n>=319)return false;
    return joinPath(dir,400,home,"LateBrakePreview") &&
        joinPath(file,440,dir,"settings.dat") &&
        joinPath(temp,440,dir,"settings.tmp");
}
static void loadSettings(){
    char dir[400],file[440],tmp[440];if(!settingsPath(dir,file,tmp))return;
    HANDLE h=CreateFileA(file,0x80000000u,1,0,3,0,0);
    if(h==(HANDLE)-1||!h)return;
    UISettings d={};DWORD got=0;
    BOOL ok=ReadFile(h,&d,sizeof(d),&got,0);CloseHandle(h);
    if(!ok||got!=sizeof(d)||d.magic!=0x33424955u||d.version!=3||d.checksum!=uiChecksum(d))return;
    if(d.car>=6||d.track>=4||d.event>=4||d.contrast>1)return;
    selectedCar=(int)d.car;selectedTrack=(int)d.track;selectedChallenge=(int)d.event;
    highContrast=d.contrast!=0;
}
static void saveSettings(){
    if(testing)return;
    char dir[400],file[440],tmp[440];if(!settingsPath(dir,file,tmp))return;
    CreateDirectoryA(dir,0);
    UISettings d={0x33424955u,3u,(unsigned)selectedCar,(unsigned)selectedTrack,
        (unsigned)selectedChallenge,highContrast?1u:0u,0};
    d.checksum=uiChecksum(d);
    HANDLE h=CreateFileA(tmp,0x40000000u,0,0,2,0x80,0);
    if(h==(HANDLE)-1||!h)return;
    DWORD written=0;BOOL ok=WriteFile(h,&d,sizeof(d),&written,0);CloseHandle(h);
    if(ok&&written==sizeof(d))MoveFileExA(tmp,file,0x1u|0x8u);
}
static void message(const char* t){int i=0;while(t[i]&&i<70){toast[i]=t[i];++i;}toast[i]=0;toastTime=2.8f;}
static float trackProfile(float c,float s){
    float c2=c*c-s*s,s2=2*c*s,c3=4*c*c*c-3*c,s3=3*s-4*s*s*s;
    if(selectedTrack==1)return 1.0f+0.085f*c3+0.040f*s2;
    if(selectedTrack==2)return 1.0f+0.062f*s3+0.065f*c2;
    if(selectedTrack==3)return 1.0f+0.045f*c3-0.080f*s2;
    return 1.0f;
}
static void shapePos(float c,float s,float* x,float* y){
    float p=trackProfile(c,s);*x=CX+trackRx*p*c;*y=CY+trackRy*p*s;
}
static float trackOffset(float x,float y){
    float xx=(x-CX)/trackRx,yy=(y-CY)/trackRy;
    float r=fsqrt(xx*xx+yy*yy);if(r<0.001f)return -1.0f;
    return r-trackProfile(xx/r,yy/r);
}
static bool onroad(float x,float y){return absf(trackOffset(x,y))<trackWidth;}
static void gatePos(int idx,float* x,float* y){
    static const float c[4]={-1,0,1,0},s[4]={0,-1,0,1};
    shapePos(c[idx],s[idx],x,y);
}
struct Opponent { float c,s,x,y,speed,stun; int rounds; };
struct Race {
    float x,y,fx,fy,speed,nitro,damage,elapsed,lapStart,best;
    int lap,nextGate,passes;
    bool finished,readyForGate,awarded,won;
    Opponent ai[3];
};
static Race race;
static int totalLapsForMode(){return selectedChallenge==1?1:selectedChallenge==2||selectedChallenge==3?2:3;}
static void beginRace(){
    race={};
    race.nitro=100.0f;race.lap=1;race.nextGate=0;
    totalLaps=totalLapsForMode();race.readyForGate=true;
    float x,y;shapePos(0.0f,1.0f,&x,&y);
    race.x=x+11.0f;race.y=y-4.0f;race.fx=-1.0f;race.fy=0.0f;
    for(int i=0;i<3;i++){
        race.ai[i].c=0.0f;race.ai[i].s=1.0f;
        race.ai[i].speed=0.70f+0.085f*i+selectedTrack*0.035f;
        shapePos(0.0f,1.0f,&race.ai[i].x,&race.ai[i].y);
        race.ai[i].x-=25.0f+i*22.0f;race.ai[i].y+=i*12.0f;
    }
    page=RACING;paused=false;showHelp=false;countdown=3.5f;countdownGo=0;gameTime=0;gateFlash=0;
}
static void reset(){beginRace();}
static bool nearGate(int idx,float x,float y){
    float gx,gy;gatePos(idx,&gx,&gy);
    float dx=x-gx,dy=y-gy;
    // Dedicated local zone prevents collision/corner cut gate farming.
    float radius=idx%2?59.0f:70.0f;
    return dx*dx+dy*dy<radius*radius;
}
static void finishRace(bool winner){
    if(race.finished)return;
    race.finished=true;race.won=winner;
    if(winner){
        int reward=trackReward[selectedTrack]+selectedChallenge*30;
        if(race.damage<30.0f)reward+=25;
        if(credits<499000)credits+=reward;
        if(completed<100000)completed++;
        if(careerStars<100000)careerStars+=(race.damage<30?3:race.damage<65?2:1);
        bonusCredits=reward;saveProgress();
    } else bonusCredits=0;
    page=RESULTS;
}
static bool correctGateDirection(int gate){
    // Progress only while actually driving the ordered circuit direction.
    // This remains prototype point-sampling; Unreal's full swept gates are
    // verified separately in the production project.
    if(race.speed<25.0f)return false;
    if(gate==0)return race.fy < -0.45f;
    if(gate==1)return race.fx > 0.45f;
    if(gate==2)return race.fy > 0.45f;
    return race.fx < -0.45f;
}
static void creditGate(){
    race.nextGate=(race.nextGate+1)%4;race.passes++;gateFlash=0.8f;
    if(race.nextGate==0){
        float duration=race.elapsed-race.lapStart;
        if(duration>0&&(race.best==0||duration<race.best))race.best=duration;
        race.lapStart=race.elapsed;
        race.lap++;
        if(race.lap>totalLaps){
            bool ok=true;
            if(selectedChallenge==1)ok=(race.elapsed<65.0f);
            if(selectedChallenge==3)ok=(race.elapsed<95.0f);
            if(selectedChallenge==2){for(int i=0;i<3;i++)if(race.ai[i].rounds>=totalLaps)ok=false;}
            race.lap=totalLaps;finishRace(ok);
        }
    }
}
static void update(float dt,bool forward,bool reverse,bool left,bool right,bool brake,bool boost){
    if(race.finished)return;
    dt=clamp(dt,0.0f,0.045f);
    if(countdown>0){countdown=maxf(0,countdown-dt);return;}
    if(countdownGo>0)countdownGo=maxf(0,countdownGo-dt);
    race.elapsed+=dt;gameTime+=dt;
    if(gateFlash>0)gateFlash-=dt;
    bool road=onroad(race.x,race.y);
    float accel=forward?290.0f*carAccel[selectedCar]:0.0f;
    if(reverse)accel-=210.0f;
    bool nitro=boost&&forward&&!reverse&&!brake&&race.nitro>0&&road;
    if(nitro){accel+=380.0f;race.nitro=maxf(0.0f,race.nitro-dt*29);}
    else race.nitro=minf(100.0f,race.nitro+dt*9.0f);
    race.speed+=accel*dt*(1.0f-race.damage*0.0043f);
    if(brake)race.speed*=maxf(0.0f,1.0f-dt*5.8f);
    race.speed*=maxf(0.0f,1.0f-dt*(road?0.57f:2.6f));
    race.speed=clamp(race.speed,-110.0f,road?((nitro?420.0f:325.0f)*carSpeed[selectedCar]):130.0f);
    float steer=(right?1.0f:0.0f)-(left?1.0f:0.0f);
    float authority=clamp(absf(race.speed)/160.0f,0,1);
    float turn=steer*dt*2.85f*authority*(race.speed<0?-1:1)*(road?1.0f:0.5f)*carGrip[selectedCar];
    float fx=race.fx,fy=race.fy;
    race.fx=fx-fy*turn;race.fy=fy+fx*turn;
    float length=fsqrt(race.fx*race.fx+race.fy*race.fy);
    if(length>0){race.fx/=length;race.fy/=length;}
    race.x+=race.fx*race.speed*dt;race.y+=race.fy*race.speed*dt;
    if(race.x<12||race.x>WIDTH-12||race.y<59||race.y>HEIGHT-70){
        race.x=clamp(race.x,12,WIDTH-12);race.y=clamp(race.y,59,HEIGHT-70);
        race.speed*=-0.15f;race.damage=clamp(race.damage+5,0,88);
    }
    for(int i=0;i<3;i++){
        Opponent& a=race.ai[i];
        float co=a.c,si=a.s;
        float angle=dt*(a.speed+(selectedChallenge==2?0.07f:0));
        a.c=co-si*angle;a.s=si+co*angle;
        float magnitude=fsqrt(a.c*a.c+a.s*a.s);a.c/=magnitude;a.s/=magnitude;
        float ox=a.x,oy=a.y;
        shapePos(a.c,a.s,&a.x,&a.y);
        if(ox<CX&&oy>CY&&a.x>=CX&&a.y>CY)a.rounds++;
        a.stun=maxf(0,a.stun-dt);
        float dx=race.x-a.x,dy=race.y-a.y;
        if(dx*dx+dy*dy<(i==0?31.0f*31.0f:25.0f*25.0f)&&a.stun<=0){
            race.damage=minf(90,race.damage+7.0f);
            race.speed*=0.59f;a.stun=1.25f;
        }
    }
    bool inAny=false;
    for(int g=0;g<4;g++)if(nearGate(g,race.x,race.y)&&onroad(race.x,race.y))inAny=true;
    if(!inAny)race.readyForGate=true;
    if(race.readyForGate&&nearGate(race.nextGate,race.x,race.y)&&onroad(race.x,race.y)&&correctGateDirection(race.nextGate)){
        race.readyForGate=false;creditGate();
    }
    if((selectedChallenge==1&&race.elapsed>85.0f)||
       (selectedChallenge==3&&race.elapsed>95.0f)||
       (selectedChallenge==2&&race.ai[2].rounds>=totalLaps))finishRace(false);
}
static void drawTrack(){
    trackRx=selectedTrack==1?290.0f:selectedTrack==2?280.0f:selectedTrack==3?285.0f:295.0f;
    trackRy=selectedTrack==1?177.0f:selectedTrack==2?174.0f:selectedTrack==3?169.0f:174.0f;
    trackWidth=selectedTrack==1?0.218f:selectedTrack==3?0.216f:0.232f;
    unsigned int grass=selectedTrack==1?color(99,72,44):selectedTrack==2?color(22,77,100):selectedTrack==3?color(24,30,58):color(32,71,49);
    for(int y=0;y<HEIGHT;y++)for(int x=0;x<WIDTH;x++){
        float q=trackOffset((float)x,(float)y),absq=absf(q);
        unsigned int n=((unsigned)(x*1777+y*947)^(unsigned)(x*y*31))%29;
        unsigned int c=grass;
        if(selectedTrack==2&&q < -trackWidth) c=color(21+n/3,97+n/3,137+n/2);
        else if(selectedTrack==3 && q < -trackWidth) c=color(26+n/5,34+n/5,66+n/4);
        else if(q < -trackWidth) c=shade(grass,1.20f+(n/40.0f));
        else c=shade(grass,0.82f+(n/65.0f));
        if(absq<trackWidth){c=color(43+n/3,47+n/3,53+n/3);}
        if(absq>trackWidth-0.029f&&absq<trackWidth+0.022f)c=color(235,226,196);
        if(absq>trackWidth-0.049f&&absq<trackWidth-0.029f)c=(x/12+y/12)%2?color(234,81,70):color(242,244,233);
        if(absq<0.008f && ((x/15+y/15)%8)<5)c=color(184,190,182);
        backdrop[y*WIDTH+x]=c;
    }
    for(int k=0;k<500;k++){
        int x=(int)(rnd()%WIDTH),y=56+(int)(rnd()%(HEIGHT-128));
        if(absf(trackOffset((float)x,(float)y))>trackWidth+0.05f){
            unsigned int c=selectedTrack==3?color(69,94,140):selectedTrack==1?color(151,108,70):color(53,108,64);
            dot(x,y,selectedTrack==1?2:1,c);
        }
    }
    for(int i=0;i<WIDTH*HEIGHT;i++)frame[i]=backdrop[i];
    // Dynamic gate geometry follows the same mathematical road as the cars.
    for(int i=0;i<4;i++){
        float gx,gy;gatePos(i,&gx,&gy);
        float c=(i==0?-1:i==2?1:0),s=(i==1?-1:i==3?1:0);
        float x0,y0,x1,y1;
        shapePos(c*(1-trackWidth),s*(1-trackWidth),&x0,&y0);
        shapePos(c*(1+trackWidth),s*(1+trackWidth),&x1,&y1);
        if(i==3){
            for(int z=0;z<18;z++){
                float t=(float)z/18.0f,u=(float)(z+1)/18.0f;
                int xx=(int)(x0+(x1-x0)*t),yy=(int)(y0+(y1-y0)*t);
                int X=(int)(x0+(x1-x0)*u),Y=(int)(y0+(y1-y0)*u);
                line(xx,yy,X,Y,5,z%2?color(236,242,236):color(27,30,31));
            }
        }else line((int)x0,(int)y0,(int)x1,(int)y1,3,color(93,175,223));
        (void)gx;(void)gy;
    }
    for(int i=0;i<WIDTH*HEIGHT;i++)backdrop[i]=frame[i];
}
static void drawCar(float x,float y,float fx,float fy,unsigned int body,unsigned int nose){
    // Longitudinal axis = unit forward. Draw rotated body and contrasting nose.
    float sx=-fy,sy=fx;
    for(int a=-14;a<=17;a++)for(int b=-9;b<=9;b++){
        int px=(int)(x+fx*a+sx*b+1.9f),py=(int)(y+fy*a+sy*b+2.7f);
        if(absf((float)b)<7.0f)put(px,py,color(16,21,28));
    }
    for(int a=-14;a<=15;a++)for(int b=-8;b<=8;b++){
        int px=(int)(x+fx*a+sx*b),py=(int)(y+fy*a+sy*b);
        if(a<0&&absf((float)b)>6.0f)continue;
        unsigned int c=(a>=9)?nose:((a>=-3&&a<=7&&absf((float)b)<6)?color(36,51,65):body);
        put(px,py,c);
    }
    for(int z=-1;z<=1;z+=2){
        dot((int)(x+fx*13+sx*z*6),(int)(y+fy*13+sy*z*6),2,color(255,255,193));
        dot((int)(x-fx*12+sx*z*6),(int)(y-fy*12+sy*z*6),2,color(241,73,74));
    }
}
static void progressBar(int x,int y,int w,int h,int pct,unsigned int fg){
    rect(x,y,w,h,color(22,29,35));rect(x+2,y+2,(w-4)*pct/100,h-4,fg);
}
static void bar(int x,int y,int w,int value,unsigned int c){progressBar(x,y,w,10,value,c);}
static void panel(int x,int y,int w,int h,const char* titleText){
    rect(x,y,w,h,color(13,21,30));rect(x,y,w,2,color(69,104,125));
    text(x+15,y+15,titleText,color(250,193,96),2);
}
static void title(){
    rect(0,0,WIDTH,52,color(15,21,32));rect(0,49,WIDTH,3,color(245,114,78));
    text(19,14,"LATEBRAKE",color(255,236,212),3);
    text(275,19,"DRIVE  /  GARAGE  /  CAREER",color(156,204,216),2);
    rect(750,12,191,26,color(42,52,63));
    text(765,21,"V3  /  2D DEMO",color(250,213,119),1);
}
static void button(int x,int y,int w,const char* s,bool active){
    rect(x,y,w,39,active?color(52,104,131):color(27,40,55));
    rect(x,y,4,39,active?color(255,201,102):color(91,119,137));
    text(x+17,y+11,s,active?color(255,245,218):color(166,188,202),2);
}
static void carCard(int x,int y,int index,bool active){
    rect(x,y,246,122,active?color(46,66,78):color(22,31,43));
    if(active){rect(x,y,246,3,color(253,198,95));}
    text(x+15,y+12,carNames[index],color(247,223,182),2);
    drawCar((float)x+47,(float)y+76,1.0f,0.0f,carPaints[index],color(255,239,170));
    text(x+85,y+49,"POWER",color(145,177,190),1);bar(x+134,y+49,99,(int)(carAccel[index]*76),color(245,132,97));
    text(x+85,y+67,"SPEED",color(145,177,190),1);bar(x+134,y+67,99,(int)(carSpeed[index]*73),color(118,209,246));
    text(x+85,y+85,"GRIP",color(145,177,190),1);bar(x+134,y+85,99,(int)(carGrip[index]*70),color(132,226,166));
    if(!(ownedMask&(1<<index))){text(x+13,y+106,"LOCKED",color(241,130,122),1);number(x+116,y+106,carPrices[index],color(251,201,114),1);}
    else text(x+13,y+106,"IN GARAGE",color(136,231,180),1);
}

// V3 UI system: all coordinate dimensions are logical 960x640 units.
// Explicit keyboard focus, hover and click targets share the same navigation.
static unsigned int NAVY(){return highContrast?color(8,13,20):color(15,23,35);}
static unsigned int PANEL(){return highContrast?color(15,23,32):color(23,35,50);}
static unsigned int BORDER(){return highContrast?color(133,167,181):color(54,83,101);}
static unsigned int CYAN(){return highContrast?color(100,236,255):color(91,213,233);}
static unsigned int CREAM(){return color(245,240,222);}
static unsigned int GOLD(){return highContrast?color(255,220,102):color(245,188,92);}
static unsigned int MUTED(){return highContrast?color(216,222,230):color(167,187,201);}
static bool within(int x,int y,int a,int b,int w,int h){return x>=a&&y>=b&&x<a+w&&y<b+h;}
static void stroke(int x,int y,int w,int h,unsigned int c,int thickness=2){rect(x,y,w,thickness,c);rect(x,y+h-thickness,w,thickness,c);rect(x,y,thickness,h,c);rect(x+w-thickness,y,thickness,h,c);}
static void tile(int x,int y,int w,int h,bool active){
    rect(x+5,y+5,w,h,color(9,15,25));
    rect(x,y,w,h,active?color(34,66,82):PANEL());
    stroke(x,y,w,h,active?CYAN():BORDER(),active?3:1);
}
static void uiButton(int x,int y,int w,int h,const char* label,bool focus,bool enabled=true){
    bool hover=within(mouseX,mouseY,x,y,w,h);
    bool chosen=focus||hover;
    rect(x,y,w,h,enabled?(chosen?color(37,103,117):color(30,47,64)):color(30,36,44));
    stroke(x,y,w,h,enabled?(chosen?CYAN():BORDER()):color(74,78,86),chosen&&enabled?2:1);
    rect(x,y,5,h,enabled?(chosen?GOLD():color(68,111,131)):color(68,70,76));
    text(x+16,y+(h-14)/2,label,enabled?CREAM():MUTED(),2);
}
static void hint(int x,int y,const char* s){text(x,y,s,MUTED(),1);}
static void headerNav(){
    rect(0,0,WIDTH,60,NAVY());rect(0,57,WIDTH,3,CYAN());
    text(24,14,"LATEBRAKE",CREAM(),3);
    text(255,21,"/  PERSONAL RACING LAB",MUTED(),2);
    rect(768,12,170,29,color(45,51,65));
    text(784,21,"V3  2D PREVIEW",GOLD(),1);
}
static void footerNav(){
    rect(0,583,WIDTH,57,NAVY());rect(0,583,WIDTH,1,BORDER());
    hint(22,594,"ARROWS NAVIGATE     ENTER SELECT     M HOME     F1 HELP     H CONTRAST");
    hint(22,615,"WASD DRIVE   SPACE BRAKE   SHIFT NITRO   P PAUSE   ESC BACK/EXIT");
}
static void trackThumbnail(int x,int y,int w,int h,int idx){
    float cx=x+w*0.5f,cy=y+h*0.5f,rx=w*0.39f,ry=h*0.37f;
    float c=1,s=0;const float cs=0.99879546f,sn=0.04906767f;
    int lastX=0,lastY=0;
    for(int j=0;j<=128;j++){
        float p=1.0f,cc=c*c-s*s,ss=2*c*s,c3=4*c*c*c-3*c,s3=3*s-4*s*s*s;
        if(idx==1)p+=0.085f*c3+0.040f*ss;
        if(idx==2)p+=0.062f*s3+0.065f*cc;
        if(idx==3)p+=0.045f*c3-0.080f*ss;
        int xx=(int)(cx+rx*p*c),yy=(int)(cy+ry*p*s);
        if(j)line(lastX,lastY,xx,yy,4,color(106,127,139));
        lastX=xx;lastY=yy;
        float nc=c*cs-s*sn;s=s*cs+c*sn;c=nc;
    }
    dot((int)(cx),(int)(cy+ry),4,GOLD());
}
static void heroCar(int cx,int cy,int id){
    unsigned int paint=carPaints[id];
    // Wide readable concept illustration for selection. It is NOT 3D art.
    rect(cx-70,cy-31,139,65,color(7,13,22));
    for(int sign=-1;sign<=1;sign+=2){
        rect(cx-56,cy+sign*35-5,34,10,color(12,17,22));
        rect(cx+28,cy+sign*35-5,31,10,color(12,17,22));
    }
    rect(cx-66,cy-27,133,54,paint);
    rect(cx-20,cy-22,42,44,color(33,65,81));
    rect(cx-59,cy-20,16,40,shade(paint,0.70f));
    rect(cx+50,cy-17,10,34,color(244,203,130));
    rect(cx-55,cy-3,112,6,color(244,246,232));
    rect(cx-63,cy-26,12,9,color(249,100,84));
    rect(cx-63,cy+17,12,9,color(249,100,84));
    rect(cx+60,cy-25,8,13,color(255,242,181));
    rect(cx+60,cy+12,8,13,color(255,242,181));
}
static void cardStat(int x,int y,const char* name,float proportion,unsigned int accent){
    text(x,y,name,MUTED(),1);
    rect(x+72,y,160,13,color(15,25,36));
    rect(x+74,y+2,(int)(156*clamp(proportion,0.0f,1.0f)),9,accent);
}
static void navActiveTag(const char* s){rect(708,71,230,27,color(35,71,83));text(722,80,s,CYAN(),1);}
static void renderMenuV3(){
    // Quiet backdrop: readable contrast and clear hierarchy instead of the
    // entire track fighting the menu for attention.
    rect(0,60,WIDTH,523,color(13,23,35));
    for(int j=0;j<20;j++){
        int x=500+j*30,y=84+(j%9)*57;
        line(x,y,x+70,y+45,1,color(19,37,51));
    }
    if(page==HOME){
        navActiveTag("OFFLINE CAREER");
        text(48,105,"YOUR NEXT",CREAM(),4);
        text(48,148,"RACE STARTS HERE",GOLD(),3);
        text(50,192,"DRIVE  /  EARN  /  UNLOCK  /  IMPROVE",MUTED(),2);
        text(50,229,"SELECT A CAR AND EVENT, THEN TAKE THE GRID.",MUTED(),1);
        uiButton(50,264,433,48,"START QUICK RACE",homeFocus==0);
        uiButton(50,326,208,48,"YOUR GARAGE",homeFocus==1);
        uiButton(275,326,208,48,"CHOOSE TRACK",homeFocus==2);
        uiButton(50,388,208,48,"EVENTS / TASKS",homeFocus==3);
        uiButton(275,388,208,48,"ONLINE PLANS",homeFocus==4);
        tile(530,121,384,335,true);
        hint(556,143,"CURRENT RIDE");
        text(556,170,carNames[selectedCar],CREAM(),2);
        heroCar(716,244,selectedCar);
        hint(558,302,"CURRENT CIRCUIT");
        text(558,324,trackNames[selectedTrack],GOLD(),2);
        trackThumbnail(724,302,170,111,selectedTrack);
        hint(559,421,"PERSONAL SAVE  /  THIS PC ONLY");
        tile(50,480,864,81,false);
        hint(75,493,"CAREER CREDITS");number(77,516,credits,GOLD(),3);
        hint(335,493,"EVENT WINS");number(338,516,completed,CREAM(),3);
        hint(571,493,"CAREER STARS");number(575,516,careerStars,CYAN(),3);
    }else if(page==GARAGE){
        navActiveTag("VEHICLE SHOWROOM");
        text(48,92,"GARAGE",CREAM(),4);
        hint(49,135,"SIX CAR TUNES  /  RACE TO EARN CREDITS  /  U TO UNLOCK");
        for(int i=0;i<6;i++){
            int y=169+i*63;bool active=selectedCar==i;
            tile(49,y,360,55,active);
            rect(61,y+12,26,28,carPaints[i]);
            text(103,y+10,carNames[i],CREAM(),2);
            hint(103,y+35,(ownedMask&(1<<i))?"OWNED / READY TO DRIVE":"LOCKED / USE CREDITS");
            if(active)text(370,y+21,">",GOLD(),2);
        }
        tile(433,169,478,392,true);
        heroCar(674,251,selectedCar);
        text(471,320,carNames[selectedCar],CREAM(),3);
        cardStat(473,365,"POWER",carAccel[selectedCar]/1.4f,color(247,144,105));
        cardStat(473,390,"SPEED",carSpeed[selectedCar]/1.4f,CYAN());
        cardStat(473,415,"GRIP",carGrip[selectedCar]/1.4f,color(149,223,171));
        hint(474,465,"YOUR CREDITS");number(477,481,credits,GOLD(),2);
        if(!(ownedMask&(1<<selectedCar))){
            hint(635,465,"UNLOCK PRICE");number(636,481,carPrices[selectedCar],CREAM(),2);
            uiButton(477,515,393,36,"U  UNLOCK CAR",false,credits>=carPrices[selectedCar]);
        }else uiButton(477,515,393,36,"ENTER  DRIVE THIS CAR",false);
    }else if(page==TRACKS){
        navActiveTag("4 DISTINCT LAYOUTS");
        text(48,93,"SELECT CIRCUIT",CREAM(),4);
        hint(50,138,"MOVE WITH ARROWS OR CLICK A TRACK TO PREVIEW");
        for(int i=0;i<4;i++){
            int x=49+(i%2)*444,y=172+(i/2)*191;
            tile(x,y,418,169,i==selectedTrack);
            hint(x+22,y+18,i==selectedTrack?"SELECTED CIRCUIT":"CIRCUIT");
            text(x+22,y+43,trackNames[i],CREAM(),2);
            trackThumbnail(x+219,y+22,175,112,i);
            text(x+22,y+98,i==0?"CLASSIC":i==1?"NARROW TURNS":i==2?"SWEEPING": "NIGHT DRIVING",MUTED(),1);
            text(x+22,y+126,"REWARD",MUTED(),1);number(x+91,y+126,trackReward[i],GOLD(),1);
        }
        hint(54,557,"ENTER TO RACE     OR RETURN HOME TO SET AN EVENT");
    }else if(page==CHALLENGES){
        navActiveTag("CAREER EVENT LIST");
        text(48,94,"RACE EVENTS",CREAM(),4);
        hint(50,138,"DIFFERENT OBJECTIVES / DIFFERENT REWARDS");
        for(int i=0;i<4;i++){
            int y=171+i*95;
            tile(48,y,862,84,i==selectedChallenge);
            rect(64,y+18,44,44,i==selectedChallenge?CYAN():color(51,74,88));
            number(77,y+29,i+1,i==selectedChallenge?NAVY():CREAM(),2);
            text(131,y+15,challengeNames[i],CREAM(),2);
            hint(132,y+45,challengeSub[i]);
            hint(705,y+16,"BASE BONUS");number(756,y+38,30*i,GOLD(),2);
        }
        hint(54,562,"ENTER STARTS SELECTED EVENT     CLICK ROW TO SELECT");
    }else{
        navActiveTag("PLANNED  /  NOT LIVE");
        text(48,100,"MULTIPLAYER",CREAM(),4);
        text(48,153,"FRIENDS, INVITES AND LOBBIES",GOLD(),2);
        tile(48,205,863,285,true);
        text(82,239,"NOT AVAILABLE IN THIS PREVIEW",CREAM(),2);
        text(82,302,"FUTURE  /  PRIVATE ROOM INVITES",MUTED(),2);
        text(82,346,"FUTURE  /  SERVER-VALIDATED RACES",MUTED(),2);
        text(82,390,"FUTURE  /  PLAYER SAFETY AND REPORTING",MUTED(),2);
        hint(82,450,"CURRENT OPPONENTS ARE LOCAL SCRIPTED CARS, NOT PEOPLE.");
        uiButton(52,513,855,46,"M OR ESC  RETURN HOME",false);
    }
    headerNav();footerNav();
}
static void renderRaceHUDV3(){
    // Designed as small peripheral cards; do not hide the next gate or car.
    rect(0,0,WIDTH,52,NAVY());rect(0,49,WIDTH,3,CYAN());
    text(20,14,"LATEBRAKE",CREAM(),3);
    text(270,20,challengeNames[selectedChallenge],MUTED(),2);
    text(730,20,trackNames[selectedTrack],GOLD(),1);
    tile(16,72,228,155,false);
    hint(32,84,"RACE PROGRESS");
    text(31,115,"LAP",MUTED(),2);number(118,109,race.lap,CREAM(),3);
    text(164,122,"/",MUTED(),1);number(180,117,totalLaps,GOLD(),2);
    hint(31,154,"NEXT CHECKPOINT");number(202,148,race.nextGate+1,CYAN(),2);
    hint(31,181,"ELAPSED SECONDS");number(181,177,(int)race.elapsed,CREAM(),2);
    tile(728,72,216,162,false);
    hint(744,85,"VEHICLE STATUS");
    text(744,108,carNames[selectedCar],CREAM(),1);
    hint(744,135,"NITRO");progressBar(806,134,122,13,(int)race.nitro,CYAN());
    hint(744,157,"DAMAGE");progressBar(806,156,122,13,(int)race.damage,color(240,113,89));
    hint(744,188,"BEST LAP");number(837,184,(int)race.best,GOLD(),2);
    tile(18,506,225,62,true);
    hint(34,516,"SPEED / SIMULATED");number(36,537,(int)absf(race.speed),CREAM(),3);
    hint(161,544,"KM/H");
    tile(732,500,208,68,false);
    hint(748,509,"GATE PROGRESS");
    for(int i=0;i<4;i++){
        rect(751+i*44,538,32,17,i<race.nextGate?CYAN():i==race.nextGate?GOLD():BORDER());
        number(762+i*44,541,i+1,NAVY(),1);
    }
    rect(0,583,WIDTH,57,NAVY());
    hint(20,594,"W/UP ACCELERATE    A/D STEER    SPACE BRAKE    SHIFT NITRO    P PAUSE");
    hint(20,616,"R RESTART    M HOME    F1 HELP    H CONTRAST    BLUE = NEXT GATE");
    if(!onroad(race.x,race.y)){
        rect(350,74,263,36,color(90,46,34));
        text(369,85,"OFF TRACK / LOW GRIP",GOLD(),2);
    }
    if(countdown>0){
        rect(360,247,234,139,color(16,26,37));stroke(360,247,234,139,GOLD(),3);
        hint(404,259,"RACE STARTING");
        int stage=(int)countdown+1;
        number(450,290,stage>3?3:stage,GOLD(),6);
        hint(398,365,"GET READY TO RACE");
    }
}
static void overlayHelp(){
    rect(176,97,609,440,NAVY());stroke(176,97,609,440,CYAN(),3);
    text(204,117,"DRIVER GUIDE",CREAM(),3);
    hint(205,159,"MOVE THROUGH MENUS WITH ARROWS / ENTER / MOUSE");
    text(205,193,"W OR UP      ACCELERATE",CREAM(),2);
    text(205,222,"S OR DOWN    REVERSE",CREAM(),2);
    text(205,251,"A / D        STEER",CREAM(),2);
    text(205,280,"SPACE        BRAKE",CREAM(),2);
    text(205,309,"SHIFT        NITRO BOOST",CREAM(),2);
    text(205,338,"P            PAUSE / RESUME",CREAM(),2);
    text(205,367,"R            RESTART RACE",CREAM(),2);
    text(205,396,"H            HIGH CONTRAST UI",CREAM(),2);
    hint(205,448,"FOLLOW GOLD CHECKPOINT MARKERS IN ORDER.");
    uiButton(206,481,546,39,"F1 / ESC  CLOSE GUIDE",false);
}
static void resultsOverlayV3(){
    rect(238,162,484,332,NAVY());stroke(238,162,484,332,race.won?CYAN():color(236,137,101),3);
    text(262,186,race.won?"EVENT CLEARED":"TRY AGAIN",race.won?CYAN():GOLD(),3);
    hint(264,231,race.won?"NICE DRIVE. YOUR CAREER WAS SAVED.":"YOU CAN RETRY THE SAME EVENT.");
    hint(264,259,"RACE TIME");number(468,253,(int)race.elapsed,CREAM(),3);
    hint(264,309,"REWARD CREDITS");number(476,304,bonusCredits,GOLD(),3);
    uiButton(263,359,429,43,"ENTER  RETRY EVENT",true);
    uiButton(263,415,206,43,"M  HOME",false);
    uiButton(486,415,206,43,"G  GARAGE",false);
}
static void pausedOverlayV3(){
    rect(260,200,442,247,NAVY());stroke(260,200,442,247,CYAN(),3);
    text(291,226,"RACE PAUSED",CREAM(),3);
    hint(292,275,"ALL VEHICLE INPUTS ARE STOPPED.");
    uiButton(290,313,378,43,"P  CONTINUE",true);
    uiButton(290,369,378,43,"M  BACK TO HOME",false);
}

static void render(){
    for(int i=0;i<WIDTH*HEIGHT;i++)frame[i]=backdrop[i];
    if(page==RACING||page==RESULTS){
        float gx,gy;gatePos(race.nextGate,&gx,&gy);
        if(!race.finished){dot((int)gx,(int)gy,13,color(255,203,81));dot((int)gx,(int)gy,8,color(55,45,31));dot((int)gx,(int)gy,4,color(255,228,145));}
        for(int i=0;i<3;i++){
            Opponent& a=race.ai[i];
            // Tangent to the underlying oriented racetrack; decorative AI pose.
            float fx=-a.s,fy=a.c,mag=fsqrt(fx*fx+fy*fy);fx/=mag;fy/=mag;
            static const unsigned int rivalColors[3]={0x4CB6F4,0xE7BE67,0xBF89F1};
            drawCar(a.x,a.y,fx,fy,rivalColors[i],color(240,243,223));
        }
        drawCar(race.x,race.y,race.fx,race.fy,carPaints[selectedCar],color(255,220,141));
    }
    if(page==RACING||page==RESULTS){
        renderRaceHUDV3();
        if(page==RESULTS)resultsOverlayV3();
        else if(paused)pausedOverlayV3();
    }else renderMenuV3();
    if(showHelp)overlayHelp();
    if(toastTime>0&&!showHelp){rect(270,546,420,30,color(50,73,82));stroke(270,546,420,30,CYAN(),1);text(286,554,toast,CREAM(),1);}
}
static bool key(int k){return (GetAsyncKeyState(k)&0x8000)!=0;}
static bool pressed(int k,bool& previous){bool down=key(k);bool edge=down&&!previous;previous=down;return edge;}
static void setPage(Page target){
    page=target;paused=false;
    if(target==TRACKS)drawTrack();
}
static void chooseHome(int id){
    if(id==0)beginRace();
    else if(id==1)setPage(GARAGE);
    else if(id==2)setPage(TRACKS);
    else if(id==3)setPage(CHALLENGES);
    else if(id==4)setPage(ONLINE_INFO);
}
static void unlockSelectedCar(){
    if(ownedMask&(1<<selectedCar))message("ALREADY IN YOUR GARAGE");
    else if(credits>=carPrices[selectedCar]){
        credits-=carPrices[selectedCar];ownedMask|=1<<selectedCar;
        saveProgress();message("CAR UNLOCKED - ENTER TO DRIVE");
    }else message("WIN RACES TO EARN MORE CREDITS");
}
static void activateSelection(){
    if(page==HOME)chooseHome(homeFocus);
    else if(page==GARAGE){
        if(ownedMask&(1<<selectedCar))beginRace();
        else message("PRESS U TO UNLOCK CAR FIRST");
    }else if(page==TRACKS||page==CHALLENGES)beginRace();
    else if(page==RESULTS)beginRace();
}
static void mouseClick(int x,int y){
    if(showHelp){showHelp=false;return;}
    if(page==HOME){
        if(within(x,y,50,264,433,48))chooseHome(0);
        else if(within(x,y,50,326,208,48))chooseHome(1);
        else if(within(x,y,275,326,208,48))chooseHome(2);
        else if(within(x,y,50,388,208,48))chooseHome(3);
        else if(within(x,y,275,388,208,48))chooseHome(4);
    }else if(page==GARAGE){
        for(int i=0;i<6;i++)if(within(x,y,49,169+i*63,360,55)){selectedCar=i;saveSettings();return;}
        if(within(x,y,477,515,393,36)){
            if(ownedMask&(1<<selectedCar))beginRace();else unlockSelectedCar();
        }
    }else if(page==TRACKS){
        for(int i=0;i<4;i++)if(within(x,y,49+(i%2)*444,172+(i/2)*191,418,169)){
            selectedTrack=i;saveSettings();drawTrack();return;
        }
    }else if(page==CHALLENGES){
        for(int i=0;i<4;i++)if(within(x,y,48,171+i*95,862,84)){
            selectedChallenge=i;saveSettings();return;
        }
    }else if(page==ONLINE_INFO){if(y>505)setPage(HOME);}
    else if(page==RESULTS){
        if(within(x,y,263,359,429,43))beginRace();
        else if(within(x,y,263,415,206,43))setPage(HOME);
        else if(within(x,y,486,415,206,43))setPage(GARAGE);
    }else if(page==RACING&&paused){
        if(within(x,y,290,313,378,43))paused=false;
        else if(within(x,y,290,369,378,43))setPage(HOME);
    }
}
static void step(float dt){
    if(toastTime>0)toastTime-=dt;
    bool enter=pressed(0x0D,lastEnter),back=pressed(0x1B,lastBack);
    bool g=pressed('G',lastGarage),t=pressed('T',lastTracks);
    bool ch=pressed('C',lastChallenge),on=pressed('O',lastOnline);
    bool home=pressed('M',lastMenu);
    bool l=pressed(0x25,lastLeft),rgt=pressed(0x27,lastRight);
    bool up=pressed(0x26,lastUp),down=pressed(0x28,lastDown);
    bool unlock=pressed('U',lastUnlock);
    bool rp=pressed('R',lastRestart),pause=pressed('P',lastPause);
    bool help=pressed(0x70,lastHelp),contrast=pressed('H',lastContrast);
    if(contrast){highContrast=!highContrast;saveSettings();}
    if(help){showHelp=!showHelp;return;}
    if(showHelp){if(back||enter)showHelp=false;return;}
    if(page==RACING){
        if(back||home||g||t||ch){setPage((back||home)?HOME:g?GARAGE:t?TRACKS:CHALLENGES);return;}
        if(pause)paused=!paused;
        if(rp){beginRace();return;}
        if(!paused)update(dt,key('W')||key(0x26),key('S')||key(0x28),key('A')||key(0x25),key('D')||key(0x27),key(0x20),key(0x10));
    }else if(page==RESULTS){
        if(enter||rp){beginRace();return;}
        if(back||home||g){setPage((back||home)?HOME:GARAGE);return;}
    }else{
        if(home){setPage(HOME);return;}
        if(back){if(page==HOME)running=false;else setPage(HOME);return;}
        if(g){setPage(GARAGE);return;}
        if(t){setPage(TRACKS);return;}
        if(ch){setPage(CHALLENGES);return;}
        if(on){setPage(ONLINE_INFO);return;}
        if(page==HOME){
            if(down||rgt)homeFocus=(homeFocus+1)%5;
            if(up||l)homeFocus=(homeFocus+4)%5;
        }else if(page==GARAGE){
            if(rgt||down)selectedCar=(selectedCar+1)%6;
            if(l||up)selectedCar=(selectedCar+5)%6;
            if(l||rgt||up||down)saveSettings();
            if(unlock)unlockSelectedCar();
        }else if(page==TRACKS){
            if(rgt||down){selectedTrack=(selectedTrack+1)%4;drawTrack();}
            if(l||up){selectedTrack=(selectedTrack+3)%4;drawTrack();}
            if(l||rgt||up||down)saveSettings();
        }else if(page==CHALLENGES){
            if(down||rgt)selectedChallenge=(selectedChallenge+1)%4;
            if(up||l)selectedChallenge=(selectedChallenge+3)%4;
            if(up||l||down||rgt)saveSettings();
        }
        if(enter)activateSelection();
    }
}
static LRESULT __stdcall WindowProc(HWND hwnd,UINT msg,WPARAM w,LPARAM l){
    if(msg==0x0200||msg==0x0201){
        RECT client={};GetClientRect(hwnd,&client);
        int cw=client.right-client.left,ch=client.bottom-client.top;
        if(cw>0&&ch>0){
            // The host window letterboxes/stretch-scales the 960x640 canvas.
            mouseX=((int)(unsigned short)(l&0xffff))*WIDTH/cw;
            mouseY=((int)(unsigned short)((l>>16)&0xffff))*HEIGHT/ch;
            if(msg==0x0201){mouseClick(mouseX,mouseY);return 0;}
        }
    }
    if(msg==0x0008){ // WM_KILLFOCUS: held keys must not drive a background race.
        if(page==RACING)paused=true;
    }
    if(msg==0x0010||msg==0x0002){running=false;PostQuitMessage(0);return 0;}
    return DefWindowProcA(hwnd,msg,w,l);
}
static bool hasSelfTestFlag(const char* s){
    if(!s)return false;
    const char* t="--self-test";
    for(;*s;s++){
        int j=0;while(t[j]&&s[j]==t[j])j++;
        if(!t[j]&&(s[j]==' '||s[j]=='\0'||s[j]=='\"'))return true;
    }
    return false;
}
static bool selftest(){
    testing=true;
    selectedTrack=0;selectedCar=0;selectedChallenge=0;
    beginRace();
    if(page!=RACING||race.lap!=1||race.nextGate!=0||race.nitro!=100)return false;
    countdown=0;update(0.015f,true,false,false,false,false,true);
    if(race.nitro>=100||race.speed<=0)return false;
    for(int tr=0;tr<4;tr++){
        selectedTrack=tr;drawTrack();
        for(int g=0;g<4;g++){
            float x,y;gatePos(g,&x,&y);
            if(!onroad(x,y)||!nearGate(g,x,y))return false;
        }
    }
    selectedTrack=0;selectedChallenge=0;drawTrack();beginRace();countdown=0;
    for(int i=0;i<3;i++)race.ai[i].stun=1000.0f; // Isolate checkpoint test from rival contacts.
    for(int lap=0;lap<3;lap++)for(int g=0;g<4;g++){
        // Synthetic ordered-gate state, not proof of actual player driving.
        float x,y;gatePos(g,&x,&y);
        race.x=x;race.y=y;race.readyForGate=true;
        race.speed=50.0f;
        if(g==0){race.fx=0.0f;race.fy=1.0f;}
        if(g==1){race.fx=-1.0f;race.fy=0.0f;}
        if(g==2){race.fx=0.0f;race.fy=-1.0f;}
        if(g==3){race.fx=1.0f;race.fy=0.0f;}
        int prev=race.passes;
        update(0.0f,false,false,false,false,false,false);
        if(race.passes!=prev)return false; // Wrong-way orientation rejected.
        if(g==0)race.fy=-1.0f;
        if(g==1)race.fx=1.0f;
        if(g==2)race.fy=1.0f;
        if(g==3)race.fx=-1.0f;
        update(0.0f,false,false,false,false,false,false);
        if(race.passes!=prev+1)return false;
        update(0.0f,false,false,false,false,false,false);
        if(race.passes!=prev+1)return false;
    }
    if(!race.finished||!race.won||race.passes!=12||page!=RESULTS)return false;
    if(credits<=0||completed<=0||careerStars<=0)return false;
    // UI consistency and keyboard/mouse selection contracts.
    setPage(HOME);homeFocus=2;activateSelection();
    if(page!=TRACKS)return false;
    mouseClick(600,400);if(selectedTrack!=3)return false;
    setPage(HOME);homeFocus=1;activateSelection();
    if(page!=GARAGE)return false;
    mouseClick(90,169+4*63+10);if(selectedCar!=4)return false;
    int previousMoney=credits;selectedCar=5;unlockSelectedCar();
    if(credits>previousMoney)return false;
    setPage(HOME);showHelp=true;mouseClick(250,300);
    if(showHelp||page!=HOME)return false;
    UISettings prefs={0x33424955u,3u,5u,2u,1u,1u,0};
    prefs.checksum=uiChecksum(prefs);
    if(prefs.checksum!=uiChecksum(prefs))return false;
    prefs.track=7; if(prefs.checksum==uiChecksum(prefs))return false;
    beginRace();if(countdown<3.0f)return false;
    float before=race.elapsed;update(0.020f,true,false,false,false,false,false);
    if(race.elapsed!=before)return false;
    return (carPrices[5]>carPrices[1] && carAccel[4]>carAccel[1]);
}
extern "C" void mainCRTStartup(){
    if(hasSelfTestFlag(GetCommandLineA()))ExitProcess(selftest()?0:3);
    loadProgress();loadSettings();drawTrack();
    HINSTANCE instance=GetModuleHandleA(0);
    WNDCLASSA wc={};wc.style=0x0003;wc.proc=WindowProc;wc.instance=instance;wc.name="LateBrakePreviewNative";
    if(!RegisterClassA(&wc))ExitProcess(5);
    HWND wnd=CreateWindowExA(0,wc.name,"LateBrake V3 - Racing Lab (2D Standalone Preview, Not Unreal)",0x00CF0000,70,70,980,710,0,0,instance,0);
    if(!wnd)ExitProcess(6);
    windowHandle=wnd;ShowWindow(wnd,5);UpdateWindow(wnd);
    BITMAPINFO bi={};bi.header.size=40;bi.header.width=WIDTH;bi.header.height=-HEIGHT;bi.header.planes=1;bi.header.bits=32;
    long long frequency=0,last=0,now=0;
    QueryPerformanceFrequency(&frequency);QueryPerformanceCounter(&last);
    if(frequency<=0)frequency=1000000;
    while(running){
        MSG msg={};while(PeekMessageA(&msg,0,0,0,1)){
            if(msg.message==0x0012){running=false;break;}
            TranslateMessage(&msg);DispatchMessageA(&msg);
        }
        QueryPerformanceCounter(&now);
        float dt=clamp((float)(now-last)/(float)frequency,0.0f,0.045f);
        last=now;
        step(dt);render();
        RECT rc={};GetClientRect(wnd,&rc);
        HDC dc=GetDC(wnd);
        if(dc){StretchDIBits(dc,0,0,rc.right-rc.left,rc.bottom-rc.top,0,0,WIDTH,HEIGHT,frame,&bi,0,0x00CC0020);ReleaseDC(wnd,dc);}
        Sleep(7);
    }
    ExitProcess(0);
}

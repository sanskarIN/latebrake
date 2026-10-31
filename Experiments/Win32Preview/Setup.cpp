// LateBrake V4 per-user setup/bootstrapper. Native Win32; NO Unreal engine.
// Embeds exact standalone game executable at build time; no network installer.
// Installs only under current user's LOCALAPPDATA. Apps & Features uninstall.
extern "C" { int _fltused=0; }
#if defined(_MSC_VER) && !defined(__clang__)
extern "C" void* memset(void*,int,unsigned long long);
#pragma function(memset)
#endif

extern "C" void* memset(void* dest,int value,unsigned long long len){
 unsigned char* p=(unsigned char*)dest;
 for(unsigned long long i=0;i<len;i++)p[i]=(unsigned char)value;
 return dest;
}
typedef void* HANDLE;
typedef void* HKEY;
typedef unsigned long DWORD;
typedef int BOOL;
struct STARTUPINFOA {DWORD cb;char* reserved;char* desktop;char* title;DWORD x,y,xsize,ysize,xchars,ychars,fill,flags;unsigned short show;unsigned short reserved2;unsigned char* reserved3;HANDLE in,out,err;};
struct PROCESS_INFORMATION {HANDLE process,thread;DWORD pid,tid;};
extern "C" {
__declspec(dllimport) void __stdcall ExitProcess(unsigned int);
__declspec(dllimport) char* __stdcall GetCommandLineA();
__declspec(dllimport) DWORD __stdcall GetEnvironmentVariableA(const char*,char*,DWORD);
__declspec(dllimport) DWORD __stdcall GetModuleFileNameA(void*,char*,DWORD);
__declspec(dllimport) BOOL __stdcall CreateDirectoryA(const char*,void*);
__declspec(dllimport) HANDLE __stdcall CreateFileA(const char*,DWORD,DWORD,void*,DWORD,DWORD,HANDLE);
__declspec(dllimport) BOOL __stdcall WriteFile(HANDLE,const void*,DWORD,DWORD*,void*);
__declspec(dllimport) BOOL __stdcall CloseHandle(HANDLE);
__declspec(dllimport) BOOL __stdcall CopyFileA(const char*,const char*,BOOL);
__declspec(dllimport) BOOL __stdcall DeleteFileA(const char*);
__declspec(dllimport) BOOL __stdcall RemoveDirectoryA(const char*);
__declspec(dllimport) BOOL __stdcall MoveFileExA(const char*,const char*,DWORD);
__declspec(dllimport) DWORD __stdcall GetTempPathA(DWORD,char*);
__declspec(dllimport) void __stdcall Sleep(DWORD);
__declspec(dllimport) BOOL __stdcall CreateProcessA(const char*,char*,void*,void*,BOOL,DWORD,void*,const char*,STARTUPINFOA*,PROCESS_INFORMATION*);
__declspec(dllimport) int __stdcall MessageBoxA(void*,const char*,const char*,unsigned int);
__declspec(dllimport) int __stdcall RegCreateKeyExA(HKEY,const char*,DWORD,char*,DWORD,DWORD,void*,HKEY*,DWORD*);
__declspec(dllimport) int __stdcall RegSetValueExA(HKEY,const char*,DWORD,DWORD,const unsigned char*,DWORD);
__declspec(dllimport) int __stdcall RegDeleteKeyA(HKEY,const char*);
__declspec(dllimport) int __stdcall RegCloseKey(HKEY);
}
static const unsigned char embeddedGame[]={
#include "payload.inc"
};
static int slen(const char* s){int i=0;while(s[i])i++;return i;}
static bool join(char* out,int cap,const char* a,const char* b){int i=0;
 while(*a&&i<cap-1)out[i++]=*a++;
 if(i<1||i>=cap-2)return false;
 if(out[i-1]!='\\')out[i++]='\\';
 while(*b&&i<cap-1)out[i++]=*b++;
 out[i]=0;return !*b;
}
static void copystr(char* dst,const char* s,int cap){int i=0;while(s[i]&&i<cap-1){dst[i]=s[i];i++;}dst[i]=0;}
static bool eq(const char* a,const char* b){while(*a&&*b&&*a==*b){a++;b++;}return !*a&&!*b;}
static bool has(const char* cmd,const char* marker){for(;*cmd;cmd++){const char*a=cmd,*b=marker;while(*a&&*b&&*a==*b){a++;b++;}if(!*b)return true;}return false;}
static bool paths(char* folder,char* game,char* setup){char home[380]={};
 DWORD got=GetEnvironmentVariableA("LOCALAPPDATA",home,379);if(!got||got>=379)return false;
 return join(folder,460,home,"LateBrakePreviewV4") &&
        join(game,540,folder,"LateBrakePreviewV4.exe")&&
        join(setup,540,folder,"LateBrakeSetupV4.exe");
}
static bool writeData(const char* name,const unsigned char* data,DWORD len){
 HANDLE h=CreateFileA(name,0x40000000u,0,0,2,0x80,0);
 if(h==(HANDLE)-1||!h)return false;
 DWORD written=0;BOOL ok=WriteFile(h,data,len,&written,0);CloseHandle(h);
 return ok&&written==len;
}
static const char* uninstallKey="Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\LateBrakePreviewV4";
static void regString(HKEY key,const char* name,const char* value){
 RegSetValueExA(key,name,0,1,(const unsigned char*)value,(DWORD)slen(value)+1);
}
static void registration(const char* folder,const char* setup){
 HKEY k=0;DWORD state=0;
 if(RegCreateKeyExA((HKEY)(unsigned long long)0x80000001u,uninstallKey,0,0,0,0x20006,0,&k,&state)==0){
  regString(k,"DisplayName","LateBrake Standalone Racing Preview V4");
  regString(k,"DisplayVersion","4.0.0-preview");
  regString(k,"Publisher","LateBrake open-source project");
  regString(k,"InstallLocation",folder);
  char cmd[700]={};int i=0;cmd[i++]='"';
  for(const char*s=setup;*s&&i<650;s++)cmd[i++]=*s;
  cmd[i++]='"';cmd[i++]=' ';for(const char*s="--uninstall";*s;s++)cmd[i++]=*s;cmd[i]=0;
  regString(k,"UninstallString",cmd);RegCloseKey(k);
 }
}
static void doneRemove(){
 char folder[460],game[540],setup[540];if(!paths(folder,game,setup))return;
 Sleep(900);
 DeleteFileA(game);DeleteFileA(setup);
 RegDeleteKeyA((HKEY)(unsigned long long)0x80000001u,uninstallKey);
 RemoveDirectoryA(folder);
 // The temporary remover is never executed from the installed directory.
 // Windows may disallow self-removal; request deletion at next reboot.
 char self[560]={};if(GetModuleFileNameA(0,self,560))MoveFileExA(self,0,4);
 MessageBoxA(0,"LateBrake Preview V4 files and uninstall registration were removed. Your local career save is preserved.","LateBrake Preview Uninstall",0x40);
}
static void uninstall(){
 int response=MessageBoxA(0,"Uninstall LateBrake Preview V4? Your career data will be preserved in LOCALAPPDATA\\LateBrakePreview.","LateBrake Preview Uninstall",0x24);
 if(response!=6)return;
 char temp[420]={};DWORD n=GetTempPathA(419,temp);
 char target[540]={};if(!n||n>=419||!join(target,540,temp,"LateBrakeV4TempUninstall.exe"))return;
 char self[540]={};if(!GetModuleFileNameA(0,self,539)||!CopyFileA(self,target,0)){
 MessageBoxA(0,"Could not prepare safe temporary uninstaller.","LateBrake Setup",0x10);return;
 }
 char cmd[660]={};int i=0;cmd[i++]='"';
 for(const char*s=target;*s&&i<570;s++)cmd[i++]=*s;
 cmd[i++]='"';cmd[i++]=' ';for(const char*s="--final-remove";*s;s++)cmd[i++]=*s;cmd[i]=0;
 STARTUPINFOA si={};si.cb=sizeof(si);PROCESS_INFORMATION p={};
 if(CreateProcessA(target,cmd,0,0,0,0,0,0,&si,&p)){CloseHandle(p.thread);CloseHandle(p.process);}
 else MessageBoxA(0,"Unable to launch temporary uninstaller.","LateBrake Setup",0x10);
}
static void install(){
 if(MessageBoxA(0,"Install the 64-bit LateBrake V4 standalone racing preview for this Windows user? The installer does not download any files or require administrator access. This is NOT the Unreal Engine game.","LateBrake Preview V4 Setup",0x24)!=6)return;
 char folder[460],game[540],setup[540];
 if(!paths(folder,game,setup)){
  MessageBoxA(0,"LOCALAPPDATA environment path is missing or too long.","LateBrake Setup",0x10);return;
 }
 CreateDirectoryA(folder,0); // Already installed is allowed; WriteFile checks permissions.
 if(!writeData(game,embeddedGame,(DWORD)sizeof(embeddedGame))){
  MessageBoxA(0,"Could not write the game under LOCALAPPDATA. Check user folder permissions.","LateBrake Setup - Failed",0x10);return;
 }
 char self[540]={};
 if(!GetModuleFileNameA(0,self,539)||(!eq(self,setup)&&!CopyFileA(self,setup,0))){
  MessageBoxA(0,"Game was installed, but could not add its uninstaller. Install from a writable directory.","LateBrake Setup - Partial",0x30);return;
 }
 registration(folder,setup);
 if(MessageBoxA(0,"Installed to %LOCALAPPDATA%\\LateBrakePreviewV4. Launch the racing preview now? You can remove it through Windows Installed Apps.","LateBrake Preview V4 Installed",0x24)==6){
  char launch[640]={};int n=0;launch[n++]='"';for(const char* c=game;*c&&n<580;c++)launch[n++]=*c;launch[n++]='"';launch[n]=0;
  STARTUPINFOA si={};si.cb=sizeof(si);PROCESS_INFORMATION pi={};
  if(CreateProcessA(game,launch,0,0,0,0,0,folder,&si,&pi)){CloseHandle(pi.thread);CloseHandle(pi.process);}
  else MessageBoxA(0,"Installed, but launching the preview failed. Open LateBrakePreviewV4.exe from the installation directory.","LateBrake Preview V4",0x30);
 }
}
extern "C" void mainCRTStartup(){
 const char* cmd=GetCommandLineA();
 if(has(cmd,"--final-remove"))doneRemove();
 else if(has(cmd,"--uninstall"))uninstall();
 else install();
 ExitProcess(0);
}


/* FHD UpdateCheck 0.3.1. Startup-only update scan + in-game chat-like frame.
   Win32 x86 / XenoNativeLoader Host API V1. No launcher, no auto-install. */
typedef unsigned int DWORD, UINT, SIZE_T; typedef unsigned short WCHAR, WORD;
typedef unsigned char BYTE; typedef unsigned long ULONG; typedef long LONG; typedef int INT;
typedef int BOOL, HRESULT; typedef void *HANDLE,*HMODULE,*HINSTANCE,*HWND,*LPVOID,*HINTERNET;
typedef const WCHAR *LPCWSTR; typedef WCHAR *LPWSTR; typedef const char *LPCSTR;
#define IMP __declspec(dllimport)
#define EXP __declspec(dllexport)
#define WINAPI __stdcall
#define CALLBACK __stdcall
#define TRUE 1
#define FALSE 0
#define INVALID_HANDLE_VALUE ((HANDLE)(-1))
#define FILE_ATTRIBUTE_NORMAL 0x80
#define FILE_ATTRIBUTE_DIRECTORY 0x10
#define FILE_ATTRIBUTE_REPARSE_POINT 0x400
#define INVALID_FILE_ATTRIBUTES 0xFFFFFFFFu
#define GENERIC_READ 0x80000000u
#define GENERIC_WRITE 0x40000000u
#define FILE_SHARE_READ 1
#define OPEN_EXISTING 3
#define CREATE_ALWAYS 2
#define FILE_END 2
#define WINHTTP_FLAG_SECURE 0x800000u
#define WINHTTP_QUERY_STATUS_CODE 19
#define WINHTTP_QUERY_FLAG_NUMBER 0x20000000u
#define PROV_RSA_AES 24
#define CRYPT_VERIFYCONTEXT 0xF0000000u
#define CALG_SHA_256 0x0000800c
#define HP_HASHVAL 2
#define SW_SHOWNORMAL 1
#define PAGE_READWRITE 0x04
#define GWL_WNDPROC (-4)
#define WM_KEYDOWN 0x0100
#define WM_KEYUP 0x0101
#define WM_LBUTTONDOWN 0x0201
#define WM_LBUTTONUP 0x0202
#define WM_LBUTTONDBLCLK 0x0203
#define WM_MOUSEMOVE 0x0200
#define WM_MOUSEWHEEL 0x020A
#define VK_F12 0x7B
#define D3DFVF_FHD 0x144u
#define D3DPOOL_MANAGED 1
#define D3DFMT_A8R8G8B8 21
#define MAX_MODS 96
#define MAX_LOCAL_MODS 256
#define MAX_SCAN_NODES 12000
#define MAX_SCAN_DEPTH 5
#define MAX_UPDATES 32
#define MAX_FONT_VERTS 12000
#define RGBA(a,r,g,b) (((DWORD)(a)<<24)|((DWORD)(r)<<16)|((DWORD)(g)<<8)|(DWORD)(b))

typedef void (WINAPI *HOSTLOG)(LPCWSTR,LPCWSTR);
typedef struct {DWORD size,requiredHostApi;WCHAR id[64],version[32],description[160];unsigned long long exclusiveCapabilities;} XenoPluginInfoV1;
typedef struct {DWORD size,apiVersion;HMODULE gameModule;LPCWSTR gameRoot,executablePath,pluginRoot,configPath;HOSTLOG log;void *getBool,*getInt,*getString;} XenoPluginHostV1;
typedef struct {DWORD low,high;} FILETIME;
typedef struct {WORD year,month,dayOfWeek,day,hour,minute,second,milliseconds;} SYSTEMTIME;
typedef struct {DWORD attributes;FILETIME created,accessed,modified;DWORD sizeHigh,sizeLow,reserved0,reserved1;WCHAR name[260],alternate[14];} WIN32_FIND_DATAW;
typedef struct {LONG left,top,right,bottom;} RECT;
typedef struct {LONG x,y;} POINT;
typedef struct {UINT BackBufferWidth,BackBufferHeight,BackBufferFormat,BackBufferCount,MultiSampleType;DWORD MultiSampleQuality;UINT SwapEffect;HWND hDeviceWindow;BOOL Windowed,EnableAutoDepthStencil;UINT AutoDepthStencilFormat;DWORD Flags,FullScreen_RefreshRateInHz,PresentationInterval;} D3DPRESENT_PARAMETERS;
typedef struct {UINT AdapterOrdinal,DeviceType;HWND hFocusWindow;DWORD BehaviorFlags;} D3DCREATION_PARAMETERS;
typedef struct {DWORD X,Y,Width,Height;float MinZ,MaxZ;} D3DVIEWPORT9;
typedef struct {INT Pitch;void *pBits;} D3DLOCKED_RECT;
typedef struct {float x,y,z,rhw;DWORD color;float u,v;} VERT;
typedef LONG(WINAPI *WNDPROC)(HWND,UINT,SIZE_T,LONG);
typedef struct {HWND hwnd;UINT message;SIZE_T wParam;LONG lParam;DWORD time;POINT pt;DWORD lPrivate;} CHATMSG;
typedef LONG(WINAPI *MSGHOOKPROC)(int,SIZE_T,LONG);

IMP HANDLE WINAPI CreateFileW(LPCWSTR,DWORD,DWORD,LPVOID,DWORD,DWORD,HANDLE);
IMP BOOL WINAPI WriteFile(HANDLE,const void*,DWORD,DWORD*,LPVOID);
IMP BOOL WINAPI ReadFile(HANDLE,LPVOID,DWORD,DWORD*,LPVOID);
IMP BOOL WINAPI CloseHandle(HANDLE);
IMP BOOL WINAPI CreateDirectoryW(LPCWSTR,LPVOID);
IMP BOOL WINAPI DeleteFileW(LPCWSTR);
IMP BOOL WINAPI MoveFileExW(LPCWSTR,LPCWSTR,DWORD);
IMP DWORD WINAPI GetFileAttributesW(LPCWSTR);
IMP HANDLE WINAPI FindFirstFileW(LPCWSTR,WIN32_FIND_DATAW*);
IMP BOOL WINAPI FindNextFileW(HANDLE,WIN32_FIND_DATAW*);
IMP BOOL WINAPI FindClose(HANDLE);
IMP DWORD WINAPI GetModuleFileNameW(HMODULE,LPWSTR,DWORD);
IMP BOOL WINAPI DisableThreadLibraryCalls(HMODULE);
IMP DWORD WINAPI GetCurrentProcessId(void);
IMP HANDLE WINAPI GetCurrentProcess(void);
IMP BOOL WINAPI ReadProcessMemory(HANDLE,const void*,void*,SIZE_T,SIZE_T*);
IMP BOOL WINAPI VirtualProtect(LPVOID,SIZE_T,DWORD,DWORD*);
IMP BOOL WINAPI GetFileTime(HANDLE,FILETIME*,FILETIME*,FILETIME*);
IMP BOOL WINAPI SystemTimeToFileTime(const SYSTEMTIME*,FILETIME*);
IMP UINT WINAPI GetPrivateProfileIntW(LPCWSTR,LPCWSTR,int,LPCWSTR);
IMP DWORD WINAPI GetPrivateProfileStringW(LPCWSTR,LPCWSTR,LPCWSTR,LPWSTR,DWORD,LPCWSTR);
IMP BOOL WINAPI WritePrivateProfileStringW(LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR);
IMP HINSTANCE WINAPI ShellExecuteW(HWND,LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR,int);
IMP LONG WINAPI InterlockedCompareExchange(volatile LONG*,LONG,LONG);
IMP HANDLE WINAPI CreateThread(LPVOID,SIZE_T,DWORD (WINAPI *)(LPVOID),LPVOID,DWORD,DWORD*);
IMP void WINAPI Sleep(DWORD);
IMP HMODULE WINAPI GetModuleHandleW(LPCWSTR);
IMP HMODULE WINAPI LoadLibraryW(LPCWSTR);
IMP LPVOID WINAPI GetProcAddress(HMODULE,LPCSTR);
IMP BOOL WINAPI FreeLibrary(HMODULE);
IMP HINTERNET WINAPI WinHttpOpen(LPCWSTR,DWORD,LPCWSTR,LPCWSTR,DWORD);
IMP HINTERNET WINAPI WinHttpConnect(HINTERNET,LPCWSTR,WORD,DWORD);
IMP HINTERNET WINAPI WinHttpOpenRequest(HINTERNET,LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR,const WCHAR**,DWORD);
IMP BOOL WINAPI WinHttpSetTimeouts(HINTERNET,int,int,int,int);
IMP BOOL WINAPI WinHttpSendRequest(HINTERNET,LPCWSTR,DWORD,LPVOID,DWORD,DWORD,SIZE_T);
IMP BOOL WINAPI WinHttpReceiveResponse(HINTERNET,LPVOID);
IMP BOOL WINAPI WinHttpQueryHeaders(HINTERNET,DWORD,LPCWSTR,LPVOID,DWORD*,DWORD*);
IMP BOOL WINAPI WinHttpReadData(HINTERNET,LPVOID,DWORD,DWORD*);
IMP BOOL WINAPI WinHttpCloseHandle(HINTERNET);
IMP BOOL WINAPI CryptAcquireContextW(HANDLE*,LPCWSTR,LPCWSTR,DWORD,DWORD);
IMP BOOL WINAPI CryptCreateHash(HANDLE,DWORD,HANDLE,DWORD,HANDLE*);
IMP BOOL WINAPI CryptHashData(HANDLE,const BYTE*,DWORD,DWORD);
IMP BOOL WINAPI CryptGetHashParam(HANDLE,DWORD,BYTE*,DWORD*,DWORD);
IMP BOOL WINAPI CryptDestroyHash(HANDLE);
IMP BOOL WINAPI CryptReleaseContext(HANDLE,DWORD);
IMP LONG WINAPI SetWindowLongW(HWND,int,LONG);
IMP LONG WINAPI GetWindowLongW(HWND,int);
IMP LONG WINAPI CallWindowProcW(WNDPROC,HWND,UINT,SIZE_T,LONG);
IMP int WINAPI GetClientRect(HWND,RECT*);
IMP DWORD WINAPI GetWindowThreadProcessId(HWND,DWORD*);
IMP HANDLE WINAPI SetWindowsHookExW(int,MSGHOOKPROC,HMODULE,DWORD);
IMP LONG WINAPI CallNextHookEx(HANDLE,int,SIZE_T,LONG);
IMP BOOL WINAPI GetModuleHandleExW(DWORD,LPCWSTR,HMODULE*);
IMP BOOL WINAPI TranslateMessage(const CHATMSG*);
IMP DWORD WINAPI GetTickCount(void);

int _fltused=0;
static volatile LONG g_start_check_taken=0;
static HMODULE g_module;
static WCHAR g_root[640],g_mod[800],g_config[900],g_installed[900],g_log[900];

static void wcpy(WCHAR*d,int c,const WCHAR*s){int i=0;if(c<1)return;while(s&&s[i]&&i<c-1){d[i]=s[i];i++;}d[i]=0;}
static int wlen(const WCHAR*s){int i=0;while(s&&s[i])i++;return i;}
static int weq(const WCHAR*a,const WCHAR*b){int i=0;for(;;i++){if(a[i]!=b[i])return 0;if(!a[i])return 1;}}
static void wcat(WCHAR*d,int cap,const WCHAR*s){int n=wlen(d),i=0;while(s&&s[i]&&n+i<cap-1){d[n+i]=s[i];i++;}d[n+i]=0;}
static void wchar_num(WCHAR*d,int cap,DWORD x){WCHAR z[20];int n=0;do{z[n++]=(WCHAR)(L'0'+x%10);x/=10;}while(x&&n<19);int k=0;while(n&&k<cap-1)d[k++]=z[--n];d[k]=0;}
static void path_join(WCHAR*out,int cap,const WCHAR*base,const WCHAR*piece){wcpy(out,cap,base);if(wlen(out)&&out[wlen(out)-1]!=L'\\')wcat(out,cap,L"\\");wcat(out,cap,piece);} 
static void directory_of(WCHAR*out,int cap,const WCHAR*path){int n;wcpy(out,cap,path);n=wlen(out);while(n>0&&out[n-1]!=L'\\')n--;if(n)out[n-1]=0;}
IMP DWORD WINAPI SetFilePointer(HANDLE,LONG,LONG*,DWORD);
static void trace(const char*s){if(!g_log[0])return;HANDLE h=CreateFileW(g_log,GENERIC_WRITE,FILE_SHARE_READ,0,4,FILE_ATTRIBUTE_NORMAL,0);if(h==INVALID_HANDLE_VALUE)return;SetFilePointer(h,0,0,FILE_END);DWORD n=0,k=0;while(s[k])k++;WriteFile(h,s,k,&n,0);WriteFile(h,"\r\n",2,&n,0);CloseHandle(h);} 
static int ascii_token(const WCHAR*s,int max,int prefix){int n=wlen(s);if(n<1||n>max)return 0;for(int i=0;i<n;i++){WCHAR c=s[i];if(!((c>=L'0'&&c<=L'9')||(c>=L'a'&&c<=L'z')||(c>=L'A'&&c<=L'Z')||c==L'-'||c==L'_'))return 0;}if(prefix&&!(n>=5&&s[0]==L'F'&&s[1]==L'H'&&s[2]==L'D'&&s[3]==L'_'))return 0;return 1;}
static int version_token(const WCHAR*s){int n=wlen(s);if(n<1||n>31)return 0;for(int i=0;i<n;i++)if(!((s[i]>=L'0'&&s[i]<=L'9')||s[i]==L'.'||s[i]==L'-'))return 0;return 1;}
static void ini_get(const WCHAR*ini,const WCHAR*section,const WCHAR*key,WCHAR*out,int cap){GetPrivateProfileStringW(section,key,L"",out,cap,ini);} 
static int module_field(const WCHAR*path,const WCHAR*key,WCHAR*out,int cap){
  BYTE raw[32768];DWORD count=0;out[0]=0;HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
  if(f==INVALID_HANDLE_VALUE)return 0;BOOL ok=ReadFile(f,raw,sizeof(raw),&count,0);CloseHandle(f);
  if(!ok||count<4||raw[0]!=0xff||raw[1]!=0xfe||(count&1))return 0;int len=(int)count/2,kn=wlen(key);
  for(int i=1;i<len;){int start=i;while(i<len&&((WORD)raw[i*2]|((WORD)raw[i*2+1]<<8))!=L'\r'&&((WORD)raw[i*2]|((WORD)raw[i*2+1]<<8))!=L'\n')i++;int end=i;while(i<len&&(((WORD)raw[i*2]|((WORD)raw[i*2+1]<<8))==L'\r'||((WORD)raw[i*2]|((WORD)raw[i*2+1]<<8))==L'\n'))i++;if(end-start<=kn+1)continue;int equal=1;for(int j=0;j<kn;j++)if(((WORD)raw[(start+j)*2]|((WORD)raw[(start+j)*2+1]<<8))!=key[j]){equal=0;break;}if(!equal||((WORD)raw[(start+kn)*2]|((WORD)raw[(start+kn)*2+1]<<8))!=L'=')continue;int begin=start+kn+1;while(begin<end&&(((WORD)raw[begin*2]|((WORD)raw[begin*2+1]<<8))==L' '))begin++;while(end>begin&&(((WORD)raw[(end-1)*2]|((WORD)raw[(end-1)*2+1]<<8))==L' '))end--;if(end-begin>=cap)return 0;for(int j=0;j<end-begin;j++)out[j]=(WCHAR)((WORD)raw[(begin+j)*2]|((WORD)raw[(begin+j)*2+1]<<8));out[end-begin]=0;return out[0]!=0;}
  return 0;
}

typedef struct {WCHAR id[80],path[1024],relative[500],version[40];} ModEntry;
static ModEntry g_modIndex[MAX_LOCAL_MODS]; static int g_modCount=0,g_scanNodes=0,g_scanError=0,g_scanDone=0;
static int regular_file(const WCHAR*p){DWORD a=GetFileAttributesW(p);return a!=INVALID_FILE_ATTRIBUTES&&!(a&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT));}
static void scan_mod_tree(const WCHAR*folder,const WCHAR*relative,int depth){
  if(depth>MAX_SCAN_DEPTH||g_scanError)return;WCHAR info[1024],name[80],sub[1024];path_join(info,1024,folder,L"ModuleInfo.txt");
  if(regular_file(info)&&module_field(info,L"Name",name,80)){
    if(ascii_token(name,70,1)&&!weq(name,L"FHD_UpdateCheck")){
      if(g_modCount>=MAX_LOCAL_MODS){g_scanError=1;return;}ModEntry*e=&g_modIndex[g_modCount++];wcpy(e->id,80,name);wcpy(e->path,1024,folder);wcpy(e->relative,500,relative);e->version[0]=0;
      if(!module_field(info,L"Version",e->version,40)||!version_token(e->version)){if(!module_field(info,L"ModVersion",e->version,40)||!version_token(e->version))e->version[0]=0;}
      if(!e->version[0]){WCHAR vfile[1024];path_join(vfile,1024,folder,L"FHD_Version.ini");ini_get(vfile,L"Mod",L"Version",e->version,40);if(!version_token(e->version))e->version[0]=0;}
      if(!e->version[0]){WCHAR oldPath[500];ini_get(g_installed,L"RelPath",name,oldPath,500);if(weq(oldPath,relative)){ini_get(g_installed,L"Installed",name,e->version,40);if(!version_token(e->version))e->version[0]=0;}}
    }
    return;
  }
  WCHAR pattern[1024];path_join(pattern,1024,folder,L"*");WIN32_FIND_DATAW fd;HANDLE h=FindFirstFileW(pattern,&fd);if(h==INVALID_HANDLE_VALUE)return;
  do{if(!(fd.attributes&FILE_ATTRIBUTE_DIRECTORY)||(fd.attributes&FILE_ATTRIBUTE_REPARSE_POINT))continue;if(fd.name[0]==L'.'&&(fd.name[1]==0||(fd.name[1]==L'.'&&fd.name[2]==0)))continue;if(++g_scanNodes>MAX_SCAN_NODES){g_scanError=1;break;}if(depth>=MAX_SCAN_DEPTH)continue;WCHAR rel[500];path_join(sub,1024,folder,fd.name);if(!relative[0])wcpy(rel,500,fd.name);else path_join(rel,500,relative,fd.name);if(wlen(rel)>450||wlen(sub)>950){g_scanError=1;break;}scan_mod_tree(sub,rel,depth+1);if(g_scanError)break;}while(FindNextFileW(h,&fd));FindClose(h);
}
static void build_inventory(void){WCHAR mods[1024];path_join(mods,1024,g_root,L"Mods");g_modCount=0;g_scanError=0;g_scanNodes=0;g_scanDone=1;DWORD a=GetFileAttributesW(mods);if(a==INVALID_FILE_ATTRIBUTES||!(a&FILE_ATTRIBUTE_DIRECTORY)||(a&FILE_ATTRIBUTE_REPARSE_POINT)){g_scanError=1;trace("Mods folder unavailable or unsafe");return;}scan_mod_tree(mods,L"Mods",0);}
static int installed_mod(const WCHAR*id,WCHAR*out,int cap,WCHAR*version,int vcap,WCHAR*rel,int relcap){if(!g_scanDone)build_inventory();if(g_scanError)return 0;int found=-1;for(int i=0;i<g_modCount;i++)if(weq(g_modIndex[i].id,id)){if(found>=0){trace("Duplicate module identity; skipping BOTH physical copies");return 0;}found=i;}if(found<0)return 0;ModEntry*e=&g_modIndex[found];if(wlen(e->path)>=cap||wlen(e->relative)>=relcap)return 0;wcpy(out,cap,e->path);wcpy(version,vcap,e->version);wcpy(rel,relcap,e->relative);return 1;}
static int file_stamp(const WCHAR*mod,FILETIME*mtime){WCHAR path[1024];path_join(path,1024,mod,L"ModuleInfo.txt");HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);if(f==INVALID_HANDLE_VALUE)return 0;FILETIME t={0};BOOL ok=GetFileTime(f,0,0,&t);CloseHandle(f);if(!ok)return 0;if(mtime)*mtime=t;return 1;}
static int fixed_digits(const WCHAR*s,int n){int v=0;for(int i=0;i<n;i++){if(s[i]<L'0'||s[i]>L'9')return -1;v=v*10+(s[i]-L'0');}return v;}
static int parse_modified_utc(const WCHAR*iso,FILETIME*ft){if(wlen(iso)!=24||iso[4]!=L'-'||iso[7]!=L'-'||iso[10]!=L'T'||iso[13]!=L':'||iso[16]!=L':'||iso[19]!=L'.'||iso[23]!=L'Z')return 0;int y=fixed_digits(iso,4),m=fixed_digits(iso+5,2),d=fixed_digits(iso+8,2),h=fixed_digits(iso+11,2),mi=fixed_digits(iso+14,2),sec=fixed_digits(iso+17,2),ms=fixed_digits(iso+20,3);if(y<2020||m<1||m>12||d<1||d>31||h>23||mi>59||sec>59||ms>999)return 0;SYSTEMTIME t={0};t.year=y;t.month=m;t.day=d;t.hour=h;t.minute=mi;t.second=sec;t.milliseconds=ms;return SystemTimeToFileTime(&t,ft)!=0;}
static int stamp_newer(const FILETIME*a,const FILETIME*b){return a->high>b->high||(a->high==b->high&&a->low>b->low);} 
static void release_key(WCHAR*out,int cap,const WCHAR*version,const WCHAR*timestamp,const WCHAR*fileid){wcpy(out,cap,version);wcat(out,cap,L"|");wcat(out,cap,timestamp);wcat(out,cap,L"|");wcat(out,cap,fileid);} 
static int version_cmp(const WCHAR*a,const WCHAR*b){if(!a[0]||!b[0])return 0;for(int seg=0;seg<6;seg++){unsigned int aa=0,bb=0;while(*a>='0'&&*a<='9'){aa=aa*10+*a++-'0';}while(*b>='0'&&*b<='9'){bb=bb*10+*b++-'0';}if(aa>bb)return 1;if(aa<bb)return -1;if(*a==L'.')a++;if(*b==L'.')b++;if(!*a&&!*b)break;}return 0;}
static int token_drive_id(const WCHAR*s){return ascii_token(s,100,0);} 
static int fetch_catalog(const WCHAR*host,const WCHAR*route,const WCHAR*target){HINTERNET ses=0,conn=0,req=0;HANDLE file=INVALID_HANDLE_VALUE;DWORD status=0,sz=sizeof(status),read=0,wrote=0,total=0;int ok=0;ses=WinHttpOpen(L"FHD-UpdateCheck/0.3.1",0,0,0,0);if(!ses)goto done;WinHttpSetTimeouts(ses,5000,8000,30000,30000);conn=WinHttpConnect(ses,host,443,0);if(!conn)goto done;req=WinHttpOpenRequest(conn,L"GET",route,0,0,0,WINHTTP_FLAG_SECURE);if(!req)goto done;if(!WinHttpSendRequest(req,0,0,0,0,0,0)||!WinHttpReceiveResponse(req,0))goto done;if(!WinHttpQueryHeaders(req,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,0,&status,&sz,0)||status!=200)goto done;file=CreateFileW(target,GENERIC_WRITE,FILE_SHARE_READ,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);if(file==INVALID_HANDLE_VALUE)goto done;{BYTE buf[8192];for(;;){if(!WinHttpReadData(req,buf,sizeof(buf),&read))goto done;if(!read)break;if(total+read>131072)goto done;if(!WriteFile(file,buf,read,&wrote,0)||wrote!=read)goto done;total+=read;}}ok=(total>80);done:if(file!=INVALID_HANDLE_VALUE)CloseHandle(file);if(req)WinHttpCloseHandle(req);if(conn)WinHttpCloseHandle(conn);if(ses)WinHttpCloseHandle(ses);if(!ok)DeleteFileW(target);return ok;}
/* One-time hash of the original EXE on disk; no patch and no periodic rescan.
   These SHA256 values correspond to the two exact supplied game builds. */
static int detect_exe_flavor(void){
  static int cached=-1;
  if(cached>=0)return cached;
  cached=0;
  WCHAR exe_path[900];
  if(!GetModuleFileNameW(0,exe_path,900))return 0;
  HANDLE exe=CreateFileW(exe_path,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
  if(exe==INVALID_HANDLE_VALUE)return 0;
  HANDLE provider=0,hash=0;
  BYTE buf[8192],digest[32];
  DWORD n=0,total=0,len=sizeof(digest);
  int ok=0;
  if(!CryptAcquireContextW(&provider,0,0,PROV_RSA_AES,CRYPT_VERIFYCONTEXT))goto hash_done;
  if(!CryptCreateHash(provider,CALG_SHA_256,0,0,&hash))goto hash_done;
  for(;;){
    if(!ReadFile(exe,buf,sizeof(buf),&n,0))goto hash_done;
    if(!n)break;
    total+=n;
    if(total>64000000u||!CryptHashData(hash,buf,n,0))goto hash_done;
  }
  if(!CryptGetHashParam(hash,HP_HASHVAL,digest,&len,0)||len!=32)goto hash_done;
  {
    const BYTE old_sha[32]={0x83,0x30,0x03,0x44,0xaf,0x80,0x2b,0xc5,0x1e,0x64,0x38,0x9c,0x58,0xf0,0x47,0xe5,0xaf,0xdf,0x19,0x5c,0x13,0x30,0x48,0x09,0x8b,0xe3,0x88,0x1f,0xae,0x29,0xed,0x98};
    const BYTE new_sha[32]={0xe4,0x3a,0x4b,0x15,0x50,0x3c,0x70,0x5e,0xeb,0xe2,0xd8,0xce,0x57,0x78,0xc2,0xa2,0xfb,0x09,0x0d,0xb5,0x15,0xa3,0xfd,0x86,0xd3,0x19,0xe5,0xc5,0xaa,0x69,0x2e,0xbf};
    int same_old=1,same_new=1;
    for(int i=0;i<32;i++){if(digest[i]!=old_sha[i])same_old=0;if(digest[i]!=new_sha[i])same_new=0;}
    if(same_old)ok=1;
    else if(same_new)ok=2;
  }
hash_done:
  if(hash)CryptDestroyHash(hash);
  if(provider)CryptReleaseContext(provider,0);
  CloseHandle(exe);
  cached=ok;
  return ok;
}
static int compatible_exe(const WCHAR*exe){
  if(weq(exe,L"any")||!exe[0])return 1;
  WCHAR preferred[32];ini_get(g_config,L"Settings",L"ExeFlavor",preferred,32);
  if(weq(preferred,L"old"))return weq(exe,L"old");
  if(weq(preferred,L"new"))return weq(exe,L"new");
  if(!preferred[0]||weq(preferred,L"auto")){
    int detected=detect_exe_flavor();
    return (detected==1&&weq(exe,L"old"))||(detected==2&&weq(exe,L"new"));
  }
  return 0;
}
static int should_offer(const WCHAR*remote_version,const WCHAR*modified,const WCHAR*path,const WCHAR*local_version){FILETIME local={0},remote={0};if(!file_stamp(path,&local))return 0;if(remote_version[0]&&local_version[0]){int cmp=version_cmp(remote_version,local_version);if(cmp<0)return 0;if(cmp>0)return 1;}if(!parse_modified_utc(modified,&remote))return 0;return stamp_newer(&remote,&local);} 
static void open_browser(const WCHAR*id){if(!token_drive_id(id))return;WCHAR url[260]=L"https://drive.google.com/uc?export=download&id=";wcat(url,260,id);HINSTANCE h=ShellExecuteW(0,L"open",url,0,0,SW_SHOWNORMAL);if((SIZE_T)h<=32){WCHAR page[260]=L"https://drive.google.com/file/d/";wcat(page,260,id);wcat(page,260,L"/view");ShellExecuteW(0,L"open",page,0,0,SW_SHOWNORMAL);}}
static void paths_init(const WCHAR*root,const WCHAR*mod){wcpy(g_root,640,root);wcpy(g_mod,800,mod);path_join(g_config,900,mod,L"UpdateCheck.ini");path_join(g_installed,900,mod,L"Observed.ini");path_join(g_log,900,mod,L"FHD_UpdateCheck.log");}

typedef struct {WCHAR id[80],local[40],remote[40],modified[100],fileid[160];} UpdateEntry;
static UpdateEntry g_updates[MAX_UPDATES]; static int g_update_count=0,g_check_done=0,g_open=0; static volatile LONG g_check_lock=0;
static void add_update(const WCHAR*id,const WCHAR*local,const WCHAR*remote,const WCHAR*modified,const WCHAR*fileid){if(g_update_count>=MAX_UPDATES)return;UpdateEntry*e=&g_updates[g_update_count++];wcpy(e->id,80,id);wcpy(e->local,40,local);wcpy(e->remote,40,remote);wcpy(e->modified,100,modified);wcpy(e->fileid,160,fileid);} 
static void check_updates(void){
  if(!GetPrivateProfileIntW(L"Settings",L"CheckOnStart",1,g_config)){g_check_done=1;return;}build_inventory();if(g_scanError){g_check_done=1;return;}
  WCHAR manifest[900];path_join(manifest,900,g_mod,L"catalog.current.ini");
  /* This published Google Apps Script builds the catalog from Drive metadata.
     WinHTTP follows the ContentService HTTPS 302 redirect to googleusercontent. */
  const WCHAR*google=L"script.google.com";
  const WCHAR*route=L"/macros/s/AKfycbwlX5E3p1MqzATUIo2ThSOBs-JckmFm0s9T7oTkSmWeo5_UHhMTmFG--KorfgTue4qazw/exec";
  int ok=fetch_catalog(google,route,manifest);
  int count=ok?GetPrivateProfileIntW(L"Updater",L"Count",0,manifest):0;
  if(!ok||GetPrivateProfileIntW(L"Updater",L"Schema",0,manifest)!=2||count<=0||count>MAX_MODS){DeleteFileW(manifest);g_check_done=1;return;}
  WCHAR channel[32];ini_get(g_config,L"Settings",L"Channel",channel,32);int test=weq(channel,L"test");int maxprompts=GetPrivateProfileIntW(L"Settings",L"MaxPromptsPerStart",10,g_config);if(maxprompts<1||maxprompts>50)maxprompts=10;
  g_update_count=0;
  for(int i=1;i<=count&&g_update_count<MAX_UPDATES&&g_update_count<maxprompts;i++){
    WCHAR sect[32]=L"Mod",k[20],id[80],v[40],modified[100],fileid[160],exe[32],category[32],target[1024],local[40],rel[500];
    wchar_num(k,20,(DWORD)i);wcat(sect,32,k);ini_get(manifest,sect,L"Id",id,80);ini_get(manifest,sect,L"Version",v,40);ini_get(manifest,sect,L"ModifiedUTC",modified,100);ini_get(manifest,sect,L"DriveFileId",fileid,160);ini_get(manifest,sect,L"Channel",category,32);ini_get(manifest,sect,L"Exe",exe,32);
    if(!ascii_token(id,70,1)||weq(id,L"FHD_UpdateCheck")||!token_drive_id(fileid))continue; if(weq(category,L"development")&&!test)continue; if(!compatible_exe(exe))continue; if(!installed_mod(id,target,1024,local,40,rel,500))continue; if(!should_offer(v,modified,target,local))continue;
    WCHAR key[300],last[300];release_key(key,300,v,modified,fileid);ini_get(g_installed,L"Offered",id,last,300); if(weq(key,last))continue; add_update(id,local,v,modified,fileid); WritePrivateProfileStringW(L"Offered",id,key,g_installed);
  }
  DeleteFileW(manifest); g_open=(g_update_count>0); g_check_done=1;
}
static DWORD WINAPI worker(LPVOID unused){(void)unused; if(InterlockedCompareExchange(&g_check_lock,1,0)!=0)return 0; check_updates(); return 0;}

/* Overlay / frame */
#include "font16.h"
#include "compat_profiles.h"
static void** g_hookvtable; static void* g_realdevice; static void* g_fontTexture; static void* g_fontOwner;
static HRESULT (WINAPI *g_original_end)(void*); static HRESULT (WINAPI *g_original_present)(void*,const RECT*,const RECT*,HWND,const void*);
static HWND g_game; static WNDPROC g_prevproc; static HANDLE g_message_hook; static int g_queue_installed=0;
static VERT g_verts[MAX_FONT_VERTS]; static UINT g_vert_count; static int g_view_w=0,g_view_h=0; static int g_panel_x=20,g_panel_y=40; static int g_scroll=0;
static volatile LONG g_ready=0; static DWORD exe_base,g_device_global_rva; static int g_profile_index=-1;

static void wcopy(WCHAR* dest,int cap,const WCHAR* s){int i=0;if(!dest||cap<=0)return;while(s&&s[i]&&i<cap-1){dest[i]=s[i];i++;}dest[i]=0;}
static int glyph_index(WCHAR c){if(c>=32&&c<=126)return c-32;if(c>=0x0400&&c<=0x045f)return 95+(c-0x0400);switch(c){case 0x0490:return 191;case 0x0491:return 192;case 0x2014:return 193;case 0x2013:return 194;case 0x2026:return 195;default:return '?' -32;}}
#define VT(obj,index,type) ((type)(*(void***)(obj))[index])
static HRESULT callEnd(void* dev){return g_original_end?g_original_end(dev):0;}
static void addrect(float x,float y,float w,float h,DWORD c,float u0,float v0,float u1,float v1){if(g_vert_count+6>MAX_FONT_VERTS)return;VERT p={x-0.5f,y-0.5f,0.0f,1.0f,c,u0,v0};VERT q={x+w-0.5f,y-0.5f,0.0f,1.0f,c,u1,v0};VERT r={x-0.5f,y+h-0.5f,0.0f,1.0f,c,u0,v1};VERT s={x+w-0.5f,y+h-0.5f,0.0f,1.0f,c,u1,v1};g_verts[g_vert_count++]=p;g_verts[g_vert_count++]=q;g_verts[g_vert_count++]=r;g_verts[g_vert_count++]=r;g_verts[g_vert_count++]=q;g_verts[g_vert_count++]=s;}
static HRESULT drawverts(void* dev){HRESULT(WINAPI *f)(void*,UINT,UINT,const void*,UINT)=VT(dev,83,void*);if(!g_vert_count)return 0;HRESULT rv=f(dev,4,g_vert_count/3,g_verts,sizeof(VERT));g_vert_count=0;return rv;}
static void fonttext(float x,float y,const WCHAR* text,DWORD color,int maxChars){int charpos=0;if(!text)return;for(int i=0;text[i]&&charpos<maxChars;i++){if(text[i]=='\r'||text[i]=='\n')continue;int idx=glyph_index(text[i]);float u=(float)(idx%32)*16.0f/512.0f,v=(float)(idx/32)*20.0f/256.0f;addrect(x+charpos*8,y,16,20,color,u,v,u+16.0f/512.0f,v+20.0f/256.0f);charpos++;}}
static int init_font(void* dev){if(g_fontTexture&&g_fontOwner==dev)return 1; if(g_fontTexture&&g_fontOwner!=dev){VT(g_fontTexture,2,DWORD(WINAPI*)(void*))(g_fontTexture);} g_fontTexture=0;g_fontOwner=dev; HRESULT(WINAPI *CreateTexture)(void*,UINT,UINT,UINT,DWORD,UINT,UINT,void**,HANDLE*)=VT(dev,23,void*); void* tex=0; if(CreateTexture(dev,512,256,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&tex,0)<0||!tex)return 0; HRESULT(WINAPI *LockRect)(void*,UINT,D3DLOCKED_RECT*,const RECT*,DWORD)=VT(tex,19,void*); HRESULT(WINAPI *UnlockRect)(void*,UINT)=VT(tex,20,void*); D3DLOCKED_RECT lr; if(LockRect(tex,0,&lr,0,0)<0){VT(tex,2,DWORD(WINAPI*)(void*))(tex);return 0;} for(int y=0;y<256;y++){DWORD* pixels=(DWORD*)((BYTE*)lr.pBits+y*lr.Pitch);for(int x=0;x<512;x++)pixels[x]=0;} for(int i=0;i<GLYPH_COUNT;i++)for(int yy=0;yy<20;yy++)for(int xx=0;xx<GLYPH_ALPHA_WIDTH;xx++){BYTE alpha=glyph_alpha8[(i*20+yy)*GLYPH_ALPHA_WIDTH+xx]; if(alpha){int x=(i%32)*16+xx,y=(i/32)*20+yy;DWORD* p=(DWORD*)((BYTE*)lr.pBits+y*lr.Pitch);p[x]=((DWORD)alpha<<24)|0x00ffffffu;}} UnlockRect(tex,0);g_fontTexture=tex;return 1;}
static void setup_pipeline(void* dev,int textmode){HRESULT(WINAPI *SetRenderState)(void*,UINT,DWORD)=VT(dev,57,void*);HRESULT(WINAPI *SetTextureStageState)(void*,DWORD,UINT,DWORD)=VT(dev,67,void*);HRESULT(WINAPI *SetSamplerState)(void*,DWORD,UINT,DWORD)=VT(dev,69,void*);HRESULT(WINAPI *SetTexture)(void*,DWORD,void*)=VT(dev,65,void*);HRESULT(WINAPI *SetFVF)(void*,DWORD)=VT(dev,89,void*);HRESULT(WINAPI *SetVertexShader)(void*,void*)=VT(dev,92,void*);HRESULT(WINAPI *SetPixelShader)(void*,void*)=VT(dev,107,void*);SetVertexShader(dev,0);SetPixelShader(dev,0);SetFVF(dev,D3DFVF_FHD);SetRenderState(dev,7,0);SetRenderState(dev,14,0);SetRenderState(dev,22,1);SetRenderState(dev,27,1);SetRenderState(dev,19,5);SetRenderState(dev,20,6);SetRenderState(dev,137,0);SetRenderState(dev,174,0);SetTextureStageState(dev,0,1,textmode?4:2);SetTextureStageState(dev,0,2,textmode?2:0);SetTextureStageState(dev,0,3,0);SetTextureStageState(dev,0,4,textmode?4:2);SetTextureStageState(dev,0,5,textmode?2:0);SetTextureStageState(dev,0,6,0);SetTextureStageState(dev,1,1,1);SetTextureStageState(dev,1,4,1);SetSamplerState(dev,0,5,1);SetSamplerState(dev,0,6,1);SetSamplerState(dev,0,1,3);SetSamplerState(dev,0,2,3);SetTexture(dev,0,textmode?g_fontTexture:0);} 
static int visible_updates(void){return 5;} 
static int row_top(int row){return g_panel_y+34+row*44;}
static int total_rows(void){return g_update_count<0?0:g_update_count;} 
static int max_scroll(void){int m=total_rows()-visible_updates();return m<0?0:m;} 
static void clamp_scroll(void){int m=max_scroll();if(g_scroll<0)g_scroll=0; if(g_scroll>m)g_scroll=m;} 
static int hit_close(int x,int y){return x>=g_panel_x+420&&x<g_panel_x+444&&y>=g_panel_y+4&&y<g_panel_y+22;} 
static int hit_download(int x,int y){for(int i=0;i<visible_updates();i++){int idx=g_scroll+i;if(idx>=g_update_count)break;int top=row_top(i);if(x>=g_panel_x+340&&x<g_panel_x+430&&y>=top+8&&y<top+32)return idx;}return -1;} 
static void draw_panel(void* dev){
  if(!g_check_done||!g_open||g_update_count<=0)return; D3DVIEWPORT9 vp; HRESULT(WINAPI *GetViewport)(void*,D3DVIEWPORT9*)=VT(dev,48,void*); if(GetViewport(dev,&vp)<0||vp.Width<500||vp.Height<320)return; g_view_w=(int)vp.Width; g_view_h=(int)vp.Height; if(g_panel_x+450>g_view_w)g_panel_x=g_view_w-450; if(g_panel_x<0)g_panel_x=0; if(g_panel_y+280>g_view_h)g_panel_y=g_view_h-280; if(g_panel_y<0)g_panel_y=0; clamp_scroll(); if(!init_font(dev))return; void* st=0; HRESULT(WINAPI *CreateStateBlock)(void*,UINT,void**)=VT(dev,59,void*); if(CreateStateBlock(dev,1,&st)<0||!st)return; VT(st,4,HRESULT(WINAPI*)(void*))(st);
  setup_pipeline(dev,0); g_vert_count=0;
  addrect(g_panel_x,g_panel_y,450,280,RGBA(178,9,18,32),0,0,0,0); addrect(g_panel_x,g_panel_y,450,26,RGBA(225,20,38,62),0,0,0,0); addrect(g_panel_x+420,g_panel_y+2,24,20,RGBA(210,24,49,64),0,0,0,0);
  for(int i=0;i<visible_updates();i++){int idx=g_scroll+i; if(idx>=g_update_count)break; int top=row_top(i); addrect(g_panel_x+8,top,434,38,RGBA(210,24,39,56),0,0,0,0); addrect(g_panel_x+340,top+8,90,24,RGBA(235,30,76,91),0,0,0,0);} addrect(g_panel_x+1,g_panel_y+279,448,1,RGBA(235,74,145,183),0,0,0,0); drawverts(dev);
  setup_pipeline(dev,1); g_vert_count=0; fonttext(g_panel_x+12,g_panel_y+4,L"Обновления модов",RGBA(255,117,230,255),20); fonttext(g_panel_x+355,g_panel_y+4,L"Google",RGBA(255,159,220,198),8); fonttext(g_panel_x+427,g_panel_y+4,L"X",RGBA(255,255,203,120),1);
  for(int i=0;i<visible_updates();i++){int idx=g_scroll+i; if(idx>=g_update_count)break; int top=row_top(i); UpdateEntry*e=&g_updates[idx]; WCHAR line[96]; line[0]=0; wcat(line,96,e->id); fonttext(g_panel_x+16,top+4,line,RGBA(255,241,247,255),34); line[0]=0; wcat(line,96,L"Уст: "); wcat(line,96,e->local[0]?e->local:L"?"); wcat(line,96,L"  ->  Drive: "); wcat(line,96,e->remote[0]?e->remote:L"по дате"); fonttext(g_panel_x+16,top+20,line,RGBA(255,165,202,215),38); fonttext(g_panel_x+350,top+11,L"СКАЧАТЬ",RGBA(255,106,234,142),8);} if(g_update_count>visible_updates()){WCHAR n[24]=L"Ещё: "; WCHAR num[12]; wchar_num(num,12,(DWORD)(g_update_count-visible_updates()-g_scroll)); wcat(n,24,num); fonttext(g_panel_x+16,g_panel_y+256,n,RGBA(255,151,195,215),10);} fonttext(g_panel_x+150,g_panel_y+256,L"Проверка только при запуске игры",RGBA(255,112,148,169),32); drawverts(dev); VT(st,5,HRESULT(WINAPI*)(void*))(st); VT(st,2,DWORD(WINAPI*)(void*))(st);
}
static void map_mouse(HWND hwnd,int *x,int *y){RECT r; if(g_view_w>0&&g_view_h>0&&GetClientRect(hwnd,&r)&&r.right>0&&r.bottom>0){*x=*x*g_view_w/r.right;*y=*y*g_view_h/r.bottom;}}
static int inside(int x,int y){return g_check_done&&g_open&&g_update_count>0&&x>=g_panel_x&&x<g_panel_x+450&&y>=g_panel_y&&y<g_panel_y+280;}
static int overlay_message(HWND hwnd,UINT m,SIZE_T w,LONG l){ if(!g_check_done||!g_open||g_update_count<=0)return 0; if((m==WM_KEYDOWN) && w==VK_F12){g_open=!g_open; return 1;} if(m==WM_MOUSEMOVE){int mx=(short)(l&0xffff),my=(short)((l>>16)&0xffff);map_mouse(hwnd,&mx,&my);if(inside(mx,my))return 1;} if(m==WM_LBUTTONDOWN){int mx=(short)(l&0xffff),my=(short)((l>>16)&0xffff);map_mouse(hwnd,&mx,&my); if(inside(mx,my)){ if(hit_close(mx,my)){g_open=0; return 1;} int idx=hit_download(mx,my); if(idx>=0&&idx<g_update_count){open_browser(g_updates[idx].fileid); return 1;} return 1; }} if(m==WM_MOUSEWHEEL){int mx=(short)(l&0xffff),my=(short)((l>>16)&0xffff); map_mouse(hwnd,&mx,&my); if(inside(mx,my)){ short delta=(short)((w>>16)&0xffff); if(delta>0)g_scroll--; else if(delta<0)g_scroll++; clamp_scroll(); return 1; }} if((m==WM_LBUTTONUP||m==WM_LBUTTONDBLCLK)&&g_open)return 1; return 0; }
static LONG WINAPI game_proc(HWND hwnd,UINT m,SIZE_T w,LONG l){ if(overlay_message(hwnd,m,w,l))return 0; return g_prevproc?CallWindowProcW(g_prevproc,hwnd,m,w,l):0; }
static LONG WINAPI overlay_queue_hook(int code,SIZE_T removed,LONG param){ if(code>=0&&removed==1&&param){ CHATMSG* msg=(CHATMSG*)(SIZE_T)param; if(msg->hwnd==g_game && overlay_message(msg->hwnd,msg->message,msg->wParam,msg->lParam)){ msg->hwnd=0; msg->message=0; msg->wParam=0; msg->lParam=0; } } return CallNextHookEx(g_message_hook,code,removed,param); }
static void ensure_queue_hook(void){ if(g_queue_installed||!g_game)return; DWORD pid=0,tid=GetWindowThreadProcessId(g_game,&pid); if(!tid||pid!=GetCurrentProcessId())return; HMODULE module=0; if(!GetModuleHandleExW(6,(LPCWSTR)(SIZE_T)overlay_queue_hook,&module))return; g_message_hook=SetWindowsHookExW(3,overlay_queue_hook,module,tid); if(g_message_hook)g_queue_installed=1; }
static int render_signature_ok(const BYTE*p,DWORD global){ static const BYTE tail[]={0x50,0x8b,0x00,0xff,0x90,0xa8,0,0,0,0x6a,0,0x6a,0,0x6a,0,0x6a,0,0xa1}; if(p[0]!=0xa1||*(const DWORD*)(p+1)!=global)return 0; for(int i=0;i<18;i++)if(p[5+i]!=tail[i])return 0; if(*(const DWORD*)(p+23)!=global)return 0; static const BYTE present[]={0x50,0x8b,0x00,0xff,0x50,0x44}; for(int i=0;i<6;i++)if(p[27+i]!=present[i])return 0; return 1; }
static int is_supported_exe(void){ exe_base=0; g_device_global_rva=0; g_profile_index=-1; BYTE*b=(BYTE*)GetModuleHandleW(0); if(!b)return 0; if(b[0]!='M'||b[1]!='Z')return 0; DWORD nt=*(DWORD*)(b+0x3c); if(nt>0x1000||nt<0x40)return 0; BYTE*h=b+nt; if(h[0]!='P'||h[1]!='E'||h[2]||h[3])return 0; if(*(WORD*)(h+4)!=0x14c||*(WORD*)(h+20)<96||*(WORD*)(h+24)!=0x10b)return 0; DWORD stamp=*(DWORD*)(h+8),size=*(DWORD*)(h+24+56); for(int i=0;i<(int)(sizeof(g_profiles)/sizeof(g_profiles[0]));i++){ const FHDGameProfile*p=&g_profiles[i]; if(stamp!=p->timestamp||size!=p->image_size)continue; DWORD global=(DWORD)(SIZE_T)b+p->device_rva; for(int j=0;j<3;j++){ if(p->render_sites[j]>size-33||!render_signature_ok(b+p->render_sites[j],global))return 0; } exe_base=(DWORD)(SIZE_T)b; g_device_global_rva=p->device_rva; g_profile_index=i; return 1; } return 0; }
static void* real_device(void){return g_device_global_rva?*(void**)(exe_base+g_device_global_rva):0;}
static HRESULT WINAPI hook_end_scene(void* dev){ if(g_ready && dev==g_realdevice) draw_panel(dev); return callEnd(dev);} 
static HRESULT WINAPI hook_present(void* dev,const RECT* src,const RECT* dest,HWND override,const void* dirty){ return g_original_present?g_original_present(dev,src,dest,override,dirty):0; }
static int patch_ptr(void** addr,void* fn,void** original){ DWORD oldProtect=0,unused=0; if(!VirtualProtect(addr,sizeof(void*),PAGE_READWRITE,&oldProtect))return 0; *original=*addr; *addr=fn; VirtualProtect(addr,sizeof(void*),oldProtect,&unused); return 1; }
static int hook_device(void* dev){ void **vt=*(void***)dev; if(!vt)return 0; if(vt==g_hookvtable){g_realdevice=dev;return 1;} if(vt[42]==(void*)hook_end_scene&&vt[17]==(void*)hook_present){g_realdevice=dev;g_ready=1;return 1;} if(!vt[42]||!vt[17])return 0; g_original_end=(HRESULT(WINAPI*)(void*))vt[42]; g_original_present=(HRESULT(WINAPI*)(void*,const RECT*,const RECT*,HWND,const void*))vt[17]; void* prev; if(!patch_ptr(&vt[17],(void*)hook_present,&prev))return 0; if(!patch_ptr(&vt[42],(void*)hook_end_scene,&prev)){ void* unused; patch_ptr(&vt[17],(void*)g_original_present,&unused); return 0; } g_hookvtable=vt; g_realdevice=dev; g_ready=1; return 1; }
static DWORD WINAPI hook_thread(LPVOID param){ (void)param; if(!is_supported_exe())return 0; for(;;){ void* dev=real_device(); if(dev && (!g_ready||dev!=g_realdevice)){ if(hook_device(dev)){ if(!g_game){ D3DCREATION_PARAMETERS p; HRESULT(WINAPI *GetParams)(void*,D3DCREATION_PARAMETERS*)=VT(dev,9,void*); if(GetParams(dev,&p)>=0&&p.hFocusWindow){ DWORD pid=0; GetWindowThreadProcessId(p.hFocusWindow,&pid); if(pid==GetCurrentProcessId()){ g_game=p.hFocusWindow; g_prevproc=(WNDPROC)(SIZE_T)GetWindowLongW(g_game,GWL_WNDPROC); if(g_prevproc){ LONG previous=SetWindowLongW(g_game,GWL_WNDPROC,(LONG)(SIZE_T)game_proc); if(previous) ensure_queue_hook(); } } } } } } else if(g_game&&!g_queue_installed) ensure_queue_hook(); Sleep(250); } }
static int overlay_bootstrap(void){HANDLE t=CreateThread(0,0,hook_thread,0,0,0); if(!t)return 0; CloseHandle(t); return 1;}

EXP BOOL WINAPI XenoPlugin_Query(XenoPluginInfoV1*info){ if(!info||info->size<520)return FALSE; info->requiredHostApi=1; wcpy(info->id,64,L"FHD_UpdateCheck"); wcpy(info->version,32,L"0.3.1"); wcpy(info->description,160,L"Startup-only FHD update check with in-game download frame"); if(info->size>=sizeof(XenoPluginInfoV1))info->exclusiveCapabilities=0; return TRUE; }
EXP DWORD WINAPI XenoPlugin_Initialize(const XenoPluginHostV1*host){ if(!host||host->apiVersion<1||host->size<32||!host->gameRoot)return 1; if(InterlockedCompareExchange(&g_start_check_taken,1,0)!=0)return 0; WCHAR file[900],native[900],mod[900]; GetModuleFileNameW(g_module,file,900); directory_of(native,900,file); directory_of(mod,900,native); paths_init(host->gameRoot,mod); HANDLE thread=CreateThread(0,0,worker,0,0,0); if(thread)CloseHandle(thread); overlay_bootstrap(); return 0; }
BOOL WINAPI DllMain(HMODULE module,DWORD reason,LPVOID reserved){ (void)reserved; if(reason==1){g_module=module; DisableThreadLibraryCalls(module);} return TRUE; }
void *memcpy(void*d,const void*s,SIZE_T n){char*a=(char*)d;const char*b=(const char*)s;for(SIZE_T i=0;i<n;i++)a[i]=b[i];return d;} void *memset(void*d,int c,SIZE_T n){char*a=(char*)d;for(SIZE_T i=0;i<n;i++)a[i]=(char)c;return d;}

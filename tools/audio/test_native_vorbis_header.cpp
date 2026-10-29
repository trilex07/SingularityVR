// Own-process native Vorbis header validation; no game process or installed files modified.
#include <windows.h>
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.VC80.CRT' version='8.0.50727.9680' processorArchitecture='x86' publicKeyToken='1fc8b3b9a1e18e3b'\"")
#include <cstdio>
#include <cstdint>
#include <vector>
#include <string>
#include <cassert>
struct Callbacks {size_t (__cdecl*read)(void*,size_t,size_t,void*);int (__cdecl*seek)(void*,int64_t,int);int (__cdecl*close)(void*);long (__cdecl*tell)(void*);};
static size_t Read(void* p,size_t n,size_t k,void* f){return fread(p,n,k,(FILE*)f);}
static int Seek(void* f,int64_t n,int w){return _fseeki64((FILE*)f,n,w);}
static int Close(void* f){return fclose((FILE*)f);}static long Tell(void* f){return ftell((FILE*)f);}
int main(int argc,char**argv){setvbuf(stdout,0,_IONBF,0);if(argc!=3)return 2;SetDllDirectoryA(argv[1]);auto lib=LoadLibraryA((std::string(argv[1])+"/vorbisfile.dll").c_str());if(!lib)return 4;
 auto open=(int(__cdecl*)(void*,void*,const char*,long,Callbacks,unsigned))GetProcAddress(lib,"ov_open_callbacks");
 auto clear=(int(__cdecl*)(void*))GetProcAddress(lib,"ov_clear");if(!open||!clear)return 5;
 FILE*f=fopen(argv[2],"rb");if(!f)return 6;alignas(16) char state[65536]{};
 // UE3's shipped export explicitly requires sizeof-state = 0x2d8 at [esp+24h].
 int opened=open(f,state,0,0,{Read,Seek,Close,Tell},0x2d8);
 printf("ov_open_callbacks=%d\n",opened);
 if(opened){fclose(f);FreeLibrary(lib);return 3;}
 clear(state);FreeLibrary(lib);return 0;
}

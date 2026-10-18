#pragma once
#include <cstdio>
#include <cstring>

// Render-thread, opt-in. Two capture groups per process, fixed 512x256 BMPs.
// GPU downsamples before readback; no original target is rebound or written.
namespace mirrorprobe {
static bool Thumbnail(IDirect3DDevice9* dev,IDirect3DSurface9* source,
                      const char* base,unsigned group,const char* role) {
    if(!dev||!source||!base||!*base)return false;
    D3DSURFACE_DESC d{};
    if(FAILED(source->GetDesc(&d))||d.MultiSampleType!=D3DMULTISAMPLE_NONE||
       (d.Format!=D3DFMT_A8R8G8B8&&d.Format!=D3DFMT_X8R8G8B8))return false;
    constexpr unsigned w=512,h=256,row=w*4;
    IDirect3DSurface9 *thumb=nullptr,*cpu=nullptr;
    HRESULT hr=dev->CreateRenderTarget(w,h,d.Format,D3DMULTISAMPLE_NONE,0,FALSE,&thumb,nullptr);
    if(SUCCEEDED(hr))hr=dev->CreateOffscreenPlainSurface(w,h,d.Format,D3DPOOL_SYSTEMMEM,&cpu,nullptr);
    if(SUCCEEDED(hr))hr=dev->StretchRect(source,nullptr,thumb,nullptr,D3DTEXF_LINEAR);
    if(SUCCEEDED(hr))hr=dev->GetRenderTargetData(thumb,cpu);
    bool saved=false;
    if(SUCCEEDED(hr)) {
        D3DLOCKED_RECT lock{};
        hr=cpu->LockRect(&lock,nullptr,D3DLOCK_READONLY);
        if(SUCCEEDED(hr)) {
            char path[MAX_PATH]{};
            const char* slash=strrchr(base,'\\');
            const size_t n=slash?size_t(slash-base+1):0;
            if(n+100<sizeof(path)) {
                memcpy(path,base,n);
                _snprintf_s(path+n,sizeof(path)-n,_TRUNCATE,"mirror-probe-%lu-%u-%s.bmp",GetCurrentProcessId(),group,role);
                BITMAPFILEHEADER fh{};BITMAPINFOHEADER ih{};
                fh.bfType=0x4d42;fh.bfOffBits=sizeof(fh)+sizeof(ih);fh.bfSize=fh.bfOffBits+row*h;
                ih.biSize=sizeof(ih);ih.biWidth=w;ih.biHeight=-int(h);ih.biPlanes=1;ih.biBitCount=32;ih.biSizeImage=row*h;
                FILE* file=nullptr;
                if(fopen_s(&file,path,"wb")==0&&file) {
                    saved=fwrite(&fh,sizeof(fh),1,file)==1&&fwrite(&ih,sizeof(ih),1,file)==1;
                    for(unsigned y=0;y<h&&saved;++y)
                        saved=fwrite((const char*)lock.pBits+y*lock.Pitch,row,1,file)==1;
                    if(fclose(file)!=0)saved=false;
                }
                Log("mirror-pixel: group=%u role=%s source=%p %ux%u fmt=%d saved=%d path=%s",group,role,source,d.Width,d.Height,(int)d.Format,saved?1:0,path);
            }
            cpu->UnlockRect();
        }
    }
    if(FAILED(hr))Log("mirror-pixel: role=%s hr=%08lX",role,(unsigned long)hr);
    if(cpu)cpu->Release();if(thumb)thumb->Release();
    return saved;
}
struct Probe {
    bool enabled=false;
    unsigned stable[2]{},done=0;
    int pending=-1;
    void Before(IDirect3DDevice9* dev,IDirect3DSurface9* xrSource,
                const char* base,int menuClass,bool focused,bool curtain) {
        pending=-1;
        if(!enabled||!focused||curtain||!xrSource)return;
        const int group=menuClass==0?0:(menuClass==4?1:-1);
        if(group<0||(done&(1u<<group)))return;
        if(++stable[group]<180)return;
        // Mark even on failure: a failing driver never causes repeated readback.
        done|=1u<<group;pending=group;
        Thumbnail(dev,xrSource,base,(unsigned)group,"xr-source");
        IDirect3DSurface9* back=nullptr;
        if(SUCCEEDED(dev->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back))&&back) {
            Thumbnail(dev,back,base,(unsigned)group,"pre-present");back->Release();
        }
    }
    void After(IDirect3DDevice9* dev,const char* base,bool mirrored) {
        if(pending<0)return;
        IDirect3DSurface9* back=nullptr;
        if(SUCCEEDED(dev->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back))&&back) {
            Thumbnail(dev,back,base,(unsigned)pending,"post-mirror");back->Release();
        }
        Log("mirror-pixel: group=%d mirrorApplied=%d (pixels require inspection)",pending,mirrored?1:0);
        pending=-1;
    }
};
}

// Own-process D3D11/WARP pixel validation; never loads game/proxy/OpenXR.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include "../nis_bridge.h"
#include "../nis/NIS_Scaler_CS.h"
#include "../nis/NIS_Sharpen_CS.h"
#pragma comment(lib,"d3d11.lib")
static void require(bool b,const char* text) { if(!b){printf("FAIL %s\n",text);exit(1);} }
static void note(const char* s){puts(s);}
static std::vector<unsigned> readPixels(ID3D11Device* dev, ID3D11DeviceContext* ctx,
                                       ID3D11Texture2D* texture) {
    D3D11_TEXTURE2D_DESC rd{};texture->GetDesc(&rd);
    rd.Usage=D3D11_USAGE_STAGING;rd.BindFlags=0;rd.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    ID3D11Texture2D* read=nullptr;
    require(SUCCEEDED(dev->CreateTexture2D(&rd,nullptr,&read)),"readback");
    ctx->CopyResource(read,texture);D3D11_MAPPED_SUBRESOURCE mapped{};
    require(SUCCEEDED(ctx->Map(read,0,D3D11_MAP_READ,0,&mapped)),"map");
    std::vector<unsigned> result(rd.Width*rd.Height);
    for(UINT y=0;y<rd.Height;++y)
        memcpy(result.data()+y*rd.Width,(char*)mapped.pData+y*mapped.RowPitch,rd.Width*4);
    ctx->Unmap(read,0);read->Release();return result;
}
int main() {
    ID3D11Device* dev=nullptr; ID3D11DeviceContext* ctx=nullptr;
    D3D_FEATURE_LEVEL fl{};
    require(SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT,nullptr,0,D3D11_SDK_VERSION,&dev,&fl,&ctx)),"device");
    NisBridge b{};
    require(NisBridge_InitIfNeeded(&b,dev,kNisScalerCS,kNisScalerCS_Size,
        kNisSharpenCS,kNisSharpenCS_Size,nullptr,note),"embedded shaders");
    const UINT w=66,h=35; // non-group-aligned; seam and last pixel are checked
    std::vector<unsigned> pixels(w*2*h);
    for(UINT y=0;y<h;++y) for(UINT x=0;x<w*2;++x)
        pixels[y*w*2+x] = x<w ? 0xff406080u : 0xffb09070u;
    D3D11_TEXTURE2D_DESC td{};td.Width=w*2;td.Height=h;td.MipLevels=1;
    td.ArraySize=1;td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_B8G8R8A8_UNORM;
    td.BindFlags=D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA data{};data.pSysMem=pixels.data();data.SysMemPitch=w*2*4;
    ID3D11Texture2D* src=nullptr;
    require(SUCCEEDED(dev->CreateTexture2D(&td,&data,&src)),"source");
    for(int mode=1;mode<=3;++mode) {
        // mode3 exercises the old mismatch: requested sharpen, resized output.
        const UINT scale=mode==3 ? 2 : 1;
        b.mode=mode;b.sharpnessPct=100;
        if(mode==3) b.mode=nis_upscale::kNISSharpen;
        require(NisBridge_EnsureFrame(&b,dev,w*2,h,w*scale,h*scale),"output/UAV");
        require(NisBridge_ProcessStereo(&b,ctx,src,w*2,h,w*scale,h*scale),"dispatch");
        for(int eye=0;eye<2;++eye) {
            const auto values=readPixels(dev,ctx,b.rt.outTex[eye]);
            const unsigned expected=eye ? 0xffb09070u : 0xff406080u;
            for(unsigned value:values) for(int channel=0;channel<3;++channel) {
                const int shift=channel*8;
                require(abs((int)((value>>shift)&255)-(int)((expected>>shift)&255))<=2,
                    "constant colour/channel order/seam/bounds");
            }
        }
    }
    require(b.numStageCreates==1 && b.rt.numOutputCreates==2,"persistent lifetime");
    // Actual embedded shader output must change with strength; initialization
    // success alone does not establish sharpening. Both eyes get a soft stripe.
    for(UINT y=0;y<h;++y) for(UINT x=0;x<w*2;++x) {
        const unsigned gray=(unsigned)(128.0+60.0*std::sin(x*.7)*std::sin(y*.6));
        pixels[y*w*2+x]=0xff000000u|gray|(gray<<8)|(gray<<16);
    }
    ctx->UpdateSubresource(src,0,nullptr,pixels.data(),w*2*4,0);
    b.mode=nis_upscale::kNISSharpen;
    UINT outputW=0,outputH=0;
    NisBridge_OutputEye(&b,200,200,w,h,&outputW,&outputH);
    require(outputW==w && outputH==h,"sharpen does not rescale");
    require(NisBridge_EnsureFrame(&b,dev,w*2,h,w,h),"sharpen shape");
    b.sharpnessPct=0;
    require(NisBridge_ProcessStereo(&b,ctx,src,w*2,h,w,h),"low strength");
    auto low=readPixels(dev,ctx,b.rt.outTex[0]);
    b.sharpnessPct=100;
    require(NisBridge_ProcessStereo(&b,ctx,src,w*2,h,w,h),"high strength");
    auto high=readPixels(dev,ctx,b.rt.outTex[0]);
    unsigned changed=0;for(size_t i=0;i<low.size();++i) if(low[i]!=high[i]) ++changed;
    require(changed>100,"sharpness changes actual pixels");
    require(!NisBridge_ShouldEngage(&b,false,true),"menu refused");
    b.mode=0; require(!NisBridge_ShouldEngage(&b,true,true),"Off refused");
    b.mode=2;b.rt.failedOnce=true;
    require(!NisBridge_ShouldEngage(&b,true,true),"failed session refused");
    printf("Strength changed %u pixels\n",changed);
    NisBridge_Shutdown(&b);ctx->ClearState();src->Release();ctx->Release();dev->Release();
    puts("NIS pixel PASS (scaler/sharpen, isolated eyes, bounds, lifetime)");
}

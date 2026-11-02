#pragma once
#include <cmath>
#include <cstring>
#include <d3d9.h>

namespace cinemastereo {
inline bool CompletedFrame(bool enabled,bool nativeCinema,unsigned drawToken,
                           unsigned frameToken,bool menuFlat){
    return enabled&&nativeCinema&&drawToken==frameToken&&!menuFlat;
}
inline bool LegacyMatteNeeded(bool monoComposed,bool stereoComplete){
    return !monoComposed&&!stereoComplete;
}
// Authored/scripted camera sequences retain the game's original aim. A
// controller ray is unrelated to the animated weapon shown on the screen.
inline bool ControllerShotCorrection(bool nativeCamera){return !nativeCamera;}
// Storage convention is a renderer fact, not a live gameplay camera pose.
// Native-camera cinema intentionally bypasses gameplay lock verification; its
// 61-frame expiry must not invalidate a previously measured ROW/COL convention.
class NativeConvention {
    int measured_=-1;
public:
    void Observe(bool verified,int convention){
        if(verified&&(convention==0||convention==1))measured_=convention;
    }
    void Reset(){measured_=-1;}
    template<class Shape> int Resolve(Shape&& shape,float* tx,float* ty)const{
        if(measured_<0)return -1;
        return shape(measured_,tx,ty)?measured_:-1;
    }
};
// Same ROW/COL convention and order as ApplyOffset -> ApplyEyeRemap.
// A native ViewProjection matrix is authoritative: no HMD yaw/pitch/root motion.
inline bool EyeMatrix(const float* native,int conv,float displacement,int eye,float* output){
    if(!native||!output||(conv!=0&&conv!=1)||(eye!=0&&eye!=1)||!std::isfinite(displacement))return false;
    for(int i=0;i<16;++i)if(!std::isfinite(native[i]))return false;
    const int axis[3]={0,conv?1:4,conv?2:8};
    float r[3]={native[axis[0]],native[axis[1]],native[axis[2]]};
    const float n=std::sqrt(r[0]*r[0]+r[1]*r[1]+r[2]*r[2]);
    if(!(n>1e-6f))return false;
    for(float& v:r)v=v*displacement/n;
    std::memcpy(output,native,64);
    if(conv==0){
        for(int k=0;k<4;++k)output[12+k]-=r[0]*native[k]+r[1]*native[4+k]+r[2]*native[8+k];
        for(int k=0;k<4;++k)output[k*4]=.5f*output[k*4]+(eye?.5f:-.5f)*output[k*4+3];
    }else{
        for(int k=0;k<4;++k)output[k*4+3]-=r[0]*native[k*4]+r[1]*native[k*4+1]+r[2]*native[k*4+2];
        for(int k=0;k<4;++k)output[k]=.5f*output[k]+(eye?.5f:-.5f)*output[12+k];
    }
    return true;
}
// Shared by the runtime and real D3D9/D3D9Ex pixel fixtures. Native state is
// restored even when binding, drawing, or either eye fails.
template<class Setter,class Draw>
HRESULT Dispatch(IDirect3DDevice9* dev,Setter set,UINT reg,const float* native,
                 const float (&eyes)[2][16],const D3DVIEWPORT9& vp,const RECT& oldRect,
                 DWORD oldScissor,Draw&& draw){
    HRESULT result=S_OK;const LONG half=(LONG)vp.Width/2;
    for(int eye=0;eye<2;++eye){
        RECT rect={(LONG)vp.X+eye*half,(LONG)vp.Y,(LONG)vp.X+(eye+1)*half,(LONG)(vp.Y+vp.Height)};
        HRESULT hr=dev->SetRenderState(D3DRS_SCISSORTESTENABLE,TRUE);
        if(SUCCEEDED(hr))hr=dev->SetScissorRect(&rect);
        if(SUCCEEDED(hr))hr=set(dev,reg,eyes[eye],4);
        if(SUCCEEDED(hr))hr=draw();
        if(FAILED(hr))result=hr;
    }
    const HRESULT a=set(dev,reg,native,4),b=dev->SetScissorRect(&oldRect),c=dev->SetRenderState(D3DRS_SCISSORTESTENABLE,oldScissor);
    return FAILED(a)?a:FAILED(b)?b:FAILED(c)?c:result;
}
}

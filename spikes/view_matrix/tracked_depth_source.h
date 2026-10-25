#pragma once
#include <d3d9.h>

// Omitting rasterization must not omit the original wrist-pose producer.
// Called ONLY after trackedprime::Cache proved an exact retained depth sibling.
// This reads the same original material-0 constants as PrepareIndependentLeft;
// it never edits GPU state or performs independent-hand/isolation transforms.
namespace trackedsource {
struct Input {
    bool enabled=false,omitted=false,eligible=false,shadow=false;
    unsigned verts=0,first=0,triangles=0;
    int bones=-1,boneRegs=0,local=-1;
};
struct Source { float local[4][4]{},left[3][4]{},right[3][4]{}; };
inline const char* Gate(const Input& i) {
    if(!i.enabled)return "off";
    if(!i.omitted)return "not-omitted-exact-sibling";
    if(!i.eligible)return "action-context";
    if(i.shadow)return "shadow";
    if((i.verts!=7444&&i.verts!=2626)||i.first!=0||i.triangles!=4402)return "not-material0";
    if(i.bones!=5||i.boneRegs!=225||i.local!=230)return "CTAB-layout";
    return nullptr;
}
inline const char* Read(IDirect3DDevice9* device,const Input& i,Source& out) {
    if(const char* gate=Gate(i))return gate;
    Source original;
    // Original cooked material-0 palette: wrist9 at slot1, wrist51 at slot19.
    if(!device||FAILED(device->GetVertexShaderConstantF(i.local,&original.local[0][0],4))||
       FAILED(device->GetVertexShaderConstantF(i.bones+1*3,&original.left[0][0],3))||
       FAILED(device->GetVertexShaderConstantF(i.bones+19*3,&original.right[0][0],3)))
        return "constant-read";
    out=original;return nullptr;
}
inline float Difference(const float* a,const float* b,unsigned count) {
    float maximum=0;
    for(unsigned n=0;n<count;++n){const float d=a[n]>b[n]?a[n]-b[n]:b[n]-a[n];if(d>maximum)maximum=d;}
    return maximum;
}
} // namespace trackedsource

// Separate TU: actual injector functions, no DllMain/init thread/MinHook entry.
#define DllMain TextureFixtureDllMainNotCalled
#include "../texture_injector.cpp"
extern "C" bool FixtureHash(IDirect3DTexture9* t,uint64_t* out){D3DSURFACE_DESC d{};return HashLevel0(t,d,*out);}
extern "C" bool FixtureRed(IDirect3DDevice9* d,IDirect3DTexture9** out){
    g_bridgeOwned=true;g_enabled=true;
    DDSImage image;image.w=image.h=4;image.levels=1;image.fmt=D3DFMT_DXT1;image.mip={{0,0xf8,0,0xf8,0,0,0,0}};
    return CreateReplacement(d,image,out);
}
extern "C" bool FixtureStore(IDirect3DTexture9* a,IDirect3DTexture9* b){return texpositive::Store(a,b);}
extern "C" bool FixtureHasReplacement(IDirect3DTexture9* t){auto* a=texpositive::Acquire(t);if(a)a->Release();return a!=nullptr;}

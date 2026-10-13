#pragma once
#include <new>
#include "../view_matrix/texture_bridge_contract.h"
namespace texpositive {
// No AddRef of original texture: avoids a source->metadata->source cycle.
// Replacement is owned until original is released, invalidated or overwritten.
class Held final:public IUnknown {
    volatile LONG refs_=1;
    IDirect3DTexture9* replacement_;
public:
    explicit Held(IDirect3DTexture9* t):replacement_(t){t->AddRef();}
    ~Held(){replacement_->Release();}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** out) override {
        if(!out)return E_POINTER;*out=nullptr;
        if(id==__uuidof(IUnknown)){*out=this;AddRef();return S_OK;}return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override{return InterlockedIncrement(&refs_);}
    ULONG STDMETHODCALLTYPE Release() override{LONG n=InterlockedDecrement(&refs_);if(!n)delete this;return n;}
    IDirect3DTexture9* Acquire(){replacement_->AddRef();return replacement_;}
};
inline bool Store(IDirect3DTexture9* original,IDirect3DTexture9* replacement){
    if(!original||!replacement||original==replacement)return false;
    original->FreePrivateData(texbridge::nativeId);
    auto* held=new(std::nothrow) Held(replacement);if(!held)return false;
    HRESULT hr=original->SetPrivateData(texbridge::positiveId,held,sizeof(IUnknown*),D3DSPD_IUNKNOWN);
    held->Release();return SUCCEEDED(hr);
}
// Only this private GUID is used; the record is our own IUnknown implementation.
inline IDirect3DTexture9* Acquire(IDirect3DTexture9* original){
    if(!original)return nullptr;Held* held=nullptr;DWORD size=sizeof(IUnknown*);
    if(FAILED(original->GetPrivateData(texbridge::positiveId,&held,&size))||!held)return nullptr;
    IDirect3DTexture9* result=held->Acquire();held->Release();return result;
}
}

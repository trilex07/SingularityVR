#pragma once
// One render-thread frame, read-only. No depth copies, locks, readbacks,
// shader dumps, GPU waits, or logging from draw hooks. Get* references are
// retained until the next Present (or Reset); every path releases them.
namespace wristtrace {
struct Event {
    const char* kind=nullptr; unsigned draw=0; int eye=-1,phase=0,moved=0,reg=-1,conv=0;
    DWORD flags=0; float clearZ=0; HRESULT hr=E_PENDING;
    IDirect3DSurface9* rt=nullptr; IDirect3DSurface9* depth=nullptr;
    D3DSURFACE_DESC rd{},dd{}; D3DVIEWPORT9 vp{}; RECT sc{};
    HRESULT rtHr=E_FAIL,depthHr=E_FAIL,vpHr=E_FAIL,scHr=E_FAIL,matrixHr=E_FAIL;
    float matrix[16]{}; ULONGLONG wrist=0,controller=0;
};
constexpr unsigned Capacity=64;
static Event events[Capacity]; static unsigned count=0,overflow=0;
static bool active=false,done=false; static unsigned stable=0;
static unsigned seen[2]{};
inline void Release(){
    for(unsigned i=0;i<count;++i){if(events[i].rt)events[i].rt->Release();
        if(events[i].depth)events[i].depth->Release();events[i]={};}
    count=overflow=0;active=false;seen[0]=seen[1]=0;
}
inline void Reset(){Release();stable=0;} // Do not silently rearm a completed trace.
inline Event* Capture(IDirect3DDevice9* dev,const char* kind,unsigned draw,int eye,
                     int phase,int moved,int reg=-1,int conv=0){
    if(!active)return nullptr;
    if(count==Capacity){++overflow;return nullptr;}
    auto& e=events[count++];e={};e.kind=kind;e.draw=draw;e.eye=eye;
    e.phase=phase;e.moved=moved;e.reg=reg;e.conv=conv;
    e.rtHr=dev->GetRenderTarget(0,&e.rt);e.depthHr=dev->GetDepthStencilSurface(&e.depth);
    if(e.rt)e.rt->GetDesc(&e.rd);if(e.depth)e.depth->GetDesc(&e.dd);
    e.vpHr=dev->GetViewport(&e.vp);e.scHr=dev->GetScissorRect(&e.sc);
    if(reg>=0&&reg<=252)e.matrixHr=dev->GetVertexShaderConstantF(reg,e.matrix,4);
    return &e;
}
inline Event* Eye(IDirect3DDevice9* dev,unsigned draw,int eye,int phase,int moved,int reg,int conv){
    if(!active||eye<0||eye>1)return nullptr;
    // First world and foreground draw for each eye after EACH depth clear.
    // No D3D query on other draws, or at all with the trace disabled.
    const unsigned bit=1u<<(phase?1:0);
    if(seen[eye]&bit)return nullptr;seen[eye]|=bit;
    return Capture(dev,"eye-draw",draw,eye,phase,moved,reg,conv);
}
inline Event* Clear(IDirect3DDevice9* dev,unsigned draw,int phase,DWORD flags,float z){
    if(!active||!(flags&D3DCLEAR_ZBUFFER))return nullptr;
    seen[0]=seen[1]=0;
    auto* e=Capture(dev,"before-depth-clear",draw,-1,phase,0);
    if(e){e->flags=flags;e->clearZ=z;}return e;
}
inline void Wrist(IDirect3DDevice9* dev,unsigned draw,ULONGLONG sample,ULONGLONG controller,
                  const float* tracking=nullptr){
    if(!active)return;
    // One publication, after the final successful two-eye arm submission.
    for(unsigned i=0;i<count;++i)if(events[i].kind&&
        events[i].kind[0]=='w')return;
    auto* e=Capture(dev,"wrist-published",draw,-1,1,0);
    if(e){e->wrist=sample;e->controller=controller;
        if(tracking){memcpy(e->matrix,tracking,sizeof(e->matrix));e->matrixHr=S_OK;}}
}
inline void Boundary(IDirect3DDevice9* dev,bool enabled,bool ready,unsigned frame){
    if(active){
        Capture(dev,"present",0,-1,0,0);
        active=false;done=true;
        Log("wrist-depth trace: frame=%u events=%u overflow=%u READ_ONLY; identities/order are not pixel occlusion proof",frame,count,overflow);
        for(unsigned i=0;i<count;++i){const auto& e=events[i];
            Log("wrist-depth event=%u kind=%s draw=%u eye=%d fg=%d moved=%d RT=%p rtHr=%08lX size=%ux%u fmt=%u DS=%p dsHr=%08lX dsSize=%ux%u dsFmt=%u ms=%u quality=%lu flags=%lu z=%.9g hr=%08lX",
                i,e.kind,e.draw,e.eye,e.phase,e.moved,e.rt,e.rtHr,e.rd.Width,e.rd.Height,(unsigned)e.rd.Format,
                e.depth,e.depthHr,e.dd.Width,e.dd.Height,(unsigned)e.dd.Format,(unsigned)e.dd.MultiSampleType,e.dd.MultiSampleQuality,e.flags,e.clearZ,e.hr);
            Log("wrist-depth state=%u vpHr=%08lX vp=%lu,%lu,%lu,%lu,%.9g,%.9g scHr=%08lX sc=%ld,%ld,%ld,%ld c=%d conv=%d matrixHr=%08lX wristMs=%llu controllerMs=%llu",
                i,e.vpHr,e.vp.X,e.vp.Y,e.vp.Width,e.vp.Height,e.vp.MinZ,e.vp.MaxZ,e.scHr,e.sc.left,e.sc.top,e.sc.right,e.sc.bottom,
                e.reg,e.conv,e.matrixHr,(unsigned long long)e.wrist,(unsigned long long)e.controller);
            if(SUCCEEDED(e.matrixHr))for(unsigned r=0;r<4;++r)
                Log("wrist-depth matrix=%u row=%u %.9g %.9g %.9g %.9g",i,r,e.matrix[r*4],e.matrix[r*4+1],e.matrix[r*4+2],e.matrix[r*4+3]);
        }
        Release();
    }
    if(!enabled||done){stable=0;return;}
    stable=ready?stable+1:0;
    if(stable>=18){Release();active=true;Log("wrist-depth trace: armed ONE frame=%u (no depth mutation)",frame);}
}
}

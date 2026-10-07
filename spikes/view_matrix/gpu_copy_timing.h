#pragma once
// GPU timestamps are harvested later without flushing or waiting. No disk I/O per frame.
struct GpuCopyTiming {
    struct Slot { ID3D11Query *disjoint=nullptr, *begin=nullptr, *end=nullptr; bool pending=false; } slots[8];
    ID3D11DeviceContext* owner=nullptr;
    int active=-1; unsigned reports=0, samples=0, skipped=0;
    double sum=0, peak=0;
    void Reset() {
        for(auto& s:slots) { if(s.disjoint)s.disjoint->Release(); if(s.begin)s.begin->Release(); if(s.end)s.end->Release(); s=Slot{}; }
        owner=nullptr; active=-1;
    }
    void Begin(ID3D11DeviceContext* ctx) {
        if(!ctx)return;
        if(owner!=ctx) {
            Reset(); owner=ctx; ID3D11Device* dev=nullptr;ctx->GetDevice(&dev);
            for(auto& s:slots) {
                D3D11_QUERY_DESC q{D3D11_QUERY_TIMESTAMP_DISJOINT,0};
                if(FAILED(dev->CreateQuery(&q,&s.disjoint)))break;
                q.Query=D3D11_QUERY_TIMESTAMP;
                if(FAILED(dev->CreateQuery(&q,&s.begin)) || FAILED(dev->CreateQuery(&q,&s.end)))break;
            }
            dev->Release();
        }
        active=-1;
        for(int i=0;i<8;++i) {
            Slot& s=slots[i];
            if(!s.disjoint || !s.begin || !s.end)continue;
            if(s.pending) {
                D3D11_QUERY_DATA_TIMESTAMP_DISJOINT f{}; UINT64 a=0,b=0;
                const UINT flags=D3D11_ASYNC_GETDATA_DONOTFLUSH;
                const HRESULT ready=ctx->GetData(s.disjoint,&f,sizeof(f),flags);
                if(ready==S_OK && ctx->GetData(s.begin,&a,sizeof(a),flags)==S_OK && ctx->GetData(s.end,&b,sizeof(b),flags)==S_OK) {
                    s.pending=false;
                    if(!f.Disjoint && f.Frequency && b>=a) {double ms=double(b-a)*1000.0/f.Frequency;sum+=ms;if(ms>peak)peak=ms;++samples;}
                } else if(FAILED(ready)) s.pending=false;
            }
            if(active<0 && !s.pending)active=i;
        }
        if(++reports%600==0) {
            Log("PerformanceBuild1 GPU XR-copy span: samples=%u avg_ms=%.4f max_ms=%.4f skipped=%u (async; not total game GPU time)",samples,samples?sum/samples:0.0,peak,skipped);
            samples=0;sum=peak=0;skipped=0;
        }
        if(active<0){++skipped;return;}
        Slot& s=slots[active];ctx->Begin(s.disjoint);ctx->End(s.begin);
    }
    void End(ID3D11DeviceContext* ctx) {
        if(active<0 || owner!=ctx)return;
        Slot& s=slots[active];ctx->End(s.end);ctx->End(s.disjoint);s.pending=true;active=-1;
    }
};
static GpuCopyTiming g_gpuCopyTiming;

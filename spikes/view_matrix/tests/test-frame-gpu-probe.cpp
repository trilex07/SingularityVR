#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cassert>
#include <cstdio>
#include <vector>
enum {D3DQUERYTYPE_TIMESTAMP=10,D3DQUERYTYPE_TIMESTAMPDISJOINT=11,D3DQUERYTYPE_TIMESTAMPFREQ=12,
    D3DISSUE_BEGIN=2,D3DISSUE_END=1};
#include "../frame_gpu_probe.h"
static int live=0,creates=0,releases=0,openRegions=0;
struct Query {
    int type;bool end=false,ready=true,error=false,broken=false;
    std::uint64_t value=0;
    explicit Query(int t,std::uint64_t v):type(t),value(v){++live;}
    HRESULT Issue(DWORD flags){
        assert(flags==D3DISSUE_BEGIN||flags==D3DISSUE_END);
        if(type==D3DQUERYTYPE_TIMESTAMPDISJOINT){
            if(flags==D3DISSUE_BEGIN){assert(!end);++openRegions;end=true;}
            else {assert(end);--openRegions;end=false;}
        }else assert(flags==D3DISSUE_END);
        return S_OK;
    }
    HRESULT GetData(void* out,DWORD size,DWORD flags){
        assert(flags==0);if(error)return E_FAIL;if(!ready)return S_FALSE;
        if(type==D3DQUERYTYPE_TIMESTAMPDISJOINT){assert(size==sizeof(BOOL));*(BOOL*)out=broken?TRUE:FALSE;}
        else {assert(size==sizeof(std::uint64_t));*(std::uint64_t*)out=value;}
        return S_OK;
    }
    void Release(){++releases;--live;delete this;}
};
struct Device {
    std::vector<Query*> queries;int failAt=-1,timestamps=0;
    HRESULT CreateQuery(int type,Query** out){
        *out=nullptr;if(creates++==failAt)return E_FAIL;
        const auto value=type==D3DQUERYTYPE_TIMESTAMPFREQ?1000000ull:
            type==D3DQUERYTYPE_TIMESTAMP?((timestamps++%2)==0?1000ull:14000ull):0ull;
        *out=new Query(type,value);queries.push_back(*out);return S_OK;
    }
};
using Probe=framegpu::Probe<Device,Query>;
int main(){
    {
        Device d;Probe p;assert(p.Prepare(&d)&&live==16);
        for(unsigned frame=0;frame<9;++frame){p.Begin(frame);p.End(true,3802,13.9,1000.0/90);p.Poll();}
        assert(p.issued==2&&p.over.samples==2&&p.within.samples==0);
        assert(std::abs(p.over.gpu/2-13)<1e-10&&p.over.maxDraws==3802);
        assert(p.over.worstWall.draws==3802&&std::abs(p.over.worstWall.gpuMs-13)<1e-10);
        assert(openRegions==0);p.Release();assert(live==0);
    }
    {
        Device d;Probe p;assert(p.Prepare(&d));for(auto q:d.queries)q->ready=false;
        for(unsigned frame=0;frame<33;++frame){p.Begin(frame);p.End(true,100,11.11,11.11);p.Poll();}
        assert(p.issued==4&&p.skipped==1&&p.within.samples==0&&p.over.samples==0);
        for(auto q:d.queries)q->ready=true;p.Poll();assert(p.within.samples==4);
        p.Poll();assert(p.within.samples==4); // no double-consumption
        p.Release();assert(live==0&&openRegions==0);
    }
    {
        Device d;Probe p;assert(p.Prepare(&d));p.Begin(1);p.End(false,3,11,11);p.Poll();
        assert(p.invalid==1&&p.within.samples==0&&p.over.samples==0);p.Release();
    }
    {
        Device d;Probe p;assert(p.Prepare(&d));d.queries[2]->broken=true;
        p.Begin(1);p.End(true,3,11,11);p.Poll();assert(p.invalid==1);p.Release();
    }
    {
        Device d;Probe p;assert(p.Prepare(&d));d.queries[3]->value=0;
        p.Begin(1);p.End(true,3,11,11);p.Poll();assert(p.invalid==1);p.Release();
    }
    {
        Device d;Probe p;assert(p.Prepare(&d));d.queries[1]->value=1;
        p.Begin(1);p.End(true,3,11,11);p.Poll();assert(p.invalid==1);p.Release();
    }
    {
        Device d;Probe p;assert(p.Prepare(&d));p.Begin(1);assert(openRegions==1);
        p.Release();assert(openRegions==0&&live==0); // Reset with a recording in flight
        assert(p.Prepare(&d));p.Release();assert(live==0);
    }
    {
        Device d;Probe p;d.failAt=creates+6;assert(!p.Prepare(&d));
        assert(p.Unavailable()&&live==0);const int attempts=creates;
        assert(!p.Prepare(&d)&&creates==attempts); // unsupported driver: no retry storm
    }
    {
        Device d;Probe p;assert(p.Prepare(&d));d.queries[1]->error=true;
        p.Begin(1);p.End(true,3,11,11);p.Poll();assert(p.failed==1&&p.Unavailable());
        p.Release();assert(live==0&&openRegions==0);
    }
    std::puts("frame GPU probe: PASS (async, full ring, invalid/disjoint/failure, Reset and ownership)");
}

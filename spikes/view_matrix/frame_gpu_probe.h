#pragma once
#include <cstdint>
#include <cmath>

// D3D9-compatible types are supplied by the includer (CPU tests use fakes).
// All methods execute on the existing render thread. No state changes, waits,
// flushes, file I/O or hot-path allocations. Timestamp span includes GPU bubbles.
namespace framegpu {
struct Sample {
    std::uint64_t frame=0;
    unsigned draws=0;
    double gpuMs=0,submissionMs=0,wallMs=0,periodMs=0;
};
struct Bucket {
    unsigned samples=0;double gpu=0,submission=0,wall=0,maxGpu=0;unsigned maxDraws=0;
    Sample worstWall{};
    void Add(const Sample& s){++samples;gpu+=s.gpuMs;submission+=s.submissionMs;wall+=s.wallMs;
        if(s.gpuMs>maxGpu)maxGpu=s.gpuMs;if(s.draws>maxDraws)maxDraws=s.draws;
        if(samples==1||s.wallMs>worstWall.wallMs)worstWall=s;}
};
template<class Device,class Query> class Probe {
    struct Slot {
        Query *begin=nullptr,*end=nullptr,*disjoint=nullptr,*frequency=nullptr;
        bool pending=false;Sample sample{};
    } slots[4];
    int recording=-1;bool prepared=false,unavailable=false;
    unsigned cadence=0;LARGE_INTEGER started{},clock{};
public:
    Probe()=default;
    Probe(const Probe&)=delete;Probe& operator=(const Probe&)=delete;
    Bucket within{},over{};unsigned skipped=0,invalid=0,failed=0,issued=0;
    bool Unavailable()const{return unavailable;}
    bool Recording()const{return recording>=0;}
    bool Prepare(Device* dev){
        if(prepared||unavailable)return prepared;
        QueryPerformanceFrequency(&clock);
        if(clock.QuadPart<=0){unavailable=true;return false;}
        for(auto& s:slots){
            if(FAILED(dev->CreateQuery(D3DQUERYTYPE_TIMESTAMP,&s.begin))||
               FAILED(dev->CreateQuery(D3DQUERYTYPE_TIMESTAMP,&s.end))||
               FAILED(dev->CreateQuery(D3DQUERYTYPE_TIMESTAMPDISJOINT,&s.disjoint))||
               FAILED(dev->CreateQuery(D3DQUERYTYPE_TIMESTAMPFREQ,&s.frequency))){
                Release();unavailable=true;return false;}
        }
        prepared=true;return true;
    }
    void Release(){
        for(auto& s:slots){
            // End a begun diagnostic region before relinquishing its queries.
            if(&s== (recording>=0?&slots[recording]:nullptr) && s.disjoint)s.disjoint->Issue(D3DISSUE_END);
            if(s.begin)s.begin->Release();if(s.end)s.end->Release();
            if(s.disjoint)s.disjoint->Release();if(s.frequency)s.frequency->Release();s={};
        }
        recording=-1;prepared=false;unavailable=false;cadence=0;
    }
    // Called ONCE per frame, by the first game draw; caller disarms its gate.
    void Begin(std::uint64_t frame){
        if(!prepared||unavailable||recording>=0)return;
        if(cadence++%8)return;
        for(unsigned i=0;i<4;++i)if(!slots[i].pending){
            auto& s=slots[i];
            if(FAILED(s.disjoint->Issue(D3DISSUE_BEGIN))){++failed;unavailable=true;return;}
            if(FAILED(s.begin->Issue(D3DISSUE_END))){s.disjoint->Issue(D3DISSUE_END);++failed;unavailable=true;return;}
            s.sample={};s.sample.frame=frame;recording=(int)i;
            QueryPerformanceCounter(&started);return;
        }
        ++skipped; // Full ring: drop sample, never wait for it.
    }
    void End(bool eligible,unsigned draws,double wallMs,double periodMs){
        if(recording<0)return;
        auto& s=slots[recording];recording=-1;
        LARGE_INTEGER now{};QueryPerformanceCounter(&now);
        // Always balance the disjoint region, including failed timestamp issue.
        const HRESULT a=s.end->Issue(D3DISSUE_END),b=s.disjoint->Issue(D3DISSUE_END),
            c=s.frequency->Issue(D3DISSUE_END);
        if(FAILED(a)||FAILED(b)||FAILED(c)){++failed;unavailable=true;return;}
        s.sample.draws=draws;s.sample.submissionMs=double(now.QuadPart-started.QuadPart)*1000/clock.QuadPart;
        s.sample.wallMs=wallMs;s.sample.periodMs=periodMs;
        // Zero period invalidates a menu/cinema/transition sample after draining.
        if(!eligible)s.sample.periodMs=0;
        s.pending=true;++issued;
    }
    // At most four slots / sixteen fixed-size GetData calls per Present.
    // S_FALSE is never spun on.
    void Poll(){
        for(auto& s:slots)if(s.pending){
            std::uint64_t a=0,b=0,hz=0;BOOL disjoint=TRUE;
            const HRESULT end=s.end->GetData(&b,sizeof(b),0);
            if(end==S_FALSE)continue;
            const HRESULT begin=s.begin->GetData(&a,sizeof(a),0),
                freq=s.frequency->GetData(&hz,sizeof(hz),0),
                joint=s.disjoint->GetData(&disjoint,sizeof(disjoint),0);
            if(FAILED(end)||FAILED(begin)||FAILED(freq)||FAILED(joint)){
                s.pending=false;++failed;unavailable=true;continue;}
            if(begin!=S_OK||freq!=S_OK||joint!=S_OK)continue;
            s.pending=false;
            if(disjoint||!hz||b<a||s.sample.periodMs<=0||s.sample.wallMs<=0){++invalid;continue;}
            s.sample.gpuMs=double(b-a)*1000/double(hz);
            if(!std::isfinite(s.sample.gpuMs)||s.sample.gpuMs>1000){++invalid;continue;}
            (s.sample.wallMs>s.sample.periodMs*1.1?over:within).Add(s.sample);
        }
    }
};
}

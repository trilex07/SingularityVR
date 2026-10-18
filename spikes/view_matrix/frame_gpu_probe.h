#pragma once
#include <cstdint>
#include <cmath>

// D3D9-compatible types are supplied by the includer (CPU tests use fakes).
// All methods execute on the existing render thread. No state changes, waits,
// flushes, file I/O or hot-path allocations. Timestamp span includes GPU bubbles.
namespace framegpu {
inline constexpr unsigned maxPasses=64;
struct ShaderPair {std::uint64_t vs=0,ps=0;unsigned draws=0;};
struct Pass {
    std::uintptr_t target=0;
    unsigned draws=0,userPointerDraws=0;
    double gpuMs=0,submissionMs=0;
    int kind=-1; // -1 no draws yet,0 buffered,1 user-pointer. Never mix kinds.
    ShaderPair pairs[4]{};unsigned unknownPairs=0;
};
struct Sample {
    std::uint64_t frame=0;
    unsigned draws=0;
    double gpuMs=0,submissionMs=0,wallMs=0,periodMs=0;
    Pass passes[maxPasses]{};unsigned passCount=0;bool passesComplete=false;
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
        Query* boundaries[maxPasses-1]{};
        bool pending=false;Sample sample{};
        LARGE_INTEGER passStarted{};bool passOverflow=false;
    } slots[4];
    int recording=-1;bool prepared=false,unavailable=false,passCapture=false;
    unsigned cadence=0;LARGE_INTEGER started{},clock{};
public:
    Probe()=default;
    Probe(const Probe&)=delete;Probe& operator=(const Probe&)=delete;
    Bucket within{},over{};unsigned skipped=0,invalid=0,failed=0,issued=0;
    bool Unavailable()const{return unavailable;}
    bool Recording()const{return recording>=0;}
    bool CapturePasses(bool on){if(prepared||Recording())return false;passCapture=on;return true;}
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
            if(passCapture)for(auto& boundary:s.boundaries)
                if(FAILED(dev->CreateQuery(D3DQUERYTYPE_TIMESTAMP,&boundary))){Release();unavailable=true;return false;}
        }
        prepared=true;return true;
    }
    void Release(){
        for(auto& s:slots){
            // End a begun diagnostic region before relinquishing its queries.
            if(&s== (recording>=0?&slots[recording]:nullptr) && s.disjoint)s.disjoint->Issue(D3DISSUE_END);
            if(s.begin)s.begin->Release();if(s.end)s.end->Release();
            if(s.disjoint)s.disjoint->Release();if(s.frequency)s.frequency->Release();
            for(auto boundary:s.boundaries)if(boundary)boundary->Release();s={};
        }
        recording=-1;prepared=false;unavailable=false;cadence=0;
    }
    // Called ONCE per frame, by the first game draw; caller disarms its gate.
    void Begin(std::uint64_t frame){
        if(!prepared||unavailable||recording>=0)return;
        // Whole-frame survey uses1/8. Focused pass capture observes EVERY
        // frame in the externally bounded window, so a one-frame burst is not
        // silently missed. Same four-slot drop-not-wait contract.
        if(cadence++%(passCapture?1u:8u))return;
        for(unsigned i=0;i<4;++i)if(!slots[i].pending){
            auto& s=slots[i];
            if(FAILED(s.disjoint->Issue(D3DISSUE_BEGIN))){++failed;unavailable=true;return;}
            if(FAILED(s.begin->Issue(D3DISSUE_END))){s.disjoint->Issue(D3DISSUE_END);++failed;unavailable=true;return;}
            s.sample={};s.sample.frame=frame;s.passOverflow=false;recording=(int)i;
            QueryPerformanceCounter(&started);return;
        }
        ++skipped; // Full ring: drop sample, never wait for it.
    }
    // Existing successful RT0 setter / game draw seams ONLY. A target is an
    // opaque observed identity, never dereferenced or retained. No Get* queries.
    void Target(std::uintptr_t target,int kind=-1){
        if(!passCapture||recording<0)return;
        auto& s=slots[recording];auto& sample=s.sample;
        if(!sample.passCount){sample.passCount=1;sample.passes[0].target=target;sample.passes[0].kind=kind;s.passStarted=started;return;}
        auto& last=sample.passes[sample.passCount-1];
        if(s.passOverflow)return;
        if(last.target==target&&(kind<0||last.kind<0||last.kind==kind)){
            if(kind>=0)last.kind=kind;return;
        }
        if(sample.passCount==maxPasses){s.passOverflow=true;return;}
        if(FAILED(s.boundaries[sample.passCount-1]->Issue(D3DISSUE_END))){s.passOverflow=true;++failed;unavailable=true;return;}
        LARGE_INTEGER now{};QueryPerformanceCounter(&now);
        last.submissionMs=double(now.QuadPart-s.passStarted.QuadPart)*1000/clock.QuadPart;s.passStarted=now;
        auto& next=sample.passes[sample.passCount++];next.target=target;next.kind=kind;
    }
    void Draw(std::uintptr_t target,bool userPointer,std::uint64_t vs=0,std::uint64_t ps=0){
        if(!passCapture||recording<0)return;
        Target(target,userPointer?1:0);auto& s=slots[recording];
        if(s.passOverflow)return;
        auto& pass=s.sample.passes[s.sample.passCount-1];++pass.draws;if(userPointer)++pass.userPointerDraws;
        for(auto& pair:pass.pairs)if(!pair.draws||(pair.vs==vs&&pair.ps==ps)){
            pair.vs=vs;pair.ps=ps;++pair.draws;return;
        }
        ++pass.unknownPairs; // Fixed census, never claim these four cover every shader.
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
        if(s.sample.passCount){
            auto& pass=s.sample.passes[s.sample.passCount-1];
            pass.submissionMs=double(now.QuadPart-s.passStarted.QuadPart)*1000/clock.QuadPart;
        }
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
            bool boundaryReady=true,boundaryFailed=false;
            std::uint64_t edges[maxPasses+1]{};edges[0]=a;
            if(passCapture&&!s.passOverflow&&s.sample.passCount){
                for(unsigned i=1;i<s.sample.passCount;++i){
                    const HRESULT edge=s.boundaries[i-1]->GetData(&edges[i],sizeof(edges[i]),0);
                    boundaryReady=boundaryReady&&(edge==S_OK);boundaryFailed=boundaryFailed||FAILED(edge);
                }
                edges[s.sample.passCount]=b;
                if(boundaryFailed){s.pending=false;++failed;unavailable=true;continue;}
                if(!boundaryReady)continue;
            }
            s.pending=false;
            if(disjoint||!hz||b<a||s.sample.periodMs<=0||s.sample.wallMs<=0){++invalid;continue;}
            s.sample.gpuMs=double(b-a)*1000/double(hz);
            if(!std::isfinite(s.sample.gpuMs)||s.sample.gpuMs>1000){++invalid;continue;}
            if(passCapture&&!s.passOverflow&&s.sample.passCount){
                bool monotonic=true;
                for(unsigned i=0;i<s.sample.passCount;++i)monotonic=monotonic&&edges[i+1]>=edges[i]&&edges[i+1]<=b;
                if(!monotonic){++invalid;continue;}
                for(unsigned i=0;i<s.sample.passCount;++i)s.sample.passes[i].gpuMs=double(edges[i+1]-edges[i])*1000/double(hz);
                s.sample.passesComplete=true;
            }
            (s.sample.wallMs>s.sample.periodMs*1.1?over:within).Add(s.sample);
        }
    }
};
}

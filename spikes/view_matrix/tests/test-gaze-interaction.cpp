#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <new>
#include "../gaze_reader.h"
#include "../psvr2_gaze_abi.h"
static void require(bool b,const char* what){if(!b){printf("FAIL %s\n",what);exit(1);}}
int main(){using namespace svr::psvr2;GazeShared shared{};shared.magic=kGazeMagic;shared.version=kGazeVersion;
 GazeSample s{},out{};require(!GazeRead(shared,out),"unpublished rejected");s.publishMs=100;s.flags=GazeCombinedValid;s.dirCombined[2]=1;
 GazeWrite(shared,s);require(GazeRead(shared,out),"seqlock round trip");float d[3]{};
 require(svrgaze::Direction(out,200,d)&&d[2]==-1,"Toolkit axes and maximum age");require(!svrgaze::Direction(out,201,d),"stale rejected");require(!svrgaze::Direction(out,99,d),"future rejected");
 s.flags|=GazeBlinkLeft|GazeBlinkRight;require(!svrgaze::Direction(s,100,d),"both blink rejected");s.flags=GazeCombinedValid;s.dirCombined[0]=std::numeric_limits<float>::quiet_NaN();require(!svrgaze::Direction(s,100,d),"NaN rejected");
 shared.seq=3;require(!GazeRead(shared,out),"torn sample rejected");shared.seq=4;shared.magic=0;require(!GazeRead(shared,out),"foreign mapping rejected");
 float x,y;require(svr_fovea::GazePixel(0,0,-1,-.8f,.9f,.7f,-.6f,2632,2684,x,y),"asymmetric eye");require(!svr_fovea::GazePixel(0,0,1,-.8f,.9f,.7f,-.6f,2632,2684,x,y),"behind rejected");
 for(unsigned w:{32u,66u,2632u})for(unsigned h:{35u,2684u})for(float px:{0.f,float(w-1)})for(float py:{0.f,float(h-1)}){auto r=svr_fovea::Around(px,py,w,h,.425f);require(r.w&&r.h&&r.x+r.w<=w&&r.y+r.h<=h,"rect clamp");}
 require(svrgaze::Sector(100,40,100,100,20,100,4)==0,"wheel up");require(svrgaze::Sector(160,100,100,100,20,100,4)==1,"wheel right");require(svrgaze::Sector(100,100,100,100,20,100,4)==-1,"centre ignored");
 svrgaze::Dwell dwell;require(dwell.Update(1,100)==-1&&dwell.Update(1,179)==-1&&dwell.Update(1,180)==1,"dwell");require(dwell.Update(-1,181)==-1&&dwell.Update(1,182)==-1,"invalid resets dwell");
 weaponwheel::State state;require(weaponwheel::Update(state,true,false,true,true,false,0,0,4)==weaponwheel::Event::Open,"R3 open");
 require(weaponwheel::Update(state,true,false,true,true,false,0,0,4,1)==weaponwheel::Event::SelectionChanged,"gaze highlights only");
 require(weaponwheel::Update(state,false,false,true,true,false,0,0,4,1)==weaponwheel::Event::Commit,"R3 release commits");
 svrgaze::Reader reader;require(!reader.Read(d),"missing provider falls back");reader.Close();reader.Close();
 // Actual process-local shared reader propagates the provider timestamp;
 // a rejected sample must clear it rather than reuse an earlier gaze.
 wchar_t name[80]{};swprintf_s(name,L"Local\\SingularityVR.Gaze.%lu",GetCurrentProcessId());
 HANDLE mapping=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,sizeof(GazeShared),name);
 require(mapping!=nullptr,"fixture mapping");
 auto* mapped=static_cast<GazeShared*>(MapViewOfFile(mapping,FILE_MAP_WRITE,0,0,sizeof(GazeShared)));
 require(mapped!=nullptr,"fixture view");new(mapped) GazeShared{};mapped->magic=kGazeMagic;mapped->version=kGazeVersion;
 s={};s.publishMs=GetTickCount64();s.flags=GazeCombinedValid;s.dirCombined[2]=1;GazeWrite(*mapped,s);
 svrgaze::Reader live;std::uint64_t stamp=0;
 require(live.Read(d,&stamp)&&stamp==s.publishMs&&d[2]==-1,"reader timestamp");
 s.flags|=GazeBlinkLeft|GazeBlinkRight;GazeWrite(*mapped,s);
 require(!live.Read(d,&stamp)&&stamp==0,"invalid reader clears timestamp");
 live.Close();UnmapViewOfFile(mapped);CloseHandle(mapping);
 static_assert(sizeof(GazeShared)==96,"cross-bitness shared size");static_assert(offsetof(GazeShared,sample)==16,"sample ABI");
 puts("gaze ABI / freshness / wheel / fovea / reader fallback: PASS");}

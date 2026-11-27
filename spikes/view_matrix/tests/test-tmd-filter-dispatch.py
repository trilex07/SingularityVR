"""CPU regression of verbatim production SetPS + exact-filter draw dispatch.

No D3D runtime/game loading. Private bytecode and generated sources stay outside Git.
Run from a VS x86 developer shell: python this.py --out PRIVATE --shader PRIVATE.
Baseline reproduces stale classification after an accepted recorded SetPixelShader.
The ordinary continuation is a sentinel, not a renderer.
"""
import argparse
import hashlib
import json
import pathlib
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[3]
SOURCE = 'spikes/view_matrix/d3d9.cpp'
BASE = 'c3caff76269deb4f40ec02c74c1f3a490304d7ea'


def extracts(text):
    start = text.index('static HRESULT STDMETHODCALLTYPE ShadowTraceSetPS(')
    brace = text.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    if text.startswith('\nstatic bool IsBoundTmdGravityDecal(', end):
        end = text.index('\n// Fix-I/', end)
    bind = text[start:end]
    draw = text.index('HRESULT STDMETHODCALLTYPE Hook_DrawIndexedPrim(')
    first = text.index('InterlockedIncrement(&g_frameDrawIdx);', draw)
    first += len('InterlockedIncrement(&g_frameDrawIdx);')
    last = text.index('if(g_tmdShortGauntlet', first)
    legacy = text.find('if(g_decalLtBound)', first, last)
    if legacy != -1:
        last = legacy
    return bind, text[first:last]


PRELUDE = r'''
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
#include "tmd_gravity_decal.h"
using HRESULT=int32_t;using UINT=unsigned;using DWORD=uint32_t;
#define STDMETHODCALLTYPE
#define SUCCEEDED(x) ((x)>=0)
constexpr HRESULT D3D_OK=0,FAIL=-1,SENTINEL=-1234;
enum D3DRENDERSTATETYPE {D3DRS_COLORWRITEENABLE,D3DRS_ZWRITEENABLE,
 D3DRS_ALPHABLENDENABLE,D3DRS_SRCBLEND,D3DRS_DESTBLEND,D3DRS_STENCILENABLE,
 D3DRS_BLENDOP,D3DRS_ZENABLE,D3DRS_ZFUNC,D3DRS_SEPARATEALPHABLENDENABLE,
 D3DRS_SRCBLENDALPHA,D3DRS_DESTBLENDALPHA,D3DRS_BLENDOPALPHA};
struct IDirect3DVertexBuffer9{int releases=0;void Release(){++releases;}};
struct IDirect3DPixelShader9{
 std::vector<unsigned char> code;int queries=0,failAt=0,refs=0,releases=0;
 void Release(){++releases;}
 HRESULT GetFunction(void* p,UINT* n){++queries;if(queries==failAt)return FAIL;
 if(p)memcpy(p,code.data(),code.size());*n=(UINT)code.size();return 0;}
};
struct IDirect3DDevice9{
 DWORD states[13]{15,0,1,9,1,0,1,1,4,1,1,2,1};
 int queries=0,failAt=-1,streamQueries=0;UINT offset=0,stride=12;
 bool streamFail=false;IDirect3DVertexBuffer9 vb;
 IDirect3DPixelShader9* actualShader=nullptr;
 bool recording=false,shaderQueryFail=false;
 int shaderQueries=0;
 HRESULT GetPixelShader(IDirect3DPixelShader9** shader) {
  ++shaderQueries;
  *shader=actualShader;
  if(*shader)++(*shader)->refs;
  return shaderQueryFail?FAIL:0;
 }
 HRESULT GetRenderState(D3DRENDERSTATETYPE t,DWORD* p){++queries;*p=states[t];return t==failAt?FAIL:0;}
 HRESULT GetStreamSource(UINT,IDirect3DVertexBuffer9** v,UINT* o,UINT* s){
 ++streamQueries;*v=&vb;*o=offset;*s=stride;return streamFail?FAIL:0;}
 // Intentionally no SetRenderState/SetShader/SetConstants method: observer/filter
 // dispatch cannot write pipeline state and still compile this production body.
};
bool g_tmdGravityDecalFix=false,g_tmdGravityDecalBound=false;
namespace shadowtrace{struct{IDirect3DPixelShader9* ps=nullptr;}bound;}
namespace shadowfix{void BindPS(void*){}}
namespace worldshadowfix{void BindPS(void*){}}
namespace projectorfix{void BindPS(void*){}}
namespace nativefx{void BindPS(void*){}}
HRESULT setResult=0;int nativeBinds=0,nativeDraws=0;
HRESULT shadowOriginalPS(IDirect3DDevice9* device,IDirect3DPixelShader9* shader){
 ++nativeBinds;
 if(SUCCEEDED(setResult)&&!device->recording)device->actualShader=shader;
 return setResult;
}
bool vr=true,game=true;bool VrModeOn(){return vr;}bool InGameplay(){return game;}
int g_haveTMD=1,g_handsStatus=1,g_gamePaused=0,g_vrUiMenu=0,g_panelOpen=0,g_inCinematic=0,g_rtIsScene=1;
constexpr int HANDS_TMD=1;int Rd(int x){return x;}
template<class... T> void Log(const char*,T...){}
int checks=0,failures=0;void check(bool b,const char* what){++checks;if(!b){++failures;printf("FAIL %s\n",what);}}
'''
TEST = r'''
int main(int argc,char**argv){
 if(argc!=2)return 90;FILE*f=nullptr;if(fopen_s(&f,argv[1],"rb")||!f)return 91;
 IDirect3DPixelShader9 ps;ps.code.resize(2132);if(fread(ps.code.data(),1,2132,f)!=2132||fgetc(f)!=EOF)return 92;fclose(f);
 const auto captured=ps.code;
 check(tmdgravitydecal::Shader(ps.code.data(),ps.code.size()),"private captured shader identity");
 IDirect3DDevice9 dev;g_tmdGravityDecalFix=true;
 check(ShadowTraceSetPS(&dev,&ps)==0&&nativeBinds==1,"native SetPS called and result retained");
 check(draw(&dev,4,5161,6)==0&&nativeDraws==0,"captured311 suppressed");
 check(draw(&dev,4,5161,64)==0&&nativeDraws==0,"captured312 suppressed");
 auto forward=[&](unsigned t=4,unsigned verts=5161,unsigned prims=6){int n=nativeDraws;
  check(draw(&dev,t,verts,prims)==SENTINEL&&nativeDraws==n+1,"nonmatch preserves native continuation/HRESULT");};
 g_tmdGravityDecalFix=false;int q=ps.queries;ShadowTraceSetPS(&dev,&ps);
 check(ps.queries==q,"OFF zero shader reads");int dq=dev.queries,sq=dev.shaderQueries;forward();
 check(dev.queries==dq&&dev.shaderQueries==sq,"OFF zero draw state reads");g_tmdGravityDecalFix=true;
 ShadowTraceSetPS(&dev,&ps);
 for(int i=0;i<13;++i){++dev.states[i];forward();--dev.states[i];}
 for(int i=0;i<13;++i){dev.failAt=i;forward();}dev.failAt=-1;
 dev.streamFail=true;forward();dev.streamFail=false;
 check(dev.vb.releases==dev.streamQueries,"stream reference release balance incl failed query");
 ++dev.stride;forward();--dev.stride;++dev.offset;forward();--dev.offset;
 sq=dev.shaderQueries;
 forward(3);forward(4,9,8);forward(4,5161,5);forward(4,5161,65);
 check(dev.shaderQueries==sq,"unrelated geometry does not query the shader");
 sq=dev.shaderQueries;
 for(int* p:{&g_haveTMD,&g_handsStatus,&g_gamePaused,&g_vrUiMenu,&g_panelOpen,&g_inCinematic,&g_rtIsScene}){
  int old=*p;*p=(old==0?1:0);forward();*p=old;}
 vr=false;forward();vr=true;game=false;forward();game=true;
 check(dev.shaderQueries==sq,"unrelated contexts do not query the shader");
 setResult=FAIL;check(ShadowTraceSetPS(&dev,nullptr)==FAIL,"failed native bind HRESULT");
 // Failed bind does not clear the still-actual native shader; inspect state only.
 check(shadowtrace::bound.ps==&ps,"failed bind preserves actual previous binding");
 check(dev.actualShader==&ps,"failed bind leaves original device shader");
 setResult=0;ShadowTraceSetPS(&dev,nullptr);forward();
 for(int failure:{1,2}){ps.queries=0;ps.failAt=failure;ShadowTraceSetPS(&dev,&ps);forward();}
 ps.failAt=0;ps.code.pop_back();ShadowTraceSetPS(&dev,&ps);forward();
 ps.code.push_back(0);ps.code[0]^=1;ShadowTraceSetPS(&dev,&ps);forward();
 ps.code=captured;
 ShadowTraceSetPS(&dev,&ps);
 if(EXPECT_DRAWTIME){
  dev.shaderQueryFail=true;forward();dev.shaderQueryFail=false;
  check(ps.refs==ps.releases,"shader COM references balanced on all exits");
 }
 const int before=failures;
 ShadowTraceSetPS(&dev,nullptr);
 dev.recording=true;ShadowTraceSetPS(&dev,&ps);dev.recording=false;
 check(draw(&dev,4,5161,6)==SENTINEL,"recorded exact shader is not the current shader");
 ShadowTraceSetPS(&dev,&ps);
 dev.recording=true;ShadowTraceSetPS(&dev,nullptr);dev.recording=false;
 check(draw(&dev,4,5161,6)==D3D_OK,"recorded null shader does not replace the current shader");
 printf("checks=%d failures=%d other_failures=%d shader_queries=%d stream_refs=%d/%d\n",
        checks,failures,before,dev.shaderQueries,dev.vb.releases,dev.streamQueries);
 return failures?1:0;
}
'''


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--out', required=True, type=pathlib.Path)
    ap.add_argument('--shader', required=True, type=pathlib.Path)
    args = ap.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    sources = {'baseline': subprocess.check_output(['git', 'show', BASE+':'+SOURCE], cwd=ROOT).decode('utf-8'),
               'candidate': (ROOT/SOURCE).read_text(encoding='utf-8')}
    results = {}
    for name, source in sources.items():
        bind, dispatch = extracts(source)
        generated = ('constexpr bool EXPECT_DRAWTIME='+('true' if name=='candidate' else 'false')+';\n') + PRELUDE + bind + '\nHRESULT draw(IDirect3DDevice9* dev,unsigned t,UINT numVertices,UINT primCount){\n' + dispatch + '\n++nativeDraws;return SENTINEL;\n}\n' + TEST
        cpp = args.out/(name+'.cpp')
        cpp.write_text(generated, encoding='utf-8')
        exe = args.out/(name+'.exe')
        subprocess.run(['cl','/nologo','/O2','/EHsc','/std:c++17','/I'+str(ROOT/'spikes/view_matrix'),str(cpp),'/Fe:'+str(exe),'/Fo:'+str(args.out/(name+'.obj'))],check=True)
        run = subprocess.run([str(exe),str(args.shader.resolve())],text=True,capture_output=True)
        (args.out/(name+'.log')).write_text(run.stdout+run.stderr,encoding='utf-8')
        print(name,run.returncode,run.stdout)
        assert run.returncode == (1 if name=='baseline' else 0)
        assert ('failures=2 other_failures=0' if name=='baseline' else 'failures=0 other_failures=0') in run.stdout
        results[name]={'exit':run.returncode,'source_sha256':hashlib.sha256(source.encode()).hexdigest(),
                       'bind_sha256':hashlib.sha256(bind.encode()).hexdigest(),'dispatch_sha256':hashlib.sha256(dispatch.encode()).hexdigest(),
                       'stdout':run.stdout}
    (args.out/'red-green.json').write_text(json.dumps(results,indent=2)+'\n',encoding='utf-8')


if __name__=='__main__':
    main()

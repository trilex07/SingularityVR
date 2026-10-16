#pragma once
#include <d3dcompiler.h>

// Controlled scene-lit approximation, NOT the entire original Mat_Knife graph.
// Light identities and depth alpha proven by private native hand shader dumps.
// No game shader bytes or textures embedded. Default OFF, held joint path only.
namespace knifelight {
struct Vertex {float p[3],n[3],t[4],uv[2];};
static_assert(sizeof(Vertex)==48,"SVNL tangent frame");
inline bool Decode(const unsigned char* p,size_t bytes,std::vector<Vertex>& out,std::vector<WORD>& indices){
    if(!p||bytes<16||std::memcmp(p,"SVNL",4)||gloves::U32(p+4)!=1)return false;
    const auto n=gloves::U32(p+8),ni=gloves::U32(p+12);
    if(n!=662||ni!=3030||bytes!=16+n*sizeof(Vertex)+ni*2)return false;
    std::vector<Vertex> v(n);std::vector<WORD> ix(ni);
    std::memcpy(v.data(),p+16,n*sizeof(Vertex));std::memcpy(ix.data(),p+16+n*sizeof(Vertex),ni*2);
    for(const auto& x:v){
        for(float f:x.p)if(!std::isfinite(f)||std::fabs(f)>1)return false;
        for(float f:x.uv)if(!std::isfinite(f)||std::fabs(f)>16)return false;
        double nn=0,tt=0,nt=0;
        for(unsigned i=0;i<3;++i){if(!std::isfinite(x.n[i])||!std::isfinite(x.t[i]))return false;
            nn+=x.n[i]*x.n[i];tt+=x.t[i]*x.t[i];nt+=x.n[i]*x.t[i];}
        if(std::fabs(nn-1)>.002||std::fabs(tt-1)>.002||std::fabs(nt)>.002||std::fabs(x.t[3])!=1)return false;
    }
    for(auto i:ix)if(i>=n)return false;
    out=std::move(v);indices=std::move(ix);return true;
}
inline std::uint64_t Hash(const void* data,unsigned bytes){
    auto p=static_cast<const unsigned char*>(data);std::uint64_t h=14695981039346656037ull;
    for(unsigned i=0;i<bytes;++i){h^=p[i];h*=1099511628211ull;}return h;
}
inline bool Values(const float* p,unsigned n){for(unsigned i=0;i<n;++i)if(!std::isfinite(p[i])||std::fabs(p[i])>1e8f)return false;return true;}
// MinZ_MaxZRatio: exact native colour PS writes x / clip.w + y.
inline float DepthAlpha(float clipW,const float depth[4]){return depth[0]/clipW+depth[1];}

static const char shader[]=R"HLSL(
row_major float4x4 ToClip : register(c0);
row_major float4x4 ToWorld : register(c4);
float4 Camera : register(c8);
struct In {float3 p:POSITION;float3 n:NORMAL;float4 t:TANGENT;float2 uv:TEXCOORD0;};
struct Out {float4 clip:POSITION;float3 n:TEXCOORD0;float3 t:TEXCOORD1;float3 b:TEXCOORD2;float3 view:TEXCOORD3;float2 uv:TEXCOORD4;float depth:TEXCOORD5;};
Out vsMain(In v){
    Out o;o.clip=mul(float4(v.p,1),ToClip);
    o.n=normalize(mul(float4(v.n,0),ToWorld).xyz);
    o.t=normalize(mul(float4(v.t.xyz,0),ToWorld).xyz);
    o.b=normalize(mul(float4(cross(v.n,v.t.xyz)*v.t.w,0),ToWorld).xyz);
    o.view=Camera.xyz-mul(float4(v.p,1),ToWorld).xyz;o.uv=v.uv;o.depth=o.clip.w;return o;
}
float4 SH[7] : register(c0);
float4 LightColor : register(c7);
float4 LightDirection : register(c8);
float4 Ambient : register(c9);
float4 Depth : register(c10);
samplerCUBE Basis0:register(s0);samplerCUBE Basis1:register(s1);
sampler2D Albedo:register(s2);sampler2D Normal:register(s3);sampler2D Specular:register(s4);
float4 psMain(Out i):COLOR0 {
    float3 tn=normalize(tex2D(Normal,i.uv).xyz*2-1);
    float3 n=normalize(tn.x*normalize(i.t)+tn.y*normalize(i.b)+tn.z*normalize(i.n));
    float4 a=texCUBE(Basis0,n)*2-1,b=texCUBE(Basis1,n)*2-1;
    // Native SH convolution, DiffusePower=2 from Mat_Knife. Constants follow
    // the captured shader's k0/k1/k2 algebra, not guessed register semantics.
    float k0=1.77245522/3, k1=6.28318548/4, k2=6.28318548*2/15;
    float4 lowBasis=float4(a.xyz*k1,a.w*k2),quadratic=b*k2;
    float3 irradiance=float3(dot(SH[1],lowBasis)+dot(SH[2],quadratic),dot(SH[3],lowBasis)+dot(SH[4],quadratic),dot(SH[5],lowBasis)+dot(SH[6],quadratic))+SH[0].xyz*k0;
    float3 view=normalize(i.view),light=normalize(LightDirection.xyz);
    float diffuse=pow(saturate(dot(n,light)),2);
    float spec=pow(saturate(dot(reflect(-light,n),view)),50);
    float3 rgb=tex2D(Albedo,i.uv).rgb*(max(irradiance,0)+max(Ambient.rgb,0)+max(LightColor.rgb,0)*diffuse);
    rgb+=tex2D(Specular,i.uv).rgb*max(LightColor.rgb,0)*spec;
    return float4(rgb,Depth.x/i.depth+Depth.y);
}
)HLSL";

class Renderer {
    IDirect3DVertexShader9* vs=nullptr;IDirect3DPixelShader9* ps=nullptr;
    IDirect3DVertexDeclaration9* declaration=nullptr;
    IDirect3DTexture9* normal=nullptr;IDirect3DTexture9* specular=nullptr;
    std::vector<Vertex> vertices;std::vector<WORD> indices;
    std::vector<unsigned char> maps[2];UINT w[2]{},h[2]{};
    std::vector<DWORD> bytecode[2];
    bool fault=false;
    struct Pair {IDirect3DVertexShader9* vs;IDirect3DPixelShader9* ps;bool match;};
    Pair cache[8]{};unsigned used=0;
public:
    unsigned matched=0,unknown=0,drawn=0;
    bool Load(const std::string& dir){
        std::vector<unsigned char> bytes;
        if(!hipknife::Read(dir+"knife.svnl",bytes,1024*1024)||!Decode(bytes.data(),bytes.size(),vertices,indices))return false;
        for(unsigned i=0;i<2;++i)
            if(!hipknife::Read(dir+(i?"knife_specular.svrt":"knife_normal.svrt"),bytes,16+2048*2048*4)||
               !hipknife::DecodeTexture(bytes.data(),bytes.size(),w[i],h[i],maps[i]))return false;
        return CompileCpu();
    }
    // Startup-only, outside DllMain and render hooks. Reset retains bytecode.
    bool CompileCpu(){
        HMODULE module=LoadLibraryExW(L"d3dcompiler_47.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
        using Compile=HRESULT(WINAPI*)(LPCVOID,SIZE_T,LPCSTR,const D3D_SHADER_MACRO*,ID3DInclude*,LPCSTR,LPCSTR,UINT,UINT,ID3DBlob**,ID3DBlob**);
        auto compile=module?reinterpret_cast<Compile>(GetProcAddress(module,"D3DCompile")):nullptr;
        bool ok=compile!=nullptr;
        for(unsigned stage=0;stage<2&&ok;++stage){ID3DBlob* code=nullptr;ID3DBlob* error=nullptr;
            ok=SUCCEEDED(compile(shader,sizeof(shader)-1,"knife_scene_lighting",nullptr,nullptr,
                stage?"psMain":"vsMain",stage?"ps_3_0":"vs_3_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&code,&error))&&code;
            if(ok){const size_t n=code->GetBufferSize();ok=n>=8&&n<=65536&&n%4==0;
                if(ok){bytecode[stage].resize(n/4);std::memcpy(bytecode[stage].data(),code->GetBufferPointer(),n);}}
            hipknife::Release(code);hipknife::Release(error);
        }
        if(module)FreeLibrary(module);return ok;
    }
    void Release(){
        hipknife::Release(vs);hipknife::Release(ps);hipknife::Release(declaration);
        hipknife::Release(normal);hipknife::Release(specular);
        for(unsigned i=0;i<used;++i){hipknife::Release(cache[i].vs);hipknife::Release(cache[i].ps);}used=0;fault=false;
    }
    bool Match(IDirect3DDevice9* dev){
        if(vertices.empty()||fault)return false;
        IDirect3DVertexShader9* v=nullptr;IDirect3DPixelShader9* p=nullptr;
        dev->GetVertexShader(&v);dev->GetPixelShader(&p);
        if(!v||!p){hipknife::Release(v);hipknife::Release(p);return false;}
        for(unsigned i=0;i<used;++i)if(cache[i].vs==v&&cache[i].ps==p){const bool match=cache[i].match;v->Release();p->Release();return match;}
        if(used==8){v->Release();p->Release();return false;}
        DWORD code[4096];UINT size=sizeof(code);bool match=false;
        // Measured FNV identities (private original shader bytes). Exact
        // pair -> CTAB: VS LightDirection c238, CameraPosition c4; PS SH c3,
        // LightColor c33, AmbientColorAndSkyFactor c34, MinZ_MaxZRatio c2.
        const bool vertex=SUCCEEDED(v->GetFunction(code,&size))&&size==2348&&Hash(code,size)==0x37754B7CC68D8D59ull;
        size=sizeof(code);
        match=vertex&&SUCCEEDED(p->GetFunction(code,&size))&&size==4696&&Hash(code,size)==0x85D66574CA9E9C00ull;
        cache[used++]={v,p,match};if(match)++matched;else ++unknown;return match;
    }
    bool Ensure(IDirect3DDevice9* dev){
        if(fault||vertices.empty())return false;if(vs&&ps&&normal&&specular&&declaration)return true;
        bool ok=!bytecode[0].empty()&&!bytecode[1].empty()&&
            SUCCEEDED(dev->CreateVertexShader(bytecode[0].data(),&vs))&&
            SUCCEEDED(dev->CreatePixelShader(bytecode[1].data(),&ps));
        const D3DVERTEXELEMENT9 elements[]={
            {0,0,D3DDECLTYPE_FLOAT3,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,0},
            {0,12,D3DDECLTYPE_FLOAT3,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_NORMAL,0},
            {0,24,D3DDECLTYPE_FLOAT4,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TANGENT,0},
            {0,40,D3DDECLTYPE_FLOAT2,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TEXCOORD,0},D3DDECL_END()};
        ok=ok&&SUCCEEDED(dev->CreateVertexDeclaration(elements,&declaration))&&
            hipknife::UploadAlbedo(dev,w[0],h[0],maps[0],normal)&&hipknife::UploadAlbedo(dev,w[1],h[1],maps[1],specular);
        if(!ok){Release();fault=true;}return ok;
    }
    // Caller has the ALL-state/RT/DS guard and private finger depth installed.
    bool Draw(IDirect3DDevice9* dev,const hipknife::Calls& calls,IDirect3DTexture9* albedo,
              const float world[4][4],const float clip[4][4]){
        float lights[11][4]{};float camera[4]{};
        if(FAILED(dev->GetPixelShaderConstantF(3,lights[0],7))||
           FAILED(dev->GetPixelShaderConstantF(33,lights[7],1))||
           FAILED(dev->GetVertexShaderConstantF(238,lights[8],1))||
           FAILED(dev->GetPixelShaderConstantF(34,lights[9],1))||
           FAILED(dev->GetPixelShaderConstantF(2,lights[10],1))||
           FAILED(dev->GetVertexShaderConstantF(4,camera,1))||
           !Values(lights[0],44)||!Values(camera,4))return false;
        const double direction=lights[8][0]*lights[8][0]+lights[8][1]*lights[8][1]+lights[8][2]*lights[8][2];
        if(direction<1e-8){lights[8][2]=1;lights[7][0]=lights[7][1]=lights[7][2]=0;}
        if(!Ensure(dev))return false;
        bool ok=calls.vsConstants&&SUCCEEDED(calls.vs(dev,vs))&&SUCCEEDED(calls.ps(dev,ps))&&
            SUCCEEDED(dev->SetVertexDeclaration(declaration))&&
            SUCCEEDED(calls.vsConstants(dev,0,&clip[0][0],4))&&
            SUCCEEDED(calls.vsConstants(dev,4,&world[0][0],4))&&
            SUCCEEDED(calls.vsConstants(dev,8,camera,1))&&
            SUCCEEDED(dev->SetPixelShaderConstantF(0,lights[0],11));
        // Original SH cube samplers 0/1 and their native states stay bound.
        IDirect3DBaseTexture9* cubes[2]{};
        for(unsigned i=0;i<2;++i){ok=SUCCEEDED(dev->GetTexture(i,&cubes[i]))&&cubes[i]&&cubes[i]->GetType()==D3DRTYPE_CUBETEXTURE&&ok;hipknife::Release(cubes[i]);}
        for(unsigned s=2;s<=4&&ok;++s){
            ok=SUCCEEDED(calls.texture(dev,s,s==2?albedo:s==3?normal:specular));
            ok=SUCCEEDED(dev->SetSamplerState(s,D3DSAMP_MINFILTER,D3DTEXF_LINEAR))&&ok;
            ok=SUCCEEDED(dev->SetSamplerState(s,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR))&&ok;
            ok=SUCCEEDED(dev->SetSamplerState(s,D3DSAMP_MIPFILTER,D3DTEXF_NONE))&&ok;
            ok=SUCCEEDED(dev->SetSamplerState(s,D3DSAMP_ADDRESSU,D3DTADDRESS_WRAP))&&ok;
            ok=SUCCEEDED(dev->SetSamplerState(s,D3DSAMP_ADDRESSV,D3DTADDRESS_WRAP))&&ok;
            ok=SUCCEEDED(dev->SetSamplerState(s,D3DSAMP_SRGBTEXTURE,s==2))&&ok;
        }
        ok=SUCCEEDED(dev->SetRenderState(D3DRS_COLORWRITEENABLE,15))&&ok;
        ok=SUCCEEDED(dev->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE))&&ok;
        ok=SUCCEEDED(dev->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE))&&ok;
        if(ok)ok=SUCCEEDED(calls.draw(dev,D3DPT_TRIANGLELIST,0,(UINT)vertices.size(),(UINT)indices.size()/3,
            indices.data(),D3DFMT_INDEX16,vertices.data(),sizeof(Vertex)));
        if(ok)++drawn;return ok;
    }
};
}

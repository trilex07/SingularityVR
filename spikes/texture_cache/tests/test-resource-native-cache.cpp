#include "../resource_native_cache.h"
#include <cstdio>
#include <cstring>
#include <vector>
static int failures=0;
static void Check(bool ok,const char* text){std::printf("%s %s\n",ok?"PASS":"FAIL",text);if(!ok)++failures;}
struct Resource {
    bool present=false, writable=true;
    DWORD reported=sizeof(texnative::Stamp), flags=99;
    texnative::Stamp value{};
    HRESULT GetPrivateData(REFGUID id,void* out,DWORD* size) {
        if(std::memcmp(&id,&texnative::nativeDecisionId,sizeof(id)) || !present)return E_FAIL;
        std::memcpy(out,&value,sizeof(value));*size=reported;return S_OK;
    }
    HRESULT SetPrivateData(REFGUID id,const void* bytes,DWORD size,DWORD newFlags) {
        if(!writable || size!=sizeof(value) || std::memcmp(&id,&texnative::nativeDecisionId,sizeof(id)))return E_FAIL;
        std::memcpy(&value,bytes,sizeof(value));flags=newFlags;present=true;return S_OK;
    }
};
struct Legacy {
    const void* pointers[512]{};unsigned count=0,hashes=0;
    void Bind(const void* p) {
        for(unsigned n=0;n<count;++n)if(pointers[n]==p)return;
        ++hashes;
        if(count<512)pointers[count++]=p;else pointers[0]=p;
    }
};
int main(){
    Resource resource;
    Check(!texnative::KnownNative(&resource),"new resource is unresolved");
    Check(texnative::RememberNative(&resource) && texnative::KnownNative(&resource),"native result round trip");
    Check(resource.flags==0,"byte metadata only, no IUnknown reference/pixel flags");
    resource.value.magic^=1;Check(!texnative::KnownNative(&resource),"foreign marker refused");
    resource.value=texnative::nativeDecision;resource.value.version=2;
    Check(!texnative::KnownNative(&resource),"unknown version refused");
    resource.value=texnative::nativeDecision;resource.reported=sizeof(resource.value)-1;
    Check(!texnative::KnownNative(&resource),"partial record refused");
    resource=Resource{};resource.writable=false;
    Check(!texnative::RememberNative(&resource) && !texnative::KnownNative(&resource),"metadata failure remains unresolved for fallback");
    Check(!texnative::KnownNative<Resource>(nullptr) && !texnative::RememberNative<Resource>(nullptr),"null resource refused");
    resource=Resource{};texnative::RememberNative(&resource);
    // Same address, new lifetime: new driver resource has no predecessor's bytes.
    resource=Resource{};Check(!texnative::KnownNative(&resource),"reused address does not retain old negative decision");
    std::vector<Resource> resources(655);
    Legacy legacy;
    for(unsigned n=0;n<512;++n)legacy.Bind(&resources[n]);
    for(unsigned round=0;round<10000;++round){legacy.Bind(&resources[512]);legacy.Bind(&resources[513]);}
    Check(legacy.hashes==20512,"exact old512 first-slot policy reproduces20000 extra hashes");
    unsigned hashes=0;bool retained=true;
    for(unsigned round=0;round<100;++round)for(auto& item:resources) {
        if(!texnative::KnownNative(&item)){++hashes;if(!texnative::RememberNative(&item))retained=false;}
    }
    Check(retained,"all resource results retained");
    Check(hashes==655,"655 resources/65500 binds hash exactly once per lifetime");
    struct Entry {void* replacement;};Entry entries[4]{{reinterpret_cast<void*>(1)},{nullptr},{reinterpret_cast<void*>(2)},{nullptr}};
    std::size_t cursor=0;
    Check(texnative::NegativeVictim(entries,4,cursor)==1 &&
          texnative::NegativeVictim(entries,4,cursor)==3 &&
          texnative::NegativeVictim(entries,4,cursor)==1,"fallback rotates instead of thrashing one slot");
    entries[1].replacement=reinterpret_cast<void*>(3);entries[3].replacement=reinterpret_cast<void*>(4);
    Check(texnative::NegativeVictim(entries,4,cursor)==SIZE_MAX,"pinned replacements never evicted");
    Check(texnative::NegativeVictim<Entry>(nullptr,0,cursor)==SIZE_MAX,"empty fallback refused");
    return failures?1:0;
}

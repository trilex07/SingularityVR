struct NativeRefillBindings {
    uintptr_t klass=0,vtable=0,method[4]{};uint32_t slot[4]{};
};
static bool ResolveNativeRefill(void* self,NativeRefillBindings& result){
    // Only called on a permitted engine request, never each frame/draw. Names
    // locate the wrapper; ABI + whole-body fingerprint establish what is called.
    const auto object=(uintptr_t)self;
    int32_t clsBits=0,vtBits=0;
    if(g_funcOffset<0||g_upChildren<=0||g_upNext<=0||
       !TryLoadAlignedI32(object+OBJ_CLASS,&clsBits)||!clsBits||
       !TryLoadAlignedI32(object,&vtBits)||!vtBits)return false;
    const uintptr_t klass=(uint32_t)clsBits,vtable=(uint32_t)vtBits;
    static NativeRefillBindings cached;
    if(cached.klass!=klass||cached.vtable!=vtable){
        NativeRefillBindings pending;pending.klass=klass;pending.vtable=vtable;
        uintptr_t cls=klass;unsigned visited=0,found=0;
        for(unsigned depth=0;cls&&depth<32&&visited<2048;++depth){
            char owner[80]{};
            if(!GuardedNameOf(cls,owner,sizeof(owner)))return false;
            if(!strcmp(owner,"RvWeaponShared")){
                int32_t bits=0;if(!TryLoadAlignedI32(cls+g_upChildren,&bits))return false;
                uintptr_t field=(uint32_t)bits;
                for(unsigned n=0;field&&n<512&&visited<2048;++n,++visited){
                    char name[80]{};
                    if(!GuardedNameOf(field,name,sizeof(name)))return false;
                    for(unsigned j=0;j<4;++j)if(!strcmp(name,nativerefill::Specs[j].name)){
                        char type[48]{};int32_t typeBits=0,funcBits=0;
                        if(pending.method[j]||!TryLoadAlignedI32(field+OBJ_CLASS,&typeBits)||
                           !GuardedNameOf((uint32_t)typeBits,type,sizeof(type))||strcmp(type,"Function")||
                           !TryLoadAlignedI32(field+g_funcOffset,&funcBits))return false;
                        const uintptr_t wrapper=(uint32_t)funcBits;
                        const std::size_t bytes=j==3?53:centurionnative::WrapperBytes;
                        if(wrapper<g_textLo||wrapper>=g_textHi||g_textHi-wrapper<bytes||
                           !Readable((void*)wrapper,bytes))return false;
                        uint32_t slot=0;
                        const bool abi=j==3?nativerefill::DecodeVoidWrapper((const uint8_t*)wrapper,bytes,slot):
                            centurionnative::DecodeWrapper((const uint8_t*)wrapper,bytes,slot);
                        int32_t methodBits=0;
                        if(!abi||!TryLoadAlignedI32(vtable+slot,&methodBits)||!methodBits)return false;
                        pending.slot[j]=slot;pending.method[j]=(uint32_t)methodBits;++found;
                    }
                    int32_t next=0;if(!TryLoadAlignedI32(field+g_upNext,&next)||field==(uint32_t)next)return false;
                    field=(uint32_t)next;
                }
                if(field)return false; // bounded walk exhausted: identity incomplete
                break;
            }
            int32_t super=0;if(!TryLoadAlignedI32(cls+g_upNext-4,&super)||cls==(uint32_t)super)return false;
            cls=(uint32_t)super;
        }
        if(found!=4)return false;
        cached=pending;
    }
    // Recheck on EVERY permit: no stale overridden/patched methods. All four
    // exact bodies must call the same inventory cast as the audited AttemptReload.
    const uintptr_t attempt=(uint32_t)Rd(g_centurionNativeTarget);
    if(!attempt||attempt<g_textLo||attempt>=g_textHi||g_textHi-attempt<centurionnative::MethodBytes||
       !Readable((void*)attempt,centurionnative::MethodBytes)||*((const uint8_t*)attempt+0x131)!=0xe8)return false;
    int32_t castRel=0;std::memcpy(&castRel,(const void*)(attempt+0x132),4);
    const uintptr_t inventoryCast=attempt+0x136+castRel;
    if(inventoryCast<g_textLo||inventoryCast>=g_textHi)return false;
    for(unsigned j=0;j<4;++j){
        const auto& spec=nativerefill::Specs[j];const uintptr_t method=cached.method[j];
        int32_t live=0,rel=0;
        if(!TryLoadAlignedI32(vtable+cached.slot[j],&live)||(uint32_t)live!=method||
           method<g_textLo||method>=g_textHi||g_textHi-method<spec.bytes||
           !Readable((void*)method,spec.bytes)||
           !nativerefill::Audited((const uint8_t*)method,spec.bytes,spec))return false;
        std::memcpy(&rel,(const void*)(method+spec.rel),4);
        if(method+spec.rel+4+rel!=inventoryCast)return false;
    }
    result=cached;return true;
}
struct NativeRefillCalls {
    void* object;NativeRefillBindings bindings;
    unsigned reads=0;nativerefill::Counts before{},after{};
    nativerefill::Counts Read(){
        const nativerefill::Counts counts{((PFN_CenturionAttemptReload)bindings.method[0])(object),
            ((PFN_CenturionAttemptReload)bindings.method[1])(object),
            ((PFN_CenturionAttemptReload)bindings.method[2])(object)};
        if(!reads++)before=counts;
        after=counts;return counts;
    }
    void Refill()const{((void (__thiscall*)(void*))bindings.method[3])(object);}
};

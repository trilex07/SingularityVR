struct NativeRefillBindings {
    uintptr_t klass=0,vtable=0,method[4]{};uint32_t slot[4]{};
};
static bool ResolveNativeRefill(void* self,NativeRefillBindings& result,nativerefill::Audit* audit=nullptr){
    // Permitted engine request or bounded read-only preflight, never per draw. Names
    // locate the wrapper; ABI + whole-body fingerprint establish what is called.
    const auto object=(uintptr_t)self;
    nativerefill::Audit local;
    auto& report=audit?*audit:local;report={};
    auto reject=[&](const char* stage){report.stage=stage;return false;};
    int32_t clsBits=0,vtBits=0;
    if(g_funcOffset<0||g_upChildren<=0||g_upNext<=0||
       !TryLoadAlignedI32(object+OBJ_CLASS,&clsBits)||!clsBits||
       !TryLoadAlignedI32(object,&vtBits)||!vtBits)return reject("layout/object");
    const uintptr_t klass=(uint32_t)clsBits,vtable=(uint32_t)vtBits;
    report.klass=(uint32_t)klass;
    static NativeRefillBindings cached;
    if(cached.klass!=klass||cached.vtable!=vtable){
        NativeRefillBindings pending;pending.klass=klass;pending.vtable=vtable;
        uintptr_t cls=klass;unsigned visited=0,found=0;
        for(unsigned depth=0;cls&&depth<32&&visited<2048;++depth){
            char owner[80]{};
            if(!GuardedNameOf(cls,owner,sizeof(owner)))return reject("class-name");
            if(!strcmp(owner,"RvWeaponShared")){
                int32_t bits=0;if(!TryLoadAlignedI32(cls+g_upChildren,&bits))return reject("children-read");
                uintptr_t field=(uint32_t)bits;
                for(unsigned n=0;field&&n<512&&visited<2048;++n,++visited){
                    report.field=(uint32_t)field;report.visited=visited+1;
                    char name[80]{};
                    if(!GuardedNameOf(field,name,sizeof(name)))return reject("field-name");
                    for(unsigned j=0;j<4;++j)if(!strcmp(name,nativerefill::Specs[j].name)){
                        report.index=j;
                        char type[48]{};int32_t typeBits=0,funcBits=0;
                        if(pending.method[j]||!TryLoadAlignedI32(field+OBJ_CLASS,&typeBits)||
                           !GuardedNameOf((uint32_t)typeBits,type,sizeof(type))||strcmp(type,"Function")||
                           !TryLoadAlignedI32(field+g_funcOffset,&funcBits))return reject("field-type/func");
                        const uintptr_t wrapper=(uint32_t)funcBits;
                        report.wrapper=(uint32_t)wrapper;
                        const std::size_t bytes=j==3?53:centurionnative::WrapperBytes;
                        if(wrapper<g_textLo||wrapper>=g_textHi||g_textHi-wrapper<bytes||
                           !Readable((void*)wrapper,bytes))return reject("wrapper-range");
                        uint32_t slot=0;
                        const bool abi=j==3?nativerefill::DecodeVoidWrapper((const uint8_t*)wrapper,bytes,slot):
                            centurionnative::DecodeWrapper((const uint8_t*)wrapper,bytes,slot);
                        int32_t methodBits=0;
                        if(!abi)return reject("wrapper-abi");
                        report.slot=slot;
                        if(!TryLoadAlignedI32(vtable+slot,&methodBits)||!methodBits)return reject("vtable-read");
                        pending.slot[j]=slot;pending.method[j]=(uint32_t)methodBits;++found;report.matched|=1u<<j;
                    }
                    // Centurion runtime: all four bindings preceded an unrelated
                    // undecodable field. Lookup is complete; body/cast audit follows.
                    if(found==4)break;
                    int32_t next=0;if(!TryLoadAlignedI32(field+g_upNext,&next)||field==(uint32_t)next)return reject("field-next");
                    field=(uint32_t)next;
                }
                if(field&&found!=4)return reject("field-budget"); // incomplete bounded walk still fails closed
                break;
            }
            int32_t super=0;if(!TryLoadAlignedI32(cls+g_upNext-4,&super)||cls==(uint32_t)super)return reject("super-read");
            cls=(uint32_t)super;
        }
        if(found!=4)return reject("missing-bindings");
        cached=pending;
    }
    report.matched=15;
    // Recheck on EVERY permit: no stale overridden/patched methods. All four
    // exact bodies must call the same inventory cast as the audited AttemptReload.
    const uintptr_t attempt=(uint32_t)Rd(g_centurionNativeTarget);
    if(!attempt||attempt<g_textLo||attempt>=g_textHi||g_textHi-attempt<centurionnative::MethodBytes||
       !Readable((void*)attempt,centurionnative::MethodBytes)||*((const uint8_t*)attempt+0x131)!=0xe8)return reject("attempt-cast-opcode");
    int32_t castRel=0;std::memcpy(&castRel,(const void*)(attempt+0x132),4);
    const uintptr_t inventoryCast=attempt+0x136+castRel;
    if(inventoryCast<g_textLo||inventoryCast>=g_textHi)return reject("cast-range");
    for(unsigned j=0;j<4;++j){
        const auto& spec=nativerefill::Specs[j];const uintptr_t method=cached.method[j];
        report.index=j;report.method=(uint32_t)method;report.slot=cached.slot[j];
        int32_t live=0,rel=0;
        if(!TryLoadAlignedI32(vtable+cached.slot[j],&live)||(uint32_t)live!=method||
           method<g_textLo||method>=g_textHi||g_textHi-method<spec.bytes||
           !Readable((void*)method,spec.bytes))return reject("method-range/slot");
        report.fingerprint=nativerefill::Fingerprint((const uint8_t*)method,spec.bytes,spec.rel);
        if(!nativerefill::Audited((const uint8_t*)method,spec.bytes,spec))return reject("method-fingerprint");
        std::memcpy(&rel,(const void*)(method+spec.rel),4);
        if(method+spec.rel+4+rel!=inventoryCast)return reject("inventory-cast-target");
    }
    report.stage="ready";result=cached;return true;
}
static void LogNativeRefillAudit(const char* source,unsigned index,bool ready,const nativerefill::Audit& audit){
    Log("next refill resolver: source=%s kind=%u ready=%d stage=%s matched=%X visited=%u binding=%s class=%08X field=%08X wrapper=%08X slot=%X method=%08X fingerprint=%016llX (READ-ONLY resolver; NO invocation)",
        source,index+1,ready?1:0,audit.stage,audit.matched,audit.visited,
        audit.index<4?nativerefill::Specs[audit.index].name:"none",audit.klass,audit.field,
        audit.wrapper,audit.slot,audit.method,(unsigned long long)audit.fingerprint);
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

#pragma once
// Evidence collection only: no UObject calls, writes, grants, selector,
// projectile spawn, timer changes, input consumption or new native hooks.
namespace NativeBodyEquipmentProbe {
// Deadlock is a TMD power, not a RvWeaponShared inventory item. Keep this
// independent of the gun-chain census. Both contracts cover ALL return paths;
// an early-ret prefix would not identify the native input producer.
static void Tmd(uintptr_t local){
    using namespace NativeOwnedInventory;
    static unsigned reports=0;static uintptr_t lastTmd=0,lastPower=0,lastHeld=0;
    if(reports>=8)return;
    const auto cls=P(local+OBJ_CLASS);
    static MemberCache tmdMember,setMember,heldMember,indexMember;
    const int tmdOff=tmdMember.Get(cls,"mTMD");
    const auto tmd=tmdOff>=0?P(local+tmdOff):0,tc=P(tmd+OBJ_CLASS);
    if(!tmd||!IsLive(tmd)||!ClassHas(tc,"RvTMD"))return;
    const int setOff=setMember.Get(tc,"mPowerSet"),heldOff=heldMember.Get(tc,"mDeadlock");
    const auto set=setOff>=0?P(tmd+setOff):0,sc=P(set+OBJ_CLASS);
    if(!set||!IsLive(set)||!ClassHas(sc,"RvTMDPowerSet")||heldOff<0)return;
    // Original TMD input consumes mIndexedPowers[ETP_Deadlock=3], not the
    // grenade inventory slot. Cross-check the reflected producer and bounds.
    if(indexMember.Get(sc,"mIndexedPowers")!=0x48||!PauseRttProbe::SafeRange(set+0x48,12))return;
    const auto table=P(set+0x48);
    const int n=*(const int*)(set+0x4c),cap=*(const int*)(set+0x50);
    if(n<=3||n>32||cap<n||cap>128||!PauseRttProbe::SafeRange(table,size_t(n)*4))return;
    const auto power=P(table+3*4),pc=P(power+OBJ_CLASS),held=P(tmd+heldOff);
    if(!power||!IsLive(power)||!ClassHas(pc,"RvTMDPower_Deadlock"))return;
    if(tmd==lastTmd&&power==lastPower&&held==lastHeld)return;
    const bool input=NativeContextCursor::Method(P(P(tmd)+0x4e8),241,0x3F880641281843C5ull);
    const bool handler=NativeContextCursor::Method(P(P(power)+0x128),1504,0xB26B7A29EA360695ull);
    if(held&&(!IsLive(held)||!ClassHas(P(held+OBJ_CLASS),"RvProp_DeadlockGrenade")))return;
    lastTmd=tmd;lastPower=power;lastHeld=held;++reports;
    Log("body equipment Deadlock: pawn=%08X tmd=%08X power=%08X held=%08X indexedCount=%d inputContract=%d handlerContract=%d [read-only; NO charge/launch/energy writes]",
        (unsigned)local,(unsigned)tmd,(unsigned)power,(unsigned)held,n,input?1:0,handler?1:0);
}
static void Poll(){
    if(!g_bodyEquipmentProbe)return;
    static ULONGLONG next=0;static unsigned reports=0;
    static uintptr_t generation=0,seen[128]{};static unsigned count=0;
    const auto now=GetTickCount64();if(now<next||reports>=32)return;next=now+1000;
    __try {
        using namespace NativeOwnedInventory;
        const auto local=CameraOwnedPawn();
        if(!local)return;
        Tmd(local);
        const auto cls=P(local+OBJ_CLASS);
        static MemberCache invMember;
        const int invOff=invMember.Get(cls,"InvManager");
        if(invOff!=int(kPawnInvManager))return;
        const auto inv=P(local+invOff),ic=P(inv+OBJ_CLASS);
        if(!inv||!ClassHas(ic,"InventoryManager"))return;
        static MemberCache chainMember;
        if(chainMember.Get(ic,"InventoryChain")!=int(kManagerChain))return;
        if(local!=generation){generation=local;count=0;std::memset(seen,0,sizeof(seen));}
        uintptr_t visited[128]{};unsigned n=0;
        for(auto item=P(inv+kManagerChain);item&&n<128;item=P(item+kItemNext)){
            for(unsigned i=0;i<n;++i)if(visited[i]==item)return;
            visited[n++]=item;
            const auto type=P(item+OBJ_CLASS);
            if(!IsLive(item)||!ClassHas(type,"RvWeaponShared"))continue;
            bool done=false;for(unsigned i=0;i<count;++i)if(seen[i]==item)done=true;
            if(done||count>=128||reports>=32)continue;
            const auto method=P(P(item)+0x554);
            // Audited complete native IsHeavyWeapon: flags+2D8 >>7 &1.
            // No bool-layout guess or call through an unverified function.
            const bool proven=NativeContextCursor::Method(method,13,0xAD9AEEB598427D84ull)&&
                PauseRttProbe::SafeRange(item+0x2d8,4);
            const int heavy=proven?int((*(const uint32_t*)(item+0x2d8)>>7)&1u):-1;
            const int nameOff=PauseRttProbe::Property(type,"mWeaponName","NameProperty");
            char object[96]{},className[96]{},weaponName[96]{};
            NameOf(item,object,sizeof(object));NameOf(type,className,sizeof(className));
            if(nameOff==int(kItemWeaponName)&&PauseRttProbe::SafeRange(item+nameOff,8))
                NameFromIndex(*(const int*)(item+nameOff),weaponName,sizeof(weaponName));
            seen[count++]=item;++reports;
            Log("body equipment census: pawn=%08X item=%08X object=%s class=%s weapon=%s heavy=%d heavyContract=%d nameOff=%X; ownership identity only, NO grenade/throw backend enabled",
                (unsigned)local,(unsigned)item,object,className,weaponName,heavy,proven?1:0,nameOff);
        }
    } __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){
        if(reports++<32)Log("body equipment census: unreadable context; no engine writes");
    }
}
}

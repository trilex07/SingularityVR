// Exact live combat-node identities captured read-only on 2026-11-02.
// All gameplay properties resolve through UE3 reflection; no captured addresses.
namespace {
static bool MutantRead(uintptr_t at, void* out, size_t bytes) {
    if(!at || !Readable((void*)at,bytes))return false;
    memcpy(out,(void*)at,bytes);return true;
}
static uintptr_t MutantPropertyAddress(uintptr_t object,const char* property) {
    uintptr_t cls=0;
    if(!MutantRead(object+OBJ_CLASS,&cls,4))return 0;
    const int off=PropOffsetInChain(cls,property);
    return off>=0 && off<0x10000 ? object+off:0;
}
static uintptr_t MutantPointer(uintptr_t object,const char* property) {
    uintptr_t value=0;MutantRead(MutantPropertyAddress(object,property),&value,4);return value;
}
static bool MutantNode(uintptr_t pawn,const char* outerName,const char* nodeName) {
    const uintptr_t node=MutantPointer(pawn,"mCombatNode");
    uintptr_t outer=0;int32_t nameIndex=-1;char name[96]{};
    if(!node || !MutantRead(node+OBJ_OUTER,&outer,4) ||
       !NameOf(outer,name,sizeof(name)) || strcmp(name,outerName))return false;
    if(!MutantRead(MutantPropertyAddress(node,"mNodeName"),&nameIndex,4) ||
       !NameFromIndex(nameIndex,name,sizeof(name)))return false;
    return !strcmp(name,nodeName);
}
struct MutantScene {
    uintptr_t Instigator(uintptr_t pawn)const {return MutantPointer(pawn,"mCurrentNodeInstigator");}
    bool PlayerInteraction(uintptr_t pawn)const {
        return IsLive(pawn) && MutantNode(pawn,"mutant_tackle_interaction_fs","mutant_player_interaction");
    }
    bool Attacking(uintptr_t object,uintptr_t pawn)const {
        uintptr_t cls=0;char name[80]{};float health=0;
        if(!object || !MutantRead(object+OBJ_CLASS,&cls,4) || !NameOf(cls,name,sizeof(name)) ||
           strcmp(name,"RvAI_PawnMutant") || !IsLive(object) ||
           MutantPointer(object,"mTarget")!=pawn ||
           !MutantRead(MutantPropertyAddress(object,"Health"),&health,4) ||
           !std::isfinite(health) || health<=0)return false;
        return MutantNode(object,"mutant_fs","Tackle_Loop");
    }
};
static bool MutantAssistedEnd(void* source,const float* start,const float* end,float* corrected) {
    if(!Rd(g_cinemaMutantAimAssist) || Rd(g_fireDepth)<=0 || (uintptr_t)source!=g_playerPawn ||
       !start || !end || !Readable(start,12) || !Readable(end,12))return false;
    // Native player node names its actual instigator. Revalidate both ends on
    // every shot; no GObjects scan, proximity heuristic, or retained pawn pointer.
    MutantScene scene;
    const uintptr_t attacker=cinemamutant::InteractionAttacker(scene,g_playerPawn);
    if(!attacker){
        static unsigned refused=0;
        if(refused++<4)Log("mutant scripted refusal: pawn=%08X live=%d interaction=%d instigator=%08X propertyOffsets node=%d instigator=%d; native ray retained",
            (unsigned)g_playerPawn,IsLive(g_playerPawn)?1:0,scene.PlayerInteraction(g_playerPawn)?1:0,
            (unsigned)scene.Instigator(g_playerPawn),
            PropOffsetInChain(g_playerPawnClass,"mCombatNode"),
            PropOffsetInChain(g_playerPawnClass,"mCurrentNodeInstigator"));
        return false;
    }
    float center[3]{};
    if(!MutantRead(attacker+ACTOR_LOCATION,center,sizeof(center)))return false;
    if(!cinemamutant::Redirect(true,true,true,(uintptr_t)source,g_playerPawn,attacker,start,end,center,corrected))return false;
    static unsigned receipts=0;
    if(receipts++<12)Log("mutant scripted aim: pawn=%08X target=%08X node=Tackle_Loop start=%g,%g,%g oldEnd=%g,%g,%g end=%g,%g,%g; native collision/damage/ammo retained",
        (unsigned)g_playerPawn,(unsigned)attacker,start[0],start[1],start[2],end[0],end[1],end[2],corrected[0],corrected[1],corrected[2]);
    return true;
}
}

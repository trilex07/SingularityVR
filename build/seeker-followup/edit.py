from pathlib import Path
p=Path('spikes/view_matrix/native_seeker_slot.h')
s=p.read_text()
s=s.replace('static unsigned receipts=0;', '''static unsigned receipts=0;
static const char* stage="not sampled";
static bool Fail(const char* why){stage=why;return false;}
static void Receipt(const seekerslot::Snapshot& s,bool forced=false){
    static const char* last=nullptr;static uint32_t lastPawn=0,lastCurrent=0,lastSeeker=0;
    static unsigned count=0;
    if(count>=64||(!forced&&last==stage&&lastPawn==s.pawn&&lastCurrent==s.current&&lastSeeker==state.seeker))return;
    last=stage;lastPawn=s.pawn;lastCurrent=s.current;lastSeeker=state.seeker;++count;
    Log("Seeker back gate: stage=%s pawn=%08X current=%08X ownedSeeker=%08X count=%u valid=%d hook=%p available=%08X thread=%lu",
        stage,s.pawn,s.current,state.seeker,s.count,s.valid,(void*)heavyTarget,(unsigned)Rd(g_seekerBackAvailable),GetCurrentThreadId());
}''')
s=s.replace('s={};s.pawn=(uint32_t)CameraOwnedPawn();','''s={};
    // PR60 physical log: cameraPawn=0, nativePawn=141F5000 during Seeker fire.
    // Reuse the reciprocal, typed native resolver already used by the game path.
    // No stale g_playerPawn fallback and no relaxation of ownership checks.
    s.pawn=(uint32_t)ReadNativeLocalPlayer().pawn;''')
# annotate Read refusal gates individually
start=s.index('static bool Read(');end=s.index('static int __fastcall Heavy',start)
part=s[start:end]
reasons=['local-pawn','pawn-fields','manager-chain-field','chain-bound-or-dead','chain-cycle','item-next-field','weapon-owner-or-name-field','weapon-name']
for reason in reasons:part=part.replace('return false;',f'return Fail("{reason}");',1)
part=part.replace('s.valid=true;return true;','s.valid=true;stage="ownership-ready";return true;')
s=s[:start]+part+s[end:]
s=s.replace('''    static MemberCache ctlField;
    const int offset=ctlField.Get(P(s.pawn+OBJ_CLASS),"Controller");
    const auto ctl=offset>=0?P(s.pawn+offset):0;''','''    const auto local=ReadNativeLocalPlayer();
    const auto ctl=local.pawn==s.pawn?local.controller:0;''')
s=s.replace('if(!item)return false;','if(!item)return Fail("no-owned-seeker");')
s=s.replace('if(!ctl||!IsLive(ctl)||!ClassHas(P(ctl+OBJ_CLASS),"RvPlayerController"))return false;','if(!ctl||!IsLive(ctl)||!ClassHas(P(ctl+OBJ_CLASS),"RvPlayerController"))return Fail("controller-reciprocal");')
s=s.replace('!NativeContextCursor::Method(heavy,13,0xAD9AEEB598427D84ull))return false;','!NativeContextCursor::Method(heavy,13,0xAD9AEEB598427D84ull))return Fail("switch-or-heavy-fingerprint");')
s=s.replace('if(attempts++>=2)return false;','if(attempts++>=2)return Fail("hook-attempt-limit");')
s=s.replace('!=MH_OK)return false;','!=MH_OK)return Fail("hook-create");')
s=s.replace('InterlockedExchange(&g_seekerBackSlot,0);return false;','InterlockedExchange(&g_seekerBackSlot,0);return Fail("hook-enable");')
s=s.replace('''        if(!requested||!ready||(uint32_t)requested!=s.pawn||!Gameplay())return;
        const auto target=state.ToggleTarget(s);if(!target)return;''','''        if(valid&&!state.seeker)stage="no-owned-seeker";
        else if(ready)stage="ready";
        Receipt(s);
        if(!requested)return;
        if(!ready||(uint32_t)requested!=s.pawn||!Gameplay()){
            stage=!ready?stage:(uint32_t)requested!=s.pawn?"request-pawn-changed":"request-context";
            Receipt(s,true);return;
        }
        const auto target=state.ToggleTarget(s);if(!target){stage="no-toggle-target";Receipt(s,true);return;}''')
s=s.replace('!NativeContextCursor::Method(selector,168,0xCB797F59504492C3ull))return;','!NativeContextCursor::Method(selector,168,0xCB797F59504492C3ull)){stage="selector-contract";Receipt(s,true);return;}')
s=s.replace('''            NativeOwnedInventory::Refresh();return;''','''            stage="selection-requested";Receipt(s,true);
            pendingPawn=s.pawn;pendingTarget=target;pendingUntil=now+1500;
            NativeOwnedInventory::Refresh();return;''')
s=s.replace('static void Poll(){','static uint32_t pendingPawn=0,pendingTarget=0;\nstatic ULONGLONG pendingUntil=0;\nstatic void Poll(){')
s=s.replace('''        const bool valid=Read(s);state.Reconcile(s);''','''        const bool valid=Read(s);state.Reconcile(s);
        if(pendingTarget&&(s.pawn!=pendingPawn||!valid||s.current==pendingTarget||now>=pendingUntil)){
            stage=!valid||s.pawn!=pendingPawn?"selection-context-lost":s.current==pendingTarget?"selection-equipped":"selection-timeout";
            Receipt(s,true);pendingPawn=pendingTarget=0;
        }''')
p.write_text(s)
# actual adapter tests now reproduce unavailable old camera resolver
p=Path('spikes/view_matrix/tests/test-seeker-native-adapter.cpp');s=p.read_text();s=s.replace('#include "../native_seeker_slot.h"','''#include "../native_local_player.h"
static nativeplayer::Identity ReadNativeLocalPlayer(){return {Ptr(controller),currentPawn,nativeplayer::Stage::Ready};}
#include "../native_seeker_slot.h"''');s=s.replace('static uintptr_t CameraOwnedPawn(){return currentPawn;}','static uintptr_t CameraOwnedPawn(){return 0;} // recorded PR60 failure')
p.write_text(s)

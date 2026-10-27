from pathlib import Path
p=Path('spikes/view_matrix/native_seeker_projectile.inl');s=p.read_text();s=s.replace('static unsigned notes=0;','''static unsigned notes=0;
static const char* localStage="not-sampled";
static void Gate(const char* why,uintptr_t weapon){
    static unsigned n=0;static const char* last=nullptr;static uintptr_t prior=0;
    if(n>=32||(last==why&&prior==weapon))return;
    last=why;prior=weapon;++n;
    Log("Seeker launch gate: stage=%s weapon=%p pawn=%p ready=%d spread=%d",why,(void*)weapon,(void*)g_playerPawn,ready,NativeScopePip::spreadOriginal!=nullptr);
}''')
s=s.replace('self!=(uintptr_t)(uint32_t)Rd(g_weaponObj)||!IsLive(self))return false;','self!=(uintptr_t)(uint32_t)Rd(g_weaponObj)||!IsLive(self)){localStage="context-or-tracking";return false;}')
s=s.replace('if(!NativeSeekerSlot::Read(s)||s.pawn!=g_playerPawn||s.current!=self)return false;','''if(!NativeSeekerSlot::Read(s)){localStage=NativeSeekerSlot::stage;return false;}
    if(s.pawn!=g_playerPawn||s.current!=self){localStage="pawn-or-current-mismatch";return false;}''')
s=s.replace('if(s.items[i].object==self&&s.items[i].seeker)return true;','if(s.items[i].object==self&&s.items[i].seeker){localStage="owned";return true;}')
s=s.replace('''    return false;
}
static seekerprojectile::Launch''','''    localStage="not-owned-seeker";return false;
}
static seekerprojectile::Launch''')
s=s.replace('''    const auto saved=active;active=Prepare(self);''','''    const auto saved=active;active=Prepare(self);
    if(!active.valid)Gate(localStage==std::string_view("owned")?"muzzle-witness-or-build":localStage,(uintptr_t)self);'''.replace('localStage==std::string_view("owned")','!strcmp(localStage,"owned")'))
s=s.replace('if(ready||!g_seekerProjectileAim||!NativeScopePip::spreadOriginal)return;','''if(ready||!g_seekerProjectileAim)return;
    if(!NativeScopePip::spreadOriginal){Gate("spread-hook-not-ready",(uintptr_t)(uint32_t)Rd(g_weaponObj));return;}''')
s=s.replace('if(!Local(weapon))return;','if(!Local(weapon)){Gate(localStage,weapon);return;}')
s=s.replace('!code(aim,213,0x538700ED62A99B3Eull)||!code(spawn,294,0x3DC3105D1A3C1A2Bull))return;','!code(aim,213,0x538700ED62A99B3Eull)||!code(spawn,294,0x3DC3105D1A3C1A2Bull)){Gate("method-fingerprint",weapon);return;}')
s=s.replace('if(!projectile)return;','if(!projectile){Gate("projectile-helper",weapon);return;}')
s=s.replace('if(attempts++>=2)return;','if(attempts++>=2){Gate("hook-attempt-limit",weapon);return;}')
s=s.replace('fireOriginal=nullptr;startOriginal=nullptr;spawnOriginal=nullptr;return;','fireOriginal=nullptr;startOriginal=nullptr;spawnOriginal=nullptr;Gate("hook-transaction",weapon);return;')
s=s.replace('barrel=(%.5f %.5f %.5f); native autoaim/guidance retained','barrel=(%.5f %.5f %.5f) expectedOrigin=(%.3f %.3f %.3f); native autoaim/guidance retained')
s=s.replace('active.axis.x,active.axis.y,active.axis.z);','active.axis.x,active.axis.y,active.axis.z,active.origin.x,active.origin.y,active.origin.z);')
p.write_text(s)
p=Path('spikes/view_matrix/tests/test-seeker-projectile-adapter.cpp');s=p.read_text().replace('namespace NativeSeekerSlot {','namespace NativeSeekerSlot {\nstatic const char* stage="test-ownership";');p.write_text(s)

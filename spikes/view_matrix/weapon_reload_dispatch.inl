// Included once in d3d9.cpp, after the existing native binding observer. Actual
// OpenXR -> CPU FSM -> native guard/tap -> native draw/overlay integration.
// All original asset reads occur at InitXRInput, NEVER from a draw hook.
static weaponreload::Kind NextKind(long vertices){
    for(const auto& p:weaponreloadproto::profiles)if(p.meshWedges==vertices)return p.kind;
    return weaponreload::Kind::None;
}
static bool NextObject(uintptr_t object,weaponreload::Kind kind){
    const auto* p=weaponreloadproto::Find(kind);if(!p||!object)return false;
    char name[80]{},cls[80]{};int32_t klass=0;
    if(!TryLoadAlignedI32(object+OBJ_CLASS,&klass)||!klass||
       !GuardedNameOf((uintptr_t)(uint32_t)klass,cls,sizeof(cls))||
       strcmp(cls,weaponreload::ClassName(kind))||!GuardedNameOf(object,name,sizeof(name)))return false;
    // Exact original archetypes from the three offline weapon packages, and
    // the base FNames already observed by RefreshArmedState. No substring match.
    const char* bare=p->objectName;
    return !strcmp(name,bare)||(!strncmp(name,"SP_",3)&&!strcmp(name+3,bare))||
        (!strncmp(name,"Shared_",7)&&!strcmp(name+7,bare));
}
static void ReleaseNextReloadGPU(){
    for(unsigned i=0;i<3;++i){
        g_nextClipRenderer[i].ReleaseGPU();hipknife::Release(g_nextIB[i]);
        g_nextIBVerified[i]=false;g_nextIBAttempts[i]=0;InterlockedExchange(&g_nextFault[i],0);
    }
    g_nextPresentation.Reset();
}
static void InitNextReload(){
    ReleaseNextReloadGPU();
    for(unsigned i=0;i<3;++i){
        g_nextReady[i]=false;g_nextAnimation[i].ready=false;
        if(!g_nextReload[i])continue;
        try{
        char path[MAX_PATH]{};if(!IniPath(path))continue;
        std::string directory(path);const auto slash=directory.find_last_of("\\/");if(slash==std::string::npos)continue;
        const auto kind=weaponreloadproto::profiles[i].kind;const std::string stem=weaponreload::Stem(kind);
        directory.resize(slash);directory+="\\vr-assets\\reload\\"+stem+"\\";
        std::vector<unsigned char> bytes,mesh,albedo;const auto& original=weaponreload::originalAssets[i];
        g_nextReady[i]=hipknife::Read(directory+stem+"_reload.svwa",bytes,256*1024)&&
            weaponreload::Hash(bytes.data(),(unsigned)bytes.size())==original.animation&&
            weaponreloadanim::Decode(bytes.data(),bytes.size(),kind,g_nextAnimation[i])&&
            g_nextTiming[i].Measure(g_nextAnimation[i])&&
            hipknife::Read(directory+stem+"_clip.svrk",mesh,2*1024*1024)&&
            weaponreload::Hash(mesh.data(),(unsigned)mesh.size())==original.mesh&&
            hipknife::Read(directory+stem+"_clip.svrt",albedo,16+2048*2048*4)&&
            weaponreload::Hash(albedo.data(),(unsigned)albedo.size())==original.albedo&&
            g_nextClipRenderer[i].LoadWithAlbedo(directory+stem+"_clip.svrk",directory+stem+"_clip.svrt")&&
            g_nextClipRenderer[i].Vertices()==original.vertices&&g_nextClipRenderer[i].Triangles()==original.triangles&&
            g_nextClipRenderer[i].TextureWidth()==original.width&&g_nextClipRenderer[i].TextureHeight()==original.height;
        Log("next reload: %s CPU assets=%s original clip=%u/%u timeline=%.0f/%.0f; NOT runtime proof",
            stem.c_str(),g_nextReady[i]?"READY":"INVALID (native unchanged)",
            g_nextClipRenderer[i].Vertices(),g_nextClipRenderer[i].Triangles(),
            g_nextTiming[i].outFrame,g_nextTiming[i].inFrame);
        }catch(const std::bad_alloc&){
            g_nextReady[i]=false;g_nextAnimation[i].ready=false;g_nextClipRenderer[i].ReleaseGPU();
            Log("next reload: CPU asset allocation failed profile=%u; native input unchanged",i);
        }
    }
}
static void UpdateNextReload(bool context,bool grip,bool supportLatched,bool circleDown){
    using namespace weaponreloadproto;
    const auto now=GetTickCount64();const auto kind=NextKind(Rd(g_gunVerts));
    const auto* profile=Find(kind);const unsigned index=weaponreload::Index(kind);
    const uintptr_t pawn=g_playerPawn,weapon=(uintptr_t)(uint32_t)Rd(g_weaponObj),hud=g_hudObject;
    int32_t health=0;
    const bool alive=pawn&&g_offHealth>=0&&TryLoadAlignedI32(pawn+g_offHealth,&health)&&health>0;
    const bool identity=profile&&index<3&&g_nextReload[index]&&g_nextReady[index]&&!Rd(g_nextFault[index])&&
        g_nextPresentation.Allowed()&&AimHand()==1&&OffHand()==0&&NextObject(weapon,kind);
    static centurionreload::AmmoObservation ammo;
    static uintptr_t hudClass=0;static int offsets[3]{-1,-1,-1};static ULONGLONG nextSample=0,nextResolve=0;
    const bool eligible=identity&&context&&alive&&pawn&&weapon&&hud;
    if(!eligible||(ammo.present&&(ammo.pawn!=(uint32_t)pawn||ammo.weapon!=(uint32_t)weapon||ammo.hud!=(uint32_t)hud))){
        ammo.Clear();nextSample=0;
    }
    if(eligible&&now>=nextSample){
        nextSample=now+100;int32_t klass=0;TryLoadAlignedI32(hud+OBJ_CLASS,&klass);
        if(klass&&(hudClass!=(uintptr_t)(uint32_t)klass||((offsets[0]<0||offsets[1]<0||offsets[2]<0)&&now>=nextResolve))){
            hudClass=(uintptr_t)(uint32_t)klass;nextResolve=now+1000;
            offsets[0]=PropOffsetInChain(hudClass,"mClipAmmo");offsets[1]=PropOffsetInChain(hudClass,"mBinAmmo");
            offsets[2]=PropOffsetInChain(hudClass,"mClipSize");
        }
        int values[3]{-1,-1,-1};for(unsigned i=0;i<3;++i)if(offsets[i]>=0){int32_t value=-1;
            if(TryLoadAlignedI32(hud+offsets[i],&value))values[i]=value;}
        ammo.Capture((uint32_t)pawn,(uint32_t)weapon,(uint32_t)hud,values,now);
        ObserveCenturionReloadBindings(weapon); // same registered wrapper, ONE shared hook
    }
    weaponreload::Witness witness;
    if(TryAcquireSRWLockShared(&g_nextVisualLock)){
        witness=g_nextWitness;ReleaseSRWLockShared(&g_nextVisualLock);
    }
    int32_t vt=0,method=0;const LONG slot=Rd(g_centurionNativeSlot);
    const bool native=identity&&slot&&TryLoadAlignedI32(weapon,&vt)&&vt&&
        TryLoadAlignedI32((uintptr_t)(uint32_t)vt+slot,&method)&&method==Rd(g_centurionNativeTarget);
    Frame f;f.kind=kind;f.objectName=profile?profile->objectName:nullptr;f.meshWedges=Rd(g_gunVerts);
    f.enabled=identity;f.aimHand=AimHand();f.magazineReleaseDown=circleDown;
    f.nativePermitRequired=true;f.nativePermitConsumed=(uint32_t)Rd(g_nextNativePermits)==(uint32_t)g_nextPermitStart+1u;
    f.liveIdentityVerified=native&&witness.Current(kind,(uint32_t)pawn,(uint32_t)weapon,now);
    auto& in=f.gesture;in.enabled=identity;in.context=eligible&&WeaponInHands()&&!Rd(g_weaponHolstered)&&
        g_xrSessionState==XR_SESSION_STATE_FOCUSED;in.pawn=(uint32_t)pawn;in.weapon=(uint32_t)weapon;
    in.supported=identity;in.tracked=g_handPositionValid[0]&&g_handPositionValid[1]&&
        g_handPoseValid[0]&&g_handPoseValid[1]&&g_handTracked[0]&&g_handTracked[1];
    in.clip=ammo.counts[0];in.reserve=ammo.counts[1];in.capacity=ammo.counts[2];in.nowMs=now;in.grip=grip;
    in.ammoFresh=ammo.Fresh((uint32_t)pawn,(uint32_t)weapon,(uint32_t)hud,now);
    in.firing=Rd(g_rawTrigger)>26;in.supportLatched=supportLatched;
    in.handBusy=hipknife::Reserves(g_hipKnifeState,0);
    const auto& left=g_handLastPos[0];const auto& right=g_handLastPos[1];
    const auto& lq=g_handAimOrientation[0];const auto& rq=g_handAimOrientation[1];
    in.hand={left.x,left.y,left.z};in.socket=witness.socket;
    in.gun={{right.x,right.y,right.z},{rq.x,rq.y,rq.z,rq.w}};
    Calibration calibration;calibration.ready=f.liveIdentityVerified;calibration.revision=1;
    calibration.gesture.socketLocal={};calibration.gesture.socketRadius=.16f;calibration.gesture.pullDistance=.30f;
    if(index<3&&g_nextReady[index])calibration.gesture.ackMs=std::max<uint64_t>(4000,
        (uint64_t)std::ceil(1000.f*g_nextAnimation[index].frames/g_nextAnimation[index].fps)+1500);
    const hipknife::V head{g_headPosXR[0],g_headPosXR[1],g_headPosXR[2]};
    hipknife::V shoulderHead=head;float shoulderYaw=g_centreYaw;
    if(g_nextRefillOnly){
        const auto& le=g_eyePose[0].position;const auto& re=g_eyePose[1].position;
        in.tracked=in.tracked&&g_haveEyes&&g_eyePositionsValid&&MidEye(
            {le.x,le.y,le.z},{re.x,re.y,re.z},shoulderHead);
        f.bodyYaw=g_headYawRad;
        shoulderYaw=weaponreload::Active(g_nextReloadState.transaction.phase)?
            g_nextReloadState.transaction.bodyYaw:f.bodyYaw;
    }
    const auto source=Supply(shoulderHead,shoulderYaw,calibration.shoulder);in.pouch=source.p;
    if(g_hipKnifeEnabled&&g_hipKnifeReady)in.knifeNear=centurionreload::Near(in.hand,
        hipknife::Hip(head,g_centreYaw,g_hipKnifeConfig).p,g_hipKnifeConfig.radius);
    const auto result=Update(g_nextReloadState,f,calibration);
    const auto& t=g_nextReloadState.transaction;
    const bool valid=in.context&&in.tracked&&in.ammoFresh&&centurionreload::AmmoValid(in)&&f.liveIdentityVerified;
    bool permit=false;ULONGLONG observedPermitMs=0;
    if(TryAcquireSRWLockExclusive(&g_centurionNativeLock)){
        const bool storedOwned=g_nextRefillOnly&&eligible&&Rd(g_weaponHolstered)&&native&&
            in.tracked&&in.ammoFresh&&centurionreload::AmmoValid(in)&&
            g_xrSessionState==XR_SESSION_STATE_FOCUSED;
        g_nextNativeGate.Publish({valid||storedOwned,in.pawn,in.weapon,now,
            !g_nextRefillOnly||!Rd(g_weaponHolstered),in.clip,in.reserve,in.capacity});
        if(valid&&result.reloadIntent){f.nativePermitConsumed=false;g_nextPermitStart=Rd(g_nextNativePermits);
            permit=g_nextNativeGate.Arm(in.pawn,in.weapon,now);}
        if(valid&&f.nativePermitConsumed)observedPermitMs=g_nextNativePermitMs;
        ReleaseSRWLockExclusive(&g_centurionNativeLock);
    }
    if(result.reloadIntent){
        if(permit)g_nextReloadTap.Update(true,true,now);
        else {g_nextReloadTap.Cancel();g_nextReloadState.transaction.phase=Phase::Cooldown;}
    }
    if(!valid||t.phase!=Phase::WaitingNative)g_nextReloadTap.Cancel();
    g_nextOwnsGrip=result.reservePhysicalLeft;
    g_nextBlockFire=valid&&weaponreload::Active(t.phase);
    if(result.event==Event::GrabFresh)g_supportLatch.Reset();
    if(TryAcquireSRWLockExclusive(&g_nextVisualLock)){
        NextReloadVisual next{in.context&&in.tracked&&identity,kind,t.phase,in.pawn,in.weapon,now,t.since,
            {in.hand,{lq.x,lq.y,lq.z,lq.w}},source,in.gun};
        if(g_nextVisual.weapon==in.weapon&&g_nextVisual.pawn==in.pawn&&next.valid){
            next.drop=g_nextVisual.drop;next.dropMs=g_nextVisual.dropMs;
            next.rootValid=g_nextVisual.rootValid;next.root=g_nextVisual.root;next.nativeEnd=g_nextVisual.nativeEnd;
            next.insertedMs=g_nextVisual.insertedMs;
        }
        if(result.ejectMagazine){next.drop=witness.clipTracking;next.dropMs=now;
            next.rootValid=true;next.root=witness.nativeRoot;next.nativeEnd=0;next.insertedMs=0;}
        if(result.reloadIntent&&permit&&index<3){
            next.insertedMs=now;
            next.nativeEnd=now+(g_nextRefillOnly?weaponreload::Animation::RefillSeatMs:
                (ULONGLONG)std::ceil(1000.f*g_nextAnimation[index].frames/g_nextAnimation[index].fps)+300);
        }
        if(!g_nextRefillOnly&&next.rootValid&&observedPermitMs&&index<3)next.nativeEnd=observedPermitMs+
            (ULONGLONG)std::ceil(1000.f*g_nextAnimation[index].frames/g_nextAnimation[index].fps)+150;
        if(result.event==Event::Cancelled||result.event==Event::Rejected||
           (!weaponreload::Active(t.phase)&&(!next.nativeEnd||now>=next.nativeEnd)))next.rootValid=false;
        if(result.event==Event::Discarded&&index<3&&weaponreload::HandMatrix(witness.clipTracking,in.gun,
            next.hand,g_nextClipRenderer[index].SourceCentre(),next.drop))next.dropMs=now;
        g_nextVisual=next;ReleaseSRWLockExclusive(&g_nextVisualLock);
    }
    static unsigned records=0;static int priorReady=-1;const int ready=valid?int(kind):0;
    if(records<160&&(result.event!=Event::None||priorReady!=ready)){
        priorReady=ready;++records;
        Log("next reload: %s kind=%d phase=%d pawn=%08X weapon=%08X clip=%d reserve=%d size=%d ready=%d native=%d renderWitness=%d permit=%d consumed=%d nativePermits=%ld nativeBlocks=%ld socketDist=%.3f shoulderDist=%.3f t=%llu nativeMagazineDraws=%ld (ammo observer; no writes)",
            Name(result.event),int(kind),int(t.phase),in.pawn,in.weapon,in.clip,in.reserve,in.capacity,
            valid?1:0,native?1:0,f.liveIdentityVerified?1:0,permit?1:0,f.nativePermitConsumed?1:0,
            Rd(g_nextNativePermits),Rd(g_nextNativeBlocks),
            hipknife::Length(in.hand-in.socket),hipknife::Length(in.hand-in.pouch),(unsigned long long)now,Rd(g_nextNativeMagazineDraws));
    }
}
static int PrepareNextReload(IDirect3DDevice9* dev,INT base,UINT min,UINT vertices,UINT first,UINT triangles,float* saved,UINT& reg,
                             weaponreload::ReplayPlan& replay){
    if(!NextReloadEnabled())return 0;
    const auto kind=NextKind(Rd(g_gunVerts));const auto* spec=weaponreloadanim::Find(kind);
    if(!spec)return 0;const unsigned index=weaponreload::Index(kind);
    const auto part=weaponreload::DrawPart(kind,base,min,vertices,first,triangles);
    if(!g_nextReload[index]||!g_nextReady[index]||Rd(g_nextFault[index])||!g_nextPresentation.Allowed()||
       part==weaponreload::Part::None||
       !dev||!g_origSetVSConstF||GetCurrentThreadId()!=g_mainThreadId||!FgMatchWindow()||g_thisDrawMoved!=1||!Rd(g_rtIsScene)||
       Rd(g_gamePaused)||Rd(g_vrUiMenu)||Rd(g_panelOpen)||Rd(g_weaponWheelOpen)||
       Rd(g_weaponHolstered)||CutsceneNativeCameraArmed()||
       g_sceneMode.Current()!=vrscene::Mode::GameplayStereo||g_xrSessionState!=XR_SESSION_STATE_FOCUSED)return 0;
    NextReloadVisual visual;
    if(!TryAcquireSRWLockShared(&g_nextVisualLock))return 0;
    visual=g_nextVisual;ReleaseSRWLockShared(&g_nextVisualLock);
    const auto now=GetTickCount64();
    if(!visual.valid||visual.kind!=kind||visual.pawn!=(uint32_t)g_playerPawn||visual.weapon!=(uint32_t)Rd(g_weaponObj)||
       now<visual.sampledMs||now-visual.sampledMs>150)return 0;
    const auto layout=g_boundShaderLayout;
    const bool rootHeld=visual.rootValid&&(weaponreload::Active(visual.phase)||
        (visual.nativeEnd&&now<visual.nativeEnd));
    const unsigned regs=part==weaponreload::Part::Attachment?3:spec->slots*3;
    if(layout.shadow||layout.bones<0||layout.boneRegs<int(regs)||unsigned(layout.bones)+regs>256||
       (part==weaponreload::Part::Body&&(layout.local<0||unsigned(layout.local)+4>256)))return 0;
    IDirect3DIndexBuffer9* ib=nullptr;if(FAILED(dev->GetIndices(&ib))||!ib)return 0;
    if(g_nextIB[index]!=ib){
        hipknife::Release(g_nextIB[index]);g_nextIB[index]=ib;ib=nullptr;g_nextIBVerified[index]=false;
        if(g_nextIBAttempts[index]<16){
            ++g_nextIBAttempts[index];D3DINDEXBUFFER_DESC desc{};void* bytes=nullptr;
            if(SUCCEEDED(g_nextIB[index]->GetDesc(&desc))&&desc.Format==D3DFMT_INDEX16&&desc.Size==spec->indexCount*2&&
               SUCCEEDED(g_nextIB[index]->Lock(0,desc.Size,&bytes,D3DLOCK_READONLY))){
                const bool hash=bytes&&weaponreload::Hash((const unsigned char*)bytes,desc.Size)==spec->indexHash;
                g_nextIBVerified[index]=SUCCEEDED(g_nextIB[index]->Unlock())&&hash;
            }
        }
    }
    hipknife::Release(ib);if(!g_nextIBVerified[index])return 0;
    armbind::Mat local,controller;float changed[16*12],parentPalette[16*12];
    if(FAILED(dev->GetVertexShaderConstantF((UINT)layout.bones,saved,regs)))return 0;
    if(part==weaponreload::Part::Attachment){
        // Original second palette is [cog_gun] for Valkyrie and Kasimov.
        // Freeze it too; otherwise the scope/lens follows native Reload while
        // the main receiver remains attached to the physical controller.
        if(!rootHeld||!armbind::Finite(visual.root))return 0;
        armbind::ToGpu(visual.root,reinterpret_cast<float(*)[4]>(changed));
    }else{
    if(FAILED(dev->GetVertexShaderConstantF((UINT)layout.local,&local.m[0][0],4)))return 0;
    if(!weaponreload::ParentPalette(g_nextAnimation[index],saved,regs,rootHeld?&visual.root:nullptr,parentPalette))return 0;
    armbind::Mat nativeRoot;
    if(!armbind::FromGpu(reinterpret_cast<const float(*)[4]>(parentPalette+spec->root*12),nativeRoot))return 0;
    BuildGunC(controller.m);hipknife::V socket;armbind::Mat clipTracking;
    if(!weaponreload::Socket(g_nextAnimation[index],parentPalette,local,controller,
        {g_centrePos[0],g_centrePos[1],g_centrePos[2]},g_centreYaw,g_baseYaw*(6.2831853f/65536.f),MetresToUU(),socket,&clipTracking))return 0;
    // Refuse an absolute/other-space matrix rather than publishing a far-away
    // or frozen interaction point. This is a validity bound, not calibration.
    const auto& hand=g_handLastPos[1];
    if(hipknife::Length(socket-hipknife::V{hand.x,hand.y,hand.z})>.75f)return 0;
    if(TryAcquireSRWLockExclusive(&g_nextVisualLock)){
        g_nextWitness={true,kind,visual.pawn,visual.weapon,now,socket,clipTracking,nativeRoot};ReleaseSRWLockExclusive(&g_nextVisualLock);
    }
    const bool drop=g_nextNativeMagazine&&visual.dropMs&&now>=visual.dropMs&&now-visual.dropMs<1200;
    if(!weaponreload::Active(visual.phase)&&!rootHeld&&!drop)return 0;
    if(weaponreload::Active(visual.phase)||rootHeld){
        const float frame=g_nextRefillOnly?g_nextTiming[index].RefillFrame(g_nextAnimation[index],visual.phase,
            visual.since,visual.insertedMs,now):g_nextTiming[index].Frame(g_nextAnimation[index],visual.phase,visual.since,now);
        if(!weaponreload::Palette(g_nextAnimation[index],frame,weaponreload::Absent(visual.phase),parentPalette,regs,changed))return 0;
    }else std::memcpy(changed,parentPalette,regs*4*sizeof(float));
    if(g_nextNativeMagazine){
        armbind::Mat world;
        if(!weaponreload::TrackingMap({g_centrePos[0],g_centrePos[1],g_centrePos[2]},g_centreYaw,
            g_baseYaw*(6.2831853f/65536.f),MetresToUU(),world))return 0;
        replay.index=index;replay.regs=regs;
        if(visual.phase==weaponreload::Phase::NeedAmmo||visual.phase==weaponreload::Phase::Carrying){
            const auto pose=visual.phase==weaponreload::Phase::Carrying?visual.hand:visual.source;
            armbind::Mat carried;
            if(!weaponreload::HandMatrix(clipTracking,visual.aim,pose,g_nextClipRenderer[index].SourceCentre(),carried,
                    visual.phase==weaponreload::Phase::Carrying?hipknife::V{0,0,-.06f}:hipknife::V{})||
               !weaponreload::SparePalette(g_nextAnimation[index],carried,local,controller,world,regs,replay.spare[replay.count]))return 0;
            ++replay.count;
        }
        if(drop){
            auto falling=visual.drop;const float elapsed=float(now-visual.dropMs)*.001f;
            falling.m[3][1]-=.5f*9.81f*elapsed*elapsed;
            if(!weaponreload::SparePalette(g_nextAnimation[index],falling,local,controller,world,regs,replay.spare[replay.count]))return 0;
            ++replay.count;
        }
        if(replay.count)std::memcpy(replay.attached,changed,regs*4*sizeof(float));
    }
    }
    reg=(UINT)layout.bones;
    if(FAILED(g_origSetVSConstF(dev,reg,changed,regs))){
        if(FAILED(g_origSetVSConstF(dev,reg,saved,regs)))InterlockedExchange(&g_nextFault[index],1);
        return 0;
    }
    return int(regs); // caller restores after BOTH StereoPair eyes, even failure
}
static void DrawNextReload(IDirect3DDevice9* dev){
    if(g_nextNativeMagazine)return; // native per-eye replay owns stock/carry/drop, no private-depth clear
    if(!NextReloadEnabled()||!dev||!g_nextPresentation.Allowed()||!VrModeOn()||!InGameplay()||!g_haveEyes||
       !Rd(g_dupDraws)||Rd(g_gamePaused)||Rd(g_vrUiMenu)||Rd(g_panelOpen)||Rd(g_weaponWheelOpen)||
       CutsceneNativeCameraArmed()||g_xrSessionState!=XR_SESSION_STATE_FOCUSED)return;
    NextReloadVisual visual;weaponreload::Witness witness;
    if(!TryAcquireSRWLockShared(&g_nextVisualLock))return;
    visual=g_nextVisual;witness=g_nextWitness;ReleaseSRWLockShared(&g_nextVisualLock);
    const auto now=GetTickCount64();const unsigned index=weaponreload::Index(visual.kind);
    if(!visual.valid||index>=3||!g_nextReady[index]||Rd(g_nextFault[index])||
       visual.pawn!=(uint32_t)g_playerPawn||visual.weapon!=(uint32_t)Rd(g_weaponObj)||
       now<visual.sampledMs||now-visual.sampledMs>150)return;
    IDirect3DSurface9* back=nullptr;D3DSURFACE_DESC desc{};
    if(FAILED(dev->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back))||!back)return;
    if(FAILED(back->GetDesc(&desc))||!g_nextClipRenderer[index].Ensure(dev,desc.Width,desc.Height)){
        InterlockedExchange(&g_nextFault[index],1);back->Release();return;
    }
    const hipknife::Calls calls{shadowNativeIndexedUP,g_origSetRenderTarget,g_origSetVS,
        shadowOriginalPS,g_origSetTexture,shadowNativeClear,g_origSetViewport};
    if(!calls.Valid()){back->Release();return;}
    hipknife::Pose eyes[2];for(unsigned e=0;e<2;++e){const auto& p=g_eyePose[e];
        eyes[e]={{p.position.x,p.position.y,p.position.z},{p.orientation.x,p.orientation.y,p.orientation.z,p.orientation.w}};}
    hipknife::StateGuard<> guard(dev,g_nextClipRenderer[index].SharedState(),calls);
    const auto result=g_nextPresentation.Render(guard,[&](){
        bool ok=true;
        if(visual.phase==weaponreload::Phase::NeedAmmo||visual.phase==weaponreload::Phase::Carrying){
            const auto pose=visual.phase==weaponreload::Phase::Carrying?visual.hand:visual.source;
            armbind::Mat matrix;
            if(!witness.Current(visual.kind,visual.pawn,visual.weapon,now)||
               !weaponreload::HandMatrix(witness.clipTracking,visual.aim,pose,g_nextClipRenderer[index].SourceCentre(),matrix,
                   visual.phase==weaponreload::Phase::Carrying?hipknife::V{0,0,-.06f}:hipknife::V{}))return false;
            ok=g_nextClipRenderer[index].Draw(dev,calls,back,pose,eyes,g_targetTanX,g_targetTanY,matrix.m);
        }
        if(ok&&visual.dropMs&&now>=visual.dropMs&&now-visual.dropMs<1200){
            auto pose=visual.drop;const float elapsed=float(now-visual.dropMs)*.001f;
            pose.m[3][1]-=.5f*9.81f*elapsed*elapsed;
            ok=g_nextClipRenderer[index].Draw(dev,calls,back,{},eyes,g_targetTanX,g_targetTanY,pose.m);
        }
        return ok;
    });
    if(result!=centurionclip::DrawResult::Success)InterlockedExchange(&g_nextFault[index],1);
    back->Release();
}

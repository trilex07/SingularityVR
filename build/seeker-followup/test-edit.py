from pathlib import Path
p=Path('tools/reload/test-seeker.ps1');s=p.read_text();s=s.replace('Push-Location $out', '''# Use the real production name registry: synthetic offsets must not hide
# unregistered names (the PR60 ownership gate defect).
$source=Get-Content (Join-Path $root 'spikes/view_matrix/d3d9.cpp') -Raw
$start=$source.IndexOf('g_wantNames[]')
if($start -lt 0){throw 'Missing production name registry'}
$end=$source.IndexOf('const int kWantNameCount',$start)
$names=[regex]::Matches($source.Substring($start,$end-$start),'\\{\\s*"([^"]+)"\\s*,\\s*-1\\s*\\}')
$literal=($names | ForEach-Object { '"'+$_.Groups[1].Value+'"' }) -join ','
('static const char* productionNames[]={' + $literal + '};') | Set-Content (Join-Path $out 'seeker_registered_names.h') -Encoding ascii
Push-Location $out''');s=s.replace('/std:c++17 `"$src`"','/std:c++17 /I. `"$src`"');p.write_text(s)
p=Path('spikes/view_matrix/tests/test-seeker-native-adapter.cpp');s=p.read_text();s=s.replace('struct MemberCache {int Get(uintptr_t,const char* n){','''#include "seeker_registered_names.h"
struct MemberCache {int Get(uintptr_t,const char* n){
    bool registered=false;for(auto name:productionNames)if(!strcmp(name,n))registered=true;
    if(!registered)return -1;''');s=s.replace('static unsigned selected=0;','static unsigned selected=0;\nstatic bool selectorCompletes=true;');s=s.replace('fields[Ptr(pawn)+0x3d4]=n.index==10?Ptr(seeker):Ptr(rifle);','if(selectorCompletes)fields[Ptr(pawn)+0x3d4]=n.index==10?Ptr(seeker):Ptr(rifle);');s=s.replace('''    assert(NativeSeekerSlot::Read(s));NativeSeekerSlot::heavyTarget=0;failEnable=true;''','''    // A selector returning is not proof of equip. Keep pending until observed.
    selectorCompletes=false;Request();assert(NativeSeekerSlot::pendingTarget==Ptr(seeker));
    NativeSeekerSlot::pendingUntil=0;NativeSeekerSlot::Poll();
    // Poll is normally throttled; force a fresh engine sample without acquisition.
    Sleep(110);NativeSeekerSlot::Poll();assert(!NativeSeekerSlot::pendingTarget);
    assert(fields[Ptr(pawn)+0x3d4]==Ptr(rifle));selectorCompletes=true;
    Request();assert(NativeSeekerSlot::pendingTarget==Ptr(seeker));
    Sleep(110);NativeSeekerSlot::Poll();assert(!NativeSeekerSlot::pendingTarget);
    assert(NativeSeekerSlot::Read(s));NativeSeekerSlot::heavyTarget=0;failEnable=true;''');p.write_text(s)
p=Path('spikes/view_matrix/tests/test-seeker.cpp');s=p.read_text();s=s.replace('assert(BackGrip(true,true,true,false,true,at,head,yaw));','''assert(BackGrip(true,true,true,false,true,at,head,yaw));
        assert(BackGripGate(true,true,true,false,false,at,head,yaw)==GripGate::Unavailable);
        assert(BackGripGate(true,false,true,false,true,at,head,yaw)==GripGate::Context);
        assert(BackGripGate(true,true,false,false,true,at,head,yaw)==GripGate::Tracking);
        assert(BackGripGate(true,true,true,true,true,at,head,yaw)==GripGate::Busy);
        assert(BackGripGate(true,true,true,false,true,at+hipknife::V{0,.5f,0},head,yaw)==GripGate::Outside);''');p.write_text(s)

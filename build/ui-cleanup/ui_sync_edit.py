from pathlib import Path
p=Path('spikes/view_matrix/d3d9.cpp');s=p.read_text(encoding='utf-8')
s=s.replace('static bool g_vrPromptTokens=false;', '#include "vr_legacy_prompts.h"\nstatic bool g_vrHideLegacyPadPrompts=true;\nstatic volatile LONG g_vrPromptControls[vrprompts::Count]{};\nstatic bool g_vrPromptTokens=false;',1)
anchor='const int kPadButtonCount = (int)(sizeof(g_padButtons) / sizeof(g_padButtons[0]));'
code='''
// Bounded source snapshot after successful xrSyncActions. Read by the native
// localization hook through atomic enum values; no engine-thread XR calls.
static vrprompts::Control BoundPromptControl(XrAction action,bool rightAxis=false){
    if(!action||!g_xrSession)return vrprompts::Unbound;
    XrBoundSourcesForActionEnumerateInfo info{XR_TYPE_BOUND_SOURCES_FOR_ACTION_ENUMERATE_INFO};
    info.action=action;XrPath sources[16]{};uint32_t count=0;
    if(XR_FAILED(xrEnumerateBoundSourcesForAction(g_xrSession,&info,16,&count,sources))||count>16)return vrprompts::Unbound;
    auto result=vrprompts::Unbound;
    for(uint32_t i=0;i<count;++i){char path[XR_MAX_PATH_LENGTH]{};uint32_t bytes=0;
        if(XR_FAILED(xrPathToString(g_xrInstance,sources[i],sizeof(path),&bytes,path)))continue;
        auto control=vrprompts::Path(path);
        if(rightAxis&&!strcmp(path,"/user/hand/right/input/thumbstick"))control=vrprompts::RightUp;
        if(control==vrprompts::Unbound)continue;
        if(result!=vrprompts::Unbound&&result!=control)return vrprompts::Unbound;
        result=control;
    }
    return result;
}
static void UpdateVrPromptControls(){
    if(!g_vrHideLegacyPadPrompts)return;
    static ULONGLONG last=0;const auto now=GetTickCount64();if(last&&now-last<1000)return;last=now;
    vrprompts::Control c[vrprompts::Count]{};
    for(int i=0;i<kPadButtonCount;++i){const auto& b=g_padButtons[i];
        auto a=vrprompts::Token(b.name,strlen(b.name));
        if(a!=vrprompts::Unknown)c[a]=BoundPromptControl(b.action);
    }
    c[vrprompts::Fire]=BoundPromptControl(g_actFire);
    c[vrprompts::Aim]=BoundPromptControl(g_actAltFire);
    c[vrprompts::Cycle]=BoundPromptControl(g_actR3);
    c[vrprompts::Reload]=c[vrprompts::Use];
    const bool layout=g_reloadControlLayout&&AimHand()==1&&!Rd(g_swapSticks);
    if(layout){
        c[vrprompts::Reload]=BoundPromptControl(g_actUiBack);
        c[vrprompts::Health]=BoundPromptControl(g_actUiSelect);
        c[vrprompts::Jump]=BoundPromptControl(g_actTurn,true);
    }
    if(BoundPromptControl(g_actTurn,true)==vrprompts::RightUp)c[vrprompts::Crouch]=vrprompts::RightDown;
    for(int i=0;i<vrprompts::Count;++i)InterlockedExchange(&g_vrPromptControls[i],c[i]);
}
'''
assert anchor in s;s=s.replace(anchor,anchor+code,1)
s=s.replace('if (!pollOff) DiagPhaseAdd(DP_SYNC, diagSync);','if (!pollOff) DiagPhaseAdd(DP_SYNC, diagSync);\n    if(!pollOff && sr==XR_SUCCESS)UpdateVrPromptControls();',1)
s=s.replace('const bool prompts=g_vrPromptTokens&&VrModeOn();','const bool cleanup=g_vrHideLegacyPadPrompts&&VrModeOn();\n    const bool prompts=(g_vrPromptTokens||cleanup)&&VrModeOn();',1)
old='rewritten=prompttokens::Rewrite(visible,prompt,sizeof(prompt),layout,AimHand()==1,polish);'
new='''if(cleanup){
            vrprompts::Control controls[vrprompts::Count]{};
            for(int i=0;i<vrprompts::Count;++i)controls[i]=(vrprompts::Control)InterlockedCompareExchange(&g_vrPromptControls[i],0,0);
            char sec[128]{},ky[128]{};
            if(section)WideFieldToUtf8(section,sec,sizeof(sec));
            if(key)WideFieldToUtf8(key,ky,sizeof(ky));
            // Notes/subtitles are not action instruction producers.
            if(!PolishNotesSection(section)){
                const auto clean=vrprompts::Rewrite(visible,prompt,sizeof(prompt),controls,polish,sec,ky);
                rewritten.changed=clean.changed;rewritten.overflow=clean.overflow;
            }
        }else rewritten=prompttokens::Rewrite(visible,prompt,sizeof(prompt),layout,AimHand()==1,polish);'''
assert old in s;s=s.replace(old,new,1)
s=s.replace('g_vrPromptTokens=GetPrivateProfileIntA("Render","VrPromptTokens",0,path)!=0;','g_vrPromptTokens=GetPrivateProfileIntA("Render","VrPromptTokens",0,path)!=0;\n    g_vrHideLegacyPadPrompts=GetPrivateProfileIntA("Render","VrHideLegacyPadPrompts",1,path)!=0;',1)
p.write_text(s,encoding='utf-8')

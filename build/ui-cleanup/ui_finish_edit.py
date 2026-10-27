from pathlib import Path
p=Path('spikes/view_matrix/d3d9.cpp');s=p.read_text(encoding='utf-8')
s=s.replace('static LONG g_uiRemasterRuntime=0,g_uiRemasterSettings=0,g_uiRemasterWheel=0,g_uiRemasterState=0;',
 'static LONG g_uiRemasterRuntime=1,g_uiRemasterSettings=1,g_uiRemasterWheel=1,g_uiRemasterMenus=1,g_uiRemasterState=0;')
s=s.replace('(!Rd(g_uiRemasterRuntime)&&!Rd(g_uiRemasterWheel))','(!Rd(g_uiRemasterRuntime)&&!Rd(g_uiRemasterWheel)&&!Rd(g_uiRemasterMenus))')
for key in ['UiRemasterRuntime','UiRemasterSettings','UiRemasterWheel']:
 s=s.replace('GetPrivateProfileIntA("Render","'+key+'",0,path)','GetPrivateProfileIntA("Render","'+key+'",1,path)')
needle='InterlockedExchange(&g_uiRemasterWheel,GetPrivateProfileIntA("Render","UiRemasterWheel",1,path)?1:0);'
s=s.replace(needle,needle+'\n    InterlockedExchange(&g_uiRemasterMenus,GetPrivateProfileIntA("Render","UiRemasterMenus",1,path)?1:0);')
p.write_text(s,encoding='utf-8')
p=Path('spikes/view_matrix/SingularityVR.ini');s=p.read_text(encoding='utf-8')
for key in ['UiRemasterRuntime','UiRemasterSettings','UiRemasterWheel']:s=s.replace(key+'=0',key+'=1')
s=s.replace('[Render]','[Render]\n; UI V2.1: exact rollback switches; missing atlases retain native rendering.\nUiRemasterMenus=1\n; Rewrite known action tokens using bound XR sources; never hide all GFx text.\nVrHideLegacyPadPrompts=1',1)
s=s.replace('separately default OFF','separately switchable').replace('default OFF','optional')
p.write_text(s,encoding='utf-8')

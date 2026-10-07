# GFx / Scaleform reconnaissance

Bounded, read-only. No hooks, no patches, no game launch.

Binary `C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries\Singularity.exe` (27444736 bytes, 8 PE sections).

RVA mapping via PE section headers; file+offset always given alongside. The game may use ASLR - RVAs are image-relative, not absolute addresses, and **no address below is claimed as a hook target**.

## PROVEN (17)

### `Scaleform` - vendor/sdk marker

Binary occurrences (up to 8 shown):

- file+0x16416B8 rva=0x16436B8 [.rdata] (ascii)

Sources/notes:

- `SingularityVR.ini.example:676`: ; The UI is Scaleform, submitted through DrawPrimitiveUP with its stage->clip
- `HISTORY.md:2184`: ### Why the settings are NOT in the game's menu — the Scaleform spike

### `GFx` - Scaleform GFx prefix

Binary occurrences (up to 8 shown):

- file+0x60EF3A rva=0x61013A [.text] (ascii)
- file+0x1630098 rva=0x1632098 [.rdata] (ascii)
- file+0x16300E0 rva=0x16320E0 [.rdata] (ascii)
- file+0x1630158 rva=0x1632158 [.rdata] (ascii)
- file+0x16301D8 rva=0x16321D8 [.rdata] (ascii)
- file+0x16302D4 rva=0x16322D4 [.rdata] (ascii)
- file+0x16302F0 rva=0x16322F0 [.rdata] (ascii)
- file+0x16303A4 rva=0x16323A4 [.rdata] (ascii)
- ... and 39 more

Sources/notes:

- `d3d9.cpp:5213`: // Defined here so the SetTexture hook can identify the private GFx sink before the overlay header
- `d3d9.cpp:10037`: // Before the vec4Count<4 bail below, deliberately: GFx sets its transform in small uploads and
- `d3d9.cpp:10559`: // GFx menu movies render into a private colour target before the scene samples that
- `d3d9.cpp:14939`: // of GFx's own - v0.x of 5534 on a 2560-wide backbuffer is neither NDC nor pixels. So something at
- `d3d9.cpp:14941`: // Two candidates, and 2010-era GFx could use either:

### `GFxMovie` - movie object

Binary occurrences (up to 8 shown):

- file+0x14F22E8 rva=0x14F42E8 [.rdata] (utf16le)
- file+0x14F2330 rva=0x14F4330 [.rdata] (utf16le)
- file+0x14F2374 rva=0x14F4374 [.rdata] (utf16le)
- file+0x14F23A0 rva=0x14F43A0 [.rdata] (utf16le)
- file+0x14F23E0 rva=0x14F43E0 [.rdata] (utf16le)
- file+0x14F2428 rva=0x14F4428 [.rdata] (utf16le)
- file+0x14F2478 rva=0x14F4478 [.rdata] (utf16le)
- file+0x14F24C4 rva=0x14F44C4 [.rdata] (utf16le)
- ... and 17 more

Sources/notes:

- `native_ui_input.h:25`: static const char* names[]={"RvUIGFxMovie_Main","RvUIGFxMovie_IIS","RvUIGFxMovie_Pause",
- `native_ui_input.h:26`: "RvUIGFxMovie_Dialog","RvUIGFxMovie_Slots","RvUIGFxMovie_SlotsBase","RvUIGFxMovie_LoadGame",
- `native_ui_input.h:27`: "RvUIGFxMovie_Review","RvUIGFxMovie_Credits","RvUIGFxMovie_Notepad","RvUIGFxMovie_Stats",
- `native_ui_input.h:28`: "RvUIGFxMovie_UseStation","RvUIGFxMovie_BarisovStation","RvUIGFxMovie_WeaponLocker"};
- `native_ui_input.h:29`: if(!strncmp(n,"RvUIGFxMovie_Options",strlen("RvUIGFxMovie_Options")))return true;

### `MovieImpl` - movie implementation

Binary occurrences (up to 8 shown):

- file+0x1639134 rva=0x163B134 [.rdata] (ascii)

### `MovieView` - movie view interface

Binary occurrences (up to 8 shown):

- file+0x1633ED4 rva=0x1635ED4 [.rdata] (ascii)
- file+0x1639644 rva=0x163B644 [.rdata] (ascii)

### `MovieDef` - movie definition interface

Binary occurrences (up to 8 shown):

- file+0x16341C0 rva=0x16361C0 [.rdata] (ascii)
- file+0x16395D0 rva=0x163B5D0 [.rdata] (ascii)

### `SetViewport` - viewport binding on the movie

Binary occurrences (up to 8 shown):

- file+0x14F2C84 rva=0x14F4C84 [.rdata] (utf16le)

Sources/notes:

- `d3d9.cpp:12035`: dev->SetViewport(&vp);
- `d3d9.cpp:12069`: dev->SetViewport(&full);
- `d3d9.cpp:12094`: //   * SetViewport with a changed MinZ/MaxZ - the classic depth-range compression for a foreground
- `d3d9.cpp:12127`: // drawn again at draws 2-3 under SetViewport MinZ 0 MaxZ 0 - a depth-prime at the near plane, the
- `d3d9.cpp:14359`: typedef HRESULT (STDMETHODCALLTYPE *PFN_SetViewport)(IDirect3DDevice9*, const D3DVIEWPORT9*);

### `GetViewport` - viewport query

Binary occurrences (up to 8 shown):

- file+0x1449C88 rva=0x144BC88 [.rdata] (utf16le)
- file+0x1462A30 rva=0x1464A30 [.rdata] (utf16le)
- file+0x1462A80 rva=0x1464A80 [.rdata] (utf16le)
- file+0x1462AD0 rva=0x1464AD0 [.rdata] (utf16le)
- file+0x1462B38 rva=0x1464B38 [.rdata] (utf16le)
- file+0x1462B88 rva=0x1464B88 [.rdata] (utf16le)
- file+0x1462BD8 rva=0x1464BD8 [.rdata] (utf16le)

Sources/notes:

- `d3d9.cpp:10737`: if (FAILED(dev->GetViewport(&full)) \|\| full.Width < 4) return draw();
- `d3d9.cpp:11984`: if (SUCCEEDED(dev->GetViewport(&full)) && full.Width >= 4) {
- `d3d9.cpp:15278`: if (SUCCEEDED(dev->GetViewport(&vp)))
- `d3d9.cpp:15379`: if (FAILED(dev->GetVertexShaderConstantF(5, c, 4)) \|\| FAILED(dev->GetViewport(&vp)))
- `d3d9.cpp:19888`: if (FAILED(dev->GetViewport(&vp)) \|\| vp.Width < 4) return;

### `AS2` - ActionScript 2 marker

Binary occurrences (up to 8 shown):

- file+0xB69073 rva=0xB6A273 [.text] (ascii)

### `AS3` - ActionScript 3 / AVM2 marker

Binary occurrences (up to 8 shown):

- file+0x5ACE4 rva=0x5BEE4 [.text] (ascii)

### `ActionScript` - scripting layer marker

Binary occurrences (up to 8 shown):

- file+0x163965C rva=0x163B65C [.rdata] (ascii)
- file+0x163ACA8 rva=0x163CCA8 [.rdata] (ascii)
- file+0x14F22E8 rva=0x14F42E8 [.rdata] (utf16le)
- file+0x14F2330 rva=0x14F4330 [.rdata] (utf16le)
- file+0x14F23A0 rva=0x14F43A0 [.rdata] (utf16le)
- file+0x14F23E0 rva=0x14F43E0 [.rdata] (utf16le)

Sources/notes:

- `HISTORY.md:2192`: have given native-looking rows for free, is dead. The rows exist only in ActionScript.
- `HISTORY.md:2193`: - The only runtime way in is `Invoke`, which needs Raven's ActionScript API, which means reading
- `HISTORY.md:2198`: decompressor for chunked packages, then SWF extraction, then ActionScript decompilation — three

### `swf` - .swf asset reference

Binary occurrences (up to 8 shown):

- file+0x1630030 rva=0x1632030 [.rdata] (ascii)
- file+0x1634228 rva=0x1636228 [.rdata] (ascii)
- file+0x1634274 rva=0x1636274 [.rdata] (ascii)
- file+0x1634294 rva=0x1636294 [.rdata] (ascii)
- file+0x163CBBC rva=0x163EBBC [.rdata] (ascii)

### `SWF` - .SWF asset reference

Binary occurrences (up to 8 shown):

- file+0xA5199 rva=0xA6399 [.text] (ascii)
- file+0x885AF6 rva=0x886CF6 [.text] (ascii)
- file+0x162FFF0 rva=0x1631FF0 [.rdata] (ascii)
- file+0x1630136 rva=0x1632136 [.rdata] (ascii)
- file+0x1630158 rva=0x1632158 [.rdata] (ascii)
- file+0x16301AC rva=0x16321AC [.rdata] (ascii)
- file+0x16301D8 rva=0x16321D8 [.rdata] (ascii)
- file+0x1630278 rva=0x1632278 [.rdata] (ascii)
- ... and 3 more

Sources/notes:

- `HISTORY.md:2194`: the menu SWF. `UI_*_SF.xxx` are **chunk-compressed** and `uedecompress` handles fully-compressed
- `HISTORY.md:2197`: ⚠️ **Not established: that the menu SWF lacks an addable row.** Only that reaching it needs a
- `HISTORY.md:2198`: decompressor for chunked packages, then SWF extraction, then ActionScript decompilation — three

### `TextField` - GFx text field object

Binary occurrences (up to 8 shown):

- file+0x1635568 rva=0x1637568 [.rdata] (ascii)
- file+0x1635610 rva=0x1637610 [.rdata] (ascii)
- file+0x1639838 rva=0x163B838 [.rdata] (ascii)
- file+0x163E5BC rva=0x16405BC [.rdata] (ascii)

### `GFxUI` - UE3 GFx UI subsystem

Binary occurrences (up to 8 shown):

- file+0x148E1B4 rva=0x14901B4 [.rdata] (utf16le)
- file+0x14F35DC rva=0x14F55DC [.rdata] (utf16le)
- file+0x14F3618 rva=0x14F5618 [.rdata] (utf16le)
- file+0x14F3644 rva=0x14F5644 [.rdata] (utf16le)
- file+0x14F3C20 rva=0x14F5C20 [.rdata] (utf16le)
- file+0x14F3C44 rva=0x14F5C44 [.rdata] (utf16le)
- file+0x14F3C78 rva=0x14F5C78 [.rdata] (utf16le)
- file+0x15521D4 rva=0x15541D4 [.rdata] (utf16le)
- ... and 3 more

Sources/notes:

- `prompt_placement.h:13`: //     Coalesced_INT.bin, Singularity.exe, RvGame.u/Engine.u/Core.u/GFxUI.u
- `HISTORY.md:2195`: packages only — `GFxUI.xxx` opened, `UI_SinglePlayer_SF.xxx` did not.
- `HISTORY.md:2201`: Reusable from it: `GFxUI.u` extracts cleanly, and `GFxMovie.Invoke`/`SetVariable` are confirmed

### `GFxInteraction` - UE3 GFx input routing

Binary occurrences (up to 8 shown):

- file+0x14F2076 rva=0x14F4076 [.rdata] (utf16le)
- file+0x14F20D8 rva=0x14F40D8 [.rdata] (utf16le)
- file+0x14F2120 rva=0x14F4120 [.rdata] (utf16le)
- file+0x14F2168 rva=0x14F4168 [.rdata] (utf16le)
- file+0x14F21C8 rva=0x14F41C8 [.rdata] (utf16le)
- file+0x14F2228 rva=0x14F4228 [.rdata] (utf16le)
- file+0x14F2288 rva=0x14F4288 [.rdata] (utf16le)
- file+0x14F39C8 rva=0x14F59C8 [.rdata] (utf16le)

### `FSCommand` - Flash FSCommand bridge

Binary occurrences (up to 8 shown):

- file+0x1638C8C rva=0x163AC8C [.rdata] (ascii)

## LIKELY (0)

None.

## SPECULATIVE (5)

### `Advance` - movie time advance - VERY generic word (generic word - could be anything)

Binary occurrences (up to 8 shown):

- file+0x1503620 rva=0x1505620 [.rdata] (ascii)
- file+0x16396B4 rva=0x163B6B4 [.rdata] (ascii)
- file+0x1639718 rva=0x163B718 [.rdata] (ascii)
- file+0x163A49C rva=0x163C49C [.rdata] (ascii)
- file+0x163E3B4 rva=0x16403B4 [.rdata] (ascii)
- file+0x1420C08 rva=0x1422C08 [.rdata] (utf16le)
- file+0x1449590 rva=0x144B590 [.rdata] (utf16le)
- file+0x14F2AB4 rva=0x14F4AB4 [.rdata] (utf16le)
- ... and 15 more

Sources/notes:

- `d3d9.cpp:556`: DWORD g_padPacketAdvances = 0;    // per second: ~0 at rest is the whole point
- `d3d9.cpp:765`: ++g_padPacketAdvances;
- `d3d9.cpp:865`: delta, total, (unsigned long)g_padPacketAdvances,
- `d3d9.cpp:891`: g_padPacketAdvances = 0;

### `Display` - movie display call - VERY generic word (generic word - could be anything)

Binary occurrences (up to 8 shown):

- file+0x16396F8 rva=0x163B6F8 [.rdata] (ascii)
- file+0x1639AE0 rva=0x163BAE0 [.rdata] (ascii)
- file+0x163A0F8 rva=0x163C0F8 [.rdata] (ascii)
- file+0x1641C68 rva=0x1643C68 [.rdata] (ascii)
- file+0x1398648 rva=0x139A648 [.rdata] (utf16le)
- file+0x1441AC0 rva=0x1443AC0 [.rdata] (utf16le)
- file+0x1441ADC rva=0x1443ADC [.rdata] (utf16le)
- file+0x1441B14 rva=0x1443B14 [.rdata] (utf16le)
- ... and 17 more

Sources/notes:

- `d3d9.cpp:80`: static XrTime g_hotPredictedDisplayTime = 0;
- `d3d9.cpp:5258`: static_cast<long long>(g_hotPredictedDisplayTime),
- `d3d9.cpp:5304`: XrTime   g_lastDisplayPeriod = 8333333; // ns; replaced by the real one from xrWaitFrame
- `d3d9.cpp:8279`: //     Present N   xrWaitFrame -> predictedDisplayTime for the frame being submitted now
- `d3d9.cpp:8291`: // Predict the hands for when the geometry will actually be seen: `predictedDisplayTime +

### `HandleEvent` - input event into the movie - generic word (generic word - could be anything)

Binary occurrences (up to 8 shown):

- file+0x15F1A70 rva=0x15F3A70 [.rdata] (utf16le)

### `Invoke` - ActionScript invoke bridge (generic word - could be anything)

Binary occurrences (up to 8 shown):

- file+0x16382D0 rva=0x163A2D0 [.rdata] (ascii)
- file+0x16384D0 rva=0x163A4D0 [.rdata] (ascii)
- file+0x1638504 rva=0x163A504 [.rdata] (ascii)
- file+0x163853C rva=0x163A53C [.rdata] (ascii)
- file+0x1638578 rva=0x163A578 [.rdata] (ascii)
- file+0x1638A84 rva=0x163AA84 [.rdata] (ascii)
- file+0x1638E00 rva=0x163AE00 [.rdata] (ascii)
- file+0x16396E0 rva=0x163B6E0 [.rdata] (ascii)
- ... and 4 more

Sources/notes:

- `d3d9.cpp:18456`: // on. So this may be `UFunction::Invoke(UObject*, FFrame&, RESULT)` rather than
- `HISTORY.md:2188`: - Singularity uses the **old, thin** UE3 GFx bridge: `GFxMovie` with `Invoke` and `SetVariable*`.
- `HISTORY.md:2193`: - The only runtime way in is `Invoke`, which needs Raven's ActionScript API, which means reading
- `HISTORY.md:2201`: Reusable from it: `GFxUI.u` extracts cleanly, and `GFxMovie.Invoke`/`SetVariable` are confirmed
- `HISTORY.md:2845`: `ProcessEvent` from `UFunction::Invoke`.

### `Flash` - Flash asset/player reference - generic word (generic word - could be anything)

Binary occurrences (up to 8 shown):

- file+0x14218C4 rva=0x14238C4 [.rdata] (utf16le)
- file+0x14218F4 rva=0x14238F4 [.rdata] (utf16le)
- file+0x1469DE0 rva=0x146BDE0 [.rdata] (utf16le)
- file+0x146B6A0 rva=0x146D6A0 [.rdata] (utf16le)
- file+0x154DB8C rva=0x154FB8C [.rdata] (utf16le)
- file+0x15516E0 rva=0x15536E0 [.rdata] (utf16le)
- file+0x1558E94 rva=0x155AE94 [.rdata] (utf16le)
- file+0x1560894 rva=0x1562894 [.rdata] (utf16le)
- ... and 9 more

Sources/notes:

- `d3d9.cpp:1538`: //   the SCRIPT path  RvGame.u: RvWeaponFX attaches the muzzle effects to `mMuzzleFlashBone`, and
- `d3d9.cpp:1542`: //                    (`SetFlashLocation` carries the HIT location for tracers - which is exactly why
- `d3d9.cpp:1667`: { "mMuzzleFlash1stPerson", -1 }, { "mLoopingMuzzleFlash1stPerson", -1 },
- `d3d9.cpp:1668`: { "mMuzzleFlashIronsights", -1 }, { "mLoopingMuzzleFlashIronsights", -1 },
- `d3d9.cpp:2444`: g_offMuzzle1st       = PropOffsetInChain(g_weaponFxClass, "mMuzzleFlash1stPerson");

## Possible imports / DLL references

None found as plain strings. The import table was not parsed - absence here proves nothing about linkage.

## Candidate call sites

None claimed. String hits are data references, not code references; converting one into a hook target requires disassembly work that is out of scope for this helper.

# Bounded shadow-map content diagnostic

Remaining symptom: square shadows with HQDecalsON. Input FPS cache physically
passes; do not mix input changes or re-enable rejected receiver correction.
Earlier traces establish an R32F512 sampler read by candidate volume receivers,
but resource identity alone does not prove completed meaningful depth pixels.

Hypothesis split: corrupt/empty source shadow map versus receiver projection
or coverage. Read pixels immediately BEFORE the final driver volume draw using
that sampler, correlate exact group/shader identity afterward. This establishes
content at consumption, not a final-present image of a potentially reused map.
It does not by itself identify the square artifact's contributing pixels.

One explicit shadow-pass request, max4 distinct512x512 R32F render-target
textures at sampler1 on8-vertex/12-triangle draws. Staging1MiB allocated at Arm,
never bound; no shader/constant/texture/render-target/state replacements.
GetRenderTargetData uses same format/size SYSTEMMEM destination; reject MSAA,
wrong pool/usage/format/size. HRESULTs retained rather than fabricating content.
Lock/Unlock/COM references balanced. Statistics include full finite/nonfinite,
zero/one counts,min/max and16x16 center samples. No raw game pixels published.
Readbacks can stall the one requested frame; no continuous GPU readback.
Trace OFF still issues zero device queries. Limit is not an error or overflow.

SDK constraints: https://learn.microsoft.com/en-us/windows/win32/api/d3d9/nf-d3d9-idirect3ddevice9-getrendertargetdata
Synthetic x86/O2 stats tests verify padded row pitch,NaN,min/max/grid and invalid
bounds. Existing trace OFF/COM/shader-size tests retained. Real driver readback
and physical shadow outcome pending; no fix or pixel-validation PASS yet.

Test next: same scene/HQDecalsON, stable head/controller pose, input-cacheON,
ReceiverOFF/ClearGateOFF first. One focused captured frame. Match readback group
to audited receiver shader; if pixels valid inspect map content and actual
producer/consumer transforms, not guessed camera constants. If readback fails,
preserve its HRESULT and choose a supported read-only resource capture route.

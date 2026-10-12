# Boot/main menu placement candidate

Base: main 8bd72ce (merged PR #13). Accepted pause DLL 1635A8F0...D625219
and dedicated native RTT package remain the rollback baseline.

User images: IIS title/start text sits near lower-left eye edge; main heading
is clipped at the top. User asks boot text closer to centre, main slightly lower.
Existing runtime evidence: class 1 native HUD scale 0.70 and boot provider=0;
class 2 uses the captured GFx provider. Therefore boot changes its native safe
area (0.45 cap), with the same inset for pointer geometry; main translates the
provider/pointer plane 8% of eye height downward, and native backbuffer HUD
fallback uses the identical -0.16 clip-Y offset. Native producer UVs unchanged.

No pause/class-3, gameplay, eye projection, hands, weapons or texture changes.
One boot/main actual XR-source capture per process (stable focus, 30 frames).
Falsification: compare final eye images and physical text placement/click hit.
State: IMPLEMENTED_NOT_VERIFIED until build/deployment and PSVR2 feedback.

Physical feedback on ed52148 / DLL 3DC51FB3...AF36BD7: "almost perfect";
user requests only main VR SETTINGS a few percent higher. Main row canvas
band 0.70-0.80 -> 0.66-0.76 (~3.36% of eye height with 0.84 inset).
Row rendering and native select interception consume this same band, so the
hit target follows its visible box. Pause row remains exactly 0.60-0.70;
boot placement/main plane offset and gameplay are unchanged.
Test: raised .68 is selectable, old lower .79 is not, both band edges pass;
PSVR2 follow-up determines final acceptance. Prior lease restored accepted
DLL and verified hash before this next candidate.

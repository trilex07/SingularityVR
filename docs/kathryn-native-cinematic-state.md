# Kathryn: native cinematic ownership correction

## Evidence and falsifier

The previous CD571D46 test did NOT fix Kathryn. The SetPlayerCinematicMode
event bridge recorded zero invocations. Repeating that hook is not a repair.
Moreover PCOwner was absent from g_wantNames: WantedIdx could never resolve
that identity check. Registration is corrected, but this does not explain why
the event was never called and is not claimed as physical proof.

Owned Engine.u SHA-256
`B6B481EFDC1FFDA164A24FB25F2FDF57783F2860D36EE1925A1DE6770B971A34`:
Engine.PlayerController.ClientSetCinematicMode export3109 begins LetBool,
instance BoolVariable, ref3897 (bCinematicMode). SetCinematicMode export3103
also assigns it. Raven override export11916 begins the same assignment using
import-85. The independently callable client path is not the watched
SetPlayerCinematicMode event. Script-to-script execution need not re-enter the
outer ProcessInternal detour.

Owned runtime .text SHA-256
`070D20A053880F4164A43A348AF3C043A007716663B6EE71A1D829781757B0E1`:
VM opcode2D registration uses data slot01BB808C ->0048BCE0. At0048BD23:
`mov edx,[edi+84]; test [ecx],edx` consumes the BoolProperty mask and instance
word. This proves mask member84 for THIS executable, not Dishonored's layout.
The loaded reader is fingerprinted before use. The on-disk Steam .text is
packed; the offline audit deliberately uses the existing runtime dump, not
its encrypted disk bytes. No native code is called or patched by this fix.

Hypothesis: the authoritative local bCinematicMode changes while the mod's
event-derived state misses the transition, leaving normal stereo/HMD policies
over an engine-owned camera. Read the bit rather than adding another event
guess. Falsifier: a valid binding reports active=0 throughout the visibly
engine-owned Kathryn scene. In that case this is NOT a proven Kathryn fix;
inspect the actual view-target producer next. No arbitrary Matinee/name/timer
fallback is permitted.

## Implemented correction (default OFF)

`[Render] NativeCinematicState=1` polls native state at Present before scene
policy and hand ownership refresh. Requires exact Camera.PCOwner local
RvPlayerController, reciprocal Pawn.Controller and the current live player
pawn. Typed BoolProperty must belong to Engine.PlayerController. Reflected
offset, array dimension, element size and a single-bit native mask are checked.
The existing Actor controls must first prove the UField layout. Named class
walk is bounded4096, superclass walk64; failed discovery retries at most1Hz.
No global UObject scan, GPU readback, new hook or engine-memory write.

Only the mod's existing cinematic signal is updated. Existing stereo/flat
composition and exit recovery stay in control. Missing identity/property or
unsupported reader retains the existing classifier; refusal is not interpreted
as native OFF. Transition logs cap at64; field-miss logs cap at4.
The prior external reader silently capped class fields at400. This justifies
removing silent truncation, but the exact runtime ordinal is NOT yet proven.

Pair with `NativeRestraintHands=1` from b6ba0db for native rope-mesh ownership
through escape. Disable `CinematicEventBridge` in the private test candidate;
do not stack an unobserved alternative classifier. Other accepted settings,
gore, weapon/scope, audio and localization are unchanged.

## Verification and next physical script

CPU: reader fingerprint refusal, 32 packed-bit isolation cases, field bounds.
Python: exact local/type/default-OFF/ordering guards plus native-action guards.
Owned-package audit proves the producer and mask reader, not the current value
in Kathryn's scene. Build: x86 PE32 /O2, no Performance define. Existing compiler
warnings remain; do not describe the build as warning-free.

No Steam deployment, launch or PSVR2 PASS in this change. Fresh user `start`
is required. One continuous headset script: load the Kathryn rescue save,
observe cinema and her character without switching to chat; watch tied hands
during escape and their release only when freed; then check normal stereo,
hands, pause/options and exit normally. MAIN checks native-cinematic binding,
ON/OFF and native-restraint transitions against the user's observations.
Dubbing remains SIDE-owned and is not generated or deployed here.

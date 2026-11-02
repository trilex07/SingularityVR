# Scripted mutant shot assist — 2026-11-02

## Evidence and mechanism

User explicitly requests assisted hits in the scripted tackle: animated pistol
cannot be aimed, only fired. Existing native-camera bypass is insufficient.
Four actual `cinema native shot` receipts (frames 19076/19131/19185/19314)
have local player source, positive fireDepth and approximately 8000 UU range.
The rays point almost vertically upward, away from the attacking mutant.
Example start `(18465.8,-24344.4,423.137)`, end
`(18084.7,-25085.1,8379.65)`. The upstream reason for that native ray remains
unproven; this is the requested tightly scoped interaction assistance, not a
global aim correction or a claimed full upstream engine fix.

Read-only process observation, PID 25756, 2026-11-02:

- Local player `72F95000`, class RvPlayerPawnSP.
- Reflected player mCombatNode: outer `mutant_tackle_interaction_fs`,
  mNodeName `mutant_player_interaction`.
- Reflected player mCurrentNodeInstigator points to `6821B000`.
- That live object is RvAI_PawnMutant, Health 350, mTarget points back to player.
- Its reflected mCombatNode has outer `mutant_fs`, mNodeName `Tackle_Loop`.
- Actor location `(18464.9727,-24315.6426,475.1683)` lies near the player and
  far off the original outgoing ray. No ZekGrabHandler instance owns this scene.

Addresses are observation receipts only, never compiled target addresses.
Private raw reflection captures and read-only scripts: build/mutant-aim/.

## Implementation

`[Render] CinemaMutantAimAssist=1` opts in; normal default is 0.
Existing SingleLineCheck hook, native cinematic camera branch only:

1. Require player's native fire window and source equal to current local pawn.
2. Require exact player combat node, live native node instigator, exact mutant
   class, living Health, mTarget back to player, and exact Tackle_Loop node.
3. Resolve gameplay properties through existing UE3 reflection. No GObjects
   scan, nearest-enemy selection, persistent attacker cache, invented offsets,
   new hook, camera rotation, or weapon mesh change.
4. Only redirect the captured 8000 UU shot family (2 UU numerical tolerance).
   Short auxiliary collision checks are untouched. Preserve origin and length;
   aim at native Actor.Location. Use a local end vector and original native SLC
   hit buffer/source/flags/extent/other arguments. Native obstruction, damage
   and ammunition rules remain authoritative. No fabricated hit or ammo edit.
5. At most 12 successful routing receipts. Leaving either combat node or losing
   ownership/liveness makes assistance fail closed immediately.

Falsifier: if actual test does not produce this scoped receipt, identify the
first differing native gate. If it does but misses, inspect native hit result
and collision geometry rather than guessing an angular/positional offset.

## Acceptance gate

Automated test uses the same templated native-instigator selection and ray
function used by runtime: wrong node, absent/wrong instigator, wrong native
target, inactive feature/cinema/fire, AI source, nonfinite/zero/short/out-of-range
vectors reject; captured ray fixture redirects collinearly and preserves range.
Physical hit acceptance PENDING. No offline test proves native collision success.

One test: replay the mutant tackle and press R2 normally without aiming the
controller; the actual native shot should hit the attacking mutant. After the
sequence, ordinary hand aiming must remain unchanged. No wheel/HUD retest is
requested for this isolated change.

No files were deployed over the running game. Enable the opt-in only through
the existing hash/backup/profile deployment after normal game closure.

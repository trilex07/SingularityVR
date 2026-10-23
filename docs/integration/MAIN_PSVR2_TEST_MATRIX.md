# MAIN sequential PSVR2 test matrix

These are **test groups**, not seven mandatory headset sessions. Combine only
groups whose feature gates and rollback targets are independent. MAIN alone
touches Steam. Before each group: record accepted DLL/INI/asset SHA256, back up
only the target files, apply only that group, verify candidate SHA256, archive
fresh logs and physical feedback, grade PASS/PARTIAL/FAIL, and restore only the
failed group with hash verification. Never change the native save/inventory to
make a test pass. Do not have the player type in chat while wearing PSVR2.

| Order / module | Smallest action | Required evidence | PASS / FAIL and rollback |
| --- | --- | --- | --- |
| 1 — M01 gameplay (ready for controlled test) | TMD-owned Valkyrie: Circle eject; Grip before reaching left shoulder, pick up, insert, R2 fire; repeat Grip inside; support Grip away from shoulder. Brief Volk/two-hand/L2/R2 check. | DLL/INI hashes, full `view_matrix.log`, bounded `valkyrie shoulder gate`, FSM carry/insertion and native ammo receipts, user view of magazine and glove. | PASS only if both pickup orders work, one magazine per action, correct palm, native rounds and shot, no false pickup/TMD corruption. FAIL: restore M01 DLL/INI to exact accepted hashes. |
| 2 — M02 UI (not deployable yet) | After default-OFF renderer adapter is reviewed: main and pause, VR Settings laser/R2 and scroll, Polish/English prompts, wheel and four icons, seated/standing. | Atlas hashes and pixel QA; D3D9 both-eye draw/IB/state receipts; one screenshot per eye; hit-test positions. | PASS only if text/icon/laser alignment and native GFx interaction work in both eyes. FAIL: switch off adapter, restore only UI DLL/assets. Current branch only stages art, so status **BLOCKED**. |
| 3 — M03 diegetic HUD (not deployable yet) | After proven dynamic producer/attachment: change ammo in four weapons and health, reload, rotate wrists, scope, TMD and two-hand. | Native object/weapon IDs, loaded/reserve/health values, final wrist/gun matrices, two eye draws and occlusion. | PASS only for changing real values with stable depth and no duplicate plate. Current SIDE GLBs have static preview values: **REWORK**. |
| 4 — M04 movies (private separate package) | Logo → loading → menu, one main/cutscene transition; use conservative 720p loading candidate, not 4K. | Original/candidate/restore BIK hashes, Bink header/frame/audio layout and actual decoder/runtime receipts. | PASS if stereo/timing/audio and menu transition remain correct. FAIL: restore just replaced `.bik` files. No SIDE physical decoder PASS. |
| 5 — M05 audio (private separate package) | After verified UE3 package route: one each Centurion/Valkyrie/Volk/Kasimov shot, Impulse, exploration, Grom, ocean/storm; check Polish speech. | Original/package/candidate hashes, cue identity, channels, loop duration, peaks, listening feedback. | PASS requires correct cues, no altered Polish voices/loop/timing, tolerable level. Ocean/storm heuristic candidate cannot be promoted without source identification. No package route now: **BLOCKED**. |
| 6 — M06 textures/effects (private separate package) | Enable only a small exact-hash DDS subset; inspect enemy diffuse/normal, blood alpha, effects, stereo and FPS/Reset. | Original mip0 ID and format, candidate DDS hash/mips/alpha, texture-cache hit, lifetime/Reset, both-eye content and frametime. | PASS if no black materials, wrong normals, missing alpha or stutter. FAIL: remove only tested mappings and verify cache recovery. Current injector match not proven: **BLOCKED**. |
| 7 — M07 Polish + final regression | After note-producer/font proof: open two notes with `ĄĆĘŁŃÓŚŹŻ ąćęłńóśźż`; inspect wrap, PL/EN menu/prompts/subtitles and play dialogue; WeaponLocker and representative combat. | Note ID/producer/package, font glyph receipt, screenshots, preserved voicepack hashes, prior module grades. | PASS if original working dialogue is unchanged and notes/glyphs/wrap are correct. Staged JSON alone is **not** a test candidate. |

Stopping rules: a crash, discomfort, stereo inversion, missing right weapon or
long TMD sleeve ends the current group. Copy logs **before** another launch.
Never mark a different group PASS merely because the game remained playable.

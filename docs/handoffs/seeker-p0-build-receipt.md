# Seeker P0 build receipt

Source commit: `d127d014ef03ea182658340eaf8d7891d9737eec`.

DLL: `spikes/view_matrix/d3d9.dll` (local/private, not committed).
SHA256: `8D7122D628447C8B50C4F3D584E6E52D4100702C21BD731E76CBBFD2F72F52AC`.
Build: x86 PE32 /O2, 2026-10-25; clean tracked source. Local ignored diagnostic reports do not enter the DLL.

Offline: PASS — three Seeker executables, native/compiled ABI (including CC Instigator and WeaponLocker literal +20C), 18 critical C++ programs, 12 Python tests and preview-path checks, standalone D3D9/Ex index/reset checks, OLED D3D11 CPU/GPU suite. The emitted ownership check is `cmp eax,0xCC` / `lea edi,[esi+0xCC]`; no old +98 load.

Physical: NOT RUN. Kabura/launch gate fix pending MAIN acceptance. Disappearing support glove remains OPEN with a bounded Seeker-specific final draw probe, not a proven pose correction. Save/load UNTESTED. Do not present this as a complete fix or stable release.

See `seeker-p0-pr62-followup.md` for evidence, limitations and one MAIN acceptance procedure. Steam untouched.

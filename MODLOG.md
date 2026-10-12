# Reverse-engineering notes

2026-10-12 pause deep RE: `docs/research/pause-menu-deep-re.md` records container,
movie offsets, package configuration, AVM1 callbacks and reproduction commands.
Native runtime offsets were not added. Proprietary extracted assets are outside
Git at `C:/Users/Trilex/vr-mods/pause-re-lab`.

2026-10-12 native pause RTT prototype: added an original, hash-pinned isolated
package builder with dedicated RTT_VRPause and repack-only control. Independent
C++ LZO/table validation and six regression tests pass; original GFx bytecode
preserved. Targeted class/CDO/enum inspection rules out serialized inherited RTT
overrides but does not prove runtime assignment, ownership, completion or input.
Handoff: `docs/research/pause-native-rtt-prototype.md`. Runtime UNVERIFIED.
No game launch, Steam deployment, production renderer/settings or DLL changes.

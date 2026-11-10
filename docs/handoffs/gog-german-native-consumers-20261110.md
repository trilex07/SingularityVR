# German native consumers — 2026-11-10

Continuation of90662118 on compatibility/german-native-consumers-20261110.
Offline candidate only: no game/setup execution, Steam changes, EXE patching,
installer publication or physical acceptance. Left-handed work remains in its
separate PR86; this branch stacks on the German prerequisites in PR85.

## Resolved identities

`tools/audit-singularity-native-consumers.py` independently resolves14 code
consumers and8 globals. Twelve code blocks are unique normalized matches;
LoadUpgrades and Upgrade each have18 lookalikes, resolved by their exact UTF16
native registrations, agreeing with the normalized candidate set. The earlier
generic mapper's REVIEW_REQUIRED result is preserved as historical evidence;
the new audit reports STATIC_CORRESPONDENCE_PASS, which means static mapping.

Texture identity EN017B441C -> DE017B35B4 is now established through:

| Witness | EN | DE |
|---|---|---|
| Allocation caller1,498 bytes /421 fixed | 005140E0 | 00514110 |
| Constructor call1 | 0051426E | 0051429E |
| Allocation caller2,302 bytes /262 fixed | 00514910 | 00514940 |
| Constructor call2 | 005149DA | 00514A0A |
| Both direct calls target constructor | 00520030 | 00520060 |
| Constructor final vtable operand (+69) | 00520099 | 005200C9 |
| Vtable deleting entry | 005206A0 | 005206D0 |
| Deleting entry's direct destructor target | 005200C0 | 005200F0 |

The constructor and destructor must store the SAME class identity. This also
rejects adjacent entries017B35B8/BC/C0. A third EN caller00513D70 did NOT match
uniquely and contributes no positive evidence. No blanket address delta used.

Owned input pins remain:
EN8D438B6C0BD51AB8F24A6A2B6AC569DED6DEF84B20DC407357C4F78E97C0BE1E;
DE49D263B2B53A9BDB770869C51AC8D13537C61BCBDAA7BA3612335CD11DAEC740.
Private report: build/shotgun-disc-gog-20261109/german-native-consumers.json.
No proprietary binaries, extracted assets or disassembly are committed.

## Consumer changes

`game_build_native.h` supplies separate measured addresses, original exact
factory/LoadUpgrades prefixes, hashes, return addresses and RHI class witnesses.
Locker tracing, availability, retained-inventory policy, loadout factory/runtime,
HasWeapon, gun-scale and RTT COM checks consume selected values. The legacy
session diagnostic now uses the already measured selected GEngine input address.
Vtable slots, thread/owner/reflection checks, COM equality and support behavior
are preserved. Upgrade remains witness-only; no purchase hook was added.

References bind to stable mutable profile storage, initialized to EN and copied
after accepted core selection. They never bind once to an EN-only temporary
profile. Fixture EN->DE->EN selection verifies references follow the copy.
Factory hash retains its historical1469598103934665603 seed; code contracts use
standard FNV64 seed14695981039346656037. Neither gate is relaxed.

## Verification

- Real owned EN and DE PEs: core/input plus19 native code contracts PASS.
  Each code block's first/last byte, independent operand references, vtable slots,
  caller rel32s and constructor/destructor stores are mutated individually and
  rejected. Wrong profiles, wrong factory prefix/hash, scale fingerprints,
  derived RVA, HasWeapon/return address and unreadable class identity rejected.
- Protected retail fixture refused; this is not a verdict on an official
  unpacked/patched retail release.
- Actual production gun-scale adapter + real x86 MinHook PASS at both EN/DE
  getter/consumer addresses:10000 ret8/x87 calls/profile, stable ESP, native
  result retained for wrong owner/caller/thread/bone/mesh and OFF contexts.
  Its mocked engine discovery is not runtime fingerprint proof; real-PE tests
  independently check the original code fingerprints.
-25 CPU hand/TMD/reload/aim/locker fixtures and9+3 Python wiring cases PASS,
  plus locker preview guards.10 locker hardening tests and7 texture-resolution
  falsification tests PASS. Separate EN/DE retention policy fixtures included
  in the portable layout runner.
- Production x86 PE32 /O2 build receipt recorded below after committed-source
  rebuild. L3 game and physical tests NOT RUN.

Reproduce with owned paths (images are read as data, never executed):

    python tools/audit-singularity-native-consumers.py <EN> <DE> --output <private-json>
    powershell -File tools/tests/test-game-build-layout.ps1 -EnglishExe <EN> -GermanExe <DE> -ProtectedRetailExe <protected-retail>
    powershell -File tools/tests/test-dual-tmd-gun-scale.ps1
    python tools/tests/test-singularity-native-consumers.py

## Remaining gate

`RuntimeAllowed` deliberately continues to refuse DE. This completes the
remaining fixed-address block identified by the previous handoff, not every
native or asset contract in the renderer. Next audit:

- Reflected native methods: context cursor Get/SetVariableBool, gameplay/NPC
  variable methods, TMD owner/aim/aging, Seeker SwitchWeapon/SelectOwned and
  body-equipment input handlers. Their existing exact hashes must be measured
  against DE and each ABI/slot checked; never accept an address shift alone.
- Input classification cache still has its EN RVA/fingerprint and fails closed;
  port its independently identified DE body before claiming equivalent coverage.
- Validate localized movie/script witnesses and source ownership on the DE data.
- After those pass, integrate a combined pre-detour cached core/input/native
  verdict before enabling a German runtime candidate. Keep object/resource gates.

No request for a headset test yet: offline work still remains.

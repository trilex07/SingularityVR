# German remaining native consumers — 2026-11-10

Follow-up: the texture ambiguity below is resolved and these consumers are now
ported behind the unchanged DE refusal. See
gog-german-native-consumers-20261110.md for the superseding evidence and tests.

Offline continuation after c6fe8548. No game/setup execution, installed changes,
DRM edits, or public compatibility enablement. Input contracts remain separate
from full DE initialization, which is still refused.

`tools/map-singularity-input.py --scope remaining` examines14 further blocks
and8 globals. Twelve code blocks have unique normalized matches:

| Target | EN | DE |
|---|---|---|
| locker start | 00F11430 | 00F110C0 |
| locker PopMenu | 00F17F60 | 00F17BF0 |
| locker load | 00F23840 | 00F234D0 |
| locker equip | 00F00F50 | 00F00BE0 |
| destroy item | 00A05790 | 00A057C0 |
| destroy callsite | 00F18059 | 00F17CE9 |
| append FName | 004AFFD0 | 004AFFD0 |
| inventory factory | 00FD2320 | 00FD1FB0 |
| game-system getter | 00EFA070 | 00EF9D00 |
| HasWeapon | 0102F700 | 0102F390 |
| scale getter block | 00B3E400 | 00B3E430 |
| scale consumer | 00AEC25D | 00AEC28D |

LoadUpgrades/Upgrade each have18 normalized128-byte candidates, so proximity
or assumed-370 deltas are insufficient. Existing read-only `resolve_wrapper`
in tools/reload/inspect_inventory_registration.py resolves their exact UTF16
native registration names `intURvUIGFxMovie_WeaponLockerexecLoadUpgrades` and
`intURvUIGFxMovie_WeaponLockerexecUpgrade`:

- EN LoadUpgrades00F87C80, DE00F87910; Upgrade EN00F87E60, DE00F87AF0.
- Static-pointer registration rows agree with those candidates. Their16-byte
  fingerprints differ through the exception-handler operand, so old exact
  production gates must be deliberately ported, not merely address-shifted.
- Factory prefix handler changes EN016EE978 -> DE016EE578; preserve its
  existing ABI/factory identity safeguards when implementing a DE consumer.

Seven further globals have agreeing independently matched references:

| Global | EN | DE |
|---|---|---|
| locker vtable | 019A0948 | 0199FD20 |
| inventory vtable | 019CB480 | 019CA858 |
| game-system vtable | 019CDBF8 | 019CCFD0 |
| game-system pointer | 01CF394C | 01CF294C |
| inventory/global world pointer | 01CF74E4 | 01CF64E4 |
| empty FString | 0179E4D0 | 0179D528 |
| surface RHI vtable | 017B4474 | 017B360C |

Texture RHI is still ambiguous: the original017B441C constructor/destructor
patterns match four candidate entries017B35B4/B8/BC/C0. Constructor body shape
alone does not identify the texture specialization. Existing exact COM/resource
ownership witnesses must remain intact; do not substitute a plausible entry.
Some original constructor absolute operands are absent from PE HIGHLOW entries,
so relocation-only normalization misses these cases. Instruction-derived masks
find the four candidates but still do not distinguish their semantic identity.

This stage is measured evidence, not completed runtime integration. Preserve
the DE refusal until remaining exact hashes, call/return and vtable consumers,
and the texture specialization are independently resolved and tested. Mapper
returns REVIEW_REQUIRED/2 for this deliberately unresolved scope; that result
must not be described as a complete compatibility PASS. Private report:
build/shotgun-disc-gog-20261109/german-remaining-map.json.

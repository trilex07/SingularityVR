# Hide knife and left belt cue after TMD acquisition

Branch `fix/sol-disable-knife-after-tmd`, based on `a4a5214` (Valkyrie shoulder
reach, optional two-hand accuracy, bounded TMD/knife diagnostic). SIDE did not
touch Steam or merge.

Physical feedback on the prior `4B34CE06…` candidate: picking up the knife
still breaks the TMD presentation. Archived log `59202323…` shows an exclusive
native cuff joint in both eyes, so the old duplicate-draw explanation is not
sufficient. The precise deforming palette is not established. The user chose
to retire the knife once TMD is acquired.

The new default `[Input] HipKnifeWithTMD=0` uses the reflected native `mTMD`
ownership read. It cancels an already-held knife and its melee pulse, declines
new pickup, skips the knife overlay and native knife/glove joint, and hides the
left-hip belt cue. The right weapon holster and TMD power controls are untouched.
`HipKnifeIgnoreTMD=1` no longer bypasses availability. Setting the new option
to 1 deliberately restores the old experimental post-TMD knife path for later
repair; no headset acceptance is claimed for that path.

Offline checks: `tools/knife/test-knife.ps1` PASS (202 hip-knife checks,
renderer/hand/holster tests, Python cases). x86 PE32 `/O2` build PASS. No
physical or real GPU verdict yet. `tools/tests/test-dual-weapon-tmd.ps1 -SkipOled`
PASS, including TMD support, both reload policies, supported shot spread,
WeaponLocker preview and glove isolation. OLED code was not changed.

MAIN acceptance: with a pre-TMD save, confirm knife pickup and sheath once;
after acquiring TMD or loading an owned-TMD save, the left knife and its belt
cue must be absent and squeezing at the left hip must not alter either hand.
Raise TMD, use L2, fire the right weapon with R2 and hold it with left Grip;
the existing complete sleeve must remain. Keep the installed accepted build
backed up and retain all unrelated INI keys.

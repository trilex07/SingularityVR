# New-game wheel follow-up: B7AD66CA physical failure

User: R3 still opens from the beginning; mutant not inspected. Other points
remain unreported, not PASS.

Actual session archived privately at
`build/cinema-framing/session-B7AD66CA/view_matrix.log`:

- line 3440: local pawn inventory empty, mask=0.
- line 10420: `test weapon grant: requested=0x9 before=0x0 after=0x9`.
- line 10421: owned inventory ready, mask=9.
- line 10436: wheel opens.

`[Testing] GrantKasimovCenturion=1` persisted from the earlier user-authorized
weapon test. `LockerVrLoadout::PollEngine` legitimately picked up the test
weapons before the wheel input; the ownership gate therefore passed. This is
not evidence of a catalog-based ownership bypass. Disable the stale testing
key, do not change the native inventory or guessed gameplay/cutscene gates.
No saves edited. Already persisted granted weapons remain in existing saves;
new-game verification requires a genuinely new game.

Cleanup: watcher failed because it did not forward the explicit profile/plan
to rollback. Guard then correctly refused a default missing profile. The
manual DLL-only rollback also detected texture INI drift. Preserved the
observed texture settings unchanged under an explicit scoped preservation
profile (not physical acceptance); all other pins remained enforced. Exact
CBF16F93 DLL restored, then the single grant key changed to 0 using a verified
deployment plan and backup. No gameplay launched.

Installed DLL: CBF16F9396740F6F9B98118CE5726F6646085AE0A969AAE68928906618222C8D.
Installed INI: 7E08498A31BCD62DBB1F5B44D9E72768C5F63B850C5A43D184E92555BD7DAD01.
INI-only correction backup: deployment/install-backups/20261102-134006.
Report: deployment/install-reports/20261102-134007.json.

Watcher now requires explicit profile/plan before READY and forwards both to
the exact DLL-only KeepIni rollback. Dynamic test checks the actual missing
profile refusal. Accepted-profile fixtures: 38 fail-closed cases PASS.

Prepared B7AD66CA stage refreshed against grant=0 and preserved texture INI;
full preflight PASS, not deployed. No new renderer binary was needed for this
configuration correction. Mutant remains unresolved; no new physical result
for empty-hand HUD or recenter. Next agreed session: new game R3 before pickup,
then actual pickup enables selection; optional previously pending HUD/shot.

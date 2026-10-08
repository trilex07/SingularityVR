# Installation plan and risk gates

No installation has been performed. The safest order for a later integration
agent is:

1. **Runtime alias/overlay** — resolve Polish strings in the VR mod or a
   locale-aware overlay while leaving original packages untouched. This is the
   preferred reversible route if the runtime exposes a string lookup hook.
2. **Separate Polish resources** — add a locale package only after confirming
   the game's locale resolver accepts `POL`/`PLK` and the Scaleform font has
   the required glyphs.
3. **Disposable Coalesced replacement** — test only on a copied installation,
   preserving the original SHA-1 and rollback files. The executable digest
   reference makes this a compatibility risk.
4. **Rebuilt UE3 `.xxx` packages** — postpone until package offsets, name
   tables, compression and language selection have a tested round trip.
5. **Executable patching** — out of scope and prohibited for this workstream.

Every future install should have a timestamped backup, a manifest of hashes,
and a one-command rollback. This branch deliberately produces only a handoff
manifest (`build_localization.py`); package generation is disabled.

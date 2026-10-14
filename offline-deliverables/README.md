# Offline Bink candidates (L1/L2)

Committed `.bik` + proof PNGs decoded **from final BIKi** (not MKV). Visual acceptance is MAIN/user L4 only.

| Folder | File | validate-video |
|---|---|---|
| `boot-vya-v1/` | `SNG_logo_vr.bik` | PASS vs stock `SNG_logo.bik` |
| `loading-vya-v1/` | `loading_screen_vr.bik` | PASS vs stock `loading_screen.bik` |

Regenerate: `tools/prepare-boot-logo-full.ps1` (boot) and `tools/title/prepare_loading_screen_bink.py` (loading).

# Workspace archive: 2026-10-27

The active local checkout was moved from historical HUD code to accepted main source `6991ef1facfe6936bba4d666329d87fecf67f77c`. This maintenance branch changes documentation only; it does not merge experiments into main.

## Published source history

`workspace-archive-20261027.json` lists 61 remote-verified archive branches under `archive/cleanup-20261027/`. They retain original commit ancestry and eligible local source/documentation changes from retired checkouts.

These are historical, unvalidated source snapshots, not new release candidates. Source snapshots exclude new private binaries, extracted game assets, generated audio and detected credentials. A filtered source snapshot is not a claim that all old creative assets or staged-index state are reproducible from Git alone. Private working files and original status records are in local recovery archives.

## Local cleanup

Removed 59 existing linked worktrees, 92 superseded installer build/test directories, 12 historical asset/capture directories, and two legacy copies. Branches were preserved. Standard git worktree removal was used without force after validated recovery and cleanup of retired working files. Only one existing SingularityVR checkout remains; missing historical registration records were not forcibly pruned.

No Steam installation, running game, AppData or external dubbing directory was changed. No physical headset validation was performed or implied.

## Retained current installer inputs

RC4-final, runtime resources RC4b, the native reconstruction preimages, latest 8K source textures, publisher keys and installer source remain local. The following artifact directories are retained:

- `Trilex-SingularityVR-1.0.0-rc.4-final`
- `authored-ui-v2`
- `centurion-recipes-v2`
- `font-recipe-v2`
- `hd-texture-recipes-v1`
- `knife-recipes-v2`
- `local-audio-essentials-v1`
- `native-hand-recipes-v1`
- `portable-settings-v2`
- `rc4-evidence`
- `rc4b-owned-source-layout`
- `runtime-resources-rc4b`
- `v7-code-20261026b`
- `v7-owned-source-native-20261026b`

The 11 input modules remain complete: runtime code, Polish dubbing, audio remaster, UI/HUD, native hands, HD textures, knife, authored UI, Polish font, portable settings and Centurion assets. Verification found 116 referenced dubbing entries and 184 HD module entries. All 361 pinned files in RC4-final/current runtime resources retained their original SHA256; copy-file hashes also passed. Recipe presence and file integrity do not establish new in-game visual/audio acceptance.

## Recovery

Local manifests and helpers are in `cleanup-audit-20261027-final/`; private ZIPs are in `cleanup-recovery-20261027/`, outside Git. Canonical-file references require the retained current artifact directories. Git-blob references require this repository's preserved source history.

To inspect without writing:

```powershell
python cleanup-audit-20261027-final/restore_archived_files.py retired-worktrees-and-builds.zip --prefix <old-directory>/
```

Add `--restore` to restore selected archived files. It verifies SHA256 and refuses to overwrite differing existing files. Recreate the desired checkout from the archived source branch first when source checkout recovery is needed. Original worktree status records in the private manifest distinguish omitted/deleted files and staged changes; the helper restores files, not the original Git index.

Private archive receipts:

- `retired-worktrees-and-builds.zip`: 6853198415 bytes; SHA256 `583e725f18a88275a74491e1d94927ee9a470b6cc288fbccda2e0335a546df2c`.
- `retired-assets-and-captures.zip`: 3022599309 bytes; SHA256 `38047dd27ca76915e21679b74887d9f82c0b1c6fe0a822e8a1063e0bb23c4dbc`.
- `retired-legacy-copies.zip`: 37441583 bytes; SHA256 `333f0148059d6f479eac24c7910937892ecd4e8389c31f701459c88f016be971`.
- `primary-checkout-before-update.zip`: 114473332 bytes; SHA256 `d2586a5bff989e5ec6f07958adc2e559964d68378311a6928b6409d555fde277`.

Measured volume free-space gain at final verification: 11.624 GB including archive costs. Concurrent disk activity can affect this figure. Shared Git storage, current reconstruction inputs and compact recovery archives remain intentionally local.

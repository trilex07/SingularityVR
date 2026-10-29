# Centurion HD offline asset handoff

Branch: `assets/centurion-hd-remaster`

Private package: `C:/Users/Trilex/vr-mods/centurion/final_handoff/`

Status: **OFFLINE_BLENDER_PASS** in Blender 5.2.2 LTS. The final animated GLB, detachable cylinder GLB and Blender scene were exported. Both GLBs reimported. The source-compatible 79-bone hierarchy, original bind matrices, five original mechanical groups, full 53-frame Reload and six source-derived mechanical clips are recorded in private manifests. The stable weapon root, cylinder movement, hub binding and detached independent movement passed Blender checks. Four maps are 4096². Seven renders include identical-lighting original/HD bind views and four reload frames. Full package contents and SHA-256 values are in the private `SHA256_manifest.json`.

Additive HD details consist of six measured-frame fasteners and two cylinder hub caps. PBR maps use an authored fine metal height tile plus existing private texture inputs. The source six-chamber cylinder geometry and all UVs remain intact. No unsupported swing-out, cartridge ejection or trigger movement was invented. The source hammer Fire motion is preserved separately.

Native integration remains MAIN-owned: runtime socket mapping, tracked-hand cylinder transfer, scale/material review and in-game visual acceptance. No Steam, DLL, runtime or PSVR2 test occurred.

Rebuild tools: `tools/reload/build_centurion_hd_mechanical_glb.py`, `tools/assets/prepare_centurion_hd_visual.py`, `tools/assets/finish_centurion_visual.py`, `tools/assets/assemble_centurion_visual.py`, `tools/assets/blender_finish_centurion.py`, `tools/assets/blender_validate_centurion.py`, `tools/assets/package_centurion_handoff.py`.

# VR agent skills source review

Reviewed 2026-10-09 for the SingularityVR UE3/D3D9/x86/OpenXR project. Only instructions and attribution were brought into this branch; no foreign repository, game asset, DLL, launcher, or installer was copied.

## Reference revisions and license posture

| Repository | Reference commit | License | Decision |
|---|---|---|---|
| [bioshock-trilogy-vr](https://github.com/VR-Stereo-Hub/bioshock-trilogy-vr) | `41f004bc6a1e4fa8be0d0e0749f9ac5a0453e75` (`main`) | MIT | Adapted simulator guards, runtime-name assertion, PE32 gate, per-eye capture and frame-budget lessons. Its code targets D3D11/Vengeance/UE2.5 lineage or a separate UE3 adapter, so no ABI or offset is reusable here. |
| [Dishonored-VR](https://github.com/VR-Stereo-Hub/Dishonored-VR) | `b15e4d32a720bd4cfdb5ce4d600f3f8fba8d2cec` (`VR-Main`) | zlib/libpng / DXVK-derived notices | Adapted the xrsim launch discipline, focus/runtime guards, explicit eye-check, per-eye pairing and capture evidence. No source or binary was copied. |
| [universal-modder](https://github.com/rehan-remade/universal-modder) | `8370faa8e114baf33acdb23079aff552a7728c4b` (`main`) | MIT | Existing local recon, automation, reverse-engineering and safe-lab skills were reviewed; only project-compatible routing and sandbox principles are retained. |
| [VR-Mod-Vibecoding-Wizard](https://github.com/bigbossafman/VR-Mod-Vibecoding-Wizard) | `40382b390e239f354ea3cecd1f9151c455883d7c` (`main`) | MIT | Existing `vr-mod-workflow` is the project-specific adaptation of stage gates, controller/IK/animation planning and headset-time batching. |
| [meta-quest/agentic-tools](https://github.com/meta-quest/agentic-tools) | `3a8553d10a5a1bd1bf9beaaa950243fbbed3b9ea` (`main`) | Apache-2.0 | Reviewed for simulator and evidence workflow patterns; Meta/Horizon/Android skills were excluded because they do not match Windows x86 UE3/D3D9. |
| [xr-dev](https://github.com/ssheleg/xr-dev) | `c01672238779e3fcbd300bd3bad906cf1edb846f` (`main`) | MIT | Reviewed lifecycle/performance/tooling conventions; Quest/Store/WebXR skills were excluded as out of scope. |

The local Dishonored checkout used for static inspection was `7c1cb8a323e889220d33e62dbb7ed2b64ec765a5` (`v1.0.3`), while the current upstream reference tip was recorded separately above. The local snapshots for universal-modder, Wizard, Meta tools and xr-dev were also checked for their exact Git revisions before adaptation.

## Simulator compatibility verdict

BioShock's `bvr_xrsim32.dll` and `tools/xrsim-launch.ps1` do exist upstream; the current upstream branch contains them, and its README/architecture docs describe a 32-bit OpenXR simulator with runtime-name and PE32 guards. They do not exist in this SingularityVR worktree and were not copied or executed.

SingularityVR already has a project-specific route: the reference `dvr_xrsim32.dll`, `XR_RUNTIME_JSON` manifest, `tools/steam-xrsim-singularity.ps1`, `.vrrec` dump/replay tools, and per-eye evidence docs. Static compatibility is confirmed at the OpenXR 32-bit ABI/manifest level and the existing L1 replay dry-run passed on a real `.vrrec` (`5573` samples, `8` fire, `2` melee, `1` recenter, `6` UI-click events). Integrated L3 compatibility remains evidence-dependent: it is confirmed only when the runner records the expected `dvr-xrsim` runtime, focused session, advancing frames, two projection views, fresh left/right captures, and the matching verified artifact SHA. No L3 launch was performed in this skills-only branch.

A separate simulator implementation is therefore **not** the next task. If a future clean-room runtime becomes necessary, track it as a separate implementation task with its own L1/L2/L3 gates; do not substitute `bvr_xrsim32.dll` for Singularity's D3D9 path.

## Adapted feature map

- UE3/D3D9 reverse engineering: evidence-first hook and pass classification, no borrowed offsets.
- OpenXR/32-bit: PE32 `0x014c`, per-process manifest, runtime-name assertion, lifecycle ordering and backend separation.
- Stereo/shaders/shadows/VFX: per-eye transaction boundaries, main-vs-light-space classification, state restoration and bounded diagnostics.
- Hands/IK/animation: coordinate/unit proof, existing skeleton preference, animation takeover/return transitions and controller calibration.
- Automated testing: L1–L4 routing, test bridge, synthetic/replay tests before Steam or PSVR2.
- Session evidence: immutable `.vrrec`, dry-run/replay, per-eye capture freshness and pair/motion checks, pacing fields.
- Parallel agents: isolated worktrees, xrsim data roots, logs and process ownership; primary agent only for Steam deployment.

## Security review result

No third-party installer, package manager command, unknown executable, foreign DLL, or runtime hook was executed. The only helper run was the project-owned, read-only `.vrrec` dump and `--dry-run` replay validation against an existing recording. The `codex` CLI was queried only for help/version; OpenCode CLI is not installed on this host.

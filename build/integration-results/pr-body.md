Integrates the reviewed HUD tip f307ad32 into the final SIDE candidate: shooter-facing horizontal reflection and opt-in final-bound left-wrist measurement. Preserves body-owned HUD projection and prior reload comfort, accuracy policy, detailed icons, single-owner texture bridge and private staging.

Validation: x86 PE32 /O2 build; 120 D3D9 + 120 D3D9Ex pixel cases, six observed pose replays, wrist probe/math and seven parser tests, critical gameplay/reload/TMD/OLED regressions, compiled WeaponLocker ABI PASS. Exact hashes and evidence: docs/handoffs/ASTRA_FINAL_CANDIDATE.json.

No Steam deployment or gameplay. Physical acceptance PENDING. HandAnchorProbe defaults OFF and does not change wrist position. Texture replacement and experimental accuracy tuning remain disabled. Separate audio/font/texture gates remain documented in the handoff. Draft for MAIN review; do not merge before review.

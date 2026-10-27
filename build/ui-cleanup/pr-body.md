Native Keyboard/ButtonN localization still emits Pad labels despite the correct M02 atlases being installed. Add VR-only action-scoped cleanup using actual XR bound sources, exact legacy alias suppression, and independent rollback switches. Connect the existing neutral atlas as a main/pause frame behind native GFx; preserve native controls, text, hits and HUD3D. This does not replace native button artwork.

Read-only audit confirms all three installed atlas hashes match source, including neutral 3EF17826. Existing settings/wheel remain reused; bounded binding receipts distinguish installation from actual draw execution.

Validation: 12 UI programs, PL/EN atlas QA, detailed icons, D3D9/D3D9Ex pixel/state failure tests, HUD3D, gameplay/TMD/reload/OLED and compiled WeaponLocker ABI PASS. x86 PE32 /O2 DLL SHA256: 9800178AF2930802182C1B3C53B995F62A5BACC6BFC1A0186212E4DA839852DB. Runtime source 0989f269; manifest and handoff in docs/handoffs/UI_REMASTER_PROMPT_CLEANUP.*.

No deployment or gameplay launch. Physical acceptance PENDING. MAIN should validate active control mappings, menu readability and unknown prompt producers before acceptance. Unknown producers remain untouched; unresolved bindings display VR rather than guessed controls.

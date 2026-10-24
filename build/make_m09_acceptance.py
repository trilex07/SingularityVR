import json, os, hashlib, re, io, os

WS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GAME = r"C:\Program Files (x86)\Steam\steamapps\common\Singularity"
PROFILE = WS + r"\deployment\acceptance-20261024-m09\accepted-profile.json"
PLAN = WS + r"\deployment\acceptance-20261024-m09\change-plan.json"

ini_path = GAME + r"\Binaries\SingularityVR.ini"
text = io.open(ini_path, encoding="utf-8-sig").read()
lines = re.split(r"\r\n|\n", text)
section, base_map = "", {}
for line in lines:
    s = line.strip()
    if not s or s.startswith(";") or s.startswith("#"):
        continue
    if re.match(r"^\[[^\]/]+\]$", s):
        section = s[1:-1].strip(); continue
    m = re.match(r"^([^=/]+?)\s*=\s*(.*?)\s*$", s)
    if section and m:
        key = section + "/" + m.group(1).strip()
        assert key not in base_map, "dup " + key
        base_map[key] = m.group(2)
assert len(base_map) == 354, len(base_map)
# promotion sanity: M01 keys are accepted state now
for k, v in [("Input/ValkyrieShoulderExtendLeftCm", "18"), ("Input/VolkShoulderRadiusCm", "18"),
             ("Input/ValkyrieManualReload", "1"), ("Input/ReloadPalmGrip", "1"),
             ("Input/ReloadNativeMagazine", "1"), ("Input/HipKnifeWithTMD", "0"),
             ("Diagnostic/ShoulderPickupProbe", "1")]:
    assert base_map[k] == v, (k, base_map.get(k))


def sha(p):
    return hashlib.sha256(open(p, "rb").read()).hexdigest().upper()


deps = {
    "Localization/PolishLanguage": [
        "Binaries/vr-assets/localization/pl/polish_text.jsonl",
        "Binaries/vr-assets/localization/pl/polish_voice.jsonl",
        "Binaries/vr-assets/localization/pl/polish_story_voice.jsonl"],
    "Render/GloveNativeEnabled": ["Binaries/vr-assets/hud/katorga/glove-animation/native_hands.svni"],
    "Render/HandThumbPoseFix": ["Binaries/vr-assets/hud/katorga/glove-animation/hand_thumb.svnt"],
    "Render/NativeGloveCuffClosure": ["Binaries/vr-assets/hud/katorga/glove-animation/native_cuffs.svnc"],
    "Input/HipKnifeEnabled": ["Binaries/vr-assets/knife/knife.svrk", "Binaries/vr-assets/knife/knife.svrt"],
    "Render/KnifeHandGripEnabled": ["Binaries/vr-assets/knife/knife_hand.svng"],
    "Render/KnifeSceneLighting": ["Binaries/vr-assets/knife/knife.svnl",
                                  "Binaries/vr-assets/knife/knife_normal.svrt",
                                  "Binaries/vr-assets/knife/knife_specular.svrt"],
}
files = [{"path": "Binaries/d3d9.dll", "sha256": sha(GAME + r"\Binaries\d3d9.dll")}]
for k in deps:
    if base_map.get(k) == "1":
        for rel in deps[k]:
            files.append({"path": rel, "sha256": sha(GAME + "\\" + rel.replace("/", "\\"))})

profile = {
    "schema": "singularityvr-accepted-profile-v1",
    "approved": True,
    "acceptedBy": "MAIN SingularityVR (OpenCode main agent), promoting the physically accepted M01/M03 state installed 2026-10-23",
    "acceptanceScope": (
        "Promoted accepted state: M01 Valkyrie/Volk TMD shoulder pickup PHYSICALLY_VERIFIED and M03 live HP/ammo "
        "PHYSICAL PASS (observed) on build bb234ba, installed DLL 78404247B78586FB42416F0A331CD11B13B4DBC0A16708B913A21F04EA374541 "
        "and INI D2E91B0B2EC0CE2D9DC1C9933D1076AF315937E24F00E5FA88A3956C75A403D8 (includes the seven scoped M01 keys and "
        "ShoulderPickupProbe=1 active in the accepted session). Pins cover d3d9.dll and the guard-required enabled-feature assets; "
        "all other game files remain outside this controlled pair install. Pre-test backup: deployment\\install-backups\\20261023-235243."
    ),
    "iniPath": "Binaries/SingularityVR.ini",
    "ini": {k: str(v) for k, v in sorted(base_map.items())},
    "ignoredIniKeys": ["Render/AutoResX", "Render/AutoResY"],
    "files": files,
}
os.makedirs(os.path.dirname(PROFILE), exist_ok=True)
io.open(PROFILE, "w", encoding="utf-8", newline="\n").write(json.dumps(profile, indent=2, ensure_ascii=False) + "\n")
profile_sha = sha(PROFILE)

plan = {
    "schema": "singularityvr-change-plan-v1",
    "profileSha256": profile_sha,
    "reason": (
        "M09 PSVR2 TMD advanced haptics candidate: current accepted baseline (integration/sol-side-master-package-20261023 "
        "+ cherry-picked 5a4ac26 haptics fix) rebuilt as integration/m09-haptics-on-current-baseline @ 56ae307. "
        "TmdPulses stays default OFF; native rumble restored through the OpenXR tracking gate. DLL-only, INI unchanged."
    ),
    "files": [
        {"path": "Binaries/d3d9.dll",
         "before": sha(GAME + r"\Binaries\d3d9.dll"),
         "after": sha(WS + r"\build\m09-candidate\d3d9.dll"),
         "reason": " x86 PE32 /O2 rebuild at 56ae307 with 5a4ac26 haptics fix on the accepted M01/M03 baseline"},
    ],
    "ini": [],
}
io.open(PLAN, "w", encoding="utf-8", newline="\n").write(json.dumps(plan, indent=2, ensure_ascii=False) + "\n")
print("pinned files:", len(files))
print("profile sha256:", profile_sha)
print("plan dll after:", plan["files"][0]["after"])

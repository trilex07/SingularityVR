import json, hashlib, re, io, sys, os

WS = r"C:\Users\Trilex\vr-mods\SingularityVR-sol-master-integration"
GAME = r"C:\Program Files (x86)\Steam\steamapps\common\Singularity"
SNAPSHOT = WS + r"\build\installed-ini-snapshot.ini"
CAND_INI = WS + r"\build\m01-shoulder-candidate\SingularityVR.ini"
PROFILE = WS + r"\deployment\acceptance-20261023-m01\accepted-profile.json"
PLAN = WS + r"\deployment\acceptance-20261023-m01\change-plan.json"
CAND_DIR = WS + r"\build\m01-shoulder-candidate"

text = io.open(SNAPSHOT, encoding="utf-8-sig").read()

os.makedirs(WS + r"\build\m01-shoulder-candidate", exist_ok=True)
os.makedirs(WS + r"\deployment\acceptance-20261023-m01", exist_ok=True)

# 1) Candidate INI: installed INI + exactly the seven task-specified keys.
input_keys = (
    "; MAIN 2026-10-23: scoped M01 shoulder-pickup candidate (691d package bb234ba).\n"
    "ValkyrieShoulderExtendLeftCm=18\n"
    "ValkyrieShoulderRadiusCm=18\n"
    "ValkyrieGripReachMs=1800\n"
    "VolkShoulderExtendLeftCm=18\n"
    "VolkShoulderRadiusCm=18\n"
    "VolkGripReachMs=1800\n"
)
m = re.search(r"(?m)^\[Input\][ \t]*$", text)
assert m, "[Input] header not found"
cand = text[: m.end()] + "\r\n" + input_keys.rstrip("\n") + text[m.end():]

m = re.search(r"(?m)^\[Diagnostic\][ \t]*$", cand)
if m:
    cand = cand[: m.end()] + "\r\n; MAIN 2026-10-23: bounded M01 shoulder receipt capture.\r\nShoulderPickupProbe=1" + cand[m.end():]
else:
    cand = cand.rstrip("\r\n") + "\r\n\r\n[Diagnostic]\r\nShoulderPickupProbe=1\r\n"

io.open(CAND_INI, "w", encoding="utf-8", newline="").write(cand)

def parse_ini(t):
    out, section = {}, ""
    for line in re.split(r"\r\n|\n", t):
        s = line.strip()
        if not s or s.startswith(";") or s.startswith("#"):
            continue
        if re.match(r"^\[[^\]/]+\]$", s):
            section = s[1:-1].strip()
            continue
        m2 = re.match(r"^([^=/]+?)\s*=\s*(.*?)\s*$", s)
        if section and m2:
            key = section + "/" + m2.group(1).strip()
            if key in out:
                raise SystemExit("duplicate INI key: " + key)
            out[key] = m2.group(2)
    return out


cand_map = parse_ini(cand)
base_map = parse_ini(text)
assert len(cand_map) == len(base_map) + 7, (len(base_map), len(cand_map))
for k, v in [("Input/ValkyrieShoulderExtendLeftCm", "18"), ("Input/ValkyrieShoulderRadiusCm", "18"),
             ("Input/ValkyrieGripReachMs", "1800"), ("Input/VolkShoulderExtendLeftCm", "18"),
             ("Input/VolkShoulderRadiusCm", "18"), ("Input/VolkGripReachMs", "1800"),
             ("Diagnostic/ShoulderPickupProbe", "1")]:
    assert cand_map[k] == v, k
assert cand_map["Input/ValkyrieManualReload"] == base_map["Input/ValkyrieManualReload"] == "1"
assert cand_map["Input/ReloadPalmGrip"] == "1" and cand_map["Input/ReloadNativeMagazine"] == "1"
assert cand_map["Input/HipKnifeWithTMD"] == "0"
print("candidate INI: 7 keys added, baseline intact (%d->%d keys)" % (len(base_map), len(cand_map)))


def sha(p):
    return hashlib.sha256(open(p, "rb").read()).hexdigest().upper()


deps = {
    "Localization/PolishLanguage": [
        "Binaries/vr-assets/localization/pl/polish_text.jsonl",
        "Binaries/vr-assets/localization/pl/polish_voice.jsonl",
        "Binaries/vr-assets/localization/pl/polish_story_voice.jsonl",
    ],
    "Render/GloveNativeEnabled": ["Binaries/vr-assets/hud/katorga/glove-animation/native_hands.svni"],
    "Render/HandThumbPoseFix": ["Binaries/vr-assets/hud/katorga/glove-animation/hand_thumb.svnt"],
    "Render/NativeGloveCuffClosure": ["Binaries/vr-assets/hud/katorga/glove-animation/native_cuffs.svnc"],
    "Input/HipKnifeEnabled": ["Binaries/vr-assets/knife/knife.svrk", "Binaries/vr-assets/knife/knife.svrt"],
    "Render/KnifeHandGripEnabled": ["Binaries/vr-assets/knife/knife_hand.svng"],
    "Render/KnifeSceneLighting": [
        "Binaries/vr-assets/knife/knife.svnl",
        "Binaries/vr-assets/knife/knife_normal.svrt",
        "Binaries/vr-assets/knife/knife_specular.svrt",
    ],
}
files = [{"path": "Binaries/d3d9.dll", "sha256": sha(GAME + r"\Binaries\d3d9.dll")}]
enabled = [k for k in deps if base_map.get(k) == "1"]
print("enabled dependency keys:", ", ".join(enabled))
pinned = ["Binaries/d3d9.dll"]
for k in enabled:
    for rel in deps[k]:
        pinned.append(rel)
        files.append({"path": rel, "sha256": sha(GAME + "\\" + rel.replace("/", "\\"))})
print("pinned files:", len(files))

profile = {
    "schema": "singularityvr-accepted-profile-v1",
    "approved": True,
    "acceptedBy": "MAIN SingularityVR (OpenCode main agent) committed on the verified 702f0c3 baseline rollback",
    "acceptanceScope": (
        "Accepted Steam baseline commit 702f0c3: DLL A61C3B534F5C4C40DC878EC68CC52A77E8599DB57C6272D85254E58B795F6E1C, "
        "INI 4EA3857A0E78033C162280AB1A8FCB8A5D6CA09B72547CE75C8D55688BC904EA; accepted Polish voices/text, knife routes, "
        "native hands/cuffs/thumbs, TMD/Volk carry, HipKnifeWithTMD=0. Hash pins cover d3d9.dll and only the guard-required "
        "enabled-feature assets; all other game files are outside this controlled pair install."
    ),
    "iniPath": "Binaries/SingularityVR.ini",
    "ini": {k: str(v) for k, v in sorted(base_map.items())},
    "ignoredIniKeys": ["Render/AutoResX", "Render/AutoResY"],
    "files": files,
}

import os
os.makedirs(os.path.dirname(PROFILE), exist_ok=True)
os.makedirs(CAND_DIR, exist_ok=True)
io.open(PROFILE, "w", encoding="utf-8", newline="\n").write(
    json.dumps(profile, indent=2, ensure_ascii=False) + "\n")
profile_sha = sha(PROFILE)

plan = {
    "schema": "singularityvr-change-plan-v1",
    "profileSha256": profile_sha,
    "reason": (
        "M01+SOL 5105359 fixed carry: admit Valkyrie TMD wrist producer; add bounded left-shoulder capsule + Grip reach "
        "window for Valkyrie/Volk. Controlled DLL replacement with seven scoped INI keys for MAIN PSVR2 acceptance; no "
        "UI, 3D HUD, haptics, audio, Bink, DDS or Polish note changes."
    ),
    "files": [
        {
            "path": "Binaries/d3d9.dll",
            "before": sha(GAME + r"\Binaries\d3d9.dll"),
            "after": sha(WS + r"\spikes\view_matrix\d3d9.dll"),
            "reason": " rebuilt from integration/sol-side-master-package-20261023 at bb234ba (cherry-pick of 5105359), x86 PE32 /O2",
        }
    ],
    "ini": [
        {"key": "Input/ValkyrieShoulderExtendLeftCm", "before": None, "after": "18",
         "reason": " new bounded left extension for the Valkyrie pickup capsule (SOL default)"},
        {"key": "Input/ValkyrieShoulderRadiusCm", "before": None, "after": "18",
         "reason": " new capsule radius for the Valkyrie shoulder pickup"},
        {"key": "Input/ValkyrieGripReachMs", "before": None, "after": "1800",
         "reason": " bounded reach window after a fresh left-Grip edge (Valkyrie)"},
        {"key": "Input/VolkShoulderExtendLeftCm", "before": None, "after": "18",
         "reason": " user-requested farther-left Volk pickup region, same bounded mechanism"},
        {"key": "Input/VolkShoulderRadiusCm", "before": None, "after": "18",
         "reason": " capsule radius for the Volk shoulder pickup"},
        {"key": "Input/VolkGripReachMs", "before": None, "after": "1800",
         "reason": " bounded reach window after a fresh left-Grip edge (Volk)"},
        {"key": "Diagnostic/ShoulderPickupProbe", "before": None, "after": "1",
         "reason": " bounded local-frame grip/shoulder receipts for the acceptance log"},
    ],
}

io.open(PLAN, "w", encoding="utf-8", newline="\n").write(
    json.dumps(plan, indent=2, ensure_ascii=False) + "\n")
print("profile sha256:", profile_sha)
print("candidate dll sha256:", sha(WS + r"\spikes\view_matrix\d3d9.dll"))
print("plan written; change-plan.js profileSha256 bound")

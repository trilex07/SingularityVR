import json, hashlib, io, os, shutil

WS = r"C:\Users\Trilex\vr-mods\SingularityVR-m02-remaster"
GAME = r"C:\Program Files (x86)\Steam\steamapps\common\Singularity"
BIN = os.path.join(GAME, "Binaries")
BASE = os.path.join(WS, r"spikes\view_matrix")
PROFILE_DIR = os.path.join(WS, "deployment", "acceptance-20261024-m02-safety")
CAND = os.path.join(WS, "build", "m02-runtime-candidate-safety")


def sha(p):
    return hashlib.sha256(open(p, "rb").read()).hexdigest().upper()


def parse_ini(t):
    out, section = {}, ""
    for line in re.split(r"\r\n|\n", t):
        s = line.strip()
        if not s or s.startswith(";") or s.startswith("#"):
            continue
        if re.match(r"^\[[^/]+\]$", s):
            section = s[1:-1].strip()
            continue
        m = re.match(r"^([^=/]+?)\s*=\s*(.*?)\s*$", s)
        if section and m:
            out[section + "/" + m.group(1).strip()] = m.group(2)
    return out


import re

live_ini_path = os.path.join(BIN, "SingularityVR.ini")
text = io.open(live_ini_path, encoding="utf-8-sig").read()
base_ini = parse_ini(text)
assert len(base_ini) == 357, len(base_ini)
assert base_ini["Render/UiRemasterRuntime"] == "1"
assert base_ini["Render/UiRemasterSettings"] == "0"
assert base_ini["Render/UiRemasterWheel"] == "0"
assert "Haptics/TmdPulses" not in base_ini  # M09 pulses stay OFF
live_dll_sha = sha(os.path.join(BIN, "d3d9.dll"))
assert live_dll_sha.startswith("34A2946C"), live_dll_sha  # current M02 candidate
cand_dll_sha = sha(os.path.join(BASE, "d3d9.dll"))
assert cand_dll_sha.startswith("107FA9C0"), cand_dll_sha  # 7d156f8 safety build

os.makedirs(CAND, exist_ok=True)
shutil.copyfile(os.path.join(BASE, "d3d9.dll"), os.path.join(CAND, "d3d9.dll"))
assert sha(os.path.join(CAND, "d3d9.dll")) == cand_dll_sha

prev = json.load(io.open(os.path.join(WS, r"deployment\acceptance-20261024-m02\accepted-profile-p0-enable.json"), encoding="utf-8"))
dep_pins = [e for e in prev["files"] if e["path"] != "Binaries/d3d9.dll"]

os.makedirs(PROFILE_DIR, exist_ok=True)
profile = {
    "schema": "singularityvr-accepted-profile-v1",
    "approved": True,
    "acceptedBy": "MAIN SingularityVR (OpenCode main agent), promoting the M02 deployment-B state before the mandatory safety rebuild",
    "acceptanceScope": (
        "Safety deployment: accept the deployment-B product state (dll 34A2946C..., 357-key INI with UiRemasterRuntime=1, "
        "UiRemasterSettings=0, UiRemasterWheel=0, no Haptics/TmdPulses) including the three verified atlas assets. "
        "Replace ONLY the dll with the 107FA9C0... build (7d156f8 = b49380d cherry-pick: native settings footer retained, "
        "atomic offscreen wheel staging, failure-injection fallback, state restoration) and keep the INI BYTE-IDENTICAL "
        "(no plan.ini allowances). P0 stays armed for its grading session; P1/P2 stay OFF."
    ),
    "iniPath": "Binaries/SingularityVR.ini",
    "ini": {k: str(v) for k, v in sorted(base_ini.items())},
    "ignoredIniKeys": ["Render/AutoResX", "Render/AutoResY"],
    "files": [{"path": "Binaries/d3d9.dll", "sha256": live_dll_sha}] + dep_pins,
}
profile_path = os.path.join(PROFILE_DIR, "accepted-profile-safety.json")
io.open(profile_path, "w", encoding="utf-8", newline="\n").write(json.dumps(profile, indent=2, ensure_ascii=False) + "\n")
plan = {
    "schema": "singularityvr-change-plan-v1",
    "profileSha256": sha(profile_path),
    "reason": (
        "M02 safety deployment: mandatory b49380d cherry-pick rebuild (dll 34A2946C->107FA9C0 @ 7d156f8). "
        "INI untouched and byte-identical; P0 gate stays armed (UiRemasterRuntime=1), P1/P2 stay OFF."
    ),
    "files": [
        {"path": "Binaries/d3d9.dll", "before": live_dll_sha, "after": cand_dll_sha,
         "reason": " safety rebuild: settings-footer retention + atomic wheel staging + fallback"},
    ],
    "ini": [],
}
plan_path = os.path.join(PROFILE_DIR, "change-plan-safety.json")
io.open(plan_path, "w", encoding="utf-8", newline="\n").write(json.dumps(plan, indent=2, ensure_ascii=False) + "\n")

print("profile sha:", sha(profile_path))
print("cand dll:", cand_dll_sha)
print("artifact:", os.path.join(CAND, "d3d9.dll"))

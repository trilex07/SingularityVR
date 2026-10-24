import json, hashlib, io, os, re, shutil

WS = r"C:\Users\Trilex\vr-mods\SingularityVR-m02-remaster"
GAME = r"C:\Program Files (x86)\Steam\steamapps\common\Singularity"
BIN = os.path.join(GAME, "Binaries")
PROFILE_DIR = os.path.join(WS, "deployment", "acceptance-20261024-m02-p1")
CAND = os.path.join(WS, "build", "m02-runtime-candidate-p1")
SAFETY_CAND = os.path.join(WS, "build", "m02-runtime-candidate-safety")


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


live_ini_path = os.path.join(BIN, "SingularityVR.ini")
text = io.open(live_ini_path, encoding="utf-8-sig").read()
base_ini = parse_ini(text)
assert len(base_ini) == 357, len(base_ini)
assert base_ini["Render/UiRemasterRuntime"] == "1"
assert base_ini["Render/UiRemasterSettings"] == "0"
assert base_ini["Render/UiRemasterWheel"] == "0"
assert "Haptics/TmdPulses" not in base_ini
live_dll_sha = sha(os.path.join(BIN, "d3d9.dll"))
assert live_dll_sha.startswith("107FA9C0"), live_dll_sha  # safety build, P0-accepted

# staged INI: only UiRemasterSettings 0->1
m = re.search(r"(?m)^UiRemasterSettings=0\r?$", text)
assert m, "UiRemasterSettings=0 not found"
cand_ini = text[: m.start()] + "UiRemasterSettings=1" + text[m.end():]
cand_map = parse_ini(cand_ini)
assert len(cand_map) == 357
assert cand_map["Render/UiRemasterRuntime"] == "1"
assert cand_map["Render/UiRemasterSettings"] == "1"
assert cand_map["Render/UiRemasterWheel"] == "0"

os.makedirs(CAND, exist_ok=True)
shutil.copyfile(os.path.join(SAFETY_CAND, "d3d9.dll"), os.path.join(CAND, "d3d9.dll"))
assert sha(os.path.join(CAND, "d3d9.dll")) == live_dll_sha  # byte-identical dll
io.open(os.path.join(CAND, "SingularityVR.ini"), "w", encoding="utf-8", newline="").write(cand_ini)

prev = json.load(io.open(os.path.join(WS, r"deployment\acceptance-20261024-m02-safety\accepted-profile-safety.json"), encoding="utf-8"))
dep_pins = [e for e in prev["files"] if e["path"] != "Binaries/d3d9.dll"]

os.makedirs(PROFILE_DIR, exist_ok=True)
profile = {
    "schema": "singularityvr-accepted-profile-v1",
    "approved": True,
    "acceptedBy": "MAIN SingularityVR (OpenCode main agent), promoting the P0-accepted safety build before arming the P1 gate",
    "acceptanceScope": (
        "P1 deployment: accept the P0-approved product state (dll 107FA9C0..., 357-key INI with UiRemasterRuntime=1, "
        "UiRemasterSettings=0, UiRemasterWheel=0, no Haptics/TmdPulses; three verified atlas assets; game config from the "
        "graded P0 session). Then flip ONLY Render/UiRemasterSettings to 1 for the P1 grading session (localized header/"
        "selected-row frame/R2 glyph styling behind the accepted native footer). dll is rewritten byte-identical; "
        "M01/M03 + M09 Session-A gameplay untouched; rollback = set UiRemasterSettings=0."
    ),
    "iniPath": "Binaries/SingularityVR.ini",
    "ini": {k: str(v) for k, v in sorted(base_ini.items())},
    "ignoredIniKeys": ["Render/AutoResX", "Render/AutoResY"],
    "files": [{"path": "Binaries/d3d9.dll", "sha256": live_dll_sha}] + dep_pins,
}
profile_path = os.path.join(PROFILE_DIR, "accepted-profile-p1-enable.json")
io.open(profile_path, "w", encoding="utf-8", newline="\n").write(json.dumps(profile, indent=2, ensure_ascii=False) + "\n")
plan = {
    "schema": "singularityvr-change-plan-v1",
    "profileSha256": sha(profile_path),
    "reason": (
        "P1 grading gate: arm ONLY Render/UiRemasterSettings=1 on the P0-accepted safety build; "
        "UiRemasterWheel stays 0; dll byte-identical (staged rewrite, same SHA allowed); "
        "native footer remains the readability authority."
    ),
    "files": [],
    "ini": [
        {"key": "Render/UiRemasterSettings", "before": "0", "after": "1",
         "reason": " P1 styling gate: header/selected-row frame/R2 glyph + localized tab"},
    ],
}
plan_path = os.path.join(PROFILE_DIR, "change-plan-p1-enable.json")
io.open(plan_path, "w", encoding="utf-8", newline="\n").write(json.dumps(plan, indent=2, ensure_ascii=False) + "\n")

print("profile sha:", sha(profile_path))
print("cand ini bytes:", len(cand_ini), "| profile pins dll:", live_dll_sha)

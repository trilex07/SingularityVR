import json, os, hashlib, io, os, re, shutil

WS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GAME = r"C:\Program Files (x86)\Steam\steamapps\common\Singularity"
BIN = os.path.join(GAME, "Binaries")
PROFILE_DIR = os.path.join(WS, "deployment", "acceptance-20261024-m02m09-combined")
CAND = os.path.join(WS, "build", "m02-m09-combined-candidate")


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
assert base_ini["Render/UiRemasterSettings"] == "1"
assert base_ini["Render/UiRemasterWheel"] == "0"
assert "Haptics/TmdPulses" not in base_ini
live_dll_sha = sha(os.path.join(BIN, "d3d9.dll"))
assert live_dll_sha.startswith("107FA9C0"), live_dll_sha  # P1-accepted safety build

m = re.search(r"(?m)^UiRemasterWheel=0\r?$", text)
assert m, "UiRemasterWheel=0 not found"
cand = text[: m.start()] + "UiRemasterWheel=1" + text[m.end():]
cand_map = parse_ini(cand)
assert cand_map["Render/UiRemasterWheel"] == "1" and len(cand_map) == 357
addition = (
    "\r\n[Haptics]\r\n"
    "; MAIN 2026-10-24: M09 f73b91d combined session - bounded native TMD power\r\n"
    "; pulses (Generic fallback 0.40/70ms), classification receipts on; P1-accepted\r\n"
    "; M02 state preserved.\r\n"
    "TmdPulses=1\r\n"
)
cand_ini = cand.rstrip("\r\n") + addition
final_map = parse_ini(cand_ini)
assert len(final_map) == 358, len(final_map)
assert final_map["Haptics/TmdPulses"] == "1"
for k in ("Render/UiRemasterRuntime", "Render/UiRemasterSettings", "Render/UiRemasterWheel"):
    assert final_map[k] == "1"

os.makedirs(CAND, exist_ok=True)
shutil.copyfile(os.path.join(WS, r"spikes\view_matrix\d3d9.dll"), os.path.join(CAND, "d3d9.dll"))
cand_dll_sha = sha(os.path.join(CAND, "d3d9.dll"))
assert cand_dll_sha.startswith("E304F225"), cand_dll_sha
io.open(os.path.join(CAND, "SingularityVR.ini"), "w", encoding="utf-8", newline="").write(cand_ini)

prev = json.load(io.open(os.path.join(WS, r"deployment\acceptance-20261024-m02-p1\accepted-profile-p1-enable.json"), encoding="utf-8"))
dep_pins = [e for e in prev["files"] if e["path"] not in ("Binaries/d3d9.dll",)]

os.makedirs(PROFILE_DIR, exist_ok=True)
profile = {
    "schema": "singularityvr-accepted-profile-v1",
    "approved": True,
    "acceptedBy": "MAIN SingularityVR (OpenCode main agent), promoting the P1-accepted M02 state for the single combined P2+M09 physical session",
    "acceptanceScope": (
        "Combined deployment: accept the P1-approved product state (dll 107FA9C0..., 357-key INI with Runtime=1, "
        "Settings=1, Wheel=0, no Haptics/TmdPulses; three verified atlas assets; P0 and P1 physically PASS). "
        "Replace the dll with the E304F225... combined build (d104a33 = accepted M02 P0/P1/P2 incl. b49380d + M09 "
        "f73b91d classification fix; both features remain independent default OFF in code) and advanced the INI by "
        "exactly: Render/UiRemasterWheel 0->1 and Haptics/TmdPulses absent->1. Rollback chain: P1-accepted pair is "
        "the preflight backup; independent switches UiRemasterWheel and TmdPulses."
    ),
    "iniPath": "Binaries/SingularityVR.ini",
    "ini": {k: str(v) for k, v in sorted(base_ini.items())},
    "ignoredIniKeys": ["Render/AutoResX", "Render/AutoResY"],
    "files": [{"path": "Binaries/d3d9.dll", "sha256": live_dll_sha}] + dep_pins,
}
profile_path = os.path.join(PROFILE_DIR, "accepted-profile-combined.json")
io.open(profile_path, "w", encoding="utf-8", newline="\n").write(json.dumps(profile, indent=2, ensure_ascii=False) + "\n")
plan = {
    "schema": "singularityvr-change-plan-v1",
    "profileSha256": sha(profile_path),
    "reason": (
        "COMBINED single deployment for ONE physical session: M02 P2 wheel skin (UiRemasterWheel=0->1) and M09 "
        "f73b91d bounded local-only TMD generic pulses (Haptics/TmdPulses absent->1) on a rebuilt dll "
        "(107FA9C0->E304F225 @ d104a33). No magazine insertion comfort, no gameplay HUD text changes, no other features."
    ),
    "files": [
        {"path": "Binaries/d3d9.dll", "before": live_dll_sha, "after": cand_dll_sha,
         "reason": " combined build: accepted M02 (safety b49380d included) + M09 classification fix"},
    ],
    "ini": [
        {"key": "Render/UiRemasterWheel", "before": "0", "after": "1",
         "reason": " P2 grading gate: remastered four-gun wheel"}, 
        {"key": "Haptics/TmdPulses", "before": None, "after": "1",
         "reason": " M09 grading gate: bounded local TMD generic pulses"},
    ],
}
plan_path = os.path.join(PROFILE_DIR, "change-plan-combined.json")
io.open(plan_path, "w", encoding="utf-8", newline="\n").write(json.dumps(plan, indent=2, ensure_ascii=False) + "\n")

print("profile sha:", sha(profile_path))
print("cand dll:", cand_dll_sha)

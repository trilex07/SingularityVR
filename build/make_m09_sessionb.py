import json, os, hashlib, io, os, re

WS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GAME = r"C:\Program Files (x86)\Steam\steamapps\common\Singularity"
DIR = os.path.join(WS, "deployment", "acceptance-20261024-m09-sessionb")
CAND_DIR = os.path.join(WS, "build", "m09-sessionb-candidate")
PROFILE = os.path.join(DIR, "accepted-profile.json")
PLAN = os.path.join(DIR, "change-plan.json")
CAND_INI = os.path.join(CAND_DIR, "SingularityVR.ini")
CAND_DLL = os.path.join(CAND_DIR, "d3d9.dll")
LIVE_INI = os.path.join(GAME, "Binaries", "SingularityVR.ini")
LIVE_DLL = os.path.join(GAME, "Binaries", "d3d9.dll")
M09_DLL = os.path.join(WS, "build", "m09-candidate", "d3d9.dll")  # same M09 candidate bytes (Session-A graded)


def sha(p):
    return hashlib.sha256(open(p, "rb").read()).hexdigest().upper()


def parse_ini(t):
    out, section = {}, ""
    for line in re.split(r"\r\n|\n", t):
        s = line.strip()
        if not s or s.startswith(";") or s.startswith("#"):
            continue
        if re.match(r"^\[[^\]/]+\]$", s):
            section = s[1:-1].strip()
            continue
        m = re.match(r"^([^=/]+?)\s*=\s*(.*?)\s*$", s)
        if section and m:
            key = section + "/" + m.group(1).strip()
            assert key not in out, "dup " + key
            out[key] = m.group(2)
    return out


text = io.open(LIVE_INI, encoding="utf-8-sig").read()
base_ini = parse_ini(text)
assert len(base_ini) == 354, len(base_ini)
assert "Haptics/TmdPulses" not in base_ini

os.makedirs(DIR, exist_ok=True)
os.makedirs(CAND_DIR, exist_ok=True)

# 1) Candidate INI: installed INI + exactly one new key under a fresh [Haptics] section.
addition = (
    "\r\n[Haptics]\r\n"
    "; MAIN 2026-10-24: M09 Session B - bounded native TMD power pulses only; "
    "Session A validated this candidate at TmdPulses=0 identity.\r\n"
    "TmdPulses=1\r\n"
)
cand = text.rstrip("\r\n") + addition
io.open(CAND_INI, "w", encoding="utf-8", newline="").write(cand)
cand_ini = parse_ini(io.open(CAND_INI, encoding="utf-8-sig", newline="").read())
assert len(cand_ini) == 355, len(cand_ini)
assert cand_ini["Haptics/TmdPulses"] == "1"
for k, v in base_ini.items():
    assert cand_ini[k] == v, k

# 2) Same M09 candidate DLL bytes (Session-A graded build 56ae307) staged unchanged.
dll_bytes = io.open(M09_DLL, "rb").read()
assert sha(M09_DLL) == sha(LIVE_DLL), "staged M09 dll must equal the installed M09 dll"
io.open(CAND_DLL, "wb").write(dll_bytes)

# 3) Promoted accepted profile = current installed state (M09 candidate 92D1BABC + 354-key INI).
prompted_files = [{"path": "Binaries/d3d9.dll", "sha256": sha(LIVE_DLL)}]
deps = json.load(io.open(os.path.join(WS, r"deployment\acceptance-20261024-m09\accepted-profile.json"), encoding="utf-8"))["files"]
for entry in deps:
    if entry["path"] != "Binaries/d3d9.dll":
        prompted_files.append({"path": entry["path"], "sha256": entry["sha256"]})

profile = {
    "schema": "singularityvr-accepted-profile-v1",
    "approved": True,
    "acceptedBy": "MAIN SingularityVR (OpenCode main agent), promoting the M09 Session-A graded state (2026-10-24 00:28 psychical session) installed 2026-10-24 00:23:49",
    "acceptanceScope": (
        "M09 Session A PHYSICAL PASS for its tested scope on candidate 92D1BABC820B901BD5E58717E0000ACE81AD18B3EA5FD92D5412E54C1F29C9C4 "
        "(integration/m09-haptics-on-current-baseline @ d97d066: cherry-pick 5a4ac26 on accepted M01/M03 baseline 27f92bf): "
        "weapon firing feedback PASS (instrumented as PSVR2 adaptive-trigger effects; session log contains zero nonzero XInput rumble captures, "
        "identical to the accepted M01 session log), normal TMD behavior PASS, input/two-hand/reload PASS. "
        "Firing with a stationary controller NOT TESTED (earlier user FAIL report withdrawn as misunderstanding - only aimed without firing). "
        "TmdPulses=0 identity during Session A (no xrApplyHapticFeedback submissions; receipts will engage in Session B). "
        "Accepted M01/M03 rollback pair preserved in deployment\\install-backups\\20261024-002349 (dll 78404247..., ini D2E91B0B...)."
    ),
    "iniPath": "Binaries/SingularityVR.ini",
    "ini": {k: str(v) for k, v in sorted(base_ini.items())},
    "ignoredIniKeys": ["Render/AutoResX", "Render/AutoResY"],
    "files": prompted_files,
}
io.open(PROFILE, "w", encoding="utf-8", newline="\n").write(
    json.dumps(profile, indent=2, ensure_ascii=False) + "\n")
profile_sha = sha(PROFILE)

plan = {
    "schema": "singularityvr-change-plan-v1",
    "profileSha256": profile_sha,
    "reason": (
        "M09 Session B controlled change: enable ONLY [Haptics] TmdPulses=1 on the Session-A graded M09 candidate. "
        "DLL bytes unchanged (92D1BABC...); no other keys added or modified; all 354 existing keys preserved."
    ),
    "files": [],
    "ini": [
        {
            "key": "Haptics/TmdPulses",
            "before": None,
            "after": "1",
            "reason": " M09 Session B: enable bounded native TMD power pulses (default OFF otherwise)",
        },
    ],
}
io.open(PLAN, "w", encoding="utf-8", newline="\n").write(json.dumps(plan, indent=2, ensure_ascii=False) + "\n")

print("candidate ini keys:", len(cand_ini))
print("profile sha256:", profile_sha)
print("candidate ini sha256:", sha(CAND_INI))
print("candidate dll sha256:", sha(CAND_DLL))

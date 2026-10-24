import json, hashlib, io, os, re, shutil

WS = r"C:\Users\Trilex\vr-mods\SingularityVR-m02-remaster"
GAME = r"C:\Program Files (x86)\Steam\steamapps\common\Singularity"
BIN = os.path.join(GAME, "Binaries")
ASSETS_SRC = os.path.join(WS, r"assets\processed\ui_remaster\atlas")
ROOT_PRIVATE = os.path.join(WS, r"build\next-weapon-reload")
CAND = os.path.join(ROOT_PRIVATE, "m02-atlas-candidate")
PROFILE_DIR = os.path.join(WS, "deployment", "acceptance-20261024-m02")


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


live_ini_path = os.path.join(BIN, "SingularityVR.ini")
text = io.open(live_ini_path, encoding="utf-8-sig").read()
base_ini = parse_ini(text)
assert len(base_ini) == 354, len(base_ini)
live_dll_sha = sha(os.path.join(BIN, "d3d9.dll"))
assert live_dll_sha.startswith("92D1BABC"), live_dll_sha  # accepted Session-A M09 candidate

ATLASES = {
    "ui_atlas_neutral.png": "E4DC0E7D209BDDF23816F101177C8A8B02AA24ED766C55C446877956CAC4F23B",
    "ui_atlas_localized_pl.png": "6436AA257F310F65A1F65C04D7435D9BF8343BFEFAB2EEE50A8A836F96FC83FD",
    "ui_atlas_localized_en.png": "53B570C4787B834270CBCF0943622877613A7EE7E2AD07FA6A363DFE11A422BA",
}
for name, want in ATLASES.items():
    got = sha(os.path.join(ASSETS_SRC, name))
    assert got == want, (name, got)

os.makedirs(os.path.join(CAND, r"payload\Binaries\vr-assets\ui\remaster"), exist_ok=True)
for name in ATLASES:
    shutil.copyfile(os.path.join(ASSETS_SRC, name),
                    os.path.join(CAND, "payload", "Binaries", "vr-assets", "ui", "remaster", name))
manifest = {
    "schema": "singularityvr-session-assets-delta-v1",
    "files": [
        {"path": "Binaries/vr-assets/ui/remaster/" + name,
         "before": None,
         "after": sha(os.path.join(CAND, "payload", "Binaries", "vr-assets", "ui", "remaster", name)).lower()}
        for name in sorted(ATLASES)
    ],
    "protected": [
        {"path": "Binaries/d3d9.dll", "sha256": live_dll_sha.lower()},
        {"path": "Binaries/SingularityVR.ini", "sha256": hashlib.sha256(open(live_ini_path, "rb").read()).hexdigest()},
    ],
}
io.open(os.path.join(CAND, "manifest.json"), "w", encoding="utf-8", newline="\n").write(
    json.dumps(manifest, indent=2) + "\n")

# ---- Deployment A2 profile/plan (dll + INI with the three M02 switches at 0) ----
m = re.search(r"(?m)^\[Render\][ \t]*$", text)
assert m, "[Render] header not found in installed INI"
cand_a = text[: m.end()] + "\r\n".join([
    "",
    "; M02 P0/P1/P2 switches - staged candidate, all default OFF (MAIN 2026-10-24).",
    "UiRemasterRuntime=0",
    "UiRemasterSettings=0",
    "UiRemasterWheel=0",
]) + text[m.end():]
cand_dir = os.path.join(WS, "build", "m02-runtime-candidate-a")
cand_dir_b = os.path.join(WS, "build", "m02-runtime-candidate-b")
os.makedirs(cand_dir, exist_ok=True)
os.makedirs(cand_dir_b, exist_ok=True)
shutil.copyfile(os.path.join(WS, r"spikes\view_matrix\d3d9.dll"), os.path.join(cand_dir, "d3d9.dll"))
cand_a_sha = hashlib.sha256(open(os.path.join(cand_dir, "d3d9.dll"), "rb").read()).hexdigest().upper()
assert cand_a_sha.startswith("34A2946C"), cand_a_sha
cand_a_ini = cand_a
io.open(os.path.join(cand_dir, "SingularityVR.ini"), "w", encoding="utf-8", newline="").write(cand_a_ini)
cand_a_ini_map = parse_ini(io.open(os.path.join(cand_dir, "SingularityVR.ini"), encoding="utf-8-sig", newline="").read())
assert len(cand_a_ini_map) == 357, len(cand_a_ini_map)

os.makedirs(PROFILE_DIR, exist_ok=True)
deps_src = json.load(io.open(os.path.join(WS, r"deployment\acceptance-20261024-m09-sessionb\accepted-profile.json"), encoding="utf-8"))["files"]
dep_pins = [e for e in deps_src if e["path"] != "Binaries/d3d9.dll"]
atlas_pins = [{"path": "Binaries/vr-assets/ui/remaster/" + n, "sha256": ATLASES[n]} for n in sorted(ATLASES)]

profile_a = {
    "schema": "singularityvr-accepted-profile-v1",
    "approved": True,
    "acceptedBy": "MAIN SingularityVR (OpenCode main agent), sequencing the accepted Session-A runtime state for the M02 first runtime UX gate",
    "acceptanceScope": (
        "Deployment A of the M02 plan: accept the M09 Session-A graded runtime pair (dll 92D1BABC..., INI 354-key map D2E91B0B...) "
        "plus the three freshly installed M02 atlas assets, then replace the dll with the 34A2946C build (3fa5655 + cherry-picks "
        "df17461/d2d54be/9ed671d - integration/m02-settings-wheel-remaster @ 478167e) and add Render/UiRemasterRuntime=0, "
        "UiRemasterSettings=0, UiRemasterWheel=0 (default OFF, visible behavior unchanged). Accepted M01/M03 + M09 Session-A "
        "gameplay preserved; M09 TmdPulses stays absent (default OFF). Atlas rollback via the session-asset manifest receipt."
    ),
    "iniPath": "Binaries/SingularityVR.ini",
    "ini": {k: str(v) for k, v in sorted(base_ini.items())},
    "ignoredIniKeys": ["Render/AutoResX", "Render/AutoResY"],
    "files": [{"path": "Binaries/d3d9.dll", "sha256": live_dll_sha}] + dep_pins + atlas_pins,
}
profile_a_path = os.path.join(PROFILE_DIR, "accepted-profile-deployment-a.json")
io.open(profile_a_path, "w", encoding="utf-8", newline="\n").write(json.dumps(profile_a, indent=2, ensure_ascii=False) + "\n")
plan_a = {
    "schema": "singularityvr-change-plan-v1",
    "profileSha256": sha(profile_a_path),
    "reason": (
        "M02 deployment A: dll 92D1BABC->34A2946C (cherry-picked M02 P0/P1/P2 onto the accepted M09 Session-A source) "
        "and exactly the three default-OFF Render switches; atlases already installed by the companion session-asset delta."
    ),
    "files": [
        {"path": "Binaries/d3d9.dll", "before": live_dll_sha, "after": cand_a_sha,
         "reason": " M02 remaster runtime slice (P0+P1+P2 pruned default-OFF) atop the accepted M09 Session-A source"},
    ],
    "ini": [
        {"key": "Render/UiRemasterRuntime", "before": None, "after": "0",
         "reason": " M02 P0 switch staged default OFF"},
        {"key": "Render/UiRemasterSettings", "before": None, "after": "0",
         "reason": " M02 P1 switch staged default OFF"},
        {"key": "Render/UiRemasterWheel", "before": None, "after": "0",
         "reason": " M02 P2 switch staged default OFF"},
    ],
}
plan_a_path = os.path.join(PROFILE_DIR, "change-plan-deployment-a.json")
io.open(plan_a_path, "w", encoding="utf-8", newline="\n").write(json.dumps(plan_a, indent=2, ensure_ascii=False) + "\n")

# ---- Deployment B profile/plan (P0 enable: Render/UiRemasterRuntime 0->1) ----
a2_ini_map = {k: v for k, v in cand_a_ini_map.items()}
assert a2_ini_map["Render/UiRemasterRuntime"] == "0"
m2 = re.search(r"(?m)^UiRemasterRuntime=0\r?$", cand_a_ini)
cand_b = cand_a_ini[: m2.start()] + "UiRemasterRuntime=1" + cand_a_ini[m2.end():]
cand_b_map = parse_ini(cand_b)
assert len(cand_b_map) == 357 and cand_b_map["Render/UiRemasterRuntime"] == "1"
for k in ("Render/UiRemasterSettings", "Render/UiRemasterWheel"):
    assert cand_b_map[k] == "0"
io.open(os.path.join(cand_dir_b, "SingularityVR.ini"), "w", encoding="utf-8", newline="").write(cand_b)
shutil.copyfile(os.path.join(cand_dir, "d3d9.dll"), os.path.join(cand_dir_b, "d3d9.dll"))

profile_b = {
    "schema": "singularityvr-accepted-profile-v1",
    "approved": True,
    "acceptedBy": "MAIN SingularityVR (OpenCode main agent), promoting the M02 deployment-A install (478167e build) before arming the P0 gate",
    "acceptanceScope": (
        "Deployment B of the M02 plan: accept the deployment-A product state (dll 34A2946C..., 357-key INI with all three M02 "
        "switches at 0). Flags remain default OFF; the physical behavior equals the accepted baseline. Then flip ONLY "
        "Render/UiRemasterRuntime to 1 for the P0 grading session (plate behind VR Settings in both eyes); P1/P2 stay OFF. "
        "M01/M03 gameplay and M09 Session-A accepted state preserved; rollback switch is the flag itself plus the deployment-A backup."
    ),
    "iniPath": "Binaries/SingularityVR.ini",
    "ini": {k: str(v) for k, v in sorted(a2_ini_map.items())},
    "ignoredIniKeys": ["Render/AutoResX", "Render/AutoResY"],
    "files": [{"path": "Binaries/d3d9.dll", "sha256": cand_a_sha}] + dep_pins + atlas_pins,
}
profile_b_path = os.path.join(PROFILE_DIR, "accepted-profile-p0-enable.json")
io.open(profile_b_path, "w", encoding="utf-8", newline="\n").write(json.dumps(profile_b, indent=2, ensure_ascii=False) + "\n")
plan_b = {
    "schema": "singularityvr-change-plan-v1",
    "profileSha256": sha(profile_b_path),
    "reason": (
        "M02 deployment B: arm ONLY Render/UiRemasterRuntime=1 for the P0 physical grading session; "
        "UiRemasterSettings and UiRemasterWheel stay 0; dll unchanged."
    ),
    "files": [],
    "ini": [
        {"key": "Render/UiRemasterRuntime", "before": "0", "after": "1",
         "reason": " P0 grading gate: plate behind VR Settings"},
    ],
}
plan_b_path = os.path.join(PROFILE_DIR, "change-plan-p0-enable.json")
io.open(plan_b_path, "w", encoding="utf-8", newline="\n").write(json.dumps(plan_b, indent=2, ensure_ascii=False) + "\n")

print("atlas pin check: OK (3)")
print("profile A sha:", sha(profile_a_path))
print("profile B sha:", sha(profile_b_path))
print("cand dll:", cand_a_sha)
print("manifest:", os.path.join(CAND, "manifest.json"))

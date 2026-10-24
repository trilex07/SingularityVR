import json, os, hashlib, io, re, shutil

WS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GAME = r"C:\Program Files (x86)\Steam\steamapps\common\Singularity"
BIN = os.path.join(GAME, "Binaries")
BASE = os.path.join(WS, r"spikes\view_matrix")
PROFILE_DIR = os.path.join(WS, "deployment", "acceptance-20261024-diegetic-centurion")
CAND = os.path.join(WS, "build", "diegetic-candidate")
COMBINED_PROFILE = (r"C:\Users\Trilex\vr-mods\SingularityVR-m02-m09"
                    r"\deployment\acceptance-20261024-m02m09-combined"
                    r"\accepted-profile-combined.json")
LIVE_DLL = "E304F22559C6EEF2D8055B73B48518870F1B24199C8BDD2753CCB5A9798E63C6"  # combined accepted
CAND_DLL = "ADF2833A70E0BE027AE4028134B5BFD2CD86759005A7D00AFD7112803B2625DF"  # @551c64d7 + diegetic


def sha(p):
    return hashlib.sha256(open(p, "rb").read()).hexdigest()


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
live_text = io.open(live_ini_path, encoding="utf-8-sig").read()
base_ini = parse_ini(live_text)
assert len(base_ini) == 358, len(base_ini)
assert base_ini["Render/UiRemasterRuntime"] == "1"
assert base_ini["Render/UiRemasterSettings"] == "1"
assert base_ini["Render/UiRemasterWheel"] == "1"
assert base_ini["Haptics/TmdPulses"] == "1"
assert "Render/DiegeticWeaponHud" not in base_ini  # default OFF so far
live_dll_sha = sha(os.path.join(BIN, "d3d9.dll")).upper()
assert live_dll_sha == LIVE_DLL, live_dll_sha
cand_dll_sha = sha(os.path.join(BASE, "d3d9.dll")).upper()
assert cand_dll_sha == CAND_DLL, cand_dll_sha
live_ini_sha = sha(live_ini_path).upper()

atlas_src = os.path.join(WS, r"assets\processed\hud\diegetic\diegetic_hud_atlas.png")
atlas_sha = sha(atlas_src).upper()

# --- candidate staging: dll + INI(payload Atlas for the session-assets delta)
payload = os.path.join(WS, "build", "next-weapon-reload", "diegetic-atlas",
                       "payload", "Binaries", "vr-assets", "hud", "diegetic")
os.makedirs(payload, exist_ok=True)
shutil.copyfile(atlas_src, os.path.join(payload, "diegetic_hud_atlas.png"))
assert sha(os.path.join(payload, "diegetic_hud_atlas.png")).upper() == atlas_sha

m = re.search(r"(?m)^UiRemasterWheel=1\r?$", live_text)
assert m, "UiRemasterWheel=1 anchor not found"
cand_text = live_text[: m.end()] + "\r\nDiegeticWeaponHud=1" + live_text[m.end():]
cand_map = parse_ini(cand_text)
assert len(cand_map) == 359, len(cand_map)
assert cand_map["Render/DiegeticWeaponHud"] == "1"
assert {k: v for k, v in cand_map.items() if k != "Render/DiegeticWeaponHud"} == base_ini
io.open(os.path.join(CAND, "SingularityVR.ini"), "w", encoding="utf-8", newline="").write(cand_text)

# --- session-assets manifest (exclusive narrow mode, hashes lowercase)
manifest = {
    "schema": "singularityvr-session-assets-delta-v1",
    "reason": "Diegetic HUD milestone 1: Centurion 06/24 counter runtime atlas (1024x512 PNG,"
              " no GLB or model is loaded at runtime). DLL/INI are protected and unchanged by this delta.",
    "files": [
        {"path": "Binaries/vr-assets/hud/diegetic/diegetic_hud_atlas.png",
         "before": None, "after": atlas_sha.lower()},
    ],
    "protected": [
        {"path": "Binaries/d3d9.dll", "sha256": live_dll_sha.lower()},
        {"path": "Binaries/SingularityVR.ini", "sha256": live_ini_sha.lower()},
    ],
}
manifest_path = os.path.join(WS, "build", "next-weapon-reload", "diegetic-atlas", "session-assets.json")
os.makedirs(os.path.dirname(manifest_path), exist_ok=True)
shutil.copyfile(os.path.join(BASE, "d3d9.dll"), os.path.join(CAND, "d3d9.dll"))
assert sha(os.path.join(CAND, "d3d9.dll")).upper() == cand_dll_sha
io.open(manifest_path, "w", encoding="utf-8", newline="\n").write(json.dumps(manifest, indent=2) + "\n")

# --- accepted profile: current live state (dll+INI+pins) incl. the atlas pin
os.makedirs(PROFILE_DIR, exist_ok=True)
dep_pins = [e for e in json.load(io.open(COMBINED_PROFILE, encoding="utf-8-sig"))["files"]
            if e["path"] != "Binaries/d3d9.dll"]
profile = {
    "schema": "singularityvr-accepted-profile-v1",
    "approved": True,
    "acceptedBy": "MAIN SingularityVR (OpenCode main agent): accepting the combined M02/M09 state"
                  " (dll E304F225..., UiRemasterWheel=1 + TmdPulses=1) plus the new diegetic atlas,"
                  " to be upgraded by the Centurion-counter plan build of integration/diegetic-hud-centurion-20261024",
    "acceptanceScope": (
        "Physically: M02 P0/P1/P2 + M09 TMD pulses accepted on PSVR2 (2026-10-24 combined session)."
        " This candidate adds ONLY [Render] DiegeticWeaponHud=1, the rebuilt renderer dll and the"
        " diegetic_hud_atlas.png runtime asset; other settings and assets stay byte-identical."),
    "iniPath": "Binaries/SingularityVR.ini",
    "ini": {k: str(v) for k, v in sorted(base_ini.items())},
    "ignoredIniKeys": ["Render/AutoResX", "Render/AutoResY"],
    "files": (["Binaries/d3d9.dll"] and [{"path": "Binaries/d3d9.dll", "sha256": live_dll_sha}]) + dep_pins + [
        {"path": "Binaries/vr-assets/hud/diegetic/diegetic_hud_atlas.png", "sha256": atlas_sha}],
}
profile_path = os.path.join(PROFILE_DIR, "accepted-profile-diegetic-centurion.json")
io.open(profile_path, "w", encoding="utf-8", newline="\n").write(json.dumps(profile, indent=2, ensure_ascii=True) + "\n")

plan = {
    "schema": "singularityvr-change-plan-v1",
    "profileSha256": sha(profile_path).upper(),
    "reason": "Diegetic HUD milestone 1 (Centurion only, READY_FOR_PHYSICAL_ACCEPTANCE off-mine): rebuild"
              " d3d9.dll (E304F225 -> ADF2833A @ 551c64d7 + diegetic_weapon_hud.h) and arm the gate via"
              " Render/DiegeticWeaponHud 0(default absent) -> 1 for ONE short PSVR2 session. Centurion ammo"
              " plate backs off while the diegetic counter draws; both switches roll back independently.",
    "files": [
        {"path": "Binaries/d3d9.dll", "before": live_dll_sha, "after": cand_dll_sha,
         "reason": " diegetic counter renderer integration (plate-pipeline per-eye draw, live mClipAmmo/mBinAmmo)"},
    ],
    "ini": [
        {"key": "Render/DiegeticWeaponHud", "before": None, "after": "1",
         "reason": " Centurion counter grading gate (default OFF in source; explicitly armed for the session)"},
    ],
}
plan_path = os.path.join(PROFILE_DIR, "change-plan-diegetic-centurion.json")
io.open(plan_path, "w", encoding="utf-8", newline="\n").write(json.dumps(plan, indent=2, ensure_ascii=True) + "\n")

print("manifest:", manifest_path, sha(manifest_path))
print("profile :", profile_path, sha(profile_path).upper())
print("plan    :", plan_path, sha(plan_path).upper())
print("dll     :", cand_dll_sha)
print("atlas   :", atlas_sha)

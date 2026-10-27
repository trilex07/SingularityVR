import json, os, hashlib, io, re, shutil

WS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GAME = r"C:\Program Files (x86)\Steam\steamapps\common\Singularity"
BIN = os.path.join(GAME, "Binaries")
BASE = os.path.join(WS, r"spikes\view_matrix")
PROFILE_DIR = os.path.join(WS, "deployment", "acceptance-20261024-diegetic-valkyrie-spikeshot")
CAND = os.path.join(WS, "build", "diegetic-upgrade-candidate")
COMBINED_PROFILE = (r"C:\Users\Trilex\vr-mods\SingularityVR-m02-m09"
                    r"\deployment\acceptance-20261024-m02m09-combined"
                    r"\accepted-profile-combined.json")
LIVE_DLL = "ADF2833A70E0BE027AE4028134B5BFD2CD86759005A7D00AFD7112803B2625DF"  # Centurion candidate (rollback point)


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
assert len(base_ini) == 359, len(base_ini)
assert base_ini["Render/DiegeticWeaponHud"] == "1"  # gate stays armed for the session
live_dll_sha = sha(os.path.join(BIN, "d3d9.dll")).upper()
assert live_dll_sha == LIVE_DLL, live_dll_sha
live_ini_sha = sha(live_ini_path).upper()
cand_dll_sha = sha(os.path.join(BASE, "d3d9.dll")).upper()

# Atlas (unchanged this milestone): compute from the deployed live file; pin it
# must also equal the repo copy imported from the side branch.
atlas_rel = r"vr-assets\hud\diegetic\diegetic_hud_atlas.png"
atlas_sha = sha(os.path.join(BIN, atlas_rel)).upper()
repo_atlas = os.path.join(WS, "assets", "processed", "hud", "diegetic", "diegetic_hud_atlas.png")
assert sha(repo_atlas).upper() == atlas_sha, "repo atlas and deployed atlas differ"

os.makedirs(CAND, exist_ok=True)
os.makedirs(PROFILE_DIR, exist_ok=True)
shutil.copyfile(os.path.join(BASE, "d3d9.dll"), os.path.join(CAND, "d3d9.dll"))
assert sha(os.path.join(CAND, "d3d9.dll")).upper() == cand_dll_sha
# INI untouched this milestone: ship the live bytes unchanged (no plan.ini allowance).
shutil.copyfile(live_ini_path, os.path.join(CAND, "SingularityVR.ini"))
assert sha(os.path.join(CAND, "SingularityVR.ini")).upper() == live_ini_sha
assert parse_ini(io.open(os.path.join(CAND, "SingularityVR.ini"), encoding="utf-8-sig").read()) == base_ini

dep_pins = [e for e in json.load(io.open(COMBINED_PROFILE, encoding="utf-8-sig"))["files"]
            if e["path"] != "Binaries/d3d9.dll"]
profile = {
    "schema": "singularityvr-accepted-profile-v1",
    "approved": True,
    "acceptedBy": "MAIN SingularityVR (OpenCode main agent): accepting the deployed Centurion-candidate"
                  " state (dll ADF2833A..., atlas 9ED4A5C3..., INI 359 keys with DiegeticWeaponHud=1)"
                  " before upgrading the renderer to the Valkyrie+Spikeshot milestone build",
    "acceptanceScope": (
        "Physical: the Centurion candidate was NOT graded (weapon not owned in this save); its deploy is"
        " preserved as the rollback point. The upgrade changes ONLY the renderer dll to the"
        " Valkyrie(11387)+Spikeshot(8297) diegetic milestone (commit bc4c32147c3b); INI and assets byte-identical."
        " Merchant save grants both listed guns; no other scope is claimed."),
    "iniPath": "Binaries/SingularityVR.ini",
    "ini": {k: str(v) for k, v in sorted(base_ini.items())},
    "ignoredIniKeys": ["Render/AutoResX", "Render/AutoResY"],
    "files": [{"path": "Binaries/d3d9.dll", "sha256": live_dll_sha}] + dep_pins + [
        {"path": "Binaries/vr-assets/hud/diegetic/diegetic_hud_atlas.png", "sha256": atlas_sha}],
}
profile_path = os.path.join(PROFILE_DIR, "accepted-profile-diegetic-v2.json")
io.open(profile_path, "w", encoding="utf-8", newline="\n").write(json.dumps(profile, indent=2, ensure_ascii=True) + "\n")

plan = {
    "schema": "singularityvr-change-plan-v1",
    "profileSha256": sha(profile_path).upper(),
    "reason": "Diegetic HUD milestone 2 (Valkyrie + Spikeshot; READY_FOR_PHYSICAL_ACCEPTANCE): rebuild"
              " d3d9.dll (ADF2833A -> " + cand_dll_sha[:12] + "... @ bc4c32147c3b /"
              " integration/diegetic-hud-centurion-20261024) with per-weapon measured mounts and"
              " '30/120' / '08/32' readouts. Gate DiegeticWeaponHud=1 stays armed for the grading"
              " session; INI byte-identical; rollback = the preserved Centurion-candidate backup.",
    "files": [
        {"path": "Binaries/d3d9.dll", "before": live_dll_sha, "after": cand_dll_sha,
         "reason": " per-weapon diegetic mounts (Valkyrie/Spikeshot/Centurion) + plate back-off extension"},
    ],
    "ini": [],
}
plan_path = os.path.join(PROFILE_DIR, "change-plan-diegetic-v2.json")
io.open(plan_path, "w", encoding="utf-8", newline="\n").write(json.dumps(plan, indent=2, ensure_ascii=True) + "\n")

print("profile :", profile_path, sha(profile_path).upper())
print("plan    :", plan_path, sha(plan_path).upper())
print("dll     :", cand_dll_sha)
print("live ini:", live_ini_sha)

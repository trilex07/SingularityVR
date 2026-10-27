import json, os, hashlib, io, re, shutil

WS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GAME = r"C:\Program Files (x86)\Steam\steamapps\common\Singularity"
BIN = os.path.join(GAME, "Binaries")
BASE = os.path.join(WS, r"spikes\view_matrix")
ACC = os.path.join(WS, "deployment", "acceptance-20261024-sol-integration")
CAND = os.path.join(WS, "build", "sol-candidate")
ASSETS = os.path.join(WS, "build", "next-weapon-reload", "icons-atlas")
COMBINED_PROFILE = (r"C:\Users\Trilex\vr-mods\SingularityVR-m02-m09"
                    r"\deployment\acceptance-20261024-m02m09-combined"
                    r"\accepted-profile-combined.json")
LIVE_DLL = "B5F2A2AD4ADB53BBA4DEC5369CD5F2A5FD1EA06373C6F1E1E58E374633B58613"  # milestone-2 candidate (rollback point)
LIVE_TOI = "895523B4986F584F6BF336F36321EE446BEED63966B3ECCED6C64B62A9228185"  # texture_override.ini live
WHEEL_OLD = "E4DC0E7D209BDDF23816F101177C8A8B02AA24ED766C55C446877956CAC4F23B"
WHEEL_NEW = "3EF1782696B4A5E1E9D93F409C5B0F6FF014B78CC42FAB06EC29110661981818"


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
assert base_ini["Render/DiegeticWeaponHud"] == "1"          # already armed (milestone 1)
assert base_ini["Render/TextureBridgeOwned"] == "1"          # already armed (accepted bridge route)
assert "Render/VrGameplayTextSafeArea" not in base_ini       # default OFF in source
assert "Input/TwoHandSpreadPercent" not in base_ini
live_dll_sha = sha(os.path.join(BIN, "d3d9.dll")).upper()
assert live_dll_sha == LIVE_DLL, live_dll_sha
live_toi_sha = sha(os.path.join(BIN, "texture_override.ini")).upper()
TOI_MODE1 = None  # set after composing the Mode-1 bytes below
assert live_toi_sha in (LIVE_TOI, "C43E3EA1E8DBCE7E64B375DCA8B8201A44A37443E91590A6A4EA9D396BAEAB25"), live_toi_sha
live_ini_sha = sha(live_ini_path).upper()

cand_dll_sha = sha(os.path.join(BASE, "d3d9.dll")).upper()
prov = json.load(io.open(os.path.join(BASE, "d3d9.build.json"), encoding="utf-8-sig"))
assert prov["commit"].startswith("09887b18"), prov["commit"]  # integration merge tip
assert prov["machine"] == "0x014C" and prov["optimization"] == "/O2"

# diegetic atlas pin (unchanged, runtime-shipped asset)
atlas_rel = r"vr-assets\hud\diegetic\diegetic_hud_atlas.png"
ATLAS_DIEGETIC = sha(os.path.join(BIN, atlas_rel)).upper()
repo_atlas = os.path.join(WS, "assets", "processed", "hud", "diegetic", "diegetic_hud_atlas.png")
assert sha(repo_atlas).upper() == ATLAS_DIEGETIC

# wheel atlas: live may be pre- or post-delta (idempotent generator); the repo
# branch file is the approved new asset (verified against the PR hash).
wheel_rel = r"vr-assets\ui\remaster\ui_atlas_neutral.png"
assert sha(os.path.join(BIN, wheel_rel)).upper() in (WHEEL_OLD, WHEEL_NEW)
WHEEL_LIVE = sha(os.path.join(BIN, wheel_rel)).upper()
src_wheel = os.path.join(WS, "assets", "processed", "ui_remaster", "atlas", "ui_atlas_neutral.png")
assert sha(src_wheel).upper() == WHEEL_NEW

# ---- texture_override.ini Mode-1 delta
# (idempotent: live may already carry Mode 1 from the applied assets delta)
toi_path = os.path.join(BIN, "texture_override.ini")
staged_toi = os.path.join(ASSETS, "payload", "Binaries", "texture_override.ini")
toi_text = io.open(toi_path, encoding="utf-8-sig").read()
import difflib
if live_toi_sha == LIVE_TOI:
    # PRE-delta: find the anchor in the pre file and insert exactly one line.
    m = re.search(r"(?m)^bridge_owned=1(?!priority)\r?$", toi_text)
    assert m, "bridge_owned anchor missing"
    toi_new = toi_text[: m.end()] + "\r\npriority_material_mode=1" + toi_text[m.end():]
    os.makedirs(os.path.dirname(staged_toi), exist_ok=True)
    io.open(staged_toi, "w", encoding="utf-8", newline="").write(toi_new)
    dev = [l for l in difflib.unified_diff(toi_text.splitlines(), toi_new.splitlines(), lineterm="")
           if l.startswith(("+", "-")) and not l.startswith(("+++", "---"))]
    assert dev == ["+priority_material_mode=1"], dev
    toi_new_sha = hashlib.sha256(toi_new.encode("utf-8")).hexdigest().upper()
else:
    # POST-delta: the executed staging file is the canonical installed bytes
    # (the live file is a verified copy of it). Recomposing would double the
    # inserted line, so the executed artifact is read back instead.
    assert os.path.exists(staged_toi), "executed assets staging is missing"
    staged_sha = sha(staged_toi).upper()
    assert staged_sha == live_toi_sha, \
        "executed staging bytes do not match the installed Mode-1 file"
    with open(staged_toi, "rb") as f:
        toi_new = f.read().decode("utf-8-sig")
    toi_new_sha = staged_sha
toi_map = parse_ini(toi_new)
assert toi_map["general/priority_material_mode"] == "1"
assert toi_map["general/remaster_apply"] == "1"      # accepted live policy kept
assert toi_map["general/effects_remaster"] == "1"    # accepted live policy kept
assert toi_map["general/mode_apply"] == "1"
assert toi_map["TextureQuality/Quality"] == "8K"

# ---- SingularityVR.ini candidate delta (365 keys)


def insert_after_header(text, header, lines):
    m2 = re.search(r"(?m)^\[?" + re.escape(header) + r"\]?[=\w]*\r?$", text)
    assert m2, "anchor not found: " + header
    add = "".join(l + "\r\n" for l in lines)
    return text[: m2.end()] + "\r\n" + add + text[m2.end():]


cand_text = insert_after_header(live_text, "Input", [
    "TwoHandSpreadPercent=100",
    "VrValkyrieSpreadPercent=70",
    "VrValkyrieTwoHandPercent=50",
    "VrVolkSpreadPercent=85",
    "VrVolkTwoHandPercent=50",
])
cand_text = insert_after_header(cand_text, "Render", [
    "VrGameplayTextSafeArea=1",
])
cand_map = parse_ini(cand_text)
assert len(cand_map) == 365, len(cand_map)
for k, v in [("Input/TwoHandSpreadPercent", "100"),
             ("Input/VrValkyrieSpreadPercent", "70"),
             ("Input/VrValkyrieTwoHandPercent", "50"),
             ("Input/VrVolkSpreadPercent", "85"),
             ("Input/VrVolkTwoHandPercent", "50"),
             ("Render/VrGameplayTextSafeArea", "1")]:
    assert cand_map[k] == v, (k, cand_map.get(k))
diff = {k for k in base_ini if base_ini[k] != cand_map.get(k)}
assert not diff, diff

# ---- payloads
os.makedirs(ASSETS + r"\payload\Binaries\vr-assets\ui\remaster", exist_ok=True)
os.makedirs(os.path.join(ASSETS, "payload", "Binaries", "vr-assets", "ui", "remaster"), exist_ok=True)
shutil.copyfile(src_wheel, os.path.join(ASSETS, "payload", "Binaries", "vr-assets", "ui", "remaster", "ui_atlas_neutral.png"))
assert sha(os.path.join(ASSETS, "payload", "Binaries", "vr-assets", "ui", "remaster", "ui_atlas_neutral.png")).upper() == WHEEL_NEW
os.makedirs(CAND, exist_ok=True)
shutil.copyfile(os.path.join(BASE, "d3d9.dll"), os.path.join(CAND, "d3d9.dll"))
assert sha(os.path.join(CAND, "d3d9.dll")).upper() == cand_dll_sha
io.open(os.path.join(CAND, "SingularityVR.ini"), "w", encoding="utf-8", newline="").write(cand_text)

manifest = {
    "schema": "singularityvr-session-assets-delta-v1",
    "reason": "PR #56 detailed weapon wheel icons: only the neutral wheel atlas is"
              " replaced (3EF17826...); wheel geometry/controls, sprite UV rects, PL/EN"
              " localized atlases and M09 haptics are untouched; no renderer change."
              " Same guarded delta adds texture_override.ini priority_material_mode=1"
              " (remaster Mode 1: log-only classification of AR_shotgun_d/n +"
              " inview_glove_d/n; every existing texture key byte-identical).",
    "files": [
        {"path": "Binaries/vr-assets/ui/remaster/ui_atlas_neutral.png",
         "before": (WHEEL_OLD if WHEEL_LIVE == WHEEL_OLD else WHEEL_NEW).lower(),
         "after": WHEEL_NEW.lower()},
    ],
    "protected": [
        {"path": "Binaries/d3d9.dll", "sha256": live_dll_sha.lower()},
        {"path": "Binaries/SingularityVR.ini", "sha256": live_ini_sha.lower()},
        {"path": "Binaries/vr-assets/hud/diegetic/diegetic_hud_atlas.png", "sha256": ATLAS_DIEGETIC.lower()},
    ],
}
manifest_path = os.path.join(ASSETS, "session-assets.json")
io.open(manifest_path, "w", encoding="utf-8", newline="\n").write(json.dumps(manifest, indent=2) + "\n")

# ---- staged texture_override.ini (installed by the session-assets run)
io.open(os.path.join(ASSETS, "payload", "Binaries", "texture_override.ini"), "w",
        encoding="utf-8", newline="").write(toi_new)
toi_new_sha = hashlib.sha256(toi_new.encode("utf-8")).hexdigest().upper()

# ---- accepted profile: PRE state for the dll run (runs AFTER the assets delta,
# so texture_override.ini is pinned at its Mode-1 post-delta value)
os.makedirs(ACC, exist_ok=True)
dep_pins = [e for e in json.load(io.open(COMBINED_PROFILE, encoding="utf-8-sig"))["files"]
            if e["path"] != "Binaries/d3d9.dll"
            and e["path"] != "Binaries/vr-assets/ui/remaster/ui_atlas_neutral.png"]  # replaced by this campaign's assets delta
existing_paths = {e["path"] for e in dep_pins}
assert "Binaries/texture_override.ini" not in existing_paths
profile = {
    "schema": "singularityvr-accepted-profile-v1",
    "approved": True,
    "acceptedBy": "MAIN SingularityVR (OpenCode main agent): accepting the post-assets-delta live state"
                  " (wheel atlas 3EF17826..., texture_override.ini Mode-1 C43E3EA1..., dll B5F2A2AD"
                  " milestone-2, INI 257F9DFB) before upgrading the renderer to the integrated SOL build",
    "acceptanceScope": (
        "Physically: M02 P0/P1/P2 + M09 accepted; diegetic HUD Valkyrie/Spikeshot is a PHYSICAL FAIL"
        " (not visible; corrected in this candidate: per-loader asset roots + frame-atomic plate back-off)."
        " Candidate: renderer dll 9500DE1E (diegetic fix + icons-neutral + accuracy eade95a + PR57 D3D9Ex"
        " bridge), SingularityVR.ini adds VrGameplayTextSafeArea=1 + the five Input accuracy keys,"
        " session-assets delta already applied the icons atlas and texture_override.ini Mode-1."
        " The four private remaster DDS are NOT deployed (Mode 2 only). No audio or dubbing changes."),
    "iniPath": "Binaries/SingularityVR.ini",
    "ini": {k: str(v) for k, v in sorted(base_ini.items())},
    "ignoredIniKeys": ["Render/AutoResX", "Render/AutoResY"],
    "files": [{"path": "Binaries/d3d9.dll", "sha256": live_dll_sha},
              {"path": "Binaries/texture_override.ini", "sha256": toi_new_sha}] + dep_pins + [
        {"path": "Binaries/vr-assets/hud/diegetic/diegetic_hud_atlas.png", "sha256": ATLAS_DIEGETIC}],
}
profile_path = os.path.join(ACC, "accepted-profile-sol.json")
io.open(profile_path, "w", encoding="utf-8", newline="\n").write(json.dumps(profile, indent=2, ensure_ascii=True) + "\n")

plan = {
    "schema": "singularityvr-change-plan-v1",
    "profileSha256": sha(profile_path).upper(),
    "reason": "Integrated SOL candidate renderer upgrade (READY_FOR_PHYSICAL_ACCEPTANCE, independently"
              " rollable per feature): d3d9.dll " + cand_dll_sha[:12] + " @ " + prov["commit"][:8]
              + " = diegetic P0 visibility fix + accuracy correction (eade95a) + PR57 D3D9Ex bridge"
              " (single slot-65 owner) + PR56-icon-compatible wheel runtime. INI gates the candidate"
              " features; wheel atlas + texture_override.ini Mode-1 already installed by the separate"
              " session-assets delta of the same guard sequence.",
    "files": [
        {"path": "Binaries/d3d9.dll", "before": live_dll_sha, "after": cand_dll_sha,
         "reason": " diegetic asset-root+atomic back-off fix; accuracy; D3D9Ex bridge"},
    ],
    "ini": [
        {"key": "Render/VrGameplayTextSafeArea", "before": None, "after": "1",
         "reason": " gameplay LocationTextDisplay safe-area reposition for candidate grading (rollback: 0)"},
        {"key": "Input/TwoHandSpreadPercent", "before": None, "after": "100",
         "reason": " accepted global two-hand policy pinned (native dispersion unchanged)"},
        {"key": "Input/VrValkyrieSpreadPercent", "before": None, "after": "70",
         "reason": " opt-in Valkyrie random-delta scaling (one hand; 35% with two-hand support)"},
        {"key": "Input/VrValkyrieTwoHandPercent", "before": None, "after": "50",
         "reason": " opt-in Valkyrie two-hand support factor"},
        {"key": "Input/VrVolkSpreadPercent", "before": None, "after": "85",
         "reason": " opt-in Volk random-delta scaling (one hand; ~43% with two-hand support)"},
        {"key": "Input/VrVolkTwoHandPercent", "before": None, "after": "50",
         "reason": " opt-in Volk two-hand support factor"},
    ],
}
plan_path = os.path.join(ACC, "change-plan-sol.json")
io.open(plan_path, "w", encoding="utf-8", newline="\n").write(json.dumps(plan, indent=2, ensure_ascii=True) + "\n")

print("manifest :", manifest_path, sha(manifest_path))
print("profile  :", profile_path, sha(profile_path).upper())
print("plan     :", plan_path, sha(plan_path).upper())
print("dll      :", cand_dll_sha)
print("wheel    :", WHEEL_OLD[:12], "->", WHEEL_NEW[:12])
print("ini      :", live_ini_sha[:12], "->", sha(os.path.join(CAND, "SingularityVR.ini")).upper()[:12], "(365 keys)")
print("tex_ovr  :", live_toi_sha[:12], "->", toi_new_sha[:12])

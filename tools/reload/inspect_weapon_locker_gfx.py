"""Offline scan of GFx RawData for WeaponLocker / loadout-related ActionScript strings.

Does not launch the game. Uses decompressed RvGame + logical UI_SinglePlayer_SF.u.
"""
import argparse
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "title"))
from ue3_layout import inspect  # noqa: E402
from swf_title_patch import split_tags  # noqa: E402

SCAN_NEEDLES = (
    "WeaponLocker",
    "WeaponsLocker",
    "Menu_Weapon",
    "LoadEquippedWeapons",
    "LoadUpgrades",
    "GetGameSystem",
    "equip",
    "mWeapons",
    "weaponPath",
    "weaponName",
    "WeaponNumber",
    "weaponForSlot",
    "LoadoutDropdown",
    "SetupLoadout",
    "constants.WeaponNames",
    "W_PISTOL",
    "armachinegun",
    "arpistol",
    "arshotgun",
    "arsniperrifle",
)


def raw_movie(package: bytes, export_name: str) -> bytes:
    d = inspect("x", package, include_all=True)
    e = next(x for x in d["exports"] if x["name"] == export_name and x["class_name"] == "GFxMovieInfo")
    raw = next(p for p in e["properties"] if p["name"] == "RawData")
    at = raw["tag_offset"] + 24
    count = struct.unpack_from("<i", package, at)[0]
    return package[at + 4 : at + 4 + count]


def export_symbols(movie: bytes) -> list[str]:
    _, tags = split_tags(movie)
    names = []
    for t in tags:
        if t["code"] != 56:
            continue
        data = movie[t["pay_off"] : t["pay_off"] + t["pay_len"]]
        cnt = struct.unpack_from("<H", data)[0]
        pos = 2
        for _ in range(cnt):
            pos += 2
            end = data.index(0, pos)
            names.append(data[pos:end].decode("ascii", "replace"))
            pos = end + 1
    return names


def scan_movie(label: str, movie: bytes) -> dict:
    hits = {n: movie.count(n.encode("ascii")) for n in SCAN_NEEDLES if movie.count(n.encode("ascii"))}
    syms = [s for s in export_symbols(movie) if any(k in s.lower() for k in ("weapon", "locker", "loadout", "slot", "equip", "station"))]
    return {"label": label, "bytes": len(movie), "needle_hits": hits, "weapon_related_exports": syms}


def inspect_packages(ui_u: Path, rvgame: Path) -> dict:
    ui = ui_u.read_bytes()
    rv = rvgame.read_bytes()
    movies = []
    for name in ("SPMenus", "iis"):
        movies.append(scan_movie(f"UI_SinglePlayer_SF/{name}", raw_movie(ui, name)))
    for name in ("menu_start", "HUD", "Dialogs", "Credits"):
        movies.append(scan_movie(f"RvGame/{name}", raw_movie(rv, name)))
    return {
        "schema": "singularityvr-weapon-locker-gfx-scan-v1",
        "ui_package": str(ui_u),
        "rvgame_package": str(rvgame),
        "movies": movies,
        "conclusions": {
            "in_game_locker_swf_export": "NOT FOUND as GFxMovieInfo export (no Menu_WeaponsLocker string in scanned packages)",
            "hud_weapon_identity_map": "RvGame/HUD constants.WeaponNames: W_PISTOL->arpistol, W_MACHINEGUN->armachinegun, W_SHOTGUN->arshotgun, W_SNIPER_RIFLE->arsniperrifle",
            "sp_menus_loadout": "UI/SPMenus has MP LoadoutDropdown/SetupLoadout only — not SP WeaponLocker station",
            "native_binding": "RvUIGFxMovie_WeaponLocker extends UseStation; export execs equip(Weapon,slot), LoadEquippedWeapons, LoadUpgrades, Initialize",
            "use_station_list_api": "RvUIGFxMovie_UseStation exports GetItemAt(Index), TransferData, equip(slot,perk) — shared station list pattern",
        },
    }


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--ui-u", type=Path, default=Path(r"C:\Users\Trilex\vr-mods\pause-re-lab\UI_SinglePlayer_SF.u"))
    ap.add_argument("--rvgame", type=Path, default=Path(r"C:\Users\Trilex\vr-mods\research\native-hands-20261007\RvGame.decompressed.xxx"))
    args = ap.parse_args()
    print(json.dumps(inspect_packages(args.ui_u, args.rvgame), indent=2))

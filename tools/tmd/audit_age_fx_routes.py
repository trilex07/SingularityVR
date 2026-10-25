"""Read-only audit of the original Age effect routes in private Singularity assets.

The report identifies serialized assets, UnrealScript field references and
selected native call sites. It never claims which particle was visible in VR.
Do not commit the input package, EXE, memory image or generated report.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "title"))
sys.path.insert(0, str(ROOT / "tools" / "reload"))
sys.path.insert(0, str(ROOT / "tools" / "fx"))

from ue3_layout import inspect  # noqa: E402
from dump_ue3_script_bytecode import parse_function_tail  # noqa: E402
from audit_rifle_scope_backend import PE  # noqa: E402


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def audit(rvgame: Path, exe: Path, memory_text: Path) -> dict:
    from capstone import Cs, CS_ARCH_X86, CS_MODE_32

    package = rvgame.read_bytes()
    executable = exe.read_bytes()
    text = memory_text.read_bytes()
    layout = inspect(str(rvgame), package, include_all=True)
    records = layout["records"]
    imports = layout["imports"]
    names = layout["names"]

    def ref(index: int) -> str:
        if index > 0:
            return records[index - 1]["name"]
        if index < 0:
            return names[imports[-index - 1][5]]
        return "None"

    by_name = {name: i for i, name in enumerate(names)}
    power = next(
        x for x in layout["exports"]
        if x["name"] == "RvTMDPower_Age" and ref(x["outer"]) == "TMDPowers_SP"
    )
    effects = []
    for prop in power["properties"]:
        if prop["name"] not in ("mStartFX", "mAgeForwardFX", "mAgeBackwardFX"):
            continue
        index = struct.unpack_from("<i", package, prop["tag_offset"] + 24)[0]
        assert 0 < index <= len(records), (prop["name"], index)
        effect = records[index - 1]
        assert effect["name"] == "RvEffectObject"
        raw = package[effect["offset"] : effect["offset"] + effect["size"]]

        def tagged_value(field: str) -> int:
            marker = struct.pack("<ii", by_name[field], 0)
            at = raw.find(marker)
            assert at >= 0 and raw.find(marker, at + 1) < 0, field
            return struct.unpack_from("<i", raw, at + 24)[0]

        socket = names[tagged_value("mBoneOrSocket")]
        asset_index = tagged_value("mParticles")
        assert 0 < asset_index <= len(records)
        effects.append({
            "field": prop["name"], "effect_export": index,
            "socket": socket, "particle_export": asset_index,
            "particle_name": ref(asset_index),
        })

    method = next(
        x for x in records
        if x["name"] == "PlayInitialEffects" and ref(x["outer"]) == "RvEffectObject"
    )
    bytecode = parse_function_tail(
        package[method["offset"] : method["offset"] + method["size"]], layout
    )
    script = bytes.fromhex(bytecode["script_hex"])
    fields = {}
    for name in ("mParticles", "mBoneOrSocket", "mbAttachToOwner"):
        index = next(
            i for i, x in enumerate(records, 1)
            if x["name"] == name and ref(x["outer"]) == "RvEffectObject"
        )
        fields[name] = [
            pos for pos in range(len(script) - 3)
            if struct.unpack_from("<i", script, pos)[0] == index
        ]
        assert fields[name], name

    pe = PE(executable)
    section = next(x for x in pe.sections if x[0] == ".text")
    base = pe.base + section[2]
    assert base <= 0x100D818 < base + len(text)
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)

    def instructions(start: int, size: int) -> list[str]:
        return [
            f"{ins.address:08X} {ins.mnemonic} {ins.op_str}"
            for ins in decoder.disasm(text[start - base : start - base + size], start)
        ]

    return {
        "input_sha256": {
            "rvgame": sha(package), "exe": sha(executable), "text": sha(text)
        },
        "age_effects": effects,
        "play_initial_effects_script_size": len(script),
        "play_initial_effects_field_refs": fields,
        "native_activation": instructions(0x100D818, 0x41),
        "native_age_extra_effect": instructions(0x100B465, 0x22),
        "evidence_limit": "Asset and call paths only; live particle transform and visible pixels remain unobserved",
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rvgame", required=True, type=Path)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--text", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    report = audit(args.rvgame, args.exe, args.text)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(f"wrote {args.output} ({len(report['age_effects'])} effect references)")


if __name__ == "__main__":
    main()

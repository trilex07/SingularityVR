#!/usr/bin/env python3
"""Stage the current XTTS story/skipped/vignette delivery into a MAIN candidate.

Offline only: validates identity and PCM format, copies payload into a candidate
folder, and patches only exact skipped/vignette source rows in the installed
story lookup. It never writes Steam or changes the installed lookup.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import struct
import wave
from pathlib import Path


EXPECTED = {"story": 1667, "skipped": 97, "vignette": 6}
LOOKUP_REL = Path("Binaries/vr-assets/localization/pl/polish_story_voice.jsonl")
AUDIO_REL = Path("Binaries/vr-assets/audio/polish/story")


def sha(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def rows(path: Path):
    return [json.loads(line) for line in path.read_text(encoding="utf-8-sig").splitlines() if line.strip()]


def validate_wav(path: Path) -> dict:
    with wave.open(str(path), "rb") as wav:
        if (wav.getnchannels(), wav.getsampwidth(), wav.getframerate(), wav.getcomptype()) != (1, 2, 24000, "NONE"):
            raise ValueError(f"Expected mono PCM16 24 kHz WAV: {path}")
        count = wav.getnframes()
        data = wav.readframes(count)
    if count <= 0 or len(data) != count * 2:
        raise ValueError(f"Empty/truncated WAV: {path}")
    samples = struct.iter_unpack("<h", data)
    peak = max(abs(sample[0]) for sample in samples)
    if peak == 0 or peak >= 32767:
        raise ValueError(f"Silent/clipped WAV: {path}")
    return {"frames": count, "peak_sample": peak}


def safe_leaf(name: str) -> bool:
    return bool(re.fullmatch(r"[A-Za-z0-9_.-]+\.wav", name, re.IGNORECASE)) and ".tmp." not in name.lower()


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--source-root", type=Path, required=True)
    ap.add_argument("--production-manifest", type=Path, required=True)
    ap.add_argument("--skipped-curated", type=Path, required=True)
    ap.add_argument("--vignette-curated", type=Path, required=True)
    ap.add_argument("--installed-lookup", type=Path, required=True)
    ap.add_argument("--accepted-profile", type=Path, required=True)
    ap.add_argument("--candidate", type=Path, required=True)
    args = ap.parse_args()

    source_root = args.source_root.resolve(strict=True)
    candidate = args.candidate.resolve(strict=True)
    if (candidate / "audio-payload-manifest.json").exists() or (candidate / "payload").exists():
        raise ValueError("Refusing to overwrite an existing candidate payload")

    production = rows(args.production_manifest)
    by_pack = {pack: [row for row in production if row.get("pack") == pack] for pack in EXPECTED}
    if {pack: len(pack_rows) for pack, pack_rows in by_pack.items()} != EXPECTED:
        raise ValueError("Unexpected production inventory: " + str({p: len(r) for p, r in by_pack.items()}))
    for pack_rows in by_pack.values():
        names = [row.get("file", "") for row in pack_rows]
        if len(set(names)) != len(names) or not all(safe_leaf(name) for name in names):
            raise ValueError("Duplicate or unsafe production filename")

    baseline_profile = json.loads(args.accepted_profile.read_text(encoding="utf-8-sig"))
    lookup_pin = next((item for item in baseline_profile.get("files", [])
                       if item.get("path", "").lower() == str(LOOKUP_REL).replace("\\", "/").lower()), None)
    original_lookup_bytes = args.installed_lookup.read_bytes()
    original_lookup_sha = hashlib.sha256(original_lookup_bytes).hexdigest()
    if not baseline_profile.get("approved") or not lookup_pin or lookup_pin.get("sha256", "").lower() != original_lookup_sha:
        raise ValueError("Installed story lookup does not match the approved accepted profile")
    lookup = [json.loads(line) for line in original_lookup_bytes.decode("utf-8-sig").splitlines() if line.strip()]
    if len(lookup) != 1672:
        raise ValueError(f"Unexpected story lookup row count: {len(lookup)}")

    by_asset = {}
    by_source = {}
    for entry in lookup:
        by_asset.setdefault(entry.get("asset"), []).append(entry)
        by_source.setdefault(entry.get("source", "").lower(), []).append(entry)
    for row in by_pack["story"]:
        if not any(entry.get("file") == row["file"] for entry in by_asset.get(row.get("asset"), [])):
            raise ValueError("Generated story WAV has no matching game lookup: " + row["file"])

    replacements = {}
    skipped = json.loads(args.skipped_curated.read_text(encoding="utf-8-sig"))
    if len(skipped) != EXPECTED["skipped"]:
        raise ValueError("Unexpected skipped identity count")
    for asset, record in skipped.items():
        if record.get("asset") != asset:
            raise ValueError("Skipped asset key mismatch")
        source = record.get("source", "").lower()
        hits = [entry for entry in by_source.get(source, [])
                if entry.get("package", "").lower() == record.get("package", "").lower()
                and entry.get("asset") == asset]
        if len(hits) != 1:
            raise ValueError(f"Skipped source lookup is not unique: {asset}")
        wav_name = asset + ".pl.wav"
        if hits[0].get("file") == wav_name:
            raise ValueError("Skipped entry unexpectedly already points to its delivery file")
        replacements[hits[0]["source"]] = wav_name

    vignette = json.loads(args.vignette_curated.read_text(encoding="utf-8-sig"))
    if len(vignette) != EXPECTED["vignette"]:
        raise ValueError("Unexpected vignette identity count")
    for asset in vignette:
        hits = [entry for entry in by_asset.get(asset, [])
                if entry.get("package", "").lower() == "sp_rt3_vignette_loc_int"]
        if len(hits) != 1:
            raise ValueError(f"Vignette source lookup is not unique: {asset}")
        replacements[hits[0]["source"]] = asset + ".pl.wav"
    if len(replacements) != EXPECTED["skipped"] + EXPECTED["vignette"]:
        raise ValueError("Ambiguous skipped/vignette lookup replacements")

    patched_lookup = []
    changed = 0
    for entry in lookup:
        entry = dict(entry)
        if entry.get("source") in replacements:
            entry["file"] = replacements[entry["source"]]
            changed += 1
        patched_lookup.append(entry)
    if changed != len(replacements):
        raise ValueError("Not all exact lookup rows were patched")

    payload_audio = candidate / "payload" / AUDIO_REL
    payload_lookup = candidate / "payload" / LOOKUP_REL
    payload_audio.mkdir(parents=True)
    payload_lookup.parent.mkdir(parents=True)
    file_plan = []
    names_seen = set()
    game_audio = Path(args.installed_lookup).resolve().parents[2] / "audio" / "polish" / "story"
    for pack in ("story", "skipped", "vignette"):
        for record in by_pack[pack]:
            name = record["file"]
            target_key = name.lower()
            if target_key in names_seen:
                raise ValueError("Cross-pack target filename collision: " + name)
            names_seen.add(target_key)
            src = source_root / pack / name
            if not src.is_file() or src.is_symlink():
                raise ValueError("Missing/reparse source WAV: " + str(src))
            wav_info = validate_wav(src)
            digest = sha(src)
            dest = payload_audio / name
            shutil.copyfile(src, dest)
            if sha(dest) != digest:
                raise ValueError("Staged WAV hash mismatch: " + name)
            target = game_audio / name
            file_plan.append({"pack": pack, "name": name, "relativePath": str(AUDIO_REL / name).replace("\\", "/"),
                              "beforeSha256": sha(target) if target.is_file() else None,
                              "candidateSha256": digest, **wav_info})

    lookup_out = "".join(json.dumps(entry, ensure_ascii=False, separators=(",", ":")) + "\n"
                         for entry in patched_lookup).encode("utf-8")
    payload_lookup.write_bytes(lookup_out)
    audio_manifest = {
        "schema": "singularityvr-xtts-audio-candidate-v1",
        "status": "OFFLINE_STAGED_STEAM_AND_PHYSICAL_AUDIO_PENDING",
        "sourceRoot": str(source_root),
        "productionManifestSha256": sha(args.production_manifest),
        "acceptedProfileSha256": sha(args.accepted_profile),
        "lookupBeforeSha256": original_lookup_sha,
        "lookupAfterSha256": hashlib.sha256(lookup_out).hexdigest(),
        "lookupRows": len(lookup),
        "lookupRowsUpdated": changed,
        "counts": EXPECTED,
        "files": file_plan,
    }
    manifest_path = candidate / "audio-payload-manifest.json"
    manifest_path.write_text(json.dumps(audio_manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"STAGED: {len(file_plan)} mono PCM16/24kHz WAVs; lookup rows updated={changed}")
    print("Lookup before:", original_lookup_sha)
    print("Lookup after: ", audio_manifest["lookupAfterSha256"])
    print("Payload manifest:", manifest_path)


if __name__ == "__main__":
    main()

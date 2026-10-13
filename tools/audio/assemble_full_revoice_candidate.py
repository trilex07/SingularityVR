"""Assemble a reversible full Polish voicepack and an indexed A/B listening reel.

The frozen full_revoice_intro candidate is read-only. The output is a new local
staging directory; generated audio and transcripts remain ignored by Git.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import sys
import wave
from pathlib import Path

sys.path.insert(0, r"C:\Users\Trilex\vr-mods\.pocket_tools")
import numpy as np
from scipy.io import wavfile
from scipy.signal import resample_poly

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_BASE = ROOT / "audio/polish/repair_candidate_20261013/full_revoice_intro"
DEFAULT_REPLACEMENTS = ROOT / "audio/polish/repair_candidate_20261013/full_revoice_final_pass/replacements"
DEFAULT_UNCHANGED_SMALL = ROOT / "audio/polish/repair_candidate_20261013/full_revoice_final_pass/unchanged_small_qa.json"
DEFAULT_OUT = ROOT / "audio/polish/repair_candidate_20261013/full_revoice_final_pass/assembled"


def jsonl(path: Path) -> list[dict]:
    return [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines() if line.strip()]


def write_jsonl(path: Path, rows: list[dict]) -> None:
    path.write_text("\n".join(json.dumps(row, ensure_ascii=False) for row in rows) + "\n",
                    encoding="utf-8")


def stage_file(source: Path, target: Path) -> None:
    target.parent.mkdir(parents=True, exist_ok=True)
    try:
        os.link(source, target)
    except OSError:
        shutil.copy2(source, target)


def wav_to_mono_24k(path: Path) -> np.ndarray:
    try:
        rate, data = wavfile.read(str(path))
    except (ValueError, OSError):
        # Some legacy voicepack rows have a .wav suffix but an MPEG audio
        # stream. Decode those read-only through PyAV for the review reel.
        import av
        container = av.open(str(path))
        stream = next(s for s in container.streams if s.type == "audio")
        resampler = av.AudioResampler(format="fltp", layout="mono", rate=24000)
        pieces: list[np.ndarray] = []
        for packet in container.demux(stream):
            for frame in packet.decode():
                for converted in resampler.resample(frame):
                    pieces.append(converted.to_ndarray().reshape(-1).astype(np.float32))
        for converted in resampler.resample(None):
            pieces.append(converted.to_ndarray().reshape(-1).astype(np.float32))
        container.close()
        if not pieces:
            raise ValueError(f"No decodable audio frames: {path}")
        return np.clip(np.concatenate(pieces), -1.0, 1.0)
    data = np.asarray(data)
    if data.ndim == 2:
        data = data.astype(np.float32).mean(axis=1)
    if np.issubdtype(data.dtype, np.integer):
        data = data.astype(np.float32) / max(abs(np.iinfo(data.dtype).min), np.iinfo(data.dtype).max)
    else:
        data = data.astype(np.float32)
    if rate != 24000:
        data = resample_poly(data, 24000, rate).astype(np.float32)
    return np.clip(data, -1.0, 1.0)


def write_review_reel(out: Path, review_rows: list[dict]) -> None:
    silence = np.zeros(round(24000 * 0.8), dtype=np.float32)
    blocks: list[np.ndarray] = []
    cursor = 0.0
    timeline: list[dict] = []
    for number, row in enumerate(review_rows, 1):
        source = Path(row["previous_audio"]) if row.get("previous_audio") else None
        candidate = Path(row["file"])
        pair_start = cursor
        if source and source.is_file():
            old = wav_to_mono_24k(source)
            blocks.append(old)
            old_start, old_end = cursor, cursor + len(old) / 24000
            cursor = old_end
            blocks.append(silence)
            cursor += len(silence) / 24000
        else:
            old_start = old_end = None
        new = wav_to_mono_24k(candidate)
        blocks.append(new)
        new_start, new_end = cursor, cursor + len(new) / 24000
        cursor = new_end
        blocks.append(silence)
        cursor += len(silence) / 24000
        timeline.append({
            "index": number,
            "line_id": row["line_id"],
            "profile": row.get("profile_before", ""),
            "old_audio": str(source) if source and source.is_file() else None,
            "candidate_audio": str(candidate),
            "old_start_s": round(old_start, 3) if old_start is not None else None,
            "old_end_s": round(old_end, 3) if old_end is not None else None,
            "candidate_start_s": round(new_start, 3),
            "candidate_end_s": round(new_end, 3),
            "source_english": row.get("source_english", ""),
            "polish": row.get("corrected_polish", ""),
            "base_flags": row.get("base_flags", []),
            "small_flags": row.get("small_flags", []),
            "disposition": row.get("disposition", ""),
            "pair_start_s": round(pair_start, 3),
        })
    audio = np.concatenate(blocks) if blocks else np.zeros(1, dtype=np.float32)
    wavfile.write(str(out / "review_ab_146.wav"), 24000,
                  np.asarray(np.rint(np.clip(audio, -1.0, 1.0) * 32767), dtype=np.int16))
    (out / "review_ab_146.json").write_text(json.dumps(timeline, ensure_ascii=False, indent=2), encoding="utf-8")
    lines = ["# Indexed Polish dubbing A/B review", "",
             "Each entry plays the previous recording (A), then the staged candidate (B).",
             "Audio is for focused human listening; automated ASR flags are included as cues only.", "",
             "| # | Time A → B | Profile | Line ID | Flags |", "|---:|---:|---|---|---|"]
    for item in timeline:
        at = "—" if item["old_start_s"] is None else f"{item['old_start_s']:.1f}–{item['candidate_end_s']:.1f}s"
        line_id = item["line_id"].replace("|", "¦")
        lines.append(f"| {item['index']} | {at} | {item['profile']} | `{line_id}` | base {','.join(item['base_flags']) or 'pass'}; small {','.join(item['small_flags']) or 'pass'} |")
    (out / "review_ab_146.md").write_text("\n".join(lines) + "\n", encoding="utf-8")

    priority = [row for row in review_rows if row.get("base_flags") or row.get("small_flags")]
    blocks: list[np.ndarray] = []
    cursor = 0.0
    priority_index: list[dict] = []
    for number, row in enumerate(priority, 1):
        audio = wav_to_mono_24k(Path(row["file"]))
        start, end = cursor, cursor + len(audio) / 24000
        blocks.extend((audio, silence))
        cursor = end + len(silence) / 24000
        priority_index.append({
            "index": number, "line_id": row["line_id"], "profile": row.get("profile_before", ""),
            "candidate_audio": row["file"], "start_s": round(start, 3), "end_s": round(end, 3),
            "base_flags": row.get("base_flags", []), "small_flags": row.get("small_flags", []),
            "source_english": row.get("source_english", ""), "polish": row.get("corrected_polish", ""),
        })
    priority_audio = np.concatenate(blocks) if blocks else np.zeros(1, dtype=np.float32)
    wavfile.write(str(out / "review_priority.wav"), 24000,
                  np.asarray(np.rint(np.clip(priority_audio, -1.0, 1.0) * 32767), dtype=np.int16))
    (out / "review_priority.json").write_text(json.dumps(priority_index, ensure_ascii=False, indent=2), encoding="utf-8")
    priority_lines = ["# Focused Polish dubbing listening set", "",
                      "Candidate clips only; includes rows where either ASR model still requests review.",
                      "ASR flags are listening cues, not a quality verdict.", "",
                      "| # | Time | Profile | Line ID | Base / small flags |", "|---:|---:|---|---|---|"]
    for item in priority_index:
        lid = item["line_id"].replace("|", "¦")
        flags = f"{','.join(item['base_flags']) or 'pass'} / {','.join(item['small_flags']) or 'pass'}"
        priority_lines.append(f"| {item['index']} | {item['start_s']:.1f}–{item['end_s']:.1f}s | {item['profile']} | `{lid}` | {flags} |")
    (out / "review_priority.md").write_text("\n".join(priority_lines) + "\n", encoding="utf-8")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--base", type=Path, default=DEFAULT_BASE)
    ap.add_argument("--replacements", type=Path, default=DEFAULT_REPLACEMENTS)
    ap.add_argument("--unchanged-small-qa", type=Path, default=DEFAULT_UNCHANGED_SMALL)
    ap.add_argument("--output-dir", type=Path, default=DEFAULT_OUT)
    args = ap.parse_args()
    base, replacements, output = args.base.resolve(), args.replacements.resolve(), args.output_dir.resolve()
    if output.exists() and any(output.iterdir()):
        raise SystemExit(f"Refusing to overwrite non-empty staging directory: {output}")
    output.mkdir(parents=True, exist_ok=True)
    pack_dir = output / "voicepack"
    reference_dir = pack_dir / "references"

    baseline = jsonl(base / "manifest.jsonl")
    base_qa = json.loads((base / "qa.json").read_text(encoding="utf-8"))
    replacement_rows = jsonl(replacements / "manifest.jsonl")
    replacement_qa = json.loads((replacements / "qa.json").read_text(encoding="utf-8"))
    replacement_small = json.loads((replacements / "qa_small.json").read_text(encoding="utf-8"))
    unchanged_small = json.loads(args.unchanged_small_qa.resolve().read_text(encoding="utf-8"))
    replace_by_id = {row["line_id"]: row for row in replacement_rows}
    qa_by_id = {row["line_id"]: row for row in replacement_qa}
    small_by_id = {row["line_id"]: row for row in replacement_small}
    base_qa_by_id = {row["line_id"]: row for row in base_qa}
    unchanged_small_by_id = {row["line_id"]: row for row in unchanged_small}

    reference_sources = {p.name: p for directory in (base / "references", replacements / "references")
                         if directory.is_dir() for p in directory.glob("*.wav")}
    for name, source in reference_sources.items():
        stage_file(source, reference_dir / name)

    final_manifest: list[dict] = []
    final_qa: list[dict] = []
    for old in baseline:
        chosen = dict(replace_by_id.get(old["line_id"], old))
        source = Path(chosen["file"])
        target = pack_dir / source.name
        stage_file(source, target)
        chosen["file"] = str(target)
        if chosen.get("reference_file"):
            reference_name = Path(chosen["reference_file"]).name
            staged_reference = reference_dir / reference_name
            if staged_reference.is_file():
                chosen["reference_file"] = str(staged_reference)
        final_manifest.append(chosen)
        qa = dict(qa_by_id.get(old["line_id"], base_qa_by_id[old["line_id"]]))
        qa["file"] = str(target)
        final_qa.append(qa)
    write_jsonl(pack_dir / "manifest.jsonl", final_manifest)
    (pack_dir / "qa_base.json").write_text(json.dumps(final_qa, ensure_ascii=False, indent=2), encoding="utf-8")

    original_review_ids = [row["line_id"] for row in base_qa if row.get("qa_flags")]
    dispositions: list[dict] = []
    for line_id in original_review_ids:
        old = base_qa_by_id[line_id]
        current = qa_by_id.get(line_id, old)
        small = small_by_id.get(line_id, unchanged_small_by_id.get(line_id, {}))
        changed = line_id in replace_by_id
        disposition = ("REGENERATED; ASR review remains" if current.get("qa_flags") or small.get("qa_flags")
                       else "REGENERATED; both automated screens pass") if changed else (
                       "RETAINED; no definite text/audio defect established from ASR alone")
        dispositions.append({
            "line_id": line_id,
            "source_english": current.get("source_english", old.get("source_english", "")),
            "polish": current.get("corrected_polish", old.get("corrected_polish", "")),
            "base_flags_before": old.get("qa_flags", []),
            "base_flags_after": current.get("qa_flags", []),
            "small_flags_after": small.get("qa_flags", []),
            "disposition": disposition,
        })
    (output / "review_dispositions_146.json").write_text(
        json.dumps(dispositions, ensure_ascii=False, indent=2), encoding="utf-8")

    review_rows: list[dict] = []
    for item in dispositions:
        line_id = item["line_id"]
        selected = dict(replace_by_id.get(line_id, next(r for r in baseline if r["line_id"] == line_id)))
        selected.update(base_flags=item["base_flags_after"], small_flags=item["small_flags_after"],
                        disposition=item["disposition"])
        review_rows.append(selected)
    write_review_reel(output, review_rows)

    errors: list[str] = []
    for row in final_manifest:
        path = Path(row["file"])
        if not path.is_file():
            errors.append(f"missing: {path}")
            continue
        try:
            with wave.open(str(path), "rb") as wav:
                if (wav.getframerate(), wav.getnchannels(), wav.getsampwidth(), wav.getnframes())[:3] != (24000, 1, 2) or wav.getnframes() <= 0:
                    errors.append(f"format: {path}")
        except (wave.Error, OSError) as exc:
            errors.append(f"unreadable: {path}: {exc}")
    if len(final_manifest) != 1886 or len({r["line_id"] for r in final_manifest}) != 1886:
        errors.append(f"manifest cardinality/uniqueness failure: {len(final_manifest)}")
    if len(original_review_ids) != 146 or len(dispositions) != 146:
        errors.append(f"review queue cardinality failure: {len(dispositions)}")
    if errors:
        raise SystemExit("Validation failed:\n" + "\n".join(errors[:20]))

    report = {
        "total_recordings": len(final_manifest),
        "replacements": len(replace_by_id),
        "original_review_queue": len(original_review_ids),
        "original_queue_base_pass": sum(not r["base_flags_after"] for r in dispositions),
        "original_queue_base_review": sum(bool(r["base_flags_after"]) for r in dispositions),
        "original_queue_small_pass": sum(not r["small_flags_after"] for r in dispositions),
        "original_queue_small_review": sum(bool(r["small_flags_after"]) for r in dispositions),
        "original_queue_both_pass": sum(not r["base_flags_after"] and not r["small_flags_after"] for r in dispositions),
        "priority_listening_count": sum(bool(r["base_flags_after"] or r["small_flags_after"]) for r in dispositions),
        "replacement_base_pass": sum(not r.get("qa_flags") for r in replacement_qa),
        "replacement_base_review": sum(bool(r.get("qa_flags")) for r in replacement_qa),
        "replacement_small_pass": sum(not r.get("qa_flags") for r in replacement_small),
        "replacement_small_review": sum(bool(r.get("qa_flags")) for r in replacement_small),
        "unchanged_small_pass": sum(not r.get("qa_flags") for r in unchanged_small),
        "unchanged_small_review": sum(bool(r.get("qa_flags")) for r in unchanged_small),
        "full_base_pass": sum(not r.get("qa_flags") for r in final_qa),
        "full_base_review": sum(bool(r.get("qa_flags")) for r in final_qa),
        "all_wavs": "PASS: 24 kHz mono PCM16 WAV, readable, non-empty",
        "frozen_base": str(base),
        "output": str(output),
        "review_audio": str(output / "review_ab_146.wav"),
        "priority_review_audio": str(output / "review_priority.wav"),
    }
    (output / "ASSEMBLY_REPORT.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps(report, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()

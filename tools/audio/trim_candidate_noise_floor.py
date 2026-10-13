"""Remove only long sub-threshold model pre-roll from staged candidate WAVs.

Reads qa.json and modifies only candidate files flagged TECHNICAL for leading
silence. Original voice packs are never touched. A 5 ms fade protects the new
edges; audio content above the speech threshold is preserved bit-for-bit.
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, r"C:\Users\Trilex\vr-mods\.pocket_tools")
import numpy as np
from scipy.io import wavfile


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("candidate_dir", type=Path)
    ap.add_argument("--threshold", type=float, default=0.002)
    ap.add_argument("--preroll-ms", type=float, default=5.0)
    args = ap.parse_args()
    root = args.candidate_dir.resolve()
    qa_path = root / "qa.json"
    manifest_path = root / "manifest.jsonl"
    qa = json.loads(qa_path.read_text(encoding="utf-8"))
    manifest = [json.loads(line) for line in manifest_path.read_text(encoding="utf-8").splitlines()
                if line.strip()]
    by_id = {row["line_id"]: row for row in manifest}
    fixed: list[str] = []
    for row in qa:
        if "TECHNICAL" not in row.get("qa_flags", []) or row.get("qa_leading_silence_s", 0) <= 0.12:
            continue
        item = by_id[row["line_id"]]
        path = Path(item["file"])
        rate, data = wavfile.read(str(path))
        if data.ndim != 1 or data.dtype != np.int16 or rate != 24000:
            continue
        audio = data.astype(np.float32) / 32768.0
        active = np.flatnonzero(np.abs(audio) >= args.threshold)
        if not active.size:
            continue
        start = max(0, int(active[0] - round(args.preroll_ms * rate / 1000.0)))
        if start == 0:
            continue
        trimmed = audio[start:].copy()
        fade = min(round(0.005 * rate), len(trimmed) // 2)
        if fade:
            trimmed[:fade] *= np.linspace(0.0, 1.0, fade, dtype=np.float32)
        wavfile.write(str(path), rate,
                      np.asarray(np.rint(np.clip(trimmed, -1.0, 1.0) * 32767.0), dtype=np.int16))
        item["generated_duration_s"] = round(len(trimmed) / rate, 3)
        fixed.append(row["line_id"])
    manifest_path.write_text("\n".join(json.dumps(row, ensure_ascii=False) for row in manifest) + "\n",
                             encoding="utf-8")
    ids_path = root / "trimmed_line_ids.json"
    ids_path.write_text(json.dumps(fixed, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps({"trimmed": len(fixed), "ids_file": str(ids_path)}, ensure_ascii=False))


if __name__ == "__main__":
    main()

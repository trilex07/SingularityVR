"""Select the strongest automated transcript candidate per line across repeats."""
from __future__ import annotations

import argparse
import json
import shutil
import sys
from pathlib import Path

sys.path.insert(0, r"C:\Users\Trilex\vr-mods\.pocket_tools")
import numpy as np
from scipy.io import wavfile

ROOT = Path(__file__).resolve().parents[2]
REPAIR = ROOT / "audio/polish/repair_candidate_20261013"


def load_json(path: Path):
    return json.loads(path.read_text(encoding="utf-8"))


def score(row: dict) -> tuple:
    flags = row.get("qa_flags", [])
    # Technical defects are always disqualifying; then favor no flags, content
    # similarity, and word coverage. This is selection for listening, not a
    # perceptual-quality claim.
    return ("TECHNICAL" in flags, len(flags), -float(row.get("qa_similarity", -1)),
            -float(row.get("qa_heard_words", 0)) / max(1, int(row.get("qa_expected_words", 1))))


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--output-dir", type=Path, default=REPAIR / "full_audit_pocket_selected")
    ap.add_argument("--candidate-dir", type=Path, action="append", default=[],
                    help="candidate directory; repeat for multiple generation variants")
    args = ap.parse_args()
    candidate_dirs = [p.resolve() for p in args.candidate_dir] or [
        REPAIR / "full_audit_pocket", REPAIR / "full_audit_pocket_v2", REPAIR / "full_audit_pocket_v3"]
    out = args.output_dir.resolve()
    out.mkdir(parents=True, exist_ok=True)
    sources = []
    for d in candidate_dirs:
        manifest = {x["line_id"]: x for x in
                    (json.loads(s) for s in (d / "manifest.jsonl").read_text(encoding="utf-8").splitlines() if s.strip())}
        qa_file = d / ("qa_base.json" if (d / "qa_base.json").is_file() else "qa.json")
        qa = {x["line_id"]: x for x in load_json(qa_file)}
        if len(manifest) != 205 or set(manifest) != set(qa):
            raise SystemExit(f"Candidate/QA mismatch in {d}")
        sources.append((d, manifest, qa))
    ids = set(sources[0][1])
    if any(set(m) != ids for _, m, _ in sources):
        raise SystemExit("Candidate variants have different line IDs")

    chosen_manifest = []
    chosen_qa = []
    for line_id in sources[0][1]:
        records = [m[line_id] for _, m, _ in sources]
        for row in records[1:]:
            for field in ("corrected_polish", "tts_text", "profile_before", "pack"):
                if row.get(field) != records[0].get(field):
                    raise SystemExit(f"Candidate text/profile mismatch for {line_id}: {field}")
        option = min(range(len(sources)), key=lambda i: score(sources[i][2][line_id]))
        source_dir, manifest, qa = sources[option]
        source = manifest[line_id]
        source_wav = Path(source["file"])
        rate, audio = wavfile.read(source_wav)
        if rate != 24000 or audio.dtype != np.int16 or audio.ndim != 1:
            raise SystemExit(f"Invalid selected WAV: {source_wav}")
        dest = out / source_wav.name
        shutil.copy2(source_wav, dest)
        selected = {**source, "file": str(dest), "selected_variant": source_dir.name,
                    "selection_method": "best automated QA among same-profile Pocket-TTS repeats"}
        chosen_manifest.append(selected)
        chosen_qa.append({**qa[line_id], "file": str(dest), "selected_variant": source_dir.name})

    (out / "manifest.jsonl").write_text(
        "\n".join(json.dumps(x, ensure_ascii=False) for x in chosen_manifest) + "\n", encoding="utf-8")
    (out / "qa.json").write_text(json.dumps(chosen_qa, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps({"rows": len(chosen_manifest),
                      "pass": sum(not x["qa_flags"] for x in chosen_qa),
                      "review": sum(bool(x["qa_flags"]) for x in chosen_qa),
                      "selected_variants": {name: sum(x["selected_variant"] == name for x in chosen_manifest)
                                             for name in sorted({x["selected_variant"] for x in chosen_manifest})},
                      "output": str(out)}, ensure_ascii=False))


if __name__ == "__main__":
    main()

"""Generate a small Polish TTS batch from a JSONL script.

The output is a listening/reference batch (MP3), not yet a game package. The
script keeps source asset IDs so a later runtime redirect can replace the
matching SoundNodeWave without changing the original files.
"""
from __future__ import annotations

import argparse
import asyncio
import json
from pathlib import Path

import edge_tts


async def generate(script: Path, out_dir: Path) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)
    for raw in script.read_text(encoding="utf-8").splitlines():
        if not raw.strip():
            continue
        row = json.loads(raw)
        out = out_dir / f"{row['id']}.mp3"
        if out.exists() and out.stat().st_size > 0:
            continue
        communicate = edge_tts.Communicate(
            row["pl"],
            row["voice"],
            rate=row.get("rate", "+0%"),
            pitch=row.get("pitch", "+0Hz"),
            volume=row.get("volume", "+0%"),
        )
        await communicate.save(str(out))
        print(f"generated {out}")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("script", type=Path)
    ap.add_argument("out", type=Path)
    args = ap.parse_args()
    asyncio.run(generate(args.script, args.out))


if __name__ == "__main__":
    main()

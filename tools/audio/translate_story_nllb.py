"""Translate all story ASR lines with the local NLLB EN->PL model."""
from __future__ import annotations

import json
import sys
from pathlib import Path

sys.path.insert(0, r"C:\Users\Trilex\vr-mods\.pocket_tools")
sys.path.insert(0, r"C:\Users\Trilex\vr-mods\.translate_tools")

import torch  # noqa: E402
from transformers import AutoModelForSeq2SeqLM, AutoTokenizer  # noqa: E402

ROOT = Path(r"C:\Users\Trilex\vr-mods\SingularityVR-cutscene")
SOURCE = ROOT / "audio" / "story_en_reference" / "story_transcripts.jsonl"
OUT = ROOT / "audio" / "story_en_reference" / "story_transcripts_pl_nllb.jsonl"
MODEL_NAME = "facebook/nllb-200-distilled-600M"


def main() -> None:
    rows = [json.loads(x) for x in SOURCE.read_text(encoding="utf-8").splitlines() if x.strip() and json.loads(x).get("text_en")]
    existing = {}
    if OUT.exists():
        for line in OUT.read_text(encoding="utf-8").splitlines():
            if line.strip():
                r = json.loads(line)
                existing[r["path"]] = r
    tokenizer = AutoTokenizer.from_pretrained(MODEL_NAME)
    model = AutoModelForSeq2SeqLM.from_pretrained(MODEL_NAME)
    tokenizer.src_lang = "eng_Latn"
    forced = tokenizer.convert_tokens_to_ids("pol_Latn")
    pending = [r for r in rows if r["path"] not in existing]
    batch_size = 16
    with OUT.open("a", encoding="utf-8") as f:
        for start in range(0, len(pending), batch_size):
            batch = pending[start : start + batch_size]
            inputs = tokenizer(
                [r["text_en"] for r in batch],
                return_tensors="pt",
                padding=True,
                truncation=True,
                max_length=384,
            )
            with torch.no_grad():
                generated = model.generate(
                    **inputs,
                    forced_bos_token_id=forced,
                    max_length=384,
                    num_beams=2,
                    early_stopping=True,
                )
            translated = tokenizer.batch_decode(generated, skip_special_tokens=True)
            for row, text in zip(batch, translated):
                out = dict(row)
                out["text_pl_machine"] = text.strip()
                out["translation_model"] = MODEL_NAME
                out["translation_status"] = "machine_draft"
                f.write(json.dumps(out, ensure_ascii=False) + "\n")
            f.flush()
            print(f"translated {min(start + len(batch), len(pending))}/{len(pending)}", flush=True)
    print("complete", flush=True)


if __name__ == "__main__":
    main()

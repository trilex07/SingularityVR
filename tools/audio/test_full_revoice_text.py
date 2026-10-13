"""Fast offline regression checks for Polish TTS input preparation."""
from __future__ import annotations

import importlib.util
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools/audio/generate_intro_narrator_full_candidate.py"
spec = importlib.util.spec_from_file_location("full_revoice_generator", SCRIPT)
if spec is None or spec.loader is None:
    raise RuntimeError(f"Cannot load {SCRIPT}")
generator = importlib.util.module_from_spec(spec)
spec.loader.exec_module(generator)


def main() -> None:
    assert generator.clean_localization_text("Naciśnij [crouch], aby kucnąć.") == \
        "Naciśnij przycisk kucania, aby kucnąć."
    assert generator.clean_localization_text("Naciśnij [impulse], aby otworzyć drzwi.") == \
        "Naciśnij przycisk Impulsu, aby otworzyć drzwi."
    assert generator.clean_localization_text("Użyj [impulse] na skałach.") == \
        "Użyj impulsu na skałach."
    assert generator.clean_localization_text("Przytrzymaj [grab], aby podnieść przedmiot.") == \
        "Przytrzymaj przycisk chwytu, aby podnieść przedmiot."
    assert generator.spoken_pieces("Pierwszy wiersz\n\nDrugi wiersz.") == \
        ["Pierwszy wiersz", "Drugi wiersz."]
    assert generator.spoken_pieces("Jedno zdanie. Drugie zdanie.") == \
        ["Jedno zdanie.", "Drugie zdanie."]
    print("PASS: Polish action aliases and multi-line utterance segmentation")


if __name__ == "__main__":
    main()

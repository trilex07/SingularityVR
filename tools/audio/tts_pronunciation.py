"""Speech-only pronunciation normalization for Polish dubbing synthesis.

This intentionally changes TTS input only. Display subtitles and localization
strings retain their normal spelling.
"""
from __future__ import annotations

import re


_REPLACEMENTS: tuple[tuple[re.Pattern[str], str], ...] = (
    (re.compile(r"\bkatorga[- ]12\b", re.IGNORECASE), "Katorga dwanaście"),
    (re.compile(r"\bsnv[- ]e\s*99\b", re.IGNORECASE), "es en we, E dziewięćdziesiąt dziewięć"),
    (re.compile(r"\be\s*99\b", re.IGNORECASE), "E dziewięćdziesiąt dziewięć"),
    (re.compile(r"\btmd\b", re.IGNORECASE), "te em de"),
    (re.compile(r"\bvolk\s+s4\b", re.IGNORECASE), "Wołk es cztery"),
    (re.compile(r"\bkasimov\b", re.IGNORECASE), "Kasimow"),
    (re.compile(r"\brocket\s+launchers\b", re.IGNORECASE), "roket lanczers"),
    (re.compile(r"\brocket\s+launcher\b", re.IGNORECASE), "roket lanczer"),
    (re.compile(r"\bxbox\s+live\b", re.IGNORECASE), "iksboks lajw"),
    (re.compile(r"\bxbox\b", re.IGNORECASE), "iksboks"),
    (re.compile(r"\btitan\b", re.IGNORECASE), "tajtan"),
    (re.compile(r"\bonline\b", re.IGNORECASE), "onlajn"),
    (re.compile(r"\boffline\b", re.IGNORECASE), "oflajn"),
)


def to_spoken_polish(text: str) -> str:
    """Convert known English game terms to pronunciation-friendly Polish."""
    result = text
    for pattern, replacement in _REPLACEMENTS:
        result = pattern.sub(replacement, result)
    return result


if __name__ == "__main__":
    assert to_spoken_polish("Rocket Launcher jest dostępny online.") == \
        "roket lanczer jest dostępny onlajn."
    assert to_spoken_polish("Rocket Launchers offline") == "roket lanczers oflajn"
    assert to_spoken_polish("Xbox LIVE i Titan 1") == "iksboks lajw i tajtan 1"
    assert to_spoken_polish("Katorga-12, E99 i TMD") == \
        "Katorga dwanaście, E dziewięćdziesiąt dziewięć i te em de"
    assert to_spoken_polish("Volk S4, Kasimov SNV-E99") == \
        "Wołk es cztery, Kasimow es en we, E dziewięćdziesiąt dziewięć"
    print("PASS: Polish TTS pronunciation rules")

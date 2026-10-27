"""Generate the complete player-facing Polish TTS working set.

This creates a reviewable WAV voice pack from the current Polish localization
draft. It does not modify Coalesced_INT.bin or game packages. Each file keeps
the original localization id in manifest.jsonl so a later runtime redirect can
bind it to the matching UI/subtitle cue.
"""
from __future__ import annotations

import hashlib
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
from pocket_tts import TTSModel
from scipy.io import wavfile
import numpy as np

ROOT = Path(r'C:\Users\Trilex\vr-mods')
DRAFT = ROOT / 'SingularityVR-localization-local' / 'polish' / 'full_draft.jsonl'
AUDIO = ROOT / 'SingularityVR-cutscene' / 'audio'
OUT = AUDIO / 'polish' / 'full_voicepack'
PROFILE_ROOT = AUDIO / 'polish' / 'user_reference'
ACCENT_ROOT = AUDIO / 'polish' / 'accent_reference'
INTRO_REFERENCE = AUDIO / 'polish' / 'repair_candidate_20261013' / 'intro_reference_experiment' / 'intro_narrator_reference.wav'
NARRATOR_REFERENCE = INTRO_REFERENCE if INTRO_REFERENCE.is_file() else PROFILE_ROOT / 'spokoj_reference.wav'

ALLOWED_SECTIONS = {
    'Subtitles', 'Tutorial', 'GameDialogs', 'TMD', 'TMDHUD', 'TMDUpgrade',
    'ActionVolume', 'Pickup', 'WeaponUpgrade', 'Perk', 'RvUI', 'RvUI3D',
    'TCR', 'CommonText', 'General', 'Menu', 'HUD', 'Station', 'Signs',
    'Progress', 'Achievements', 'GameMessage', 'RvGameTypes',
    'RvMPAnnouncerMessage', 'MPGame', 'MPLobby', 'MPPreGame', 'MPLoadout',
    'GameOver', 'Scoreboard', 'CVSScoreboard', 'RICH PRESENCE',
}

TOKEN_WORDS = {
    'use': 'użycia', 'fire': 'strzału', 'grab': 'chwytania', 'age': 'TMD',
    'impulse': 'impulsu', 'flashlight': 'latarki', 'aim': 'celowania',
    'jump': 'skoku', 'crouch': 'kucania', 'dash': 'biegu',
    'reload': 'przeładowania', 'switchwpn': 'zmiany broni',
    'health': 'zdrowia',
}


def clean_for_tts(text: str) -> str:
    text = re.sub(r'<[^>]+>', ' ', text)
    text = re.sub(r'\[([^\]]+)\]', lambda m: TOKEN_WORDS.get(m.group(1).lower(), m.group(1)), text)
    text = text.replace('`~', '').replace('\\n', ' ')
    text = re.sub(r'\s+', ' ', text).strip()
    return text


def choose_profile(row: dict) -> tuple[str, Path]:
    blob = f"{row.get('key','')} {row.get('en','')} {row.get('pl','')}".lower()
    if 'demichev' in blob or ' dem ' in blob:
        return 'demichev_ruslan', ACCENT_ROOT / 'ru_ruslan_reference.wav'
    if 'barisov' in blob or ' bar ' in blob:
        return 'barisov_dmitri', ACCENT_ROOT / 'ru_dmitri_reference.wav'
    if 'kathryn' in blob or ' kat ' in blob:
        return 'kathryn_tetiana', ACCENT_ROOT / 'uk_tetiana_reference.wav'
    if 'devlin' in blob:
        return 'devlin_mykyta', ACCENT_ROOT / 'uk_mykyta_reference.wav'
    return 'narrator_user', NARRATOR_REFERENCE


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    manifest_path = OUT / 'manifest.jsonl'
    done = set()
    if manifest_path.exists():
        for line in manifest_path.read_text(encoding='utf-8').splitlines():
            if line.strip():
                done.add(json.loads(line)['id'])

    rows = []
    for line in DRAFT.read_text(encoding='utf-8').splitlines():
        row = json.loads(line)
        if row.get('section') not in ALLOWED_SECTIONS:
            continue
        if row.get('status') in {'skip', 'empty'}:
            continue
        text = clean_for_tts(row.get('pl', ''))
        if not text or row['id'] in done:
            continue
        row['_tts_text'] = text
        rows.append(row)

    model = TTSModel.load_model(
        config='hf://shefowl/pocket-tts-polish-6l/config.yaml',
        temp=0.45,
        quantize=True,
    )
    refs: dict[str, object] = {}
    for name, ref in {
        'narrator_user': NARRATOR_REFERENCE,
        'barisov_dmitri': ACCENT_ROOT / 'ru_dmitri_reference.wav',
        'demichev_ruslan': ACCENT_ROOT / 'ru_ruslan_reference.wav',
        'kathryn_tetiana': ACCENT_ROOT / 'uk_tetiana_reference.wav',
        'devlin_mykyta': ACCENT_ROOT / 'uk_mykyta_reference.wav',
    }.items():
        refs[name] = model.get_state_for_audio_prompt(str(ref))

    with manifest_path.open('a', encoding='utf-8') as manifest:
        for count, row in enumerate(rows, 1):
            profile, _ = choose_profile(row)
            stem = f"{count:04d}_{hashlib.sha1(row['id'].encode('utf-8')).hexdigest()[:12]}"
            path = OUT / f'{stem}.wav'
            try:
                audio = model.generate_audio(refs[profile], row['_tts_text']).numpy().squeeze().astype(np.float32)
                peak = float(np.max(np.abs(audio)))
                if peak > 0:
                    audio *= (10 ** (-3 / 20)) / peak
                wavfile.write(str(path), model.sample_rate, (audio * 32767.0).astype(np.int16))
                manifest.write(json.dumps({
                    'id': row['id'], 'record': row.get('record'),
                    'section': row.get('section'), 'key': row.get('key'),
                    'category': row.get('category'), 'en': row.get('en'),
                    'pl': row.get('pl'), 'tts_text': row['_tts_text'],
                    'profile': profile, 'file': path.name,
                    'sample_rate': model.sample_rate, 'peak_dbfs': -3.0,
                }, ensure_ascii=False) + '\n')
                manifest.flush()
                if count == 1 or count % 25 == 0:
                    print(f'generated {count}/{len(rows)}', flush=True)
            except Exception as exc:
                print(f'ERROR {row["id"]}: {exc}', flush=True)
    print('complete', flush=True)


if __name__ == '__main__':
    main()

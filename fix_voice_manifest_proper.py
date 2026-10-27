import json
import os

# Load the original voice manifest (source of truth for structure)
voice_entries = []
with open('C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish/full_voicepack/manifest.jsonl', 'r', encoding='utf-8') as f:
    for line in f:
        if line.strip():
            d = json.loads(line)
            voice_entries.append(d)

print(f"Original manifest has {len(voice_entries)} entries")

# Load full_draft for Polish text
entry_map = {}
with open('C:/Users/Trilex/vr-mods/SingularityVR-polish-integration/localization/pl/full_draft.jsonl', 'r', encoding='utf-8') as f:
    for line in f:
        d = json.loads(line)
        entry_map[d.get('id')] = d.get('pl', '')

# Fix TTS text for each entry
fixed = 0
for v in voice_entries:
    vid = v.get('id', '')
    pl = v.get('pl', '')
    tts = v.get('tts_text', v.get('pl', ''))
    original_tts = tts
    
    # Fix TTS text - remove brackets, use natural Polish
    new_tts = tts
    
    # Replace bracket placeholders with natural Polish
    replacements = {
        "[age]": "klucz wiek",
        "[grab]": "klucz chwyt",
        "[impulse]": "klucz impuls",
        "[fire]": "klucz ogień",
        "[jump]": "klucz skok",
        "[switchwpn]": "klucz zmień broń",
        "[use]": "klucz użyj",
        "[flashlight]": "klucz latarka",
        "[aim]": "klucz celuj",
        "[dash]": "klucz sprint",
        "[crouch]": "klucz kucaj",
        "[reload]": "klucz przeładuj",
        "[switch]": "klucz zmień",
        "[use]": "klucz użyj",
    }
    
    for bracket, phonetic in replacements.items():
        new_tts = new_tts.replace(bracket, phonetic)
    
    # Fix common mispronunciations
    new_tts = new_tts.replace("kriogenicznych", "kryogenicznych")
    new_tts = new_tts.replace("kliogenicznych", "kryogenicznych")
    new_tts = new_tts.replace("kwiogenicznych", "kryogenicznych")
    new_tts = new_tts.replace("krątsz chwyt", "klucz chwyt")
    new_tts = new_tts.replace("klucz chwyt", "klucz chwyt")
    
    # Fix E99 pronunciation
    new_tts = new_tts.replace("E99", "e dziewięćdziesiąt dziewięć")
    new_tts = new_tts.replace("E 99", "e dziewięćdziesiąt dziewięć")
    new_tts = new_tts.replace("E 99", "e dziewięćdziesiąt dziewięć")
    
    # Fix deadlock
    new_tts = new_tts.replace("deadlock", "dedlok")
    new_tts = new_tts.replace("dedlok", "ded lok")
    
    # Fix Cryo
    new_tts = new_tts.replace("kriogenicznych", "kryogenicznych")
    new_tts = new_tts.replace("kliogenicznych", "kriogenicznych")
    new_tts = new_tts.replace("kwiogenicznych", "kriogenicznych")
    
    # Fix klucz chwyt pronunciation
    new_tts = new_tts.replace("krątsz chwyt", "klucz chwyt")
    new_tts = new_tts.replace("krątsz", "klucz")
    
    # Fix E99 pronunciation
    new_tts = new_tts.replace("E99", "e dziewięćdziesiąt dziewięć")
    new_tts = new_tts.replace("E 99", "e dziewięćdziesiąt dziewięć")
    new_tts = new_tts.replace("E 99", "e dziewięćdziesiąt dziewięć")
    
    # Fix dedlok pronunciation
    new_tts = new_tts.replace("dedlok", "ded lok")
    
    v['tts_text'] = new_tts
    fixed += 1

print(f"Fixed {len(voice_entries)} entries")

# Save voice manifest
with open('C:/Users/Trilex/vr-mods/SingularityVR-polish-integration/localization/pl/voice_manifest.jsonl', 'w', encoding='utf-8') as f:
    for v in voice_entries:
        f.write(json.dumps(v, ensure_ascii=False) + '\n')

print("Updated voice_manifest.jsonl")

# Also update full_draft.jsonl with corrected PL text
entry_map = {}
with open('C:/Users/Trilex/vr-mods/SingularityVR-polish-integration/localization/pl/full_draft.jsonl', 'r', encoding='utf-8') as f:
    for line in f:
        d = json.loads(line)
        entry_map[d.get('id')] = d.get('pl', '')

# Update full_draft with corrected PL text for voice entries
for v in voice_entries:
    vid = v.get('id', '')
    if vid in entry_map:
        entry_map[vid] = v['pl']

with open('localization/pl/full_draft.jsonl', 'w', encoding='utf-8') as f:
    for vid, pl_text in entry_map.items():
        f.write(json.dumps({'id': vid, 'pl': pl_text}, ensure_ascii=False) + '\n')

print("Updated full_draft.jsonl")
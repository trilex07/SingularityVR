import json
import os

# Load the corrected full_draft.jsonl
entry_map = {}
with open('C:/Users/Trilex/vr-mods/SingularityVR-polish-integration/localization/pl/full_draft.jsonl', 'r', encoding='utf-8') as f:
    for line in f:
        d = json.loads(line)
        entry_map[d.get('id')] = d.get('pl', '')

# Get all WAV files
audio_dir = 'C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish/full_voicepack'
wav_files = [f for f in os.listdir(audio_dir) if f.endswith('.wav')]

# Load existing voice manifest to get metadata
voice_entries = []
with open('C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish/full_voicepack/manifest.jsonl', 'r', encoding='utf-8') as f:
    for line in f:
        if line.strip():
            d = json.loads(line)
            voice_entries.append(d)

# Build lookup from existing manifest
existing_manifest = {v['file']: v for v in voice_entries}

# Load full_draft for Polish text
entry_map = {}
with open('C:/Users/Trilex/vr-mods/SingularityVR-polish-integration/localization/pl/full_draft.jsonl', 'r', encoding='utf-8') as f:
    for line in f:
        d = json.loads(line)
        entry_map[d.get('id')] = d.get('pl', '')

# Create new voice manifest
new_voice_entries = []
for wav_file in sorted(wav_files):
    # Try to find matching entry in existing manifest
    existing = existing_manifest.get(wav_file, {})
    
    # Extract ID from filename or use existing
    vid = existing.get('id', '')
    if not vid:
        # Try to construct ID from filename
        vid = wav_file.replace('.wav', '')
    
    # Get Polish text from full_draft
    pl_text = ''
    # Try to find matching entry in full_draft
    for vid_key, pl_text in entry_map.items():
        if wav_file.replace('.wav', '') in vid_key or vid in vid_key:
            break
    
    # If no match, use a default
    if not pl_text:
        pl_text = existing.get('pl', '')
    
    # TTS text - remove brackets, fix pronunciation
    tts_text = existing.get('tts_text', pl_text)
    
    # Fix TTS text - remove brackets, use natural Polish
    tts_text = tts_text.replace('[age]', 'klucz wiek')
    tts_text = tts_text.replace('[grab]', 'klucz chwyt')
    tts_text = tts_text.replace('[impulse]', 'klucz impuls')
    new_tts = tts_text.replace('[fire]', 'klucz ogień')
    new_tts = new_tts.replace('[jump]', 'klucz skok')
    new_tts = new_tts.replace('[switchwpn]', 'klucz zmień broń')
    new_tts = new_tts.replace('[use]', 'klucz użyj')
    new_tts = new_tts.replace('[flashlight]', 'klucz latarka')
    new_tts = new_tts.replace('[aim]', 'klucz celuj')
    new_tts = new_tts.replace('[dash]', 'klucz sprint')
    new_tts = new_tts.replace('[crouch]', 'klucz kucaj')
    new_tts = new_tts.replace('[reload]', 'klucz przeładuj')
    new_tts = new_tts.replace('[switch]', 'klucz zmień')
    new_tts = new_tts.replace('[use]', 'klucz użyj')
    
    # Fix common mispronunciations
    new_tts = new_tts.replace('kriogenicznych', 'kriogenicznych')
    new_tts = new_tts.replace('kliogenicznych', 'kriogenicznych')
    new_tts = new_tts.replace('kwiogenicznych', 'kriogenicznych')
    new_tts = new_tts.replace('krątsz chwyt', 'klucz chwyt')
    new_tts = new_tts.replace('klucz chwyt', 'klucz chwyt')
    
    # Fix E99 pronunciation
    new_tts = new_tts.replace('E99', 'e dziewięćdziesiąt dziewięć')
    new_tts = new_tts.replace('E 99', 'e dziewięćdziesiąt dziewięć')
    new_tts = new_tts.replace('E 99', 'e dziewięćdziesiąt dziewięć')
    
    # Fix deadlock
    new_tts = new_tts.replace('deadlock', 'dedlok')
    new_tts = new_tts.replace('dedlok', 'ded lok')
    
    # Fix Cryo
    new_tts = new_tts.replace('kriogenicznych', 'kriogenicznych')
    new_tts = new_tts.replace('kliogenicznych', 'kriogenicznych')
    new_tts = new_tts.replace('kwiogenicznych', 'kriogenicznych')
    
    # Fix klucz chwyt pronunciation
    new_tts = new_tts.replace('krątsz chwyt', 'klucz chwyt')
    new_tts = new_tts.replace('krątsz', 'klucz')
    
    v = {
        'id': wav_file.replace('.wav', ''),
        'file': wav_file,
        'pl': pl_text,
        'tts_text': new_tts,
        'profile': 'narrator_user',
        'category': 'ui',
        'sample_rate': 24000,
        'peak_dbfs': -3.0
    }
    
    # Try to get profile from existing
    if 'profile' in existing:
        v['profile'] = existing['profile']
    if 'category' in existing:
        v['category'] = existing['category']
    
    voice_entries.append(v)

# Save voice manifest
with open('C:/Users/Trilex/vr-mods/SingularityVR-polish-integration/localization/pl/voice_manifest.jsonl', 'w', encoding='utf-8') as f:
    for v in voice_entries:
        f.write(json.dumps(v, ensure_ascii=False) + '\n')

print(f'Created voice_manifest.jsonl with {len(voice_entries)} entries')
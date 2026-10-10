import json

# Load the corrected full_draft.jsonl
entry_map = {}
with open('localization/pl/full_draft.jsonl', 'r', encoding='utf-8') as f:
    for line in f:
        d = json.loads(line)
        entry_map[d.get('id')] = d.get('pl', '')

# Load the voice manifest
voice_entries = []
with open('localization/pl/voice_manifest.jsonl', 'rb') as f:
    content = f.read()
    if content.startswith(b'\xff\xfe'):
        content = content[2:].decode('utf-16')
    elif content.startswith(b'\xfe\xff'):
        content = content[2:].decode('utf-16-be')
    elif content.startswith(b'\xef\xbb\xbf'):
        content = content[3:].decode('utf-8')
    else:
        content = content.decode('utf-8')
    lines = content.splitlines()

# Sync TTS text with corrected PL text from full_draft
for v in voice_entries:
    vid = v.get('id')
    if vid in entry_map:
        v['pl'] = entry_map[vid]
        v['tts_text'] = entry_map[vid]

# Save voice manifest
with open('localization/pl/voice_manifest.jsonl', 'w', encoding='utf-8') as f:
    for v in voice_entries:
        f.write(json.dumps(v, ensure_ascii=False) + '\n')

print('Synced voice_manifest.jsonl with corrected PL text')
import json
import os

# Load existing NLLB story voice manifest (has Polish translations)
nllb_entries = []
with open('C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish/story_voicepack_nllb/manifest.jsonl', 'r', encoding='utf-8') as f:
    for line in f:
        if line.strip():
            nllb_entries.append(json.loads(line))

# Create story manifest with NLLB Polish translations
story_entries = []
for v in nllb_entries:
    vid = v.get('path', '')
    if vid:
        vid = vid.replace('/', '|')
        if not vid.startswith('..\\'):
            vid = '..\\RvGame\\Localization\\INT\\' + vid
    
    pl_text = v.get('text_pl_machine', v.get('text_en', ''))
    
    entry = {
        'source': v.get('path', ''),
        'package': v.get('package', ''),
        'asset': v.get('asset', ''),
        'speaker_guess': v.get('speaker_guess', ''),
        'profile': v.get('profile', 'narrator_user'),
        'text_en': v.get('text_en', ''),
        'text_pl': pl_text,
        'translation_status': 'machine_draft',
        'source_duration_s': v.get('duration', 0),
        'generated_duration_s': v.get('duration', 0),
        'file': v.get('file', ''),
        'sample_rate': v.get('sample_rate', 24000),
        'peak_dbfs': -3.0
    }
    story_entries.append(entry)

# Save story manifest
output_path = 'C:/Users/Trilex/vr-mods/SingularityVR-polish-integration/localization/pl/polish_story_voice.jsonl'
with open(output_path, 'w', encoding='utf-8') as f:
    for entry in story_entries:
        f.write(json.dumps(entry, ensure_ascii=False) + '\n')

print(f'Created story manifest with {len(story_entries)} entries at {output_path}')
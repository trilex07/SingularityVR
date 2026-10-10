import json
import os

# Load existing story voice manifest
source_entries = []
with open('C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish/story_voicepack/manifest.jsonl', 'r', encoding='utf-8') as f:
    for line in f:
        if line.strip():
            source_entries.append(json.loads(line))

# Load full_draft for Polish translations
entry_map = {}
with open('C:/Users/Trilex/vr-mods/SingularityVR-polish-integration/localization/pl/full_draft.jsonl', 'r', encoding='utf-8') as f:
    for line in f:
        d = json.loads(line)
        entry_map[d.get('id')] = d.get('pl', '')

# Create story manifest with corrected Polish text
story_entries = []
for v in source_entries:
    vid = v.get('source', '')
    # Convert source path to id format
    # "SP_BT_A_zone01_LOC_INT/SoundNodeWave/RL6_9006_BAR.ogg" -> "..\RvGame\Localization\INT\SP_BT_A_zone01_LOC_INT|SoundNodeWave|RL6_9006_BAR"
    if vid:
        vid = vid.replace('/', '|')
        if not vid.startswith('..\\'):
            vid = '..\\RvGame\\Localization\\INT\\' + vid
    
    # Get Polish translation from full_draft
    pl_text = ''
    if vid in entry_map:
        pl_text = entry_map[vid]
    
    # Create entry
    entry = {
        'source': v.get('source', ''),
        'package': v.get('package', ''),
        'asset': v.get('asset', ''),
        'speaker_guess': v.get('speaker_guess', ''),
        'profile': v.get('profile', 'narrator_user'),
        'text_en': v.get('text_en', ''),
        'text_pl': pl_text if pl_text else v.get('text_en', ''),
        'translation_status': 'machine_draft',
        'source_duration_s': v.get('source_duration_s', 0),
        'generated_duration_s': v.get('generated_duration_s', 0),
        'file': v.get('file', ''),
        'sample_rate': v.get('sample_rate', 24000),
        'peak_dbfs': v.get('peak_dbfs', -3.0)
    }
    story_entries.append(entry)

# Save story manifest
output_path = 'C:/Users/Trilex/vr-mods/SingularityVR-polish-integration/localization/pl/polish_story_voice.jsonl'
with open(output_path, 'w', encoding='utf-8') as f:
    for entry in story_entries:
        f.write(json.dumps(entry, ensure_ascii=False) + '\n')

print(f'Created story manifest with {len(story_entries)} entries at {output_path}')
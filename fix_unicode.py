import json

# Load the correct local draft (source of truth)
local_entries = {}
with open(r'C:\Users\Trilex\vr-mods\SingularityVR-localization-local\polish\full_draft.jsonl', 'r', encoding='utf-8') as f:
    for line in f:
        d = json.loads(line)
        local_entries[d.get('id')] = d.get('pl', '')

# Load the integration branch draft (with broken chars)
with open('localization/pl/full_draft.jsonl', 'rb') as f:
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

# Fix broken entries
fixed = 0
for i, line in enumerate(lines):
    d = json.loads(line)
    vid = d.get('id', '')
    pl = d.get('pl', '')
    
    if vid in local_entries:
        correct_pl = local_entries[vid]
        if pl != correct_pl:
            d['pl'] = correct_pl
            fixed += 1

# Save fixed draft
with open('localization/pl/full_draft.jsonl', 'w', encoding='utf-8') as f:
    for line in lines:
        f.write(line + '\n')

print('Fixed {} entries in full_draft.jsonl'.format(fixed))
import json
with open('master_inventory.jsonl', 'rb') as f:
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
    
    # Stats
    categories = {}
    profiles = {}
    statuses = {}
    translated = 0
    
    for line in lines:
        if line.strip():
            d = json.loads(line)
            cat = d.get('category', 'none')
            categories[cat] = categories.get(cat, 0) + 1
            prof = d.get('profile') or 'none'
            profiles[prof] = profiles.get(prof, 0) + 1
            status = d.get('status', 'none')
            statuses[status] = statuses.get(status, 0) + 1
            if d.get('pl') and d.get('pl') != '' and d.get('pl') != d.get('en'):
                translated += 1
    
    print('Categories:', sorted(categories.items()))
    print('Profiles:', sorted(profiles.items()))
    print('Statuses:', sorted(statuses.items()))
    print('Translated:', translated)
    print('Total:', len(lines))
    
    # Sample entries by category
    out_lines = []
    for cat in sorted(categories.keys()):
        out_lines.append('\n=== Category: ' + cat + ' ===')
        count = 0
        for line in lines:
            if line.strip():
                d = json.loads(line)
                if d.get('category') == cat and d.get('pl') and d.get('pl') != '' and d.get('pl') != d.get('en'):
                    sec = d.get('section', '')
                    key = d.get('key', '')
                    en = d.get('en', '')[:80]
                    pl = d.get('pl', '')[:80]
                    prof = d.get('profile') or 'none'
                    out_lines.append('  [' + prof + '] ' + sec + '|' + key + ': ' + en + ' -> ' + pl)
                    count += 1
                    if count >= 10:
                        break
    
    with open('inventory_analysis.txt', 'w', encoding='utf-8') as out:
        out.write('\n'.join(out_lines))
    print('Done - written to inventory_analysis.txt')
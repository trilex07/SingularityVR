import json
with open('full_draft.jsonl', 'rb') as f:
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
    translated = 0
    for line in lines:
        if line.strip():
            d = json.loads(line)
            if d.get('pl') and d.get('pl') != '' and d.get('pl') != d.get('en'):
                translated += 1
    print('Translated entries (non-empty PL):', translated)
    
    count = 0
    out_lines = []
    for line in lines:
        if line.strip():
            d = json.loads(line)
            if d.get('pl') and d.get('pl') != '' and d.get('pl') != d.get('en'):
                sec = d.get('section', '')
                key = d.get('key', '')
                en = d.get('en', '')[:60]
                pl = d.get('pl', '')[:60]
                out_lines.append(sec + '|' + key + ': ' + en + ' -> ' + pl)
                count += 1
                if count >= 50:
                    break
    
    with open('sample_translated.txt', 'w', encoding='utf-8') as out:
        out.write('\n'.join(out_lines))
import json

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

with open('check_output.txt', 'w', encoding='utf-8') as out:
    for i, line in enumerate(lines[:60]):
        d = json.loads(line)
        pl = d.get('pl', '')
        if i in [3, 5, 6, 34, 35, 44, 45, 46, 55]:
            out.write('Line {}: {}\n'.format(i, pl[:80]))
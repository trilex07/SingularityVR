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
    broken = 0
    for i, line in enumerate(lines[:100]):
        d = json.loads(line)
        pl = d.get('pl', '')
        if '┼' in pl or 'Ä' in pl or '├' in pl or '┤' in pl or '│' in pl or '┌' in pl or '└' in pl or '┐' in pl or '┘' in pl:
            broken += 1
            with open('broken_check.txt', 'a', encoding='utf-8') as f:
                f.write('Line {}: BROKEN: {}\n'.format(i, pl[:80]))
    print('Total broken lines in first 100: {}'.format(broken))
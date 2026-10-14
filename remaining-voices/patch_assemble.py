import re
src = open('assemble_all.py', encoding='utf-8').read()
src = src.replace('json.load(open(', 'load_json(')
open('assemble_all.py', 'w', encoding='utf-8').write(src)
print('ok')

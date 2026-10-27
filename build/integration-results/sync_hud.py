from pathlib import Path
p=Path('STATUS.md'); s=p.read_text(encoding='utf-8-sig')
lines=s.splitlines(True)
assert sum(x.startswith('<<<<<<<') for x in lines)==1
p.write_text(''.join(x for x in lines if not x.startswith(('<<<<<<<','=======','>>>>>>>'))),encoding='utf-8')

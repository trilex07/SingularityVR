import io
p = 'assemble_all.py'
s = io.open(p, encoding='utf-8').read()
s = s.replace(", encoding='utf-8'))", ")")
io.open(p, 'w', encoding='utf-8').write(s)
print('ok')

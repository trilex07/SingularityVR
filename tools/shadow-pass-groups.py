import sys,re,struct,collections
def parse(path):
    G=collections.defaultdict(dict);cur=None;sh=None;draws=collections.Counter()
    for line in open(path,errors='replace'):
        w=line.split()
        if not w:continue
        if w[0] in('pipeline','geometry','texture','constant'):
            m=re.search(r'group=(\d+)',line);cur=int(m.group(1));G[cur].setdefault(w[0],[]).append(line.strip());sh=None
        elif w[0]=='event':
            m=re.search(r'group=(\d+)',line)
            if m:draws[int(m.group(1))]+=1
        elif w[0]=='shader':
            sh=[w[2],[]];G[cur].setdefault('shaders',[]).append(sh)
        elif sh is not None and re.fullmatch(r'[0-9A-F]{8}',w[0]):
            sh[1]+= [int(x,16) for x in w if re.fullmatch(r'[0-9A-F]{8}',x)]
    return G,draws
def names(words):
    b=b''.join(struct.pack('<I',x) for x in words)
    return sorted(set(s.decode() for s in re.findall(rb'[A-Za-z_][A-Za-z0-9_]{5,}',b) if not s.startswith(b'Microsoft')))
G,draws=parse(sys.argv[1])
for g in sorted(G):
    d=G[g];p=d.get('pipeline',[''])[0]
    ps=[names(w) for st,w in d.get('shaders',[]) if 'PS' in st]
    n=' '.join(x for l in ps for x in l)
    if re.search(sys.argv[2],n+p):
        geo=d.get('geometry',[''])[0];tex=[t for t in d.get('texture',[]) if 'id=00000000' not in t]
        print(g,'draws',draws[g],re.sub(r'.*?(colorWrite=\d).*?(zWrite=\d).*?(blend=\d) (srcBlend=\d+) (dstBlend=\d+).*',r'\1 \2 \3 \4 \5',p),'|',geo[9:80],'|',n[:200]);[print('   ',t[8:]) for t in tex]

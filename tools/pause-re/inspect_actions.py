"""Bounded AVM1 instruction evidence; no ActionScript execution or guessed paths."""
import argparse
import json
import struct
from pathlib import Path

OPS={0x1c:'GetVariable',0x1d:'SetVariable',0x3d:'CallFunction',0x4e:'GetMember',
     0x4f:'SetMember',0x52:'CallMethod',0x53:'NewMethod',0x96:'Push',0x88:'ConstantPool',
     0x9b:'DefineFunction',0x8e:'DefineFunction2',0x99:'Jump',0x9d:'If',0x26:'Trace',0x17:'Pop'}
def actions(b,base=0,pool=(),depth=0):
    if depth>16:raise ValueError('nesting cap')
    out=[];p=0
    while p<len(b):
        offset=p;op=b[p];p+=1
        if not op:break
        n=struct.unpack_from('<H',b,p)[0] if op>=128 else 0
        if op>=128:p+=2
        data=b[p:p+n];p+=n
        if len(data)!=n:raise ValueError('action range')
        vals=[];children=[]
        if op==0x88:
            count=struct.unpack_from('<H',data)[0]; vals=[s.decode('latin1') for s in data[2:].split(b'\0')[:-1]]
            if len(vals)!=count:raise ValueError('constant count')
            pool=vals
        elif op==0x96:
            q=0
            while q<len(data):
                typ=data[q];q+=1
                if typ==0:
                    end=data.index(0,q);vals.append(data[q:end].decode('latin1'));q=end+1
                elif typ in (8,9):
                    idx=data[q] if typ==8 else struct.unpack_from('<H',data,q)[0];q+=1 if typ==8 else 2
                    vals.append(pool[idx] if idx<len(pool) else f'BAD_CONST_{idx}')
                elif typ==7:vals.append(struct.unpack_from('<i',data,q)[0]);q+=4
                elif typ==1:vals.append(struct.unpack_from('<f',data,q)[0]);q+=4
                elif typ==6:vals.append(struct.unpack('<d',data[q+4:q+8]+data[q:q+4])[0]);q+=8
                elif typ in (4,5):vals.append(f'type{typ}:{data[q]}');q+=1
                elif typ in (2,3):vals.append('null' if typ==2 else 'undefined')
                else:raise ValueError(f'push type {typ}')
        elif op in (0x9b,0x8e):
            end=data.index(0);fn=data[:end].decode('latin1');q=end+1
            count=struct.unpack_from('<H',data,q)[0];q+=2
            if op==0x8e:q+=3
            for _ in range(count):
                if op==0x8e:q+=1
                q=data.index(0,q)+1
            size=struct.unpack_from('<H',data,q)[0];q+=2
            vals=[fn or '<anonymous>',count,size]
            if q==len(data):
                if p+size>len(b):raise ValueError('function body range')
                children=actions(b[p:p+size],base+p,pool,depth+1);p+=size
            elif q+size==len(data):
                children=actions(data[q:q+size],base+offset+3+q,pool,depth+1)
            else:raise ValueError('function body range')
        out.append(dict(offset=hex(base+offset),op=OPS.get(op,hex(op)),values=vals,children=children))
    return out

def tags(b,start,end):
    p=start
    while p+2<=end:
        head=struct.unpack_from('<H',b,p)[0];p+=2;tag,n=head>>6,head&63
        if n==63:n=struct.unpack_from('<I',b,p)[0];p+=4
        if p+n>end:raise ValueError('tag range')
        yield tag,p,n
        p+=n
        if tag==0:break

def run(path,query):
    b=Path(path).read_bytes(); bits=b[8]>>3;p=8+(5+4*bits+7)//8+4
    blocks=[]
    def walk(start,end,owner):
        for tag,p,n in tags(b,start,end):
            if tag in (12,59):blocks.append((owner,actions(b[p+(2 if tag==59 else 0):p+n],p+(2 if tag==59 else 0))))
            elif tag==39:walk(p+4,p+n,f'sprite:{struct.unpack_from("<H",b,p)[0]}')
    walk(p,len(b),'root')
    selected=[]
    def visit(owner,records):
        for i,r in enumerate(records):
            if r['op']!='ConstantPool' and any(query.lower() in str(v).lower() for v in r['values']):
                window=r['children'] if r['children'] and r['values'][0]==query else records[max(0,i-3):i+7]
                selected.append(dict(owner=owner,context=[{k:v for k,v in x.items() if k!='children'} for x in window[:120] if x['op']!='ConstantPool']))
            if r['children']:visit(owner+'/'+str(r['values'][0]),r['children'])
    for owner,records in blocks:visit(owner,records)
    return dict(blocks=len(blocks),matches=selected[:25],truncated=len(selected)>25)

if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('movie');ap.add_argument('query');a=ap.parse_args()
    print(json.dumps(run(a.movie,a.query),indent=2))

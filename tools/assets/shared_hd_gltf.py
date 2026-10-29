"""Offline lossless SIDE sidecar resolver. Never loaded by the game/render thread.

Each sidecar is verified against its hash-pinned original package, including all
images and non-image bufferViews. Shared URI traversal, missing/corrupt payloads,
and altered nodes/animation/accessor data fail closed; no silent GLB fallback.
The native runtime continues sharing its existing material textures on replay.
"""
import copy, hashlib, json
from pathlib import Path

WEAPONS={'volk_s4_hd':'volk_s4','valkyrie_ar9_hd':'valkyrie_ar9',
         'kasimov_hd':'kasimov_snv_e99','centurion_hd':'centurion'}


def resolve(parent, uri):
    if not isinstance(uri,str) or not uri or '\\' in uri or ':' in uri or '%' in uri:
        raise ValueError('unsupported external URI')
    p=(parent/uri).resolve()
    if Path(uri).is_absolute() or not p.is_relative_to(parent.resolve()):raise ValueError('external URI escapes package')
    return p


def load_shared(original, root):
    from prepare_volk_runtime import _read_glb_document
    weapon=next((v for k,v in WEAPONS.items() if original.stem.startswith(k)),None)
    if weapon is None:raise ValueError('unknown shared HD package')
    path=Path(root)/weapon/(original.stem+'.gltf');parent=path.parent
    doc=json.loads(path.read_text(encoding='utf-8'))
    if len(doc.get('buffers',[]))!=1:raise ValueError('shared buffer count')
    raw=resolve(parent,doc['buffers'][0]['uri']).read_bytes()
    if len(raw)!=doc['buffers'][0]['byteLength']:raise ValueError('shared buffer length')
    source,source_raw=_read_glb_document(original)
    for key in ('meshes','nodes','materials','textures','animations','scenes','scene','skins','asset'):
        if source.get(key)!=doc.get(key):raise ValueError('shared structure changed: '+key)
    image_views={i['bufferView'] for i in source['images']}
    remaining=[i for i in range(len(source['bufferViews'])) if i not in image_views]
    remap={old:new for new,old in enumerate(remaining)}
    accessors=copy.deepcopy(source.get('accessors',[]))
    for a in accessors:
        if 'bufferView' in a:a['bufferView']=remap[a['bufferView']]
        for entry in a.get('sparse',{}).values():
            if isinstance(entry,dict) and 'bufferView' in entry:entry['bufferView']=remap[entry['bufferView']]
    if accessors!=doc.get('accessors',[]):raise ValueError('shared accessor changed')
    if len(doc['bufferViews'])!=len(remaining):raise ValueError('shared view count')
    for old,v in zip(remaining,doc['bufferViews']):
        before=source['bufferViews'][old];expected=copy.deepcopy(before)
        expected['byteOffset']=v.get('byteOffset',0)
        if expected!=v:raise ValueError('shared view layout changed')
        start=v.get('byteOffset',0);end=start+v['byteLength'];a=before.get('byteOffset',0)
        if start<0 or end>len(raw) or raw[start:end]!=source_raw[a:a+before['byteLength']]:raise ValueError('shared geometry/animation changed')
    if len(source['images'])!=len(doc['images']):raise ValueError('shared image count')
    for a,b in zip(source['images'],doc['images']):
        data=resolve(parent,b['uri']).read_bytes();v=source['bufferViews'][a['bufferView']];at=v.get('byteOffset',0)
        if data!=source_raw[at:at+v['byteLength']] or Path(b['uri']).stem!=hashlib.sha256(data).hexdigest():raise ValueError('shared image changed')
    return doc,raw

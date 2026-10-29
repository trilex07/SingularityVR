"""Convert the HD UE3/DirectX green channel for Blender/glTF +Y tangent space.

Only the local handoff copy is written; original and HD source maps stay intact.
"""
import argparse
import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image


def convert(source, output):
    rgb = np.asarray(Image.open(source).convert('RGB'),dtype=np.uint8)
    if rgb.shape != (4096,4096,3):
        raise ValueError('unexpected Volk HD normal dimensions')
    converted=rgb.copy()
    converted[:,:,1]=255-rgb[:,:,1]
    if not (np.array_equal(converted[:,:,0],rgb[:,:,0]) and
            np.array_equal(converted[:,:,2],rgb[:,:,2]) and
            np.array_equal(converted[:,:,1],255-rgb[:,:,1])):
        raise ValueError('normal conversion channel check failed')
    output.parent.mkdir(parents=True,exist_ok=True)
    Image.fromarray(converted,'RGB').save(output)
    decoded=converted.astype(np.float32)/127.5-1
    lengths=np.linalg.norm(decoded,axis=2)
    report={'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),
            'converted_sha256':hashlib.sha256(output.read_bytes()).hexdigest(),
            'resolution':[4096,4096], 'channel_transform':'R unchanged; G=255-G; B unchanged',
            'normal_length_min':float(lengths.min()),
            'normal_length_max':float(lengths.max()),
            'conversion_basis':'HD visual source documents native UE3 normal encoding; glTF uses +Y tangent-space normal texture'}
    (output.parent/'normal_conversion_manifest.json').write_text(json.dumps(report,indent=2)+'\n')
    return report


if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--source',required=True,type=Path)
    ap.add_argument('--output',required=True,type=Path)
    args=ap.parse_args()
    print(json.dumps(convert(args.source,args.output),indent=2))

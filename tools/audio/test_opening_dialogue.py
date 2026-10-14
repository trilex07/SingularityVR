"""Fixture gates on private owned game copies; no audio/assets committed."""
import argparse
import hashlib
from pathlib import Path
from patch_opening_dialogue import build, bulks, TARGETS
from ue3_sound_package import inspect

ap = argparse.ArgumentParser()
ap.add_argument('--fixture', type=Path, required=True)
args = ap.parse_args()
original = (args.fixture/'SP_VI_intro_Kismet_LOC_INT.original.xxx').read_bytes()
logical = (args.fixture/'SP_VI_intro_Kismet_LOC_INT.u').read_bytes()
replacements = {p.stem:p.read_bytes() for p in (args.fixture/'fitted').glob('INTRO_*.ogg')}
assert set(replacements) == TARGETS
control, packed = build(original, logical, {})
assert control == logical, 'repack-only control must retain every logical byte'
changed, physical = build(original, logical, replacements)
assert changed == (args.fixture/'polish-opening.roundtrip.u').read_bytes()
assert physical == (args.fixture/'polish-opening.xxx').read_bytes()
old = inspect(logical)['exports']
new = inspect(changed)['exports']
assert len(old) == len(new) == 143
speech = 0
for a, b in zip(old, new):
    if a['name'] in TARGETS:
        speech += 1
        assert bulks(changed,b)[1][1] == replacements[a['name']]
    elif a['offset'] == b['offset']:
        assert logical[a['offset']:a['offset']+a['size']] == changed[b['offset']:b['offset']+b['size']]
    else:
        # Only the last INTRO scream moves; its properties/audio stay exact.
        assert a['name'] == 'INTRO_1015_GST'
        assert logical[a['offset']:a['property_end']] == changed[b['offset']:b['property_end']]
        assert [p for h,p in bulks(logical,a)] == [p for h,p in bulks(changed,b)]
assert speech == 10
def refuses(fn):
    try:
        fn()
    except (ValueError,AssertionError):
        return
    raise AssertionError('malformed input accepted')
refuses(lambda:build(original+b'bad',logical,replacements))
refuses(lambda:build(original,logical+b'bad',replacements))
refuses(lambda:build(original,logical,dict(unknown=b'OggS')))
bad = dict(replacements)
bad[next(iter(bad))] = b'OggS' # truncated Vorbis header
refuses(lambda:build(original,logical,bad))
print('PASS: 10 exact native voices, 128 other sounds + scream preserved, metadata, relocation, roundtrip, refusal gates')

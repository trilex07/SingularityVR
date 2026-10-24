"""Optional local original-asset identity test; no game bytes in the repository."""
import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from inspect_gameplay_hud import inspect_hud, raw_hud


def fnv64(data):
    value = 0xcbf29ce484222325
    for byte in data:
        value = ((value ^ byte) * 0x100000001b3) & 0xffffffffffffffff
    return value


def verify(path):
    movie = raw_hud(path.read_bytes())
    d = inspect_hud(movie)
    assert d['bytes'] == 187541
    assert fnv64(movie) == 0x21ACFC3AD63D0F40
    exports = {x['name']: x['character'] for x in d['exports']}
    assert exports['LocationTextDisplay'] == 414
    assert exports['ObjectiveTextDisplay'] == 410
    assert exports['levlLocTxt'] == 408
    fields = {x['character']: x for x in d['fields']}
    for char in (411, 412, 413):
        assert fields[char]['bounds'] == [-40, 4680, -40, 805]
    placed = [x for x in d['placements'] if x['owner'] == 414 and x['character'] in (411, 412, 413)]
    assert {x['name'] for x in placed} == {'msgTxt0', 'msgTxt1', 'msgTxt2'}
    assert {x['character'] for x in placed} == {411, 412, 413}
    shape = next(x for x in d['placements'] if x['owner'] == 414 and x['character'] == 218)
    assert shape['matrix']['x_twips'] == 4940 and shape['matrix']['y_twips'] == 820
    assert shape['matrix']['sy'] == 4.0
    assert 'ShowInformation' in d['script_names'] and 'locationTxt' in d['script_names']
    assert 'txtMsgContainer' in d['script_names']
    return d['sha256']


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('rvgame', type=Path)
    a = p.parse_args()
    print('original gameplay HUD identity PASS', verify(a.rvgame))

"""Scoped native cursor shape and post-Initialize title fixes; offline only.

Never patches arbitrary strings or removes input handling. The original Mouse
sprite placement/actions survive; only its exclusive leaf geometry changes.
"""
import math
import struct
from swf_title_patch import split_tags, encode_tag_header, fit_title_field


def require(ok, message):
    if not ok:
        raise ValueError(message)


class Bits:
    def __init__(self):
        self.bits = []

    def put(self, value, width):
        require(width > 0 and -(1 << (width - 1)) <= value < (1 << width), 'bit width')
        value &= (1 << width) - 1
        self.bits.extend((value >> shift) & 1 for shift in range(width - 1, -1, -1))

    def bytes(self):
        padding = (-len(self.bits)) % 8
        bits = self.bits + [0] * padding
        return bytes(sum(bits[i + j] << (7 - j) for j in range(8)) for i in range(0, len(bits), 8))


def signed_width(values):
    return max(2, max(abs(v).bit_length() + 1 for v in values))


def dot_shape(character):
    """DefineShape RGB even-odd contours: outer ring, transparent gap, centre dot.

    Origin is the native pointer's hit point, not a screen-coordinate guess.
    Thirty-two fixed edges/contour; no textures, font, or renderer hook needed.
    """
    rect = Bits()
    rect.put(9, 5)
    for v in (-180, 180, -180, 180):
        rect.put(v, 9)
    edges = Bits()
    edges.put(1, 4)  # one fill index bit
    edges.put(1, 4)  # one line index bit
    for contour, radius in enumerate((160, 115, 42)):
        points = [(round(radius * math.cos(i * math.tau / 32)),
                   round(radius * math.sin(i * math.tau / 32))) for i in range(32)]
        # Move + fill1 + line1; preserve one even-odd fill across nested contours.
        edges.put(0, 1)
        edges.put(13, 5)
        width = signed_width(points[0])
        edges.put(width, 5)
        edges.put(points[0][0], width)
        edges.put(points[0][1], width)
        edges.put(1, 1)
        edges.put(1, 1)
        for a, b in zip(points, points[1:] + points[:1]):
            dx, dy = b[0] - a[0], b[1] - a[1]
            width = signed_width((dx, dy))
            edges.put(1, 1); edges.put(1, 1)  # edge, straight
            edges.put(width - 2, 4)
            edges.put(1, 1)  # general edge (both components)
            edges.put(dx, width); edges.put(dy, width)
    edges.put(0, 6)  # EndShape
    # Cream-white fill and thin black contour keep contrast on light/dark scenes.
    styles = bytes((1, 0, 245, 245, 245, 1)) + struct.pack('<H', 12) + bytes((8, 12, 16))
    return struct.pack('<H', character) + rect.bytes() + styles + edges.bytes()


def replace_tag(movie, tag, payload):
    result = (movie[:tag['tag_off']] + encode_tag_header(tag['code'], len(payload)) + payload +
              movie[tag['pay_off'] + tag['pay_len']:])
    result = result[:4] + struct.pack('<I', len(result)) + result[8:]
    split_tags(result)
    return result


def exported_char(movie, wanted):
    _, tags = split_tags(movie)
    matches = []
    for tag in tags:
        if tag['code'] != 56:
            continue
        data = movie[tag['pay_off']:tag['pay_off'] + tag['pay_len']]
        count = struct.unpack_from('<H', data)[0]; pos = 2
        for _ in range(count):
            char = struct.unpack_from('<H', data, pos)[0]
            end = data.index(0, pos + 2)
            name = data[pos + 2:end].decode('ascii')
            if name == wanted:
                matches.append(char)
            pos = end + 1
        require(pos == len(data), 'export tail')
    require(len(matches) == 1, 'exclusive native export not found')
    return matches[0]


def patch_cursor(movie):
    char = exported_char(movie, 'Mouse')
    _, tags = split_tags(movie)
    sprite = [t for t in tags if t['code'] == 39 and
              struct.unpack_from('<H', movie, t['pay_off'])[0] == char]
    require(len(sprite) == 1, 'Mouse sprite identity')
    data = movie[sprite[0]['pay_off']:sprite[0]['pay_off'] + sprite[0]['pay_len']]
    # Both proved native movies have this one-frame, one-leaf layout.
    require(len(data) == 16 and data[2:] == bytes.fromhex('01008606060100') + data[9:11] +
            bytes.fromhex('0040000000'), 'unknown native Mouse placement layout')
    leaf = struct.unpack_from('<H', data, 9)[0]
    shape = [t for t in tags if t['code'] == 2 and
             struct.unpack_from('<H', movie, t['pay_off'])[0] == leaf]
    require(len(shape) == 1 and shape[0]['pay_len'] == 32, 'native arrow leaf identity')
    # Reject any additional placement of this character, rather than alter unrelated UI.
    reference = b'\x06\x01\x00' + struct.pack('<H', leaf) + b'\x00'
    require(movie.count(reference) == 1, 'cursor leaf has unexpected shared placements')
    return replace_tag(movie, shape[0], dot_shape(leaf)), dict(sprite=char, shape=leaf,
        pointer='native concentric ring/dot', placement_actions_unchanged=True)


def action(code, payload=b''):
    return bytes((code,)) + (struct.pack('<H', len(payload)) + payload if code >= 128 else b'')


def push(*strings):
    return action(0x96, b''.join(b'\0' + s.encode('ascii') + b'\0' for s in strings))


def brand_actions():
    # Proven field path from Initialize's original assignment, not a guessed GFx variable.
    obj = push('Title') + action(0x1c) + push('MajorTitle') + action(0x4e)
    write = obj + push('text', 'SINGULARITY VR') + action(0x4f)
    test = obj + push('text') + action(0x4e) + push('SINGULARITY') + action(0x13) + action(0x12)
    return test + action(0x9d, struct.pack('<h', len(write))) + write


def action_records(data):
    """Function code follows its metadata action length in native AVM1.

    Validate body boundaries recursively. Metadata Length deliberately excludes
    CodeSize bytes; treating the entire body as payload corrupts native functions.
    """
    rows = []; pos = 0
    while pos < len(data):
        start = pos; code = data[pos]; pos += 1
        size = struct.unpack_from('<H', data, pos)[0] if code >= 128 else 0
        pos += 2 if code >= 128 else 0
        end = pos + size
        require(end <= len(data), 'action bounds')
        row = dict(start=start, code=code, payload=pos, metadata_end=end, end=end)
        if code in (0x8e, 0x9b):
            z = data.index(0, pos, end)
            row['name'] = data[pos:z].decode('ascii')
            count = struct.unpack_from('<H', data, z + 1)[0]
            offset = z + 3 + (3 if code == 0x8e else 0)
            for _ in range(count):
                offset += 1 if code == 0x8e else 0
                offset = data.index(0, offset, end) + 1
            require(offset + 2 == end, 'function metadata layout')
            body_size = struct.unpack_from('<H', data, offset)[0]
            row.update(size_field=offset, body_start=end, body_end=end + body_size, end=end + body_size)
            require(row['end'] <= len(data), 'function code bounds')
            row['body'] = action_records(data[end:row['end']])
        rows.append(row); pos = row['end']
    return rows


def insert_in_body(data, at, injected):
    rows = action_records(data)
    require(at in {r['start'] for r in rows} | {len(data)}, 'insert is not an action boundary')
    staged = bytearray(data)
    for row in rows:
        if row['code'] not in (0x99, 0x9d):
            continue
        require(row['metadata_end'] - row['payload'] == 2, 'branch size')
        offset = struct.unpack_from('<h', data, row['payload'])[0]
        target = row['end'] + offset
        require(target in {r['start'] for r in rows} | {len(data)}, 'branch target not an action boundary')
        new_start = row['end'] + (len(injected) if row['start'] >= at else 0)
        new_target = target + (len(injected) if target > at else 0)
        delta = new_target - new_start
        require(-32768 <= delta <= 32767, 'relocated branch overflow')
        struct.pack_into('<h', staged, row['payload'], delta)
    return bytes(staged[:at]) + injected + bytes(staged[at:])


def patch_main_brand(movie):
    _, tags = split_tags(movie)
    candidates = []
    for candidate in tags:
        if candidate['code'] != 12:
            continue
        block = movie[candidate['pay_off']:candidate['pay_off'] + candidate['pay_len']]
        roots = action_records(block)
        if any(r.get('name') == 'Initialize' for r in roots):
            candidates.append(candidate)
    require(len(candidates) == 1, 'main root Initialize action identity')
    tag = candidates[0]
    data = movie[tag['pay_off']:tag['pay_off'] + tag['pay_len']]
    roots = action_records(data)
    funcs = [r for r in roots if r.get('name') == 'Initialize']
    require(len(funcs) == 1, 'Initialize identity')
    fn = funcs[0]; body = data[fn['body_start']:fn['body_end']]
    # Exact typed toUpperCase -> CallMethod -> SetMember assignment witnessed offline.
    marker = bytes.fromhex('960300090001524f')
    require(body.count(marker) == 1, 'native title assignment contract changed')
    at = body.index(marker) + len(marker)
    changed = insert_in_body(body, at, brand_actions())
    delta = len(changed) - len(body)
    staged = bytearray(data)
    struct.pack_into('<H', staged, fn['size_field'], len(changed))
    result = bytes(staged[:fn['body_start']]) + changed + bytes(staged[fn['body_end']:])
    # Relocate branches in the root action stream that straddle this function.
    for row in roots:
        if row['code'] in (0x99, 0x9d):
            require(False, 'root branches require independently proved relocation')
    action_records(result)
    return replace_tag(movie, tag, result), dict(function='Initialize', field='Title.MajorTitle.text',
        exact_match='SINGULARITY', replacement='SINGULARITY VR', delta=delta)


def polish_movie(movie, logo):
    changed, cursor = patch_cursor(movie)
    changed, fitted = fit_title_field(changed, b'SINGULARITY', b'SINGULARITY VR')
    if logo:
        title = fitted
    else:
        changed, runtime = patch_main_brand(changed)
        title = {'fitted': fitted, 'runtime': runtime}
    return changed, dict(cursor=cursor, title=title)

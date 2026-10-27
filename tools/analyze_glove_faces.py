"""Read-only ActorX face/weight census for the noTMD first-person mesh."""
import struct
from collections import Counter
from pathlib import Path

path = Path(r"C:\Users\Trilex\vr-mods\research\native-hands-20261007\assets\RvGame\SkeletalMesh3\ch_inview_noTMD.psk")
data = path.read_bytes()
chunks = {}
pos = 0
while pos < len(data):
    raw_name, _, size, count = struct.unpack_from("<20siii", data, pos)
    pos += 32
    chunks[raw_name.split(b"\0")[0].decode()] = (size, count, data[pos:pos + size * count])
    pos += size * count

size, count, raw = chunks["REFSKELT"]
names = [raw[i * size:i * size + 64].split(b"\0")[0].decode() for i in range(count)]
size, count, raw = chunks["RAWWEIGHTS"]
weights = {}
for i in range(count):
    weight, point, bone = struct.unpack_from("<fii", raw, i * size)
    if weight > .0001:
        weights.setdefault(point, []).append((weight, bone))

size, count, raw = chunks["VTXW0000"]
wedges = [struct.unpack_from("<I", raw, i * size)[0] for i in range(count)]
size, count, raw = chunks["FACE0000"]
totals = Counter()
runs = []
last = None
cross_hand = []
for i in range(count):
    a, b, c, mat, _, _ = struct.unpack_from("<3HBBI", raw, i * size)
    if mat != 0:
        continue
    triangle_bones = {bone for wedge in (a, b, c) for _, bone in weights.get(wedges[wedge], [])}
    if any(names[bone].startswith("l_") for bone in triangle_bones) and any(names[bone].startswith("r_") for bone in triangle_bones):
        cross_hand.append(i)
    arm = {bone for bone in triangle_bones if names[bone] == "tmd_base" or any(s in names[bone] for s in
           ("forearm", "elbow", "bicep", "shoulder", "clavicle", "sleeve_fix"))}
    role = "arm" if arm else "hand"
    totals[role] += 1
    for bone in triangle_bones:
        totals[(role, names[bone])] += 1
    if role != last:
        runs.append([role, i, i])
        last = role
    else:
        runs[-1][2] = i

print("faces:", totals["hand"], "hand /", totals["arm"], "arm")
print("runs:", len(runs), "hand runs:", sum(r[0] == "hand" for r in runs))
print("first runs:", runs[:25])
print("hand runs:", [(a, b) for role, a, b in runs if role == "hand"])
print("arm bones:", sorted((k[1], v) for k, v in totals.items() if isinstance(k, tuple) and k[0] == "arm"))
print("hand bones:", sorted((k[1], v) for k, v in totals.items() if isinstance(k, tuple) and k[0] == "hand"))
print("cross-hand faces:", cross_hand[:40], "count", len(cross_hand))

import collections
from pathlib import Path

log = Path(__file__).with_name("view_matrix_full.log")
routes = []
for i, line in enumerate(log.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
    if not line.startswith("dual-tmd route:"):
        continue
    d = {"raw": line}
    for tok in line.split():
        if "=" in tok:
            k, v = tok.split("=", 1)
            d[k] = v
    d["line"] = str(i)
    routes.append(d)

m1 = [r for r in routes if r.get("part") == "1"]
by = collections.defaultdict(list)
for r in m1:
    key = (
        r.get("hands"),
        r.get("support"),
        r.get("route"),
        r.get("palette"),
        r.get("segments"),
        r.get("stereoEyes"),
    )
    by[key].append((r.get("frame"), r.get("line")))

lines = ["# Material part=1 (min=2626 verts=4818 tris=7704)\n"]
for key in sorted(by.keys()):
    frames = [f[0] for f in by[key]]
    sample = ",".join(frames[:12])
    if len(frames) > 12:
        sample += "..."
    lines.append(
        f"hands={key[0]} support={key[1]} route={key[2]} palette={key[3]} "
        f"segments={key[4]} stereoEyes={key[5]} count={len(by[key])} frames={sample}"
    )
lines.append("\n# All route rows (chronological)\n")
for r in routes:
    lines.append(
        f"L{r['line']} frame={r.get('frame')} hands={r.get('hands')} support={r.get('support')} "
        f"part={r.get('part')} route={r.get('route')} palette={r.get('palette')} "
        f"segments={r.get('segments')} stereoEyes={r.get('stereoEyes')}"
    )
Path(__file__).with_name("route-grouped-material1.txt").write_text(
    "\n".join(lines), encoding="utf-8"
)
print(f"routes={len(routes)} m1={len(m1)} groups={len(by)}")

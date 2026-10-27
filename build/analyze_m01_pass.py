import re, io, collections, json

L = r"C:\Users\Trilex\vr-mods\SingularityVR-sol-master-integration\build\m01-pass-session-view_matrix.log"
OUT = r"C:\Users\Trilex\vr-mods\SingularityVR-sol-master-integration\docs\evidence\m01-bb234ba-psvr2-pass"
import os
os.makedirs(OUT, exist_ok=True)

lines = io.open(L, encoding="utf-8", errors="replace").read().splitlines()

ev = re.compile(r'next reload: (\S+) kind=(\d) phase=(\d) .*?clip=(-?\d+) reserve=(\d+)')
cnt = collections.Counter(); timeline = []
stage = collections.Counter(); wrist = []
for i, ln in enumerate(lines):
    m = ev.search(ln)
    if m:
        e, k, ph, c, r = m.groups()
        cnt[(e, k)] += 1
        timeline.append((m.start(" e") if False else e, k, c, r))
    m2 = re.search(r'next reload draw gate: stage=(\d+) kind=(\d)', ln)
    if m2:
        stage[(m2.group(1), m2.group(2))] += 1
    m3 = re.search(r'next carry wrist: kind=(\d) .*segments=(\d+)', ln)
    if m3:
        wrist.append((m3.group(1), m3.group(2)))

print("== reload event counts (event/kind) ==")
for k, v in sorted(cnt.items()):
    print("%-18s kind=%s x%d" % (k[0], k[1], v))

print("\n== draw gates (stage/kind) ==")
for k, v in sorted(stage.items()):
    print("stage=%s kind=%s x%d" % (k[0], k[1], v))

print("\n== carry wrist publishes ==")
for w, s in wrist:
    print("kind=%s segments=%s" % (w, s))

print("\n== Valkyrie (kind=1) ordered unique state ==")
seen = set(); out = []
for e, k, c, r in timeline:
    if k == '1':
        key = (e, c, r)
        if key not in seen:
            seen.add(key); out.append("%s clip %s reserve %s" % (e, c, r))
print("  ->  ".join(out))

print("\n== Volk (kind=2) ordered unique state ==")
seen = set(); out = []
for e, k, c, r in timeline:
    if k == '2':
        key = (e, c, r)
        if key not in seen:
            seen.add(key); out.append("%s clip %s reserve %s" % (e, c, r))
print("  ->  ".join(out))

# staged receipts (pickup gates) grouping
gates = []
for ln in lines:
    m = re.search(r'shoulder pickup gate: kind=(\d) event=(\S+).*?(?:pointDist=([\d.]+) regionDist=([\d.]+) local=\(([-\d.]+),([-\d.]+),([-\d.]+)\) radius=([\d.]+) extendLeft=([\d.]+))', ln)
    if m:
        gates.append(m.groups())
print("\n== shoulder pickup gates: %d ==" % len(gates))
for g in gates:
    print(" kind=%s %s point=%s region=%s local=(%s,%s,%s) r=%s ext=%s" % (g[0], g[1], g[2], g[3], g[4], g[5], g[6], g[7], g[8]))

# capsules actually used? compare regionDist < pointDist (capsule accepted beyond sphere)
cap = [g for g in gates if float(g[3]) < float(g[2])]
print("gates accepted via capsule beyond original sphere (regionDist<pointDist): %d/%d" % (len(cap), len(gates)))

# write extraction files
with io.open(OUT + r"\shoulder-gates.txt", "w", encoding="utf-8", newline="\n") as f:
    f.write("M01 bb234ba PSVR2 PASS — shoulder pickup gate receipts (%d)\n" % len(gates))
    for ln in lines:
        if 'shoulder pickup gate' in ln:
            f.write(ln.rstrip() + "\n")

with io.open(OUT + r"\next-reload-transcript.txt", "w", encoding="utf-8", newline="\n") as f:
    f.write("M01 bb234ba PSVR2 PASS — dedicated reload pipeline events\n")
    for ln in lines:
        if re.search(r'next reload: (magazine-ejected|grab-fresh|discarded|inserted/request|native-ammo-ack)', ln) \
           or 'next carry wrist' in ln or 'next reload draw gate: stage=1' in ln \
           or 'next reload draw gate: stage=2' in ln or 'next reload draw gate: stage=10' in ln \
           or 'next reload draw gate: stage=11' in ln or 'next reload draw gate: stage=12' in ln:
            f.write(ln.rstrip() + "\n")
print("\nextraction files written")

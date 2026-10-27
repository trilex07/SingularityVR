import re, io, json, collections

LOG = r"C:\Users\Trilex\vr-mods\SingularityVR-m09-current\docs\evidence\m09-session-a-20261024\view_matrix.log"
lines = io.open(LOG, encoding="utf-8", errors="replace").read().splitlines()
n = len(lines)

# 1) session markers / startup banners
starts = [i for i, l in enumerate(lines) if re.search(r"SingularityVR.*(start|init|attach)|OpenXR (session|instance).*(creat|init)|bridge start|===|SESSION", l, re.I)][:20]
print("total lines:", n)
print("--- first 6 lines ---")
for l in lines[:6]:
    print(" ", l[:150])
print("--- startup-marker candidates:", len(starts))
for i in starts[:8]:
    print("  line", i + 1, ":", lines[i][:150])

# 2) haptics receipts
rumble = [(i, l) for i, l in enumerate(lines) if "XINPUT RUMBLE" in l]
submit = [(i, l) for i, l in enumerate(lines) if "Haptics Trace] SUBMIT" in l]
stop = [(i, l) for i, l in enumerate(lines) if "Haptics Trace] STOP" in l]
print("\nrumble receipts:", len(rumble), "| SUBMIT:", len(submit), "| STOP:", len(stop))
for i, l in rumble:
    print("  R", i + 1, l.strip()[:170])

# 3) SUBMIT gates summary
gates = collections.Counter()
bad = []
for i, l in submit:
    m = re.search(r"nativeAmp=([\d.eE+-]+) \(raw=(-?\d+)\)(?: L|-?\d+)?", l)
    am = re.search(r"nativeAmp=([\d.eE+-]+) \(raw=(-?\d+)\).*overlay=([\d.eE+-]+) merged=([\d.eE+-]+).*\| amp=([\d.eE+-]+) -> xrResult=(-?\d+)", l)
    gt = re.search(r"allowed=(\d) tracked=(\d) poseValid=(\d) posValid=(\d) focus=(\d)", l)
    hand = re.search(r"hand=(\d) \(LEFT|RIGHT\)", l)
    if am and gt:
        key = (gt.groups(), am.group(6))
        gates[key] += 1
        if gt.group(1) == "0" or gt.group(2) == "0" or gt.group(5) == "0" or am.group(6) != "0":
            bad.append((i + 1, l.strip()[:200]))
print("\ngate/result distribution (allowed,tracked,pose,pos,focus) xrResult -> count:")
for k, c in gates.most_common():
    print("  ", k, c)
print("BAD gated/result submit rows:", len(bad))
for b in bad[:10]:
    print("  ", b)

# 4) tail of session: last 25 lines
print("\n--- last 25 lines ---")
for l in lines[-25:]:
    print(" ", l[:170])

# 5) any other haptics-adjacent diag lines
diag = [(i, l) for i, l in enumerate(lines) if re.search(r"haptic|rumble", l, re.I) and ("Trace" not in l)]
print("\nnon-Trace haptics/rumble lines:", len(diag))
for i, l in diag[:12]:
    print("  line", i + 1, ":", l.strip()[:170])

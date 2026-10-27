import hashlib, io, re

BIN = r"C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries"
live = open(BIN + r"\texture_override.ini", "rb").read()
print("live bytes:", len(live), "sha:", hashlib.sha256(live).hexdigest().upper()[:16])
print("starts with BOM:", live[:3] == b"\xef\xbb\xbf")
text = live.decode("utf-8-sig")
m = re.search(r"(?m)^bridge_owned=1\r?$", text)
com = text[: m.end()] + "\r\npriority_material_mode=1" + text[m.end():]
cand = com.encode("utf-8")
print("recomputed:", len(cand), "sha:", hashlib.sha256(cand).hexdigest().upper()[:16])
n = min(len(cand), len(live))
for i in range(n):
    if cand[i] != live[i]:
        print("first diff at byte", i)
        print("  cand:", cand[max(0, i-24):i+16])
        print("  live:", live[max(0, i-24):i+16])
        break
else:
    print("common prefix equal; length diff", len(cand) - len(live))

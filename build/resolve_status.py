import io, re

P = r"C:\Users\Trilex\vr-mods\SingularityVR-integration\STATUS.md"
lines = io.open(P, encoding="utf-8", newline="").read().split("\n")
i_s = next(i for i, l in enumerate(lines) if l.startswith("<<<<<<<"))
i_m = next(i for i, l in enumerate(lines) if l.rstrip() == "=======")
i_e = next(i for i, l in enumerate(lines) if l.startswith(">>>>>>>"))
head = lines[i_s + 1:i_m]   # diegetic/HEAD section (incl. trailing blank)
side = lines[i_m + 1:i_e]   # icons section
out = lines[:i_s] + side + head + lines[i_e + 1:]
txt = "\n".join(out)
assert "<<<<<<<" not in txt and ">>>>>>>" not in txt
assert "## 2026-10-24" in txt and txt.count("## 2026-10-24") >= 3
io.open(P, "w", encoding="utf-8", newline="").write(txt)
print("resolved: head-side lines", len(head), "icons-side lines", len(side))
print("top headings in order:", [l[:64] for l in out if l.startswith("## ")][:4])

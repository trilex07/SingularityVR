import io
p = "STATUS.md"
t = io.open(p, encoding="utf-8").read()
h = "# STATUS — session handoff\n\n"
entry = (
    "## 2026-10-24 — M02 P2 wheel PHYSICAL PASS + M09 TMD pulses PHYSICAL PASS"
    " (single combined session; receipts + user)\n\n"
    "- Archived `docs/evidence/m02-m09-combined-20261024/` (log `305C4C2D…96C4`,"
    " attached 10:12:09 ends 10:13:23; dll `E304F225…3C6`; INI `DA63A2A9…9FBC`"
    " Wheel=1 + `TmdPulses=1`). Root cause PROVEN: the old `IsLive`-early-return"
    " rejected a READABLE receiver (`live=0`, `class=RvTMDPower_Impulse` resolves"
    " fine) → `decision=Impulse`, `EVENT accepted=1 armed=1` (cooldown ret=3"
    " correctly refused), `SUBMIT overlay=0.800 xrResult=0`, `STOP result=0`,"
    " 0 NON-LOCAL, 0 XINPUT, wheel failure scan 0. User: \"PASS, everything"
    " works.\" Grades: **M02 P2 wheel PHYSICAL PASS**; **M09 TMD pulses PHYSICAL"
    " PASS**; weapon icon detail = ACCEPTED COSMETIC FOLLOW-UP (SOL).\n"
)
assert t.startswith(h) and not t.startswith(h + entry)
io.open(p, "w", encoding="utf-8", newline="").write(h + entry + t[len(h):])
print("prepended")

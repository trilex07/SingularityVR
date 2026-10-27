import io

p = r"spikes\view_matrix\tests\test-diegetic-centurion.cpp"
t = io.open(p, encoding="utf-8").read()
t = t.replace("""    auto sameCell = [&](int qi, const Cell& c) {
        return Near(q[qi].u0, c.x / AtlasW(), 1e-6f) && Near(q[qi].v0, c.y / AtlasH(), 1e-6f);
    };""",
"""    auto sameCell = [](const Quad& a, const Cell& c) {
        return Near(a.u0, c.x / AtlasW(), 1e-6f) && Near(a.v0, c.y / AtlasH(), 1e-6f);
    };""")
t = t.replace("CHECK(r == 24 && h[12] == 'P' && h[13] == 'N' && h[14] == 'G');",
              "CHECK(r == 24 && h[12] == 'I' && h[13] == 'H' && h[14] == 'D'); // IHDR chunk")
pairs = [
    ("CHECK(sameCell(1, Digit(false, 0)));  // leading zero of", "CHECK(sameCell(q[1], Digit(false, 0)));  // leading zero of"),
    ("CHECK(sameCell(2, Digit(false, 6)));", "CHECK(sameCell(q[2], Digit(false, 6)));"),
    ("CHECK(sameCell(3, Slash()));", "CHECK(sameCell(q[3], Slash()));"),
    ("CHECK(sameCell(4, Digit(false, 2)));", "CHECK(sameCell(q[4], Digit(false, 2)));"),
    ("CHECK(sameCell(5, Digit(false, 4)));", "CHECK(sameCell(q[5], Digit(false, 4)));"),
]
for a, b in pairs:
    assert a in t, a
    t = t.replace(a, b)
a = """    for (int i = 0; i < 6; ++i)
        printf("case7 q%d: u %.6f v %.6f (x %.1f y %.1f)\\n", i, lq[i].u0, lq[i].v0, Digit(true, i).x, Digit(true, i).y);
    CHECK(sameCell(1, Digit(true, 0)) && sameCell(2, Digit(true, 2)));"""
b = "    CHECK(sameCell(lq[1], Digit(true, 0)) && sameCell(lq[2], Digit(true, 2)));"
assert a in t
t = t.replace(a, b)
a = """    for (int i = 0; i < 7; ++i)
        printf("case9 q%d: u %.6f v %.6f\\n", i, rq[i].u0, rq[i].v0);
    CHECK(sameCell(4, Digit(false, 1)) && sameCell(5, Digit(false, 2)) && sameCell(6, Digit(false, 0)));"""
b = "    CHECK(sameCell(rq[4], Digit(false, 1)) && sameCell(rq[5], Digit(false, 2)) && sameCell(rq[6], Digit(false, 0)));"
assert a in t
t = t.replace(a, b)
io.open(p, "w", encoding="utf-8", newline="").write(t)
print("fixture patched")

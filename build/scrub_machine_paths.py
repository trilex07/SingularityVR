import io, subprocess, os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

def tracked():
    out = subprocess.run(["git", "ls-files", "build"], cwd=ROOT, capture_output=True, text=True).stdout
    return [l.strip() for l in out.splitlines() if l.strip()]

py_root_expr = "os.path.dirname(os.path.dirname(os.path.abspath(__file__)))"
repl = {
    'r"C:\\Users\\Trilex\\vr-mods\\SingularityVR-m02-m09"': py_root_expr,
    'r"C:\\Users\\Trilex\\vr-mods\\SingularityVR-m02-remaster"': py_root_expr,
    'r"C:\\Users\\Trilex\\vr-mods\\SingularityVR-m09-current"': py_root_expr,
    'C:\\Users\\Trilex\\vr-mods\\SingularityVR-m02-m09\\spikes\\view_matrix': '$($root)\\spikes\\view_matrix',
    'C:\\Users\\Trilex\\vr-mods\\SingularityVR-m02-remaster\\spikes\\view_matrix': '$($root)\\spikes\\view_matrix',
}
changed = []
for f in tracked():
    if not (f.endswith(".py") or f.endswith(".ps1")):
        continue
    p = os.path.join(ROOT, f)
    t = io.open(p, encoding="utf-8").read()
    orig = t
    for a, b in repl.items():
        t = t.replace(a, b)
    if t != orig:
        io.open(p, "w", encoding="utf-8", newline="").write(t)
        changed.append(f)
# make the py modules import os if the expression needs it and 'import os' is absent
for f in changed:
    if f.endswith(".py"):
        p = os.path.join(ROOT, f)
        t = io.open(p, encoding="utf-8").read()
        if "import os" not in t:
            t = t.replace("import json", "import json, os", 1)
            t = t.replace("import io", "import io, os", 1)
            t = t.replace("import re", "import re, os", 1)
            io.open(p, "w", encoding="utf-8", newline="").write(t)
        import ast
        ast.parse(io.open(p, encoding="utf-8").read())
print("scrubbed:", ", ".join(changed) if changed else "(none)")

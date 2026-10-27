import io, re
for f in [r"spikes\view_matrix\tests\test-ui-remaster-d3d9.cpp",
          r"spikes\view_matrix\tests\test-ui-remaster-runtime.cpp"]:
    t = io.open(f, encoding="utf-8").read()
    inc = re.findall(r'#include [<"]([^">]+)[">]', t)
    lib = re.findall(r'#pragma comment\(lib,\s*["\']([\w.]+)["\']\)', t)
    m = re.search(r"int main\(([^)]*)\)", t)
    print(f, "| main:", m.group(1) if m else None, "| libs:", lib,
          "| tail-includes:", ",".join(i.split("/")[-1] for i in inc)[-150:])

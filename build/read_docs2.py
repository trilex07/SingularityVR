import subprocess

def show(ref_and_path, cwd=r"C:\Users\Trilex\vr-mods\SingularityVR-diegetic"):
    r = subprocess.run(["git", "show", ref_and_path], capture_output=True, cwd=cwd)
    return r.stdout.decode("utf-8", errors="replace")

t = show("5cc9150ac8bf:docs/handoffs/side-material-remaster-d3d9ex-20261024.md")
print("=== side-material-remaster handoff ===")
print(t[:12000].replace("\r\n", "\n"))

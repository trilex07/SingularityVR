import subprocess

def show(ref_and_path, cwd=r"C:\Users\Trilex\vr-mods\SingularityVR-diegetic"):
    r = subprocess.run(["git", "show", ref_and_path], capture_output=True, cwd=cwd)
    return r.stdout.decode("utf-8", errors="replace")

t = show("eade95ac14f1:docs/handoffs/side-vr-weapon-accuracy.md")
print("=== side-vr-weapon-accuracy.md ===")
print(t[:9000].replace("\r\n", "\n"))

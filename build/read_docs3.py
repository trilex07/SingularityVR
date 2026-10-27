import subprocess

def git(args, cwd=r"C:\Users\Trilex\vr-mods\SingularityVR-diegetic"):
    r = subprocess.run(["git"] + args, capture_output=True, cwd=cwd)
    return r.stdout.decode("utf-8", errors="replace") + r.stderr.decode("utf-8", errors="replace")

print("=== 3223b241 stat ===")
print(git(["show", "3223b241e357", "--stat", "--format=medium"])[:2600].replace("\r\n", "\n"))

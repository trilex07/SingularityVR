import subprocess, hashlib

def blobref(ref, cwd=r"C:\Users\Trilex\vr-mods\SingularityVR-diegetic"):
    return subprocess.run(["git", "show", ref], capture_output=True, cwd=cwd).stdout

b = blobref("3223b241e357:assets/processed/ui_remaster/atlas/ui_atlas_neutral.png")
print("branch ui_atlas_neutral.png: bytes=%d sha256=%s" % (len(b), hashlib.sha256(b).hexdigest().upper()))
print("task-specified              :                    3EF1782696B4A5E1E9D93F409C5B0F6FF014B78CC42FAB06EC29110661981818")

old = blobref("3223b241e357~1:assets/processed/ui_remaster/atlas/ui_atlas_neutral.png")
print("previous ui_atlas_neutral   : bytes=%d sha256=%s" % (len(old), hashlib.sha256(old).hexdigest().upper()))

# live deployed wheel atlas
live = open(r"C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries\vr-assets\ui\remaster\ui_atlas_neutral.png", "rb").read()
print("live deployed ui_atlas_neutral: bytes=%d sha256=%s" % (len(live), hashlib.sha256(live).hexdigest().upper()))

t = subprocess.run(["git", "show", "origin/main:tools/integration/session-assets.ps1"], capture_output=True,
                   cwd=r"C:\Users\Trilex\vr-mods\SingularityVR-diegetic").stdout.decode("utf-8", errors="replace")
i = t.find("cnotmatch")
print("=== whitelist line(s) ===")
print(t[i-40:i+700].replace("\r\n", "\n"))

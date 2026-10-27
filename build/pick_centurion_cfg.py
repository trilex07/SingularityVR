import subprocess
b = subprocess.run(["git", "show", "ff29efd:tools/hud/prepare_diegetic_models_v2.py"],
                   capture_output=True, cwd=r"C:\Users\Trilex\vr-mods\SingularityVR-promo").stdout
t = b.decode("utf-8", errors="replace")
mark = "'id': 'centurion'"
i = t.find(mark, t.find("configs = ["))
print(t[i:i+800].replace("\r\n", "\n"))

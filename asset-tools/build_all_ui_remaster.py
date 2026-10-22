"""
Singularity VR - UI Utility Graphics Remaster Master Build Script
Executes the approved V2.1 production pipeline to rebuild all vector SVGs,
transparent PNG production slices (neutral and localized), texture atlases with UV maps,
and 12 presentation concept mockups (PL & EN).
"""

import os
import subprocess
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

def main():
    print("=====================================================================")
    print("Singularity VR — Building UI Remaster V2.1 Production Package")
    print("=====================================================================")
    master_script = os.path.join(SCRIPT_DIR, "build_v2_1_production.py")
    ret = subprocess.run([sys.executable, master_script], cwd=SCRIPT_DIR)
    if ret.returncode != 0:
        print(f"ERROR: build_v2_1_production.py failed with return code {ret.returncode}")
        sys.exit(ret.returncode)
    print("\n=====================================================================")
    print("SUCCESS: V2.1 Remaster Package Fully Rebuilt & Validated.")
    print("=====================================================================")

if __name__ == "__main__":
    main()

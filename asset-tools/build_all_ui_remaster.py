"""
Singularity VR - UI Utility Graphics Remaster Master Build Script
Executes all generator scripts in sequence to rebuild all SVG sources,
processed transparent PNG exports, and presentation concept mockups.
"""

import os
import subprocess
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

SCRIPTS = [
    "gen_weapon_wheel.py",
    "gen_weapon_wheel_svg.py",
    "gen_vr_settings.py",
    "gen_vr_settings_svg.py",
    "gen_pause_menu.py",
    "gen_pause_menu_svg.py",
    "gen_interaction_icons.py",
    "gen_interaction_icons_svg.py",
    "gen_laser_pointer.py",
    "gen_laser_pointer_svg.py",
    "gen_utility_hud.py",
    "gen_utility_hud_svg.py"
]

def main():
    print("===============================================================")
    print("Singularity VR — Building UI Utility Graphics Remaster Package")
    print("===============================================================")
    for script in SCRIPTS:
        script_path = os.path.join(SCRIPT_DIR, script)
        print(f"\n--- Running {script} ---")
        ret = subprocess.run([sys.executable, script_path], cwd=SCRIPT_DIR)
        if ret.returncode != 0:
            print(f"ERROR: {script} failed with return code {ret.returncode}")
            sys.exit(ret.returncode)
    print("\n===============================================================")
    print("SUCCESS: All UI Remaster assets and mockups rebuilt.")
    print("===============================================================")

if __name__ == "__main__":
    main()

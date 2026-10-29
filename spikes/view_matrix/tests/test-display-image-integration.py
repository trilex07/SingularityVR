"""Offline wiring regression for the five-row DISPLAY IMAGE page and XR pipeline."""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
cpp = (root / "d3d9.cpp").read_text(encoding="utf-8")
oled = (root / "oled_grading.h").read_text(encoding="utf-8")

page = "{ PR_NIS_SHARPEN, PR_NIS_STRENGTH, PR_OLED_PRESET, PR_TEXTURES, PR_BACK }"
assert page in cpp
assert "PR_VFXLOOK, PR_GOTO_NIS, PR_BACK" in cpp
counts = re.search(r"kPageCount\[kPanelPages\]\s*=\s*\{([^}]+)\}", cpp)
assert counts
counts = [int(v.strip()) for v in counts[1].split(',')]
assert len(counts) == 13 and counts[12] == 5  # unrelated pages may gain controls
assert "g_panelImageParent=g_panelPage; g_panelPage=12" in cpp
assert "g_panelPage == 12 ? g_panelImageParent : 0" in cpp
assert 'WritePrivateProfileStringA("OLED", "Preset", v, path)' in cpp
assert 'WritePrivateProfileStringA("Upscaling", "Mode"' in cpp
assert 'WritePrivateProfileStringA("Upscaling", "Sharpness"' in cpp
assert "case PR_TEXTURES:" in cpp
assert "next == 1 ? nis_upscale::kNISSharpen" in cpp
assert "next == 2 ? nis_upscale::kNIS" in cpp
assert "g_oledGrading.IsEnabled()" in cpp
assert "g_oledGrading.Process(g_dev11, g_ctx11, displaySrc" in cpp
assert "NisBridge_ProcessStereo(&g_nis, g_ctx11, displaySrc" in cpp
assert cpp.index("g_oledGrading.Process(g_dev11, g_ctx11, displaySrc") < cpp.index(
    "NisBridge_ProcessStereo(&g_nis, g_ctx11, displaySrc")
assert "sourceDesc.Format != DXGI_FORMAT_B8G8R8A8_UNORM" in oled
assert "sourceDesc.Width != width || sourceDesc.Height != height" in oled
print("DISPLAY rows, navigation, persistence, OFF bypass, OLED->NIS->XR, format gate: PASS")

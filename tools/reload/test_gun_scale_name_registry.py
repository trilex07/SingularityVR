"""Regression for the live UE3 property-name gate used by gun_scale.

The x86 MinHook fixture supplies synthetic offsets, so it cannot catch a
missing name in the real WantedIdx registry. This checks that exact
producer fields reach the registry before PropOffsetInChain is called.
"""
from pathlib import Path
import re


source = (Path(__file__).resolve().parents[2] / "spikes/view_matrix/d3d9.cpp").read_text(
    encoding="utf-8", errors="replace"
)
view_matrix = Path(__file__).resolve().parents[2] / "spikes/view_matrix"
assert '#include "native_wanted_names.h"' in source
registry = (view_matrix / "native_wanted_names.h").read_text(encoding="utf-8").split("WantedName g_wantNames[] = {", 1)[1].split("};", 1)[0]
names = set(re.findall(r'\{\s*"([^"]+)"\s*,\s*-1\s*\}', registry))
required = {"mGunScaleCtrl", "mInviewMesh", "ControlName", "BoneScale"}
assert required <= names, f"unregistered native gun_scale properties: {required - names}"
assert "const int32_t idx = WantedIdx(name);" in source
assert '#include "native_property_list.inl"' in source
assert "if (wantIdx < 0" in (view_matrix / "native_property_list.inl").read_text(encoding="utf-8")
print("PASS: all four gun_scale fields registered in the live WantedIdx gate")

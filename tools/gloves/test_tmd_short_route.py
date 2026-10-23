"""Static seam check paired with the C++ executable's real policy/dispatch test."""
from pathlib import Path

source = (Path(__file__).resolve().parents[2] / "spikes/view_matrix/d3d9.cpp").read_text(encoding="utf-8")
foreground = source.index("static bool FgHidden(IDirect3DDevice9* dev,UINT verts, UINT first, UINT triangles,\n")
foreground = source.index("static bool FgHidden(IDirect3DDevice9* dev,UINT verts, UINT first, UINT triangles,\n", foreground + 1)
admit = source.index("if(gate==tmdgauntlet::Gate::Pass)", foreground)
arm_hide = source.index("if (GlovesUntilWeaponNow() &&\n        gloveisolate::IsArmFleshSection", foreground)
assert foreground < admit < arm_hide, "SVTG admission must precede the arm-flesh early return"

hook = source.index("HRESULT STDMETHODCALLTYPE Hook_DrawIndexedPrim(", foreground)
hide = source.index("FgHidden(dev,numVertices, startIndex, primCount,minIndex,t,&shortTmdAdmitted)", hook)
prepare = source.index("const int independentRegs=t==D3DPT_TRIANGLELIST ? PrepareIndependentLeft", hook)
post = source.index("tmdgauntlet::AfterPalette(shortTmdAdmitted", prepare)
isolate = source.index("const bool isolateTmdHardware =", post)
dispatch = source.index("tmdgauntlet::DispatchBound(shortTmdBuffer!=nullptr", isolate)
restore = source.index("const HRESULT nativeRestore=nativeIndices.Restore()", dispatch)
assert hide < prepare < post < isolate < dispatch < restore, "foreground/palette/SVTG order regressed"
assert "if(shortTmdAdmitted && admittedGate!=tmdgauntlet::Gate::Pass)" in source[post:isolate]
print("PASS: real FgHidden -> PrepareIndependentLeft -> filtered SVTG dispatch order")

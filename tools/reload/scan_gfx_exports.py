"""Scan UE3 packages for GFxMovieInfo / locker-related export names."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "title"))
from ue3_layout import inspect  # noqa: E402


def scan(root: Path, cap_mb: float = 15.0) -> list:
    rows = []
    for p in sorted(root.glob("*.xxx")):
        if p.stat().st_size > cap_mb * 1024 * 1024:
            continue
        try:
            b = p.read_bytes()
            if b[:4] != b"\xc1\x83\x2a\x9e":
                continue
            d = inspect("x", b, include_all=True)
        except Exception:
            continue
        for e in d["exports"]:
            n = e["name"].lower()
            if e["class_name"] == "GFxMovieInfo" and any(
                k in n for k in ("locker", "loadout", "weapon")
            ):
                rows.append((p.name, e["name"], e["class_name"], e["size"]))
            elif "locker" in n or ("weapon" in n and "menu" in n):
                rows.append((p.name, e["name"], e["class_name"], e["size"]))
    return rows


if __name__ == "__main__":
    root = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(
        r"C:\Program Files (x86)\Steam\steamapps\common\Singularity\RvGame\CookedPC"
    )
    for row in scan(root):
        print("\t".join(str(x) for x in row))

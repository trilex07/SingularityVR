#!/usr/bin/env python3
"""Procedural hip holster + medkit pouch GLB (no PSK)."""
from __future__ import annotations

from pathlib import Path

import numpy as np
import trimesh

ROOT = Path(__file__).resolve().parents[2]
PACK = ROOT / "assets" / "hud" / "katorga" / "3d" / "hud"


def hip_pouch_mesh(belt_len: float = 14.0, outward: float = 7.0, vertical: float = 4.5):
    """Box pouch: X = along belt, Y = outward from body, Z = up on hip."""
    sx, sy, sz = belt_len * 0.5, outward * 0.5, vertical * 0.5
    corners = np.array(
        [
            [-sx, 0.0, -sz],
            [sx, 0.0, -sz],
            [sx, 0.0, sz],
            [-sx, 0.0, sz],
            [-sx, sy, -sz],
            [sx, sy, -sz],
            [sx, sy, sz],
            [-sx, sy, sz],
        ],
        dtype=np.float64,
    )
    faces = np.array(
        [
            [4, 5, 6],
            [4, 6, 7],
            [0, 2, 1],
            [0, 3, 2],
            [0, 1, 5],
            [0, 5, 4],
            [2, 7, 6],
            [2, 3, 7],
            [0, 4, 7],
            [0, 7, 3],
            [1, 2, 6],
            [1, 6, 5],
        ]
    )
    uvs = np.zeros((8, 2))
    uvs[[4, 5, 6, 7]] = [[0, 0], [1, 0], [1, 1], [0, 1]]
    mesh = trimesh.Trimesh(vertices=corners, faces=faces, process=False)
    mesh.visual = trimesh.visual.TextureVisuals(uv=uvs)
    return mesh


def export(path: Path, mesh: trimesh.Trimesh, slot_id: str, side: str):
    path.parent.mkdir(parents=True, exist_ok=True)
    scene = trimesh.Scene(mesh)
    scene.metadata = {"holster_slot": {"id": slot_id, "side": side, "outward_axis": "+Y_local"}}
    scene.export(path, file_type="glb")
    print("wrote", path, "bytes", path.stat().st_size)


def main():
    weapon = hip_pouch_mesh(belt_len=15.0, outward=8.0, vertical=5.0)
    medkit = hip_pouch_mesh(belt_len=11.0, outward=7.5, vertical=6.0)
    # Left pouch: mirror so outward (+Y) points to body's -Y (left).
    medkit.vertices[:, 1] *= -1.0
    medkit.faces = np.fliplr(medkit.faces)

    export(PACK / "holster_weapon_right.glb", weapon, "weapon_holster_right", "right")
    export(PACK / "holster_medkit_left.glb", medkit, "medkit_pouch_left", "left")


if __name__ == "__main__":
    main()

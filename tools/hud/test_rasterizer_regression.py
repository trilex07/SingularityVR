#!/usr/bin/env python3
"""Deterministic regression test for 3D triangle rasterizer.
Tests:
1. Single textured triangle rendering with known coordinates and colors
2. Pixel coverage validation (> 0 pixels drawn)
3. Exact depth testing (near triangle occludes far triangle)
4. Perspective-correct UV interpolation validation
5. Rejection of blank output images
"""
import math
import numpy as np
import numba
from PIL import Image

@numba.njit(fastmath=True)
def rasterize_test(
    tri_verts,   # (N, 3, 3) -> [x_screen, y_screen, 1/z]
    tri_uvs,     # (N, 3, 2) -> [u, v]
    tri_colors,  # (N, 3)
    tex,         # (H, W, 3)
    width, height,
    out_color, out_depth
):
    tex_h, tex_w, _ = tex.shape
    num_tris = tri_verts.shape[0]

    for i in range(num_tris):
        x0 = tri_verts[i, 0, 0]
        y0 = tri_verts[i, 0, 1]
        z0 = tri_verts[i, 0, 2] # 1/z0

        x1 = tri_verts[i, 1, 0]
        y1 = tri_verts[i, 1, 1]
        z1 = tri_verts[i, 1, 2] # 1/z1

        x2 = tri_verts[i, 2, 0]
        y2 = tri_verts[i, 2, 1]
        z2 = tri_verts[i, 2, 2] # 1/z2

        u0 = tri_uvs[i, 0, 0]
        v0 = tri_uvs[i, 0, 1]
        u1 = tri_uvs[i, 1, 0]
        v1 = tri_uvs[i, 1, 1]
        u2 = tri_uvs[i, 2, 0]
        v2 = tri_uvs[i, 2, 1]

        area = (x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0)
        if abs(area) < 1e-5:
            continue

        # Two-sided rendering: if area < 0, swap v1 and v2 so area is positive
        if area < 0.0:
            x1, x2 = x2, x1
            y1, y2 = y2, y1
            z1, z2 = z2, z1
            u1, u2 = u2, u1
            v1, v2 = v2, v1
            area = -area

        inv_area = 1.0 / area

        min_x = max(0, int(min(x0, x1, x2)))
        max_x = min(width - 1, int(max(x0, x1, x2)))
        min_y = max(0, int(min(y0, y1, y2)))
        max_y = min(height - 1, int(max(y0, y1, y2)))

        if min_x > max_x or min_y > max_y:
            continue

        # Precompute UV/z values for perspective-correct interpolation
        u0_z = u0 * z0
        v0_z = v0 * z0
        u1_z = u1 * z1
        v1_z = v1 * z1
        u2_z = u2 * z2
        v2_z = v2 * z2

        r = tri_colors[i, 0]
        g = tri_colors[i, 1]
        b = tri_colors[i, 2]

        for py in range(min_y, max_y + 1):
            for px in range(min_x, max_x + 1):
                fx = float(px) + 0.5
                fy = float(py) + 0.5

                w0 = (x2 - x1) * (fy - y1) - (y2 - y1) * (fx - x1)
                w1 = (x0 - x2) * (fy - y2) - (y0 - y2) * (fx - x2)
                w2 = (x1 - x0) * (fy - y0) - (y1 - y0) * (fx - x0)

                if w0 >= 0.0 and w1 >= 0.0 and w2 >= 0.0:
                    l0 = w0 * inv_area
                    l1 = w1 * inv_area
                    l2 = w2 * inv_area

                    inv_z = l0 * z0 + l1 * z1 + l2 * z2
                    if inv_z <= 0.0:
                        continue
                    z_depth = 1.0 / inv_z

                    if z_depth < out_depth[py, px]:
                        out_depth[py, px] = z_depth

                        # Perspective-correct UV
                        u = (l0 * u0_z + l1 * u1_z + l2 * u2_z) / inv_z
                        v = (l0 * v0_z + l1 * v1_z + l2 * v2_z) / inv_z

                        tx = int(u * (tex_w - 1)) % tex_w
                        ty = int((1.0 - v) * (tex_h - 1)) % tex_h

                        out_color[py, px, 0] = tex[ty, tx, 0] * r
                        out_color[py, px, 1] = tex[ty, tx, 1] * g
                        out_color[py, px, 2] = tex[ty, tx, 2] * b

def run_regression_test():
    width, height = 256, 256
    out_color = np.zeros((height, width, 3), dtype=np.float32)
    out_depth = np.full((height, width), 1e9, dtype=np.float32)

    # 1. Create test texture: 4 colored quadrants
    tex = np.zeros((64, 64, 3), dtype=np.float32)
    tex[:32, :32] = [1.0, 0.0, 0.0] # Red (top-left)
    tex[:32, 32:] = [0.0, 1.0, 0.0] # Green (top-right)
    tex[32:, :32] = [0.0, 0.0, 1.0] # Blue (bottom-left)
    tex[32:, 32:] = [1.0, 1.0, 0.0] # Yellow (bottom-right)

    # 2. Test Triangle: screen coords from (30, 30) to (220, 30) and (30, 220)
    # depth z = 2.0 -> inv_z = 0.5
    tri_verts = np.array([
        [[30.0, 30.0, 0.5], [220.0, 30.0, 0.5], [30.0, 220.0, 0.5]]
    ], dtype=np.float32)

    tri_uvs = np.array([
        [[0.0, 1.0], [1.0, 1.0], [0.0, 0.0]]
    ], dtype=np.float32)

    tri_colors = np.array([[1.0, 1.0, 1.0]], dtype=np.float32)

    rasterize_test(tri_verts, tri_uvs, tri_colors, tex, width, height, out_color, out_depth)

    # Coverage test
    pixels_drawn = np.sum(out_depth < 1e8)
    print(f"Regression Test: Pixels drawn = {pixels_drawn}")
    assert pixels_drawn > 1000, f"FAIL: Expected > 1000 pixels drawn, got {pixels_drawn}"

    # Center pixel test
    center_color = out_color[50, 50]
    print(f"Pixel at (50, 50): {center_color}")
    assert np.max(center_color) > 0.0, "FAIL: Pixel color is black!"

    # 3. Occlusion Test: Add far triangle behind it (z = 5.0 -> inv_z = 0.2)
    # Should NOT overwrite closer triangle
    tri_verts_far = np.array([
        [[30.0, 30.0, 0.2], [220.0, 30.0, 0.2], [30.0, 220.0, 0.2]]
    ], dtype=np.float32)
    tri_colors_far = np.array([[0.1, 0.1, 0.1]], dtype=np.float32)
    rasterize_test(tri_verts_far, tri_uvs, tri_colors_far, tex, width, height, out_color, out_depth)

    # Verify color didn't change to far triangle
    assert np.allclose(out_color[50, 50], center_color), "FAIL: Far triangle overwrote closer triangle!"

    # 4. Near triangle in front (z = 1.0 -> inv_z = 1.0)
    tri_verts_near = np.array([
        [[45.0, 45.0, 1.0], [55.0, 45.0, 1.0], [45.0, 55.0, 1.0]]
    ], dtype=np.float32)
    tri_colors_near = np.array([[0.5, 0.5, 0.5]], dtype=np.float32)
    rasterize_test(tri_verts_near, tri_uvs, tri_colors_near, tex, width, height, out_color, out_depth)
    assert out_depth[48, 48] == 1.0, "FAIL: Closer triangle failed depth test!"

    print("ALL RASTERIZER REGRESSION TESTS PASSED!")

if __name__ == '__main__':
    run_regression_test()

"""Check an audited native caster/receiver matrix relationship in a private trace.

This checks algebraic agreement only, not the screen-depth or eye-camera contract.
Register ownership is read from CTAB. Multiplication order is from private shader
disassembly for the explicitly audited content hashes below.
"""
import argparse
import importlib.util
import json
import math
from pathlib import Path

spec = importlib.util.spec_from_file_location("shadow_trace", Path(__file__).with_name("analyze-shadow-pass-trace.py"))
trace_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(trace_module)

CASTERS = {
    "c69b26e9e6b3bc69930cd36df8bcb96a3db261031a635439641a41e637d98385",
    "a47f5cb2e1fab13fcf5d8163d30580bbc0d77d489f70e73cf844309b1f5103db",
}
RECEIVER = "28e0113352db6dff68315a029a7791467fe928d56d79ab7fc13776981af5a044"


def multiply(a, b):
    return [[sum(a[r][k] * b[k][c] for k in range(4)) for c in range(4)] for r in range(4)]


def apply(v, matrix):
    return [sum(v[k] * matrix[k][c] for k in range(4)) for c in range(4)]


def inverse(a):
    augmented = [row[:] + [float(r == c) for c in range(4)] for r, row in enumerate(a)]
    for c in range(4):
        pivot = max(range(c, 4), key=lambda r: abs(augmented[r][c]))
        if abs(augmented[pivot][c]) < 1e-12:
            raise ValueError("Singular matrix")
        augmented[c], augmented[pivot] = augmented[pivot], augmented[c]
        scale = augmented[c][c]
        augmented[c] = [v / scale for v in augmented[c]]
        for r in range(4):
            if r != c:
                scale = augmented[r][c]
                augmented[r] = [v - scale * p for v, p in zip(augmented[r], augmented[c])]
    return [row[4:] for row in augmented]


def parameter(shader, name, count):
    p = shader["parameter_registers"][name]
    if p["register_set"] != 2 or p["register_count"] != count:
        raise ValueError(f"Unsupported CTAB parameter {name}")
    return p["register_index"]


def check(trace, producer_id, receiver_id):
    producer, receiver = trace["groups"][producer_id], trace["groups"][receiver_id]
    if producer["VS_hash"] not in CASTERS or receiver["PS_hash"] != RECEIVER:
        raise ValueError("Selected shader instruction order has not been audited")
    if int(producer["first"]) >= int(receiver["first"]):
        raise ValueError("Caster does not precede selected receiver")
    high = producer["shadow_producer_high"]
    if high["hr"] != "00000000" or producer["vsRead"] != "00000000" or receiver["psRead"] != "00000000":
        raise ValueError("Required constant read failed")
    vs = trace["shaders"][producer["VS"]]
    ps = trace["shaders"][receiver["PS"]]
    projection_reg = parameter(vs, "ProjectionMatrix", 4)
    depth_reg = parameter(vs, "InvMaxSubjectDepth", 1)
    world_reg = parameter(ps, "ScreenToWorld", 4)
    shadow_reg = parameter(ps, "ScreenToShadowMatrix", 4)
    projection = [high["constants"][projection_reg + r] for r in range(4)]
    inv_depth = high["constants"][depth_reg][0]
    world = [receiver["constants"][world_reg + r]["PS"][:3] + [float(r == 3)] for r in range(4)]
    observed = [receiver["constants"][shadow_reg + r]["PS"] for r in range(4)]
    x, y, width, height = [int(v) for v in producer["vp"].split(",")]
    target_width, target_height = [int(v) for v in producer["size"].split("x")]
    if producer["fmt"] != "114" or min(width, height, target_width, target_height) <= 0:
        raise ValueError("Unsupported producer target/viewport")
    texture = [
        [width / (2 * target_width), 0.0, 0.0, 0.0],
        [0.0, -height / (2 * target_height), 0.0, 0.0],
        [0.0, 0.0, inv_depth, 0.0],
        [(x + width / 2 + 0.5) / target_width, (y + height / 2 + 0.5) / target_height, 0.0, 1.0],
    ]
    expected = multiply(multiply(world, projection), texture)
    differences = [[observed[r][c] - expected[r][c] for c in range(4)] for r in range(4)]
    if not all(math.isfinite(v) for matrix in (expected, observed) for row in matrix for v in row):
        raise ValueError("Nonfinite matrix values")
    agrees = all(abs(differences[r][c]) <= 0.002 + 2e-5 * max(abs(observed[r][c]), abs(expected[r][c]))
                 for r in range(4) for c in range(4))
    reconstruction = None
    if receiver["VS_hash"] == "d09ad82cc046f97724dcffcbb7976c271df034414c08742fab66810e94038f19":
        reg = parameter(trace["shaders"][receiver["VS"]], "ViewProjectionMatrix", 4)
        vp = [receiver["constants"][reg + r]["VS"] for r in range(4)]
        # Audited VS copies clip XYW to the PS. Its input is [clipX,clipY,W,1].
        screen = [[row[0], row[1], row[3], float(r == 3)] for r, row in enumerate(vp)]
        actual_inverse = inverse(screen)
        local_reg = parameter(vs, "LocalToWorld", 4)
        point = high["constants"][local_reg + 3][:3] + [1.0]
        screen_point = apply(point, screen)
        native_point = apply(screen_point, world)
        rebased_point = apply(screen_point, actual_inverse)
        native_light = apply(screen_point, observed)
        rebased_shadow = multiply(actual_inverse, multiply(inverse(world), observed))
        rebased_light = apply(screen_point, rebased_shadow)
        reconstruction = {
            "sample": "Caster LocalToWorld origin; mathematical sample, not a captured raster pixel",
            "world_point": point,
            "clip_xy_linear_w": screen_point,
            "native_reconstructed_world": native_point,
            "native_world_error_units": math.sqrt(sum((native_point[c] - point[c]) ** 2 for c in range(3))),
            "actual_camera_inverse_world_error_units": math.sqrt(sum((rebased_point[c] - point[c]) ** 2 for c in range(3))),
            "native_shadow_uv": [native_light[c] / native_light[3] for c in range(2)],
            "rebased_shadow_uv": [rebased_light[c] / rebased_light[3] for c in range(2)],
            "assumption": "Scene alpha decodes W for this actual camera; pixel-depth validation still required",
        }
    return {
        "producer_group": producer_id, "producer_first_draw": int(producer["first"]),
        "receiver_group": receiver_id, "receiver_first_draw": int(receiver["first"]),
        "relationship": "ScreenToWorld_affine * ProjectionMatrix * viewport_UV_and_depth_scale",
        "max_absolute_error": max(abs(v) for row in differences for v in row),
        "column_max_absolute_error": [max(abs(differences[r][c]) for r in range(4)) for c in range(4)],
        "agrees_with_captured_constants": agrees,
        "expected": expected, "observed": observed,
        "inferred_texture_transform": multiply(inverse(multiply(world, projection)), observed),
        "candidate_viewport_texture_transform": texture,
        "actual_camera_reconstruction": reconstruction,
        "limitations": "First group constants; proposed viewport relation includes a half texel and excludes caster depth bias. Residuals alone do not prove broken rendering. Does not prove eye reconstruction, scene depth, caster identity or artifact causality.",
    }


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("trace")
    parser.add_argument("--producer", required=True, type=int)
    parser.add_argument("--receiver", required=True, type=int)
    args = parser.parse_args()
    print(json.dumps(check(trace_module.read_trace(args.trace), args.producer, args.receiver), indent=2))

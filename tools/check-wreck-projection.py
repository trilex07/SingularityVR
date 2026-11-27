"""Compare the damaged Huey's depth and color draws using a locally exported ActorX mesh."""
import argparse
import importlib.util
import json
import math
from pathlib import Path
import re
import statistics
import struct


DEPTH_VS = "080f2d4e51dab73a6945e2944c829a7e5e6f2e13ae97119442a58b3f5a818fb0"
COLOR_VS = "aa2a42fc10cac42b033ce3870045e18c579263d27ff26bba7735f57376a4db21"
spec = importlib.util.spec_from_file_location("shadow_trace", Path(__file__).with_name("analyze-shadow-pass-trace.py"))
shadow_trace = importlib.util.module_from_spec(spec)
spec.loader.exec_module(shadow_trace)


def mesh_points(path):
    data = path.read_bytes()
    pos = 0
    while pos < len(data):
        name, _, size, count = struct.unpack_from("<20siii", data, pos)
        pos += 32
        end = pos + size * count
        if size < 0 or count < 0 or end > len(data):
            raise ValueError("Invalid ActorX chunk")
        if name.rstrip(b"\0") == b"PNTS0000":
            if size != 12 or not count:
                raise ValueError("Invalid ActorX points")
            return list(struct.iter_unpack("<fff", data[pos:end]))
        pos = end
    raise ValueError("Missing ActorX points")


def fields(line):
    return dict(part.split("=", 1) for part in line.split() if "=" in part)


def project(point, regs, viewport):
    # Both verified vertex programs use identical row-vector position instructions.
    vertex = (*point, 1)
    world = [sum(vertex[r] * regs[r + 5][c] for r in range(4)) for c in range(4)]
    clip = [sum(world[r] * regs[r][c] for r in range(4)) for c in range(4)]
    if not all(map(math.isfinite, clip)) or clip[3] <= 0:
        raise ValueError("Mesh extends outside the supported front-facing comparison")
    x, y, w, h = viewport
    return (x + .5 * w * (1 + clip[0] / clip[3]),
            y + .5 * h * (1 - clip[1] / clip[3]),
            clip[2] / clip[3])


def compare(path, points):
    text = path.read_text()
    counts = re.search(r"^wreck-draw-count=(\d+) overflow=(\d+)$", text, re.M)
    draws = [fields(s) for s in text.splitlines() if s.startswith("wreck-draw=")]
    if not counts or int(counts[2]) or int(counts[1]) != len(draws) or len(draws) != 4:
        raise ValueError("Expected exactly four individually captured hull draws")
    trace = shadow_trace.read_trace(path)
    if not trace["complete_budget"]:
        raise ValueError("Incomplete frame capture")
    events = {event["draw"]: event for event in trace["events"]}
    eyes = {}
    for draw in draws:
        event = events.get(draw["wreck-draw"], {})
        expected = {"group": draw["group"], "type": "4", "verts": "3440", "prims": "4093"}
        if any(event.get(key) != value for key, value in expected.items()):
            raise ValueError("Recorded state does not belong to the expected hull draw")
        g = trace["groups"][int(draw["group"])]
        pipe = g["pipeline"]
        if any(int(draw[k],16) for k in ("viewportRead","vbRead","ibRead","constantsRead")):
            raise ValueError("Draw-state read failed")
        if not int(draw["VB"], 16) or not int(draw["IB"], 16):
            raise ValueError("No bound vertex or index buffer")
        if g["failed"] != "0" or pipe["valid"] != "1" or pipe["zWrite"] != "1":
            raise ValueError("Unverified submitted draw")
        stages = {("0",DEPTH_VS):"depth", ("15",COLOR_VS):"color"}
        stage = stages.get((pipe["colorWrite"], g["VS_hash"]))
        if stage is None:
            raise ValueError("Unaudited shader or color-write mask")
        viewport = tuple(map(int,draw["vp"].split(",")))
        x,y,w,h = viewport
        rect = tuple(map(int,g["sc"].split(",")))
        if rect not in [(x,y,x+w//2,y+h),(x+w//2,y,x+w,y+h)]:
            raise ValueError("Unexpected eye scissor")
        eye = "left" if rect[0] == x else "right"
        regs = {int(r):list(map(float, v.split(","))) for r,v in re.findall(
            rf"^wreck-constant draw={draw['wreck-draw']} reg=(\d+) VS=(\S+)$", text, re.M)}
        pair = eyes.setdefault(eye,{})
        if stage in pair or set(regs) != set(range(9)):
            raise ValueError("Ambiguous draw or incomplete transform")
        pair[stage] = draw, [project(p,regs,viewport) for p in points]
    result = {}
    for eye in ("left","right"):
        depth, a = eyes[eye]["depth"]
        color, b = eyes[eye]["color"]
        if int(depth["wreck-draw"]) >= int(color["wreck-draw"]):
            raise ValueError("Depth did not precede color")
        distances = [math.hypot(p[0]-q[0], p[1]-q[1]) for p,q in zip(a,b)]
        # A shift along the view ray can preserve XY and still change occlusion.
        # This is vertex output before viewport depth mapping and raster depth bias.
        depth_deltas = [abs(p[2]-q[2]) for p,q in zip(a,b)]
        result[eye] = {"depth_draw":int(depth["wreck-draw"]), "color_draw":int(color["wreck-draw"]),
                       "same_ib":depth["IB"]==color["IB"], "same_vb":depth["VB"]==color["VB"],
                       "same_stream_layout":(depth["offset"],depth["stride"])==(color["offset"],color["stride"]),
                       "points":len(points), "median_delta_px":statistics.median(distances),
                       "max_delta_px":max(distances),
                       "max_abs_delta_ndc_z":max(depth_deltas)}
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace",type=Path)
    parser.add_argument("mesh",type=Path)
    args = parser.parse_args()
    print(json.dumps(compare(args.trace,mesh_points(args.mesh)), indent=2))

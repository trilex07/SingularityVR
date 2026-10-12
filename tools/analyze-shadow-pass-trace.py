"""Read private bounded traces; compare content hashes, never cross-session COM addresses.

Producer/consumer edges are candidates, not proof a pass generates artifact pixels.
"""
import argparse
import hashlib
import json
import re
import struct
from pathlib import Path


def read_trace(path, shader_out=None):
    text = Path(path).read_text(encoding="utf-8", errors="replace")
    header = re.search(r"draws=(\d+) groups=(\d+) overflow=(\d+)", text)
    if not header:
        raise ValueError("Not a shadow-pass-trace-v1 file")
    groups, shaders = {}, {}
    copies, clears = [], []
    lines = text.splitlines()
    for i, line in enumerate(lines):
        if line.startswith("copy "):
            copies.append(dict(re.findall(r"(\w+)=([^ ]+)",line)))
        elif line.startswith("clear "):
            clears.append(dict(re.findall(r"(\w+)=([^ ]+)",line)))
        elif line.startswith("group="):
            data = dict(re.findall(r"(\w+)=([^ ]+)", line))
            data["textures"] = {}
            groups[int(data["group"])] = data
        elif line.startswith("texture group="):
            data = dict(re.findall(r"(\w+)=([^ ]+)", line))
            groups[int(data["group"])]["textures"][data["stage"]] = data
        elif line.startswith("geometry group="):
            data = dict(re.findall(r"(\w+)=([^ ]+)", line))
            groups[int(data["group"])]["geometry"] = data
        elif line.startswith("pipeline group="):
            data=dict(re.findall(r"(\w+)=([^ ]+)",line))
            if data.get("valid")=="1":
                data["writes_alpha"]=bool(int(data["colorWrite"]) & 8)
            groups[int(data["group"])]["pipeline"]=data
        elif line.startswith(("constant group=", "tailconstant group=")):
            match=re.match(r"(?:tail)?constant group=(\d+) reg=(\d+) VS=([^ ]+) PS=([^ ]+)",line)
            if match:
                target="tail_constants" if line.startswith("tailconstant") else "constants"
                groups[int(match[1])].setdefault(target,{})[int(match[2])] = {
                    "VS": [float(v) for v in match[3].split(",")],
                    "PS": [float(v) for v in match[4].split(",")],
                }
        elif line.startswith("shader "):
            match = re.match(r"shader (\S+) stage=(\w+) words=(\d+)", line)
            if not match:
                continue
            words = []
            for code in lines[i + 1:]:
                if not re.fullmatch(r"(?:[0-9A-F]{8}[ \t]*)+", code):
                    break
                words.extend(int(w, 16) for w in code.split())
            if len(words) != int(match[3]):
                raise ValueError(f"Incomplete shader {match[1]}")
            raw = struct.pack("<" + "I" * len(words), *words)
            digest=hashlib.sha256(raw).hexdigest()
            if shader_out:
                output=Path(shader_out)
                output.mkdir(parents=True,exist_ok=True)
                (output / (match[2]+"-"+digest+".bin")).write_bytes(raw)
            names = re.findall(rb"[A-Za-z_][A-Za-z0-9_]{4,80}\x00", raw)
            shaders[match[1]] = {
                "stage": match[2], "sha256": digest,
                "words": len(words), "strings": sorted({s[:-1].decode() for s in names}),
            }
    edges = []
    for p, producer in groups.items():
        owner = producer["owner"]
        if not owner.strip("0") or owner in {"(nil)", "00000000"}:
            continue
        for c, consumer in groups.items():
            for stage, texture in consumer["textures"].items():
                if texture["id"] == owner and int(producer["first"]) < int(consumer["last"]):
                    edges.append({"producer_group": p, "consumer_group": c, "stage": stage,
                                  "resource": owner, "status": "candidate_edge"})
    for group in groups.values():
        for stage in ("VS", "PS"):
            group[stage + "_hash"] = shaders.get(group[stage], {}).get("sha256", "UNAVAILABLE")
    return {
        "path": str(Path(path).resolve()), "draws": int(header[1]),
        "group_count": int(header[2]), "overflow": int(header[3]),
        "complete_budget": int(header[3]) == 0, "shaders": shaders,
        "groups": groups, "candidate_edges": edges, "copies": copies, "clears": clears,
    }


def signatures(trace):
    return {
        (g["VS_hash"], g["PS_hash"], g["fmt"], g["type"],
         g.get("geometry", {}).get("phase"),
         tuple((t["stage"], t["fmt"]) for t in g["textures"].values()))
        for g in trace["groups"].values()
    }


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("trace")
    parser.add_argument("--compare", help="HQ decals control trace; option state confirmed separately")
    parser.add_argument("--shader-out",help="Private output directory for bytecode disassembly, never commit")
    args = parser.parse_args()
    result = read_trace(args.trace,args.shader_out)
    if args.compare:
        control = read_trace(args.compare)
        first, second = signatures(result), signatures(control)
        result["comparison"] = {
            "control": control["path"], "control_overflow": control["overflow"],
            "only_first": sorted(first - second, key=str),
            "only_control": sorted(second - first, key=str),
            "warning": "Differing passes are candidates, not confirmed artifact producers.",
        }
    print(json.dumps(result, indent=2))

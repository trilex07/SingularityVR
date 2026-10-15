"""Read private bounded traces; compare content hashes, never cross-session COM addresses.

Producer/consumer edges are candidates, not proof a pass generates artifact pixels.
"""
import argparse
import hashlib
import json
import re
import struct
from pathlib import Path

def fnv1a(raw):
    value=14695981039346656037
    for byte in raw:value=((value^byte)*1099511628211)&0xffffffffffffffff
    return f'{value:016X}'


def ctab_registers(raw):
    """Read bounded SM3 CTAB comment metadata, never infer register ownership."""
    for pos in range(4, len(raw)-7, 4):
        token=struct.unpack_from("<I",raw,pos)[0]
        size=((token>>16)&0x7fff)*4
        if token&0xffff != 0xfffe or size<32 or raw[pos+4:pos+8]!=b"CTAB":
            continue
        if pos+4+size>len(raw):
            return {}
        block=raw[pos+8:pos+4+size]
        header=struct.unpack_from("<7I",block)
        count,offset=header[3],header[4]
        if header[0]<28 or header[0]>len(block) or count>256 or offset+count*20>len(block):
            return {}
        result={}
        for index in range(count):
            name,regset,reg,number,_,_,_=struct.unpack_from("<I4H2I",block,offset+index*20)
            if name>=len(block):return {}
            end=block.find(b"\0",name,min(name+128,len(block)))
            if end<0:return {}
            try: label=block[name:end].decode("ascii")
            except UnicodeDecodeError:return {}
            result[label]={"register_set":regset,"register_index":reg,"register_count":number}
        return result
    return {}


def alpha_policy(state):
    """Mask is not proof of a write; blend ZERO/ONE can preserve destination."""
    if state.get("valid")!="1":
        return {"alpha_write_enabled":None,"alpha_state_preserves_dst":None}
    allowed=bool(int(state["colorWrite"]) & 8)
    preserved=None
    if not allowed:preserved=True
    elif state.get("blend")=="0":preserved=False
    else:
        separate=state.get("separateAlpha")=="1"
        src=state.get("srcAlpha" if separate else "srcBlend")
        dst=state.get("dstAlpha" if separate else "dstBlend")
        op=state.get("alphaOp" if separate else "rgbOp")
        if op is not None:preserved=op=="1" and src=="1" and dst=="2"
    return {"alpha_write_enabled":allowed,"alpha_state_preserves_dst":preserved}


def read_trace(path, shader_out=None):
    text = Path(path).read_text(encoding="utf-8", errors="replace")
    header = re.search(r"draws=(\d+) groups=(\d+) overflow=(\d+)", text)
    if not header:
        raise ValueError("Not a shadow-pass-trace-v1 file")
    groups, shaders = {}, {}
    copies, clears = [], []
    map_pixels=[]
    events=[]
    lines = text.splitlines()
    for i, line in enumerate(lines):
        if line.startswith("map-pixels "):
            map_pixels.append(dict(re.findall(r"(\w+)=([^ ]+)",line)))
        elif line.startswith("map-grid "):
            match=re.match(r"map-grid index=(\d+) row=(\d+) (.+)",line)
            if match:
                for m in map_pixels:
                    if m["index"]==match[1]:m.setdefault("grid",{})[int(match[2])]=[float(v) for v in match[3].split()]
        elif line.startswith("event "):
            events.append(dict(re.findall(r"(\w+)=([^ ]+)",line)))
        elif line.startswith("copy "):
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
            data.update(alpha_policy(data))
            groups[int(data["group"])]["pipeline"]=data
        elif line.startswith("shadow-producer-high "):
            data=dict(re.findall(r"(\w+)=([^ ]+)",line))
            groups[int(data["group"])]["shadow_producer_high"]={
                "first":int(data["first"]),"start":int(data["start"]),
                "count":int(data["count"]),"hr":data["hr"],"constants":{},
            }
        elif line.startswith("shadow-producer-constant "):
            match=re.match(r"shadow-producer-constant group=(\d+) reg=(\d+) VS=([^ ]+)",line)
            if match:
                high=groups[int(match[1])]["shadow_producer_high"]
                high["constants"][int(match[2])]=[float(v) for v in match[3].split(",")]
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
                "fnv1a64": fnv1a(raw), "bytes": len(raw),
                "parameter_registers": ctab_registers(raw),
            }
    edges = []
    for p, producer in groups.items():
        pipe=producer.get("pipeline",{})
        if pipe.get("valid")=="1" and pipe.get("colorWrite")=="0":
            continue # Bound target is not a color producer for masked volume draws.
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
        depth=shaders.get(group["PS"],{}).get("parameter_registers",{}).get("MinZ_MaxZRatio",{})
        if depth.get("register_set")==2 and depth.get("register_count")==1:
            reg=depth["register_index"]
            if reg in group.get("constants",{}):group["depth_parameters"]=group["constants"][reg]["PS"]
            if reg in group.get("tail_constants",{}):group["tail_depth_parameters"]=group["tail_constants"][reg]["PS"]
    return {
        "path": str(Path(path).resolve()), "draws": int(header[1]),
        "group_count": int(header[2]), "overflow": int(header[3]),
        "complete_budget": int(header[3]) == 0, "shaders": shaders,
        "shader_budget_overflow": text.count('shader-budget-overflow '),
        "shader_unavailable": text.count('shader-unavailable '),
        "groups": groups, "candidate_edges": edges, "copies": copies, "clears": clears,"events":events,"map_pixels":map_pixels,
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

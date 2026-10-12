"""Synthetic metadata only: no proprietary shader bytecode."""
import importlib.util
import struct
from pathlib import Path

spec=importlib.util.spec_from_file_location("trace",Path(__file__).parents[1]/"analyze-shadow-pass-trace.py")
trace=importlib.util.module_from_spec(spec)
spec.loader.exec_module(trace)

def test_ctab():
    name=b"MinZ_MaxZRatio\0"
    table=struct.pack("<7I",28,0,0xffff0300,1,28,0,0)
    table+=struct.pack("<I4H2I",48,2,2,1,0,0,0)+name
    table+=b"\0"*((-len(table))%4)
    comment=b"CTAB"+table
    shader=struct.pack("<2I",0xffff0300,0xfffe | ((len(comment)//4)<<16))+comment
    shader+=struct.pack("<I",0xffff)
    assert trace.ctab_registers(shader)["MinZ_MaxZRatio"]=={
        "register_set":2,"register_index":2,"register_count":1}
    assert trace.ctab_registers(shader[:24])=={}
    broken=bytearray(shader);struct.pack_into("<I",broken,12+28,999999)
    assert trace.ctab_registers(broken)=={}

def test_alpha():
    state={"valid":"1","colorWrite":"15","blend":"1","separateAlpha":"1",
           "srcAlpha":"1","dstAlpha":"2","alphaOp":"1"}
    assert trace.alpha_policy(state)["alpha_state_preserves_dst"] is True
    assert trace.alpha_policy({**state,"alphaOp":"2"})["alpha_state_preserves_dst"] is False
    old={k:v for k,v in state.items() if k!="alphaOp"}
    assert trace.alpha_policy(old)["alpha_state_preserves_dst"] is None
    assert trace.alpha_policy({**state,"colorWrite":"0"})["alpha_write_enabled"] is False
    assert trace.alpha_policy({**state,"blend":"0"})["alpha_state_preserves_dst"] is False
    assert trace.alpha_policy({"valid":"0"})["alpha_write_enabled"] is None

test_ctab()
test_alpha()
print("PASS: bounded CTAB register metadata; mask/blend alpha policy; missing-state uncertainty")

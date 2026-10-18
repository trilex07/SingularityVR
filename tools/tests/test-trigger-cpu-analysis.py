"""Own synthetic CSV only; no game code, proprietary data or live processes."""
import importlib.util
import pathlib
import struct
import tempfile

spec=importlib.util.spec_from_file_location('trigger_cpu',pathlib.Path(__file__).parents[1]/'analyze-trigger-cpu.py')
module=importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
fixture='''BeginHeader
I-Start,TimeStamp,Process Name ( PID),BaseAddr,EndAddr,FileName
SampledProfile,TimeStamp,Process Name ( PID),ThreadID,PrgrmCtr
EndHeader
I-Start,10,Singularity.exe (42),0x400000,0x500000,Singularity.exe
SampledProfile,2000000,Singularity.exe (42),7,0x401020
Stack,2000000,7,Singularity.exe!0x401020,Singularity.exe!0x401030,Singularity.exe!0x401030
SampledProfile,2000001,Singularity.exe (99),8,0x401020
Stack,2000001,8,Other.exe!0x401020
'''
with tempfile.TemporaryDirectory(dir=pathlib.Path(__file__).parent) as folder:
    path=pathlib.Path(folder)/'own.csv'
    path.write_text(fixture)
    result=module.analyze(path,42,7)
    assert result['sample_count']==1
    assert result['hot'][0]['module']=='Singularity.exe'
    assert result['hot'][0]['rva']=='0x1020'
    assert result['threads']['7']['samples']==1
    assert dict(result['sampled_stack_frame_hits']['2'])=={'Singularity.exe!0x401020':1,'Singularity.exe!0x401030':1}
print('PASS: actual xperf I-Start/PrgrmCtr schema, PID filter, unique sampled stack frames')

multiline='''BeginHeader
I-Start,TimeStamp,Process Name ( PID),BaseAddr,EndAddr,FileName
SampledProfile,TimeStamp,Process Name ( PID),ThreadID,PrgrmCtr
Stack,TimeStamp,ThreadID,No.,Address,Image!Function
EndHeader
I-Start,10,Singularity.exe (42),0x400000,0x500000,Singularity.exe
SampledProfile,2000000,Singularity.exe (42),7,0x401020
Stack,2000000,7,0,0x401020,Singularity.exe!0x401020
Stack,2000000,7,1,0x401030,Singularity.exe!0x401030
Stack,2000000,7,2,0x401030,Singularity.exe!0x401030
SampledProfile,2000001,Singularity.exe (99),8,0x401020
Stack,2000001,8,0,0x401020,Other.exe!0x401020
'''
with tempfile.TemporaryDirectory(dir=pathlib.Path(__file__).parent) as folder:
    path=pathlib.Path(folder)/'own-multiline.csv'
    path.write_text(multiline)
    result=module.analyze(path,42,7)
    assert result['sample_count']==1
    assert dict(result['sampled_stack_frame_hits']['2'])=={'Singularity.exe!0x401020':1,'Singularity.exe!0x401030':1}
print('PASS: current one-frame-per-row stacks, full callers, no ordinals/addresses, unique hits, target PID')

with tempfile.TemporaryDirectory(dir=pathlib.Path(__file__).parent) as folder:
    root=pathlib.Path(folder);image=root/'own.dll';symbol_map=root/'own.map'
    raw=bytearray(512);struct.pack_into('<I',raw,0x3c,0x80);raw[0x80:0x84]=b'PE\0\0'
    struct.pack_into('<HHI',raw,0x84,0x14c,2,0x1234);struct.pack_into('<H',raw,0x94,0xe0)
    struct.pack_into('<H',raw,0x98,0x10b);struct.pack_into('<I',raw,0x98+28,0x10000000)
    for index,(rva,flags) in enumerate(((0x1000,0x20000000),(0x2000,0x40000000))):
        offset=0x178+index*40;struct.pack_into('<II',raw,offset+8,0x100,rva);struct.pack_into('<I',raw,offset+36,flags)
    image.write_bytes(raw)
    symbol_map.write_text('Timestamp is 1234\nPreferred load address is 10000000\n 0001:00000000 first 10001000\n 0001:00000020 second 10001020\n')
    symbols=module.map_symbols(symbol_map,image)
    assert symbols(0x1001)=='first' and symbols(0x1021)=='second'
    assert symbols(0x2001) is None and symbols(0x1100) is None
    path=root/'own.csv';path.write_text(multiline)
    result=module.analyze(path,42,7,symbol_map,image,0x400000)
    assert result['symbol_self_hits']['7']==[('second',1)]
    assert result['hot'][0]['module_base']=='0x400000'
    for args in ((symbol_map,None,None),(symbol_map,image,0x900000)):
        try:module.analyze(path,42,7,*args)
        except ValueError:pass
        else:raise AssertionError('missing identity accepted')
    symbol_map.write_text(symbol_map.read_text().replace('1234','5678'))
    try:module.map_symbols(symbol_map,image)
    except ValueError:pass
    else:raise AssertionError('stale map accepted')
print('PASS: PE32/map identity gate, module base disambiguation, code-only IP self samples, stale/partial identity rejected')

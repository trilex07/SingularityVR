"""Own synthetic CSV only; no game code, proprietary data or live processes."""
import importlib.util
import pathlib
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

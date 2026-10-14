param([Parameter(Mandatory)][int]$GameProcessId)
# MAIN-only, read-only UE3 reflection. No injection, writes, input or deployment.
$ErrorActionPreference = 'Stop'
$game = Get-Process -Id $GameProcessId
if ($game.ProcessName -ne 'Singularity') { throw 'PID is not Singularity' }
Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
public static class CinematicEngineRead {
    [DllImport("kernel32.dll", SetLastError=true)] static extern IntPtr OpenProcess(uint access, bool inherit, int pid);
    [DllImport("kernel32.dll")] static extern bool ReadProcessMemory(IntPtr process, IntPtr address, byte[] data, UIntPtr size, out UIntPtr read);
    [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr handle);
    static IntPtr process;
    static uint[] names;
    static Dictionary<uint,string> cache;
    static byte[] Read(uint address, int size) {
        if (address < 0x10000 || size < 1 || size > 8000000) return null;
        byte[] b=new byte[size]; UIntPtr got;
        return ReadProcessMemory(process,new IntPtr((long)address),b,new UIntPtr((uint)size),out got) && got.ToUInt64()==(ulong)size ? b : null;
    }
    static uint Word(byte[] b,int off) {return BitConverter.ToUInt32(b,off);}
    static uint[] Table(uint address) {
        byte[] h=Read(address,12); if(h==null) throw new Exception("Missing UE3 array");
        uint count=Word(h,4),capacity=Word(h,8);
        if(count<100 || count>1000000 || capacity<count || capacity>2000000) throw new Exception("Invalid UE3 array bounds");
        byte[] b=Read(Word(h,0),(int)count*4); if(b==null) throw new Exception("Unreadable UE3 array");
        uint[] result=new uint[count]; Buffer.BlockCopy(b,0,result,0,b.Length); return result;
    }
    static string Name(uint index) {
        string value; if(cache.TryGetValue(index,out value)) return value;
        if(index>=names.Length) return "<invalid>";
        byte[] b=Read(names[index],208); if(b==null) return "<unreadable>";
        value=(Word(b,8)&1)!=0 ? Encoding.Unicode.GetString(b,16,192) : Encoding.ASCII.GetString(b,16,192);
        int end=value.IndexOf('\0'); if(end>=0)value=value.Substring(0,end);
        cache[index]=value; return value;
    }
    static string ObjectName(uint address) {byte[] b=Read(address,64);return b==null?"<unreadable>":Name(Word(b,0x2c));}
    public static string[] Snapshot(int pid) {
        process=OpenProcess(0x1010,false,pid); if(process==IntPtr.Zero)throw new Exception("Read-only access denied");
        try {
            cache=new Dictionary<uint,string>(); names=Table(0x01cd4a1c); uint[] objects=Table(0x01cd4a4c);
            if(Name(0)!="None")throw new Exception("GNames control failed");
            var lines=new List<string>(); uint controllerClass=0,actorClass=0;
            var controllers=new List<uint>();
            foreach(uint address in objects) {
                byte[] b=Read(address,64);if(b==null)continue;
                string name=Name(Word(b,0x2c));
                if(name=="RvPlayerController" && ObjectName(Word(b,0x28))=="RvGame")controllerClass=address;
                if(name=="Actor" && ObjectName(Word(b,0x28))=="Engine")actorClass=address;
            }
            if(controllerClass==0||actorClass==0)throw new Exception("Class controls missing");
            bool location=false,rotation=false;
            byte[] actor=Read(actorClass,256); uint prop=Word(actor,0x4c);
            var controlSeen=new HashSet<uint>();
            while(prop!=0 && controlSeen.Count<400 && controlSeen.Add(prop)) {
                byte[] b=Read(prop,256);if(b==null)break;
                string name=Name(Word(b,0x2c)); uint off=Word(b,0x64);
                if(name=="Location" && off==0x54)location=true;
                if(name=="Rotation" && off==0x60)rotation=true;
                prop=Word(b,0x40);
            }
            if(!location||!rotation)throw new Exception("UProperty layout controls failed");
            foreach(uint address in objects) {
                byte[] b=Read(address,64);if(b==null || Word(b,0x34)!=controllerClass)continue;
                string name=Name(Word(b,0x2c)),outer=ObjectName(Word(b,0x28));
                if(name.StartsWith("Default__") || outer!="PersistentLevel")continue;
                controllers.Add(address);lines.Add(String.Format("CONTROLLER {0:X8} outer={1}",address,outer));
                // Native UObject::execGetStateName at 004B7B60 was read live:
                // self+18 -> FStateFrame, frame+28 -> StateNode, node+2C -> FName.
                byte[] frame=Read(Word(b,0x18),0x30);
                if(frame!=null)lines.Add("CONTROLLER_STATE "+ObjectName(Word(frame,0x28)));
            }
            lines.Add(String.Format("CONTROLS names={0} objects={1} actor-layout=PASS controllerClass={2:X8}",names.Length,objects.Length,controllerClass));
            uint cls=controllerClass;var classesSeen=new HashSet<uint>();
            while(cls!=0 && classesSeen.Count<64 && classesSeen.Add(cls)) {
                byte[] c=Read(cls,256);if(c==null)break;string clsName=Name(Word(c,0x2c));
                prop=Word(c,0x4c); var seen=new HashSet<uint>();
                while(prop!=0 && seen.Count<400 && seen.Add(prop)) {
                    byte[] b=Read(prop,256);if(b==null)break;
                    string name=Name(Word(b,0x2c));uint off=Word(b,0x64);
                    if(name.IndexOf("Cinematic",StringComparison.OrdinalIgnoreCase)>=0 || name=="GetStateName" || name=="IgnoreMoveInput" || name=="IgnoreLookInput" || name=="PlayerCamera" || name=="ViewTarget") {
                        string type=ObjectName(Word(b,0x34));
                        lines.Add(String.Format("PROPERTY {0}.{1} ptr={2:X8} type={3} offset={4:X} size={5}",clsName,name,prop,type,off,Word(b,0x48)));
                        if(type.EndsWith("Property") || name=="GetStateName")lines.Add("PROPERTY_BYTES "+BitConverter.ToString(b));
                        if(name=="GetStateName") {
                            for(int at=0x80;at<=0xb0;at+=4) {
                                uint function=Word(b,at);
                                if(function<0x402000 || function>=0x017a0000)continue;
                                byte[] code=Read(function,128);
                                if(code!=null)lines.Add(String.Format("NATIVE_GetStateName funcSlot={0:X} entry={1:X8} code={2}",at,function,BitConverter.ToString(code)));
                            }
                        }
                        if(type=="ScriptStruct") {
                            uint member=Word(b,0x4c); var members=new HashSet<uint>();
                            while(member!=0 && members.Count<40 && members.Add(member)) {
                                byte[] mb=Read(member,160);if(mb==null)break;
                                lines.Add(String.Format("STRUCT_MEMBER {0}.{1} ptr={2:X8} type={3} off={4:X} size={5} bytes={6}",name,Name(Word(mb,0x2c)),member,ObjectName(Word(mb,0x34)),Word(mb,0x64),Word(mb,0x48),BitConverter.ToString(mb)));
                                member=Word(mb,0x40);
                            }
                        }
                        if(type=="Enum") {
                            for(int at=0x40;at<0x80;at+=4) {
                                uint length=Word(b,at+4),cap=Word(b,at+8);
                                if(length<2 || length>32 || cap<length || cap>64)continue;
                                byte[] enumNames=Read(Word(b,at),(int)length*8);if(enumNames==null)continue;
                                var labels=new List<string>();bool valid=true;
                                for(int e=0;e<length;e++){string label=Name(Word(enumNames,e*8));if(label.StartsWith("<")){valid=false;break;}labels.Add(label);}
                                if(valid)lines.Add("ENUM_NAMES at="+at.ToString("X")+" "+String.Join(",",labels.ToArray()));
                            }
                        }
                        foreach(uint instance in controllers) {
                            if(!type.EndsWith("Property") || off>0x8000)continue;
                            byte[] value=Read(instance+off,16);
                            if(value!=null)lines.Add(String.Format("VALUE controller={0:X8} property={1} bytes={2}",instance,name,BitConverter.ToString(value)));
                        }
                    }
                    prop=Word(b,0x40);
                }
                cls=Word(c,0x3c);
            }
            return lines.ToArray();
        }finally{CloseHandle(process);process=IntPtr.Zero;}
    }
}
'@
[CinematicEngineRead]::Snapshot($GameProcessId)

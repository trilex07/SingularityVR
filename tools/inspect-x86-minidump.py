"""Read x86 exception, module identities and captured memory without execution.

Stack words annotated with modules are candidates, not an unwound call stack.
Uses the documented MINIDUMP stream layouts; no debugger, symbol downloads,
live process access, package dependencies or writes.
"""
import argparse
import struct
from pathlib import Path


def inspect(path):
    data = Path(path).read_bytes()

    def unpack(fmt, off):
        size = struct.calcsize(fmt)
        if off < 0 or off + size > len(data):
            raise ValueError("Truncated minidump field")
        return struct.unpack_from(fmt, data, off)

    signature, _, count, directory = unpack("<4sIII", 0)
    if signature != b"MDMP" or count > 4096:
        raise ValueError("Not a bounded minidump")
    streams = {kind: (size, rva) for kind, size, rva in
               (unpack("<III", directory + 12 * i) for i in range(count))}
    print("Streams:", sorted(streams))
    if 6 not in streams:
        raise ValueError("No exception stream")
    _, exc = streams[6]
    tid, = unpack("<I", exc)
    code, flags, record, address, nparams, _ = unpack("<IIQQII", exc + 8)
    params = unpack("<15Q", exc + 40)
    print(f"Exception thread={tid} code={code:#x} flags={flags:#x} address={address:#x}")
    print("Parameters:", [hex(x) for x in params[:min(nparams, 15)]])
    ctx_size, ctx = unpack("<II", exc + 160)
    if ctx_size < 204 or unpack("<I", ctx)[0] & 0x10000 == 0:
        raise ValueError("Not a complete x86 context")
    regs = dict(zip(("edi", "esi", "ebx", "edx", "ecx", "eax", "ebp", "eip", "cs", "eflags", "esp", "ss"),
                    unpack("<12I", ctx + 156)))
    print("Saved CONTEXT (may be exception-handler context):", " ".join(f"{name}={value:#010x}" for name, value in regs.items()))
    modules = []
    if 4 in streams:
        _, mod = streams[4]
        number, = unpack("<I", mod)
        if number > 4096:
            raise ValueError("Unbounded module list")
        for i in range(number):
            entry = mod + 4 + i * 108
            base, size, _, timestamp, name = unpack("<QIIII", entry)
            name_size, = unpack("<I", name)
            if name_size > 65536 or name + 4 + name_size > len(data):
                raise ValueError("Invalid module name")
            label = data[name + 4:name + 4 + name_size].decode("utf-16le", errors="replace")
            modules.append((base, size, label))
            if "d3d9.dll" in label.lower() or "singularity.exe" in label.lower():
                print(f"Module {label}: base={base:#x} size={size:#x} PE timestamp={timestamp:#x}")

    def describe(va):
        for base, size, label in modules:
            if base <= va < base + size:
                return f"{label.rsplit(chr(92), 1)[-1]}+{va - base:#x}"
        return "unmapped module"

    ranges = []
    if 5 in streams:
        _, mem = streams[5]
        number, = unpack("<I", mem)
        if number > 1000000:
            raise ValueError("Unbounded memory list")
        for i in range(number):
            va, size, rva = unpack("<QII", mem + 4 + 16 * i)
            ranges.append((va, size, rva))
    if 9 in streams:
        _, mem = streams[9]
        number, rva = unpack("<QQ", mem)
        if number > 1000000:
            raise ValueError("Unbounded memory64 list")
        for i in range(number):
            va, size = unpack("<QQ", mem + 16 + 16 * i)
            ranges.append((va, size, rva))
            rva += size

    def memory(va, size):
        for start, length, rva in ranges:
            if start <= va and va + size <= start + length:
                off = rva + va - start
                if off + size > len(data):
                    raise ValueError("Truncated captured memory")
                return data[off:off + size]
        return None

    print("Exception record instruction:", describe(address))
    print("Saved CONTEXT instruction:", describe(regs["eip"]))
    for name in ("eip", "eax", "ecx", "edx", "esi", "edi"):
        buf = memory(regs[name], 32)
        print(name, "memory:", buf.hex(" ") if buf else "not captured")
    print("Captured stack words (NOT unwound):")
    for i in range(80):
        va = regs["esp"] + i * 4
        buf = memory(va, 4)
        if buf:
            value, = struct.unpack("<I", buf)
            print(f"{va:#010x}: {value:#010x} {describe(value)}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dump")
    inspect(parser.parse_args().dump)

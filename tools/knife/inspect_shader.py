"""Offline D3D bytecode disassembly; explicit private input/output, no game access."""
import argparse
import ctypes as c
from pathlib import Path


def disassemble(path):
    dll = c.WinDLL('d3dcompiler_47.dll')
    fn = dll.D3DDisassemble
    fn.argtypes = [c.c_void_p, c.c_size_t, c.c_uint, c.c_char_p, c.POINTER(c.c_void_p)]
    fn.restype = c.c_long
    raw = Path(path).read_bytes()
    if not 8 <= len(raw) <= 1024 * 1024:
        raise ValueError('bounded shader input required')
    buffer = c.create_string_buffer(raw)
    blob = c.c_void_p()
    hr = fn(buffer, len(raw), 0, None, c.byref(blob))
    if hr < 0 or not blob.value:
        raise RuntimeError(f'D3DDisassemble failed {hr:#x}')
    vtable = c.cast(blob, c.POINTER(c.POINTER(c.c_void_p))).contents
    pointer = c.WINFUNCTYPE(c.c_void_p, c.c_void_p)(vtable[3])
    size = c.WINFUNCTYPE(c.c_size_t, c.c_void_p)(vtable[4])
    release = c.WINFUNCTYPE(c.c_ulong, c.c_void_p)(vtable[2])
    try:
        return c.string_at(pointer(blob), size(blob)).rstrip(b'\0').decode('utf-8')
    finally:
        release(blob)


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('input', type=Path)
    p.add_argument('--output', type=Path)
    a = p.parse_args()
    result = disassemble(a.input)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True)
        a.output.write_text(result, encoding='utf-8')
    else:
        print(result)

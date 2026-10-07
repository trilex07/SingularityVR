#!/usr/bin/env python3
"""SingularityVR bounded GFx / Scaleform reconnaissance helper.

READ-ONLY. Never hooks, patches, launches the game, or writes outside the
given markdown report. Scans:
  1. Singularity.exe bytes (ASCII + UTF-16LE) for Scaleform/GFx evidence,
     mapping file offsets to RVAs via the PE section table where possible;
  2. the mod's own sources and research notes for corroborating references.

Findings are separated into PROVEN / LIKELY / SPECULATIVE. No address is
claimed as a hook target unless the evidence supports it (it will not: string
presence alone never identifies a hook target).

Usage:
    python gfx_recon.py [--exe PATH] [--src DIR]... [--markdown out.md]

Exit code: 0 always; 2 on usage/IO error.
"""

import argparse
import os
import re
import struct
import sys

DEFAULT_EXE = (r"C:\Program Files (x86)\Steam\steamapps\common\Singularity"
               r"\Binaries\Singularity.exe")

# term -> (class, note). Class is a starting hint; final placement also
# depends on WHERE the term was found (binary vs notes) - see main().
TERMS = [
    ("Scaleform", "vendor/sdk marker"),
    ("GFx", "Scaleform GFx prefix"),
    ("GFxMovie", "movie object"),
    ("GFxMovieDef", "movie definition"),
    ("GFxMovieView", "movie view"),
    ("MovieImpl", "movie implementation"),
    ("MovieView", "movie view interface"),
    ("MovieDef", "movie definition interface"),
    ("SetViewport", "viewport binding on the movie"),
    ("GetViewport", "viewport query"),
    ("Advance", "movie time advance - VERY generic word"),
    ("Display", "movie display call - VERY generic word"),
    ("HandleEvent", "input event into the movie - generic word"),
    ("Invoke", "ActionScript invoke bridge"),
    ("AS2", "ActionScript 2 marker"),
    ("AS3", "ActionScript 3 / AVM2 marker"),
    ("ActionScript", "scripting layer marker"),
    ("swf", ".swf asset reference"),
    ("SWF", ".SWF asset reference"),
    ("Flash", "Flash asset/player reference - generic word"),
    ("TextField", "GFx text field object"),
    ("UIFlash", "UE3 GFx UI container class"),
    ("GFxUI", "UE3 GFx UI subsystem"),
    ("GFxInteraction", "UE3 GFx input routing"),
    ("FSCommand", "Flash FSCommand bridge"),
]

GENERIC_WORDS = {"Advance", "Display", "HandleEvent", "Flash", "Invoke"}


def parse_sections(data):
    """Return list of (name, vaddr, vsize, raw, rawsize) or [] if unparseable."""
    try:
        if len(data) < 64 or data[0:2] != b"MZ":
            return []
        pe = struct.unpack_from("<I", data, 0x3C)[0]
        if data[pe:pe + 4] != b"PE\x00\x00":
            return []
        nsec = struct.unpack_from("<H", data, pe + 6)[0]
        opt_size = struct.unpack_from("<H", data, pe + 20)[0]
        sec_off = pe + 24 + opt_size
        sections = []
        for i in range(nsec):
            base = sec_off + i * 40
            if base + 40 > len(data):
                break
            name = data[base:base + 8].split(b"\x00")[0].decode("ascii", "replace")
            vsize, vaddr, rawsize, raw = struct.unpack_from("<IIII", data, base + 8)
            sections.append((name, vaddr, vsize, raw, rawsize))
        return sections
    except (struct.error, IndexError):
        return []


def offset_to_rva(sections, off):
    for name, vaddr, vsize, raw, rawsize in sections:
        if raw <= off < raw + rawsize:
            return vaddr + (off - raw), name
    return None, None


def ascii_strings(data, min_len=4):
    """Yield (offset, text) for printable ASCII runs."""
    run = bytearray()
    start = 0
    for i, b in enumerate(data):
        if 32 <= b < 127:
            if not run:
                start = i
            run.append(b)
        else:
            if len(run) >= min_len:
                yield start, run.decode("ascii")
            run = bytearray()
    if len(run) >= min_len:
        yield start, run.decode("ascii")


def utf16_strings(data, min_len=4):
    """Yield (offset, text) for UTF-16LE printable runs (even offsets)."""
    run = []
    start = 0
    for i in range(0, len(data) - 1, 2):
        lo, hi = data[i], data[i + 1]
        if hi == 0 and 32 <= lo < 127:
            if not run:
                start = i
            run.append(chr(lo))
        else:
            if len(run) >= min_len:
                yield start, "".join(run)
            run = []
    if len(run) >= min_len:
        yield start, "".join(run)


def scan_binary(path):
    with open(path, "rb") as f:
        data = f.read()
    sections = parse_sections(data)
    hits = {}  # term -> list of (offset, rva_or_None, section_or_None, encoding)
    dll_refs = []
    for off, text in ascii_strings(data):
        for term, _note in TERMS:
            if term in text:
                rva, sec = offset_to_rva(sections, off)
                hits.setdefault(term, []).append((off, rva, sec, "ascii"))
                if len(hits[term]) > 25:
                    continue
        low = text.lower()
        if ("gfx" in low or "scaleform" in low) and text.lower().endswith(".dll"):
            rva, sec = offset_to_rva(sections, off)
            dll_refs.append((off, rva, sec, text))
    for off, text in utf16_strings(data):
        for term, _note in TERMS:
            if term in text and len(hits.get(term, [])) < 25:
                rva, sec = offset_to_rva(sections, off)
                hits.setdefault(term, []).append((off, rva, sec, "utf16le"))
    return len(data), sections, hits, dll_refs


def scan_text_tree(roots, terms):
    """Search sources/notes for the same terms. Returns term -> [(file, line)]."""
    hits = {}
    want = set(terms)
    for root in roots:
        if os.path.isfile(root):
            files = [root]
        elif os.path.isdir(root):
            files = []
            for dirpath, _dirs, names in os.walk(root):
                for nm in names:
                    if nm.endswith((".cpp", ".h", ".md", ".txt", ".ini", ".example")):
                        files.append(os.path.join(dirpath, nm))
        else:
            print("warning: src root not found: %s" % root, file=sys.stderr)
            continue
        for path in files:
            try:
                with open(path, encoding="utf-8", errors="replace") as f:
                    for no, line in enumerate(f, 1):
                        for term in want:
                            if term in line:
                                hits.setdefault(term, []).append((path, no, line.strip()[:160]))
                                if len(hits[term]) > 15:
                                    pass
            except OSError as e:
                print("warning: cannot read %s: %s" % (path, e), file=sys.stderr)
    return hits


def main(argv=None):
    ap = argparse.ArgumentParser(description="Bounded read-only GFx/Scaleform recon.")
    ap.add_argument("--exe", default=DEFAULT_EXE, help="Game binary to scan (read-only)")
    ap.add_argument("--src", action="append", default=[],
                    help="Source/note root or file to cross-reference (repeatable)")
    ap.add_argument("--markdown", default=None, help="Write report to PATH")
    ap.add_argument("--repo", default=None, help="Repo root for default --src dirs")
    args = ap.parse_args(argv)

    if not os.path.isfile(args.exe):
        print("error: game binary not found: %s" % args.exe, file=sys.stderr)
        return 2

    size, sections, bhits, dll_refs = scan_binary(args.exe)

    if not args.src:
        base = args.repo or os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
        args.src = [os.path.join(base, "spikes", "view_matrix"),
                    os.path.join(base, "ENGINE_NOTES.md"),
                    os.path.join(base, "STATUS.md"),
                    os.path.join(base, "HISTORY.md")]
        args.src = [s for s in args.src if os.path.exists(s)]
    thits = scan_text_tree(args.src, [t for t, _n in TERMS])
    notes = {t: n for t, n in TERMS}

    proven, likely, speculative = [], [], []
    for term, note in TERMS:
        b = bhits.get(term, [])
        t = thits.get(term, [])
        if b and term not in GENERIC_WORDS:
            proven.append((term, note, b, t))
        elif b and term in GENERIC_WORDS:
            speculative.append((term, note + " (generic word - could be anything)", b, t))
        elif t:
            likely.append((term, note + " (in own sources/notes only - needs binary corroboration)", [], t))

    def fmt_off(o, rva, sec, enc):
        if rva is not None:
            return "file+0x%X rva=0x%X [%s] (%s)" % (o, rva, sec, enc)
        return "file+0x%X (no section map) (%s)" % (o, enc)

    print("=" * 70)
    print("GFx / SCALEFORM RECON (read-only, no hooks, no patches)")
    print("binary: %s (%d bytes, %d sections)" % (args.exe, size, len(sections)))
    print("PROVEN=%d LIKELY=%d SPECULATIVE=%d dll_refs=%d"
          % (len(proven), len(likely), len(speculative), len(dll_refs)))
    print("String presence alone NEVER identifies a hook target.")
    print("=" * 70)
    for tag, group in (("PROVEN", proven), ("LIKELY", likely), ("SPECULATIVE", speculative)):
        print("== %s (%d) ==" % (tag, len(group)))
        for term, note, b, t in group:
            print("[%s] %s" % (term, note))
            for o, rva, sec, enc in b[:8]:
                print("    bin: %s" % fmt_off(o, rva, sec, enc))
            if len(b) > 8:
                print("    ... and %d more binary occurrences" % (len(b) - 8))
            for path, no, line in t[:5]:
                print("    src: %s:%d: %s" % (os.path.basename(path), no, line[:120]))
    if dll_refs:
        print("== possible GFx/Scaleform DLL references ==")
        for o, rva, sec, text in dll_refs[:10]:
            print("    %s : %s" % (fmt_off(o, rva, sec, "ascii"), text))
    else:
        print("no *GFx*/*.Scaleform* .dll import names found as plain strings "
              "(imports may still exist - this scan only covers string tables).")

    if args.markdown:
        d = os.path.dirname(os.path.abspath(args.markdown))
        if d and not os.path.isdir(d):
            os.makedirs(d)
        with open(args.markdown, "w", encoding="utf-8") as out:
            out.write("# GFx / Scaleform reconnaissance\n\n")
            out.write("Bounded, read-only. No hooks, no patches, no game launch.\n\n")
            out.write("Binary `%s` (%d bytes, %d PE sections).\n\n" % (args.exe, size, len(sections)))
            out.write("RVA mapping via PE section headers; file+offset always given "
                      "alongside. The game may use ASLR - RVAs are image-relative, "
                      "not absolute addresses, and **no address below is claimed as "
                      "a hook target**.\n\n")
            for tag, group in (("PROVEN", proven), ("LIKELY", likely), ("SPECULATIVE", speculative)):
                out.write("## %s (%d)\n\n" % (tag, len(group)))
                if not group:
                    out.write("None.\n\n")
                for term, note, b, t in group:
                    out.write("### `%s` - %s\n\n" % (term, note))
                    if b:
                        out.write("Binary occurrences (up to 8 shown):\n\n")
                        for o, rva, sec, enc in b[:8]:
                            out.write("- %s\n" % fmt_off(o, rva, sec, enc))
                        if len(b) > 8:
                            out.write("- ... and %d more\n" % (len(b) - 8))
                        out.write("\n")
                    if t:
                        out.write("Sources/notes:\n\n")
                        for path, no, line in t[:5]:
                            out.write("- `%s:%d`: %s\n"
                                      % (os.path.basename(path), no,
                                         line[:140].replace("|", "\\|")))
                        out.write("\n")
            out.write("## Possible imports / DLL references\n\n")
            if dll_refs:
                for o, rva, sec, text in dll_refs[:10]:
                    out.write("- %s : `%s`\n" % (fmt_off(o, rva, sec, "ascii"), text))
            else:
                out.write("None found as plain strings. The import table was not "
                          "parsed - absence here proves nothing about linkage.\n")
            out.write("\n## Candidate call sites\n\nNone claimed. String hits are "
                      "data references, not code references; converting one into "
                      "a hook target requires disassembly work that is out of "
                      "scope for this helper.\n")
        print("markdown report: %s" % args.markdown)
    return 0


if __name__ == "__main__":
    sys.exit(main())

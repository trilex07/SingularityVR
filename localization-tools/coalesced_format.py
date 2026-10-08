#!/usr/bin/env python3
"""Reader/writer for Singularity's serialized Coalesced *.bin files.

The format is deliberately kept separate from any translation edits.  This
module is used by the inventory tool and has an untouched byte-for-byte
round-trip test so the original container can be audited safely.
"""
from __future__ import annotations

import struct
from typing import List, Tuple

String = Tuple[str, bool]
Pair = Tuple[String, String]
Section = Tuple[String, List[Pair]]
Record = Tuple[String, List[Section]]


def parse(data: bytes) -> List[Record]:
    pos = 0

    def integer() -> int:
        nonlocal pos
        if pos + 4 > len(data):
            raise ValueError("truncated Coalesced integer")
        value = struct.unpack_from("<i", data, pos)[0]
        pos += 4
        return value

    def string() -> String:
        nonlocal pos
        length = integer()
        if length == 0:
            return "", False
        size = abs(length) * (2 if length < 0 else 1)
        if size > len(data) - pos:
            raise ValueError("truncated Coalesced string")
        raw = data[pos : pos + size]
        pos += size
        encoding = "utf-16le" if length < 0 else "latin1"
        text = raw.decode(encoding)
        if not text.endswith("\0"):
            raise ValueError("unterminated Coalesced string")
        return text[:-1], length < 0

    records: List[Record] = []
    for _ in range(integer()):
        record_name = string()
        sections: List[Section] = []
        for _ in range(integer()):
            section_name = string()
            pairs: List[Pair] = []
            for _ in range(integer()):
                pairs.append((string(), string()))
            sections.append((section_name, pairs))
        records.append((record_name, sections))
    if pos != len(data):
        raise ValueError(f"trailing bytes in Coalesced file: {len(data) - pos}")
    return records


def encode(records: List[Record]) -> bytes:
    out = bytearray()

    def integer(value: int) -> None:
        out.extend(struct.pack("<i", value))

    def string(value: String) -> None:
        text, wide = value
        if not text:
            integer(0)
            return
        raw = (text + "\0").encode("utf-16le" if wide else "latin1")
        integer(-len(raw) // 2 if wide else len(raw))
        out.extend(raw)

    integer(len(records))
    for record_name, sections in records:
        string(record_name)
        integer(len(sections))
        for section_name, pairs in sections:
            string(section_name)
            integer(len(pairs))
            for key, value in pairs:
                string(key)
                string(value)
    return bytes(out)


def iter_entries(records: List[Record]):
    """Yield stable inventory tuples without changing the parsed records."""
    for record_name, sections in records:
        for section_name, pairs in sections:
            for key, value in pairs:
                yield record_name[0], section_name[0], key[0], value[0]


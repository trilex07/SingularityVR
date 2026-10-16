#!/usr/bin/env python3
"""Replace candidate Bink audio packets with stock stereo audio (video unchanged)."""
from __future__ import annotations

import argparse
import hashlib
import struct
from pathlib import Path


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest().upper()


def parse_index(data: bytes) -> tuple[dict, list[tuple[int, int]]]:
    if data[:4] != b"BIKi":
        raise ValueError("not BIKi")
    frame_count = struct.unpack_from("<I", data, 8)[0]
    has_audio = struct.unpack_from("<I", data, 40)[0]
    off = 44
    audio_meta = None
    if has_audio:
        audio_meta = struct.unpack_from("<IHH I", data, off)
        off += 12
    first_off = struct.unpack_from("<I", data, off)[0] & ~1
    table_entries = (first_off - off) // 4
    if table_entries not in (frame_count, frame_count + 1):
        raise ValueError(f"unexpected offset table entries: {table_entries} for {frame_count} frames")
    index: list[tuple[int, int]] = []
    for i in range(frame_count):
        raw = struct.unpack_from("<I", data, off + i * 4)[0]
        index.append((raw & ~1, raw & 1))
    header = {
        "frame_count": frame_count,
        "has_audio": has_audio,
        "audio_meta": audio_meta,
        "index_offset": off,
        "table_end": first_off,
    }
    return header, index


def frame_size(index: list[tuple[int, int]], total_size: int, i: int) -> int:
    start = index[i][0]
    end = index[i + 1][0] if i + 1 < len(index) else total_size
    return end - start


def split_payload(payload: bytes, has_audio: bool) -> tuple[bytes, bytes]:
    if not has_audio:
        return b"", payload
    audio_len = struct.unpack_from("<I", payload, 0)[0]
    return payload[: 4 + audio_len], payload[4 + audio_len :]


def rebuild(stock: Path, candidate: Path, dest: Path) -> None:
    stock_data = stock.read_bytes()
    cand_data = candidate.read_bytes()
    stock_h, stock_idx = parse_index(stock_data)
    cand_h, cand_idx = parse_index(cand_data)
    if stock_h["frame_count"] != cand_h["frame_count"]:
        raise ValueError("frame count mismatch")
    if not stock_h["has_audio"]:
        raise ValueError("stock has no audio")

    merged_payloads: list[bytes] = []
    for i in range(stock_h["frame_count"]):
        s_chunk = stock_data[stock_idx[i][0] : stock_idx[i][0] + frame_size(stock_idx, len(stock_data), i)]
        c_chunk = cand_data[cand_idx[i][0] : cand_idx[i][0] + frame_size(cand_idx, len(cand_data), i)]
        stock_audio, _ = split_payload(s_chunk, True)
        _, cand_video = split_payload(c_chunk, True)
        merged = stock_audio + cand_video
        if len(merged) & 3:
            raise ValueError(f"frame {i} not 4-byte aligned")
        merged_payloads.append(merged)

    index_offset = cand_h["index_offset"]
    frame_count = cand_h["frame_count"]
    # RAD Bink 1 (BIKi) requires N+1 uint32 offsets in the frame table; entry N is EOF.
    header = bytearray(cand_data[:index_offset] + b"\x00" * ((frame_count + 1) * 4))
    if stock_h["audio_meta"]:
        struct.pack_into("<IHH I", header, 44, *stock_h["audio_meta"])

    body = b"".join(merged_payloads)
    offset = len(header)
    for i in range(frame_count):
        struct.pack_into("<I", header, index_offset + i * 4, offset | stock_idx[i][1])
        offset += len(merged_payloads[i])
    struct.pack_into("<I", header, index_offset + frame_count * 4, offset)

    total_size = len(header) + len(body)
    max_frame_size = max((len(p) for p in merged_payloads), default=0)
    struct.pack_into("<I", header, 4, total_size - 8)
    struct.pack_into("<I", header, 12, max_frame_size)

    dest.parent.mkdir(parents=True, exist_ok=True)
    dest.write_bytes(bytes(header) + body)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--stock", type=Path, required=True)
    ap.add_argument("--candidate-video", type=Path, required=True)
    ap.add_argument("--output", type=Path, required=True)
    args = ap.parse_args()
    rebuild(args.stock, args.candidate_video, args.output)
    print(f"Wrote {args.output} sha256={sha256(args.output)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

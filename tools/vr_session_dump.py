#!/usr/bin/env python3
"""Inspect a SingularityVR .vrrec session without launching the game.

The DLL writes a fixed header followed by packed samples.  This tool is the
first half of replay support: it turns a headset run into a compact CSV that
can be fed to the deterministic desktop tests or reviewed alongside the game
log.  It deliberately never edits the recording.
"""
import csv
import struct
import sys
from pathlib import Path

HEADER = struct.Struct("<IIII")
# Keep this in lockstep with vr_session_recorder.h.
SAMPLE = struct.Struct("<QI I 3f 2f 6f 6f 4f 4f 2f 3f 3f II".replace(" ", ""))
MAGIC = 0x31524356

def read(path):
    data = Path(path).read_bytes()
    if len(data) < HEADER.size:
        raise ValueError("recording is shorter than its header")
    magic, version, sample_bytes, _ = HEADER.unpack_from(data)
    if magic != MAGIC or version != 1 or sample_bytes != SAMPLE.size:
        raise ValueError(f"unsupported recording header magic={magic:#x} version={version} sample={sample_bytes}")
    body = data[HEADER.size:]
    if len(body) % sample_bytes:
        raise ValueError("truncated sample at end of recording")
    for off in range(0, len(body), sample_bytes):
        yield SAMPLE.unpack_from(body, off)

def main(argv):
    if len(argv) not in (2, 3):
        print(f"usage: {argv[0]} session.vrrec [output.csv]", file=sys.stderr)
        return 2
    rows = list(read(argv[1]))
    if len(argv) == 3:
        names = ("frame", "time_ms", "flags", "head_x", "head_y", "head_z", "head_yaw", "head_roll",
                 "left_x", "left_y", "left_z", "right_x", "right_y", "right_z",
                 "left_yaw", "right_yaw", "left_pitch", "right_pitch", "left_roll", "right_roll",
                 "left_trigger", "right_trigger", "left_grip", "right_grip",
                 "left_stick_x", "left_stick_y", "right_stick_x", "right_stick_y",
                 "ui_u", "ui_v", "muzzle_x", "muzzle_y", "muzzle_z", "hip_x", "hip_y", "hip_z",
                 "weapon", "prompt_action")
        with open(argv[2], "w", newline="", encoding="utf-8") as f:
            w = csv.writer(f); w.writerow(names); w.writerows(rows)
        print(f"wrote {len(rows)} samples to {argv[2]}")
    else:
        print(f"samples={len(rows)} first_frame={rows[0][0] if rows else 'n/a'} last_frame={rows[-1][0] if rows else 'n/a'}")
        if rows:
            flags = {}
            for row in rows:
                flags[row[2]] = flags.get(row[2], 0) + 1
            print("flag runs:", ", ".join(f"{k:#x}={v}" for k, v in sorted(flags.items())))
    return 0

if __name__ == "__main__":
    raise SystemExit(main(sys.argv))

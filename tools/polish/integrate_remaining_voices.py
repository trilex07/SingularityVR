import os
import sys
import json
import struct
import wave
import shutil
import numpy as np

BASE = r'C:\Users\Trilex\vr-mods\SingularityVR-polish-remaining-voices\remaining-voices'
STEAM_STORY_DIR = r'C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries\vr-assets\audio\polish\story'
STEAM_MANIFEST_PATH = r'C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries\vr-assets\localization\pl\polish_story_voice.jsonl'
REPO_MANIFEST_PATH = r'C:\Users\Trilex\vr-mods\SingularityVR-main-plates\localization\pl\polish_story_voice.jsonl'

CP1250 = bytes(range(256)).decode('cp1250', errors='replace')

def load_json_repair(path):
    try:
        with open(path, encoding='utf-8') as f:
            return json.load(f)
    except Exception:
        pass
    with open(path, 'rb') as fh:
        raw = fh.read()
    out = []
    i = 0
    while i < len(raw):
        b = raw[i]
        if b < 0x80:
            out.append(chr(b)); i += 1
        elif 0xC2 <= b <= 0xDF and i+1 < len(raw) and 0x80 <= raw[i+1] <= 0xBF:
            out.append(bytes([b, raw[i+1]]).decode('utf-8', errors='replace')); i += 2
        elif 0xE0 <= b <= 0xEF and i+2 < len(raw) and all(0x80 <= raw[i+k] <= 0xBF for k in (1, 2)):
            out.append(bytes([b, raw[i+1], raw[i+2]]).decode('utf-8', errors='replace')); i += 3
        else:
            out.append(CP1250[b]); i += 1
    return json.loads(''.join(out))

def convert_to_pcm16_mono(src_path, dst_path, target_peak_dbfs=-1.0):
    with open(src_path, 'rb') as fh:
        raw = fh.read()
    pos = 12; sr = 24000; channels = 1; bits = 16; fmt_tag = 1; data = None
    while pos + 8 <= len(raw):
        cid = raw[pos:pos+4]; sz = struct.unpack('<I', raw[pos+4:pos+8])[0]
        body = raw[pos+8:pos+8+sz]
        if cid == b'fmt ':
            fmt_tag, channels, sr = struct.unpack('<HHI', body[:8])
            bits = struct.unpack('<H', body[14:16])[0]
        elif cid == b'data':
            data = body
        pos += 8 + sz + (sz & 1)
    
    if data is None:
        raise ValueError(f"No data chunk found in {src_path}")

    if fmt_tag == 3 or bits == 32:
        samples = np.frombuffer(data, dtype=np.float32)
    elif bits == 16:
        samples = np.frombuffer(data, dtype=np.int16).astype(np.float32) / 32768.0
    else:
        raise ValueError(f'Unsupported format in {src_path}: tag={fmt_tag}, bits={bits}')
    
    if channels == 2:
        samples = (samples[0::2] + samples[1::2]) * 0.5
    
    max_val = np.max(np.abs(samples))
    target_val = 10.0 ** (target_peak_dbfs / 20.0)
    if max_val > 0:
        samples = samples * (target_val / max_val)
    pcm16 = np.clip(samples * 32767.0, -32768, 32767).astype(np.int16)
    
    dirname = os.path.dirname(dst_path)
    if dirname:
        os.makedirs(dirname, exist_ok=True)
    with wave.open(dst_path, 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(sr)
        w.writeframes(pcm16.tobytes())

def main():
    print("=== Singularity VR: Remaining Dubbing Integration ===")
    
    # Backup existing Steam manifest
    if os.path.exists(STEAM_MANIFEST_PATH):
        bak_path = STEAM_MANIFEST_PATH + '.bak_r21'
        shutil.copy2(STEAM_MANIFEST_PATH, bak_path)
        print(f"Backed up steam manifest to {bak_path}")

    manifest_rows = []
    with open(STEAM_MANIFEST_PATH, 'r', encoding='utf-8', errors='replace') as f:
        for line in f:
            line = line.strip()
            if line:
                manifest_rows.append(json.loads(line))
    print(f"Loaded existing steam manifest with {len(manifest_rows)} rows")

    row_by_asset = {}
    for idx, row in enumerate(manifest_rows):
        asset = row.get('asset')
        if asset:
            row_by_asset[asset] = idx

    all_candidates = load_json_repair(os.path.join(BASE, 'ALL_MANIFEST_ROWS.json'))
    bark_speech = load_json_repair(os.path.join(BASE, 'bark-speech.json'))

    converted_count = 0

    # 1) single_825b (1 item)
    print("\n--- Integrating 825b ---")
    src_825b = os.path.join(BASE, 'out', '825b', '825b608f491f738b.pl.wav')
    dst_825b = os.path.join(STEAM_STORY_DIR, '825b608f491f738b.wav')
    convert_to_pcm16_mono(src_825b, dst_825b)
    converted_count += 1
    if 'VI99_0305_KID' in row_by_asset:
        r = manifest_rows[row_by_asset['VI99_0305_KID']]
        r['native_replaced'] = "1"
    print("825b integrated: 825b608f491f738b.wav")

    # 2) vignette (6 items)
    print("\n--- Integrating Vignette (6 items) ---")
    for v in all_candidates['vignette']:
        asset = v['asset']
        src_wav = os.path.join(BASE, 'out', 'vignette', v['source_wav'])
        idx = row_by_asset[asset]
        target_row = manifest_rows[idx]
        dest_filename = target_row['file']
        dst_wav = os.path.join(STEAM_STORY_DIR, dest_filename)
        convert_to_pcm16_mono(src_wav, dst_wav)
        converted_count += 1
        target_row['native_replaced'] = "1"
        target_row['text_pl'] = v['polish']
        target_row['tts_text'] = v['polish']
        target_row['source_duration_s'] = v.get('source_duration', target_row.get('source_duration_s', 0))
        target_row['generated_duration_s'] = v.get('source_duration', target_row.get('generated_duration_s', 0))
    print(f"Vignette integrated: 6 files")

    # 3) skipped (150 items)
    print("\n--- Integrating Skipped candidates (150 items) ---")
    skipped_count = 0
    for s in all_candidates['skipped']:
        asset = s['asset']
        src_wav = os.path.join(BASE, 'out', 'skipped', s['source_wav'])
        alt_wav = os.path.join(BASE, 'out', 'skipped', s['package'] + '__' + s['asset'] + '.pl.wav')
        if not os.path.exists(src_wav) and os.path.exists(alt_wav):
            src_wav = alt_wav
        idx = row_by_asset[asset]
        target_row = manifest_rows[idx]
        dest_filename = target_row['file']
        dst_wav = os.path.join(STEAM_STORY_DIR, dest_filename)
        convert_to_pcm16_mono(src_wav, dst_wav)
        converted_count += 1
        skipped_count += 1
        target_row['native_replaced'] = "1"
        if s.get('dur'):
            target_row['generated_duration_s'] = s['dur']
    print(f"Skipped integrated: {skipped_count} files")

    # 4) intro (10 items)
    print("\n--- Integrating Intro (10 items) ---")
    intro_count = 0
    for item in all_candidates['intro']:
        asset = item['asset']
        src_wav = os.path.join(BASE, 'out', 'intro', item['source_wav'])
        dest_filename = f"{asset}.wav"
        dst_wav = os.path.join(STEAM_STORY_DIR, dest_filename)
        convert_to_pcm16_mono(src_wav, dst_wav)
        converted_count += 1
        intro_count += 1
        new_row = {
            "source": f"SP_VI_intro_Kismet_LOC_INT/SoundNodeWave/{asset}.ogg",
            "package": "SP_VI_intro_Kismet_LOC_INT",
            "asset": asset,
            "speaker_guess": "intro",
            "profile": "narrator_user",
            "text_en": item['english'],
            "text_pl": item['polish'],
            "translation_status": "machine_draft",
            "source_duration_s": item.get('source_duration', 0),
            "generated_duration_s": item.get('source_duration', 0),
            "file": dest_filename,
            "sample_rate": item.get('rate', 24000),
            "peak_dbfs": -1.0,
            "tts_text": item['polish'],
            "native_replaced": "1"
        }
        manifest_rows.append(new_row)
    print(f"Intro integrated: {intro_count} files")

    # 5) barks (256 items)
    print("\n--- Integrating Barks (256 items) ---")
    barks_count = 0
    for b in all_candidates['barks']:
        asset = b['asset']
        src_wav = os.path.join(BASE, 'out', 'barks', b['source_wav'])
        dest_filename = f"{asset}.wav"
        dst_wav = os.path.join(STEAM_STORY_DIR, dest_filename)
        convert_to_pcm16_mono(src_wav, dst_wav)
        converted_count += 1
        barks_count += 1
        meta = bark_speech.get(asset, {})
        pkg = meta.get('package', 'SP_VI5_kismet03_LOC_INT')
        new_row = {
            "source": f"{pkg}/SoundNodeWave/{asset}.ogg",
            "package": pkg,
            "asset": asset,
            "speaker_guess": "soldier",
            "profile": "soldier_natan",
            "text_en": b['en'],
            "text_pl": b['pl'],
            "translation_status": "machine_draft",
            "source_duration_s": b.get('budget', 0),
            "generated_duration_s": b.get('dur', 0),
            "file": dest_filename,
            "sample_rate": meta.get('rate', 24000),
            "peak_dbfs": -1.0,
            "tts_text": b['pl'],
            "native_replaced": "1"
        }
        manifest_rows.append(new_row)
    print(f"Barks integrated: {barks_count} files")

    # 6) multiplayer (409 items)
    print("\n--- Integrating Multiplayer (409 items) ---")
    mp_count = 0
    for m in all_candidates['mp']:
        package = m['package']
        asset = m['asset']
        src_wav = os.path.join(BASE, 'out', 'mp', f"{package}__{asset}.pl.wav")
        dest_filename = f"{package}__{asset}.wav"
        dst_wav = os.path.join(STEAM_STORY_DIR, dest_filename)
        convert_to_pcm16_mono(src_wav, dst_wav)
        converted_count += 1
        mp_count += 1
        new_row = {
            "source": f"{package}/SoundNodeWave/{asset}.ogg",
            "package": package,
            "asset": asset,
            "speaker_guess": "mp_announcer",
            "profile": "announcer_cml_ref",
            "text_en": m['en'],
            "text_pl": m['pl'],
            "translation_status": "machine_draft",
            "source_duration_s": m.get('budget', 0),
            "generated_duration_s": m.get('dur', 0),
            "file": dest_filename,
            "sample_rate": 24000,
            "peak_dbfs": -1.0,
            "tts_text": m['pl'],
            "native_replaced": "1"
        }
        manifest_rows.append(new_row)
    print(f"Multiplayer integrated: {mp_count} files")

    # Save to Steam manifest
    print(f"\nWriting updated manifest ({len(manifest_rows)} rows) to Steam...")
    with open(STEAM_MANIFEST_PATH, 'w', encoding='utf-8') as f:
        for r in manifest_rows:
            f.write(json.dumps(r, ensure_ascii=False) + '\n')
    print(f"Saved: {STEAM_MANIFEST_PATH}")

    # Save to repository manifest
    os.makedirs(os.path.dirname(REPO_MANIFEST_PATH), exist_ok=True)
    with open(REPO_MANIFEST_PATH, 'w', encoding='utf-8') as f:
        for r in manifest_rows:
            f.write(json.dumps(r, ensure_ascii=False) + '\n')
    print(f"Saved: {REPO_MANIFEST_PATH}")

    print(f"\nSUCCESS! Converted and deployed {converted_count} audio files.")
    print(f"Total manifest entries: {len(manifest_rows)}")

if __name__ == '__main__':
    main()

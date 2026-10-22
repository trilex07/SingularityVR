"""Reproducible offline extraction and inventory tool for Singularity (UE3 v584) audio assets.

Inspects offline cooked packages (.xxx), unpacks chunks or decompress whole-file,
extracts raw Ogg Vorbis payloads (preserving exact SHA-256), decodes to working WAV,
analyzes SoundCue-to-SoundNodeWave linkages, and generates the audio inventory.
No Steam writes. Audio files stay in private gitignored directories.
"""
import argparse
import hashlib
import json
import os
import struct
import subprocess
import sys
from pathlib import Path

# Add title tools for package inspection
SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parents[1]
sys.path.insert(0, str(SCRIPT_DIR))
from ue3_sound_package import inspect
from patch_opening_dialogue import bulks


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def detect_package_type(data: bytes) -> str:
    if len(data) < 32:
        return 'INVALID'
    tag = struct.unpack_from('<I', data, 0)[0]
    if tag != 0x9E2A83C1:
        return 'NOT_UE3'
    
    # Check for chunked summary table (unpack_chunks)
    matches = 0
    for p in range(12, min(len(data) - 24, 256)):
        flag = struct.unpack_from('<I', data, p)[0]
        if flag == 2:
            n = struct.unpack_from('<I', data, p + 4)[0]
            if 1 <= n <= 32 and p + 8 + n * 16 <= len(data):
                good = True
                for j in range(n):
                    r_off = p + 8 + j * 16
                    off, clen = struct.unpack_from('<II', data, r_off + 8)
                    if clen < 16 or off + clen > len(data):
                        good = False
                        break
                    if struct.unpack_from('<I', data, off)[0] != 0x9E2A83C1:
                        good = False
                        break
                if good:
                    matches += 1
    if matches == 1:
        return 'CHUNKED'
    
    # Check for whole-file compressed (uedecompress)
    block, csz, usz = struct.unpack_from('<III', data, 4)
    if block > 0 and block <= 1048576 and usz > 0:
        n_chunks = (usz + block - 1) // block
        hdr_len = 16 + n_chunks * 8
        if hdr_len <= len(data):
            sum_comp = sum(struct.unpack_from('<I', data, 16 + i * 8)[0] for i in range(n_chunks))
            if hdr_len + sum_comp <= len(data) + 65536:
                return 'WHOLE_COMPRESSED'
    
    ver, lver = struct.unpack_from('<HH', data, 4)
    if ver == 584:
        return 'UNCOMPRESSED'
        
    return 'UNKNOWN'


def unpack_package(pkg_path: Path, out_u_path: Path, tools_dir: Path) -> bool:
    if out_u_path.exists():
        return True
    
    data = pkg_path.read_bytes()
    pkg_type = detect_package_type(data)
    
    if pkg_type == 'UNCOMPRESSED':
        out_u_path.write_bytes(data)
        return True
    elif pkg_type == 'CHUNKED':
        unpack_exe = tools_dir / 'uedecompress' / 'unpack_chunks.exe'
        if not unpack_exe.exists():
            raise FileNotFoundError(f"unpack_chunks.exe not found at {unpack_exe}")
        res = subprocess.run([str(unpack_exe), str(pkg_path), str(out_u_path)], capture_output=True, text=True)
        return res.returncode == 0 and out_u_path.exists()
    elif pkg_type == 'WHOLE_COMPRESSED':
        uedecomp_exe = tools_dir / 'uedecompress' / 'uedecompress.exe'
        if not uedecomp_exe.exists():
            raise FileNotFoundError(f"uedecompress.exe not found at {uedecomp_exe}")
        res = subprocess.run([str(uedecomp_exe), str(pkg_path), str(out_u_path)], capture_output=True, text=True)
        return res.returncode == 0 and out_u_path.exists()
    else:
        raise ValueError(f"Unsupported package type {pkg_type} for {pkg_path}")


def classify_asset(package_name: str, asset_name: str, cue_names: list, num_channels: int, duration: float) -> str:
    name_lower = asset_name.lower()
    pkg_lower = package_name.lower()
    cues_lower = [c.lower() for c in cue_names]
    
    # Check for Dialogue / Voice
    if any(k in name_lower for k in ('vo_', 'voice', 'dialogue', 'speech', 'line_', 'bark')) or \
       'loc_' in pkg_lower or name_lower.startswith('intro_') or name_lower.startswith('sp_') and 'kismet' in name_lower:
        return 'DIALOGUE'
    
    # Check for Music
    if 'music' in pkg_lower or name_lower.startswith('concept') or 'mx' in name_lower or \
       any('music' in c for c in cues_lower) or any(k in name_lower for k in ('combatboss', 'explore', 'combattactical', 'combathorror', 'combatmixed', 'combatzek')):
        if duration >= 15.0 or 'music' in pkg_lower:
            return 'MUSIC'
    
    # Check for Weapon Mechanical (must be checked before WEAPON_FIRE)
    if any(k in name_lower for k in ('reload', 'cock', 'pump', 'cartin', 'cartout', 'dry', 'clipeject', 'boltaction', 'holster', 'draw', 'lowammo', 'handle', 'shellimpact', 'bulletshell')):
        return 'WEAPON_MECHANICAL'
    
    # Check for Weapon Fire
    if any(k in name_lower for k in ('fire', 'shot', 'gunfire', 'burst', 'latereflections')) and not any(k in name_lower for k in ('dry', 'lowammo', 'impact')):
        if any(w in pkg_lower or w in name_lower for w in ('pistol', 'machinegun', 'shotgun', 'sniper', 'railgun', 'rocket', 'gattling', 'weapon', 'revolver')):
            return 'WEAPON_FIRE'
    
    # Check for TMD / E99 Effects
    if any(k in name_lower for k in ('tmd', 'e99', 'chrono', 'deadlock', 'impulse', 'ageable', 'timeshift', 'disruptor')):
        return 'E99_EFFECT'
    
    # Check for Explosions
    if any(k in name_lower for k in ('explode', 'explosion', 'detonate', 'blast')) and not 'impulse' in name_lower:
        return 'EXPLOSION'
    
    # Check for Ambience
    if any(k in name_lower for k in ('drone', 'wind', 'rain', 'water', 'thunder', 'ambient', 'hum', 'creak', 'buzz', 'flies', 'metalcrush', 'howl', 'stinger')) or \
       'audio' in pkg_lower and duration >= 3.0:
        return 'AMBIENCE'
    
    return 'UNKNOWN'


def parse_package_full(data: bytes) -> dict:
    """Parse UE3 package exports and properties across all classes for SoundCue DAG analysis."""
    tag = struct.unpack_from('<I', data, 0)[0]
    assert tag == 0x9E2A83C1
    ver, lver = struct.unpack_from('<HH', data, 4)
    folder_len = struct.unpack_from('<i', data, 12)[0]
    p = 16 + abs(folder_len) * (2 if folder_len < 0 else 1) + 4
    nc, no, ec, eo, ic, io = struct.unpack_from('<6I', data, p)
    
    names = []
    p = no
    for _ in range(nc):
        n = struct.unpack_from('<i', data, p)[0]
        p += 4
        stop = p + abs(n) * (2 if n < 0 else 1)
        raw = data[p:stop]
        names.append(raw.decode('utf-16le' if n < 0 else 'latin1').rstrip('\0'))
        p = stop + 8
        
    def get_name(i):
        return names[i] if 0 <= i < len(names) else f"Name_{i}"
        
    imports = []
    for j in range(ic):
        imp = struct.unpack_from('<7i', data, io + j * 28)
        imports.append(imp)
        
    exports = []
    p = eo
    for j in range(ec):
        rec_off = p
        e = struct.unpack_from('<6iQ4i', data, p)
        p += 68 + e[10] * 4
        exports.append({
            'index': j + 1,
            'cls_idx': e[0],
            'super_idx': e[1],
            'outer_idx': e[2],
            'name_idx': e[3],
            'name': get_name(e[3]),
            'size': e[7],
            'offset': e[8],
            'rec_offset': rec_off
        })
        
    def resolve_ref(idx):
        if idx > 0:
            return exports[idx - 1]['name']
        elif idx < 0:
            imp = imports[-idx - 1]
            return get_name(imp[5])
        return 'None'

    for e in exports:
        e['class_name'] = resolve_ref(e['cls_idx'])

    # Property parser for all objects
    for e in exports:
        p = e['offset'] + 4
        end = e['offset'] + e['size']
        props = []
        try:
            while p < end:
                ni, num = struct.unpack_from('<ii', data, p)
                p += 8
                nm = get_name(ni)
                if nm == 'None':
                    break
                ti, tn, size, idx = struct.unpack_from('<4i', data, p)
                p += 16
                typ = get_name(ti)
                
                val = None
                if typ == 'BoolProperty':
                    val = bool(struct.unpack_from('<I', data, p)[0])
                    p += 4
                elif typ == 'ObjectProperty' and size == 4:
                    obj_idx = struct.unpack_from('<i', data, p)[0]
                    p += 4
                    val = {'index': obj_idx, 'name': resolve_ref(obj_idx)}
                elif typ == 'ArrayProperty':
                    count = struct.unpack_from('<i', data, p)[0]
                    p += 4
                    elem_bytes = (size - 4) // count if count > 0 else 0
                    elems = []
                    if elem_bytes == 4 and nm == 'ChildNodes':
                        for _ in range(count):
                            c_idx = struct.unpack_from('<i', data, p)[0]
                            p += 4
                            elems.append({'index': c_idx, 'name': resolve_ref(c_idx)})
                    else:
                        p += (size - 4)
                        elems = f"{count} items"
                    val = elems
                elif typ == 'FloatProperty' and size == 4:
                    val = struct.unpack_from('<f', data, p)[0]
                    p += 4
                elif typ == 'IntProperty' and size == 4:
                    val = struct.unpack_from('<i', data, p)[0]
                    p += 4
                elif typ == 'NameProperty' and size == 8:
                    val = get_name(struct.unpack_from('<i', data, p)[0])
                    p += 8
                else:
                    p += size
                    val = f"<{typ} {size}b>"
                props.append({'name': nm, 'type': typ, 'value': val})
        except Exception:
            pass
        e['properties'] = props

    return {'names': names, 'imports': imports, 'exports': exports}


def analyze_sound_cues(logical_data: bytes, package_info: dict) -> tuple:
    """Map SoundNodeWave names to referencing SoundCues with explicit confidence:
    - VERIFIED_REFERENCE: Parsed directly from UE3 SoundCue DAG / object pointers.
    - HEURISTIC_MATCH: Name/prefix similarity without direct pointer proof.
    - UNRESOLVED: Standalone wave with no verified reference.
    Returns: (wave_to_cues, wave_to_confidence, wave_to_paths)
    """
    names = package_info['names']
    exports = package_info['exports']
    
    cues = [e for e in exports if e.get('class_name') in ('SoundCue', 'RSoundNodeCue')]
    waves = {e['index']: e['name'] for e in exports if e.get('class_name') == 'SoundNodeWave'}
    
    verified_w2c = {}
    verified_w2paths = {}
    
    # Trace SoundCue DAG
    for cue in cues:
        cue_name = cue['name']
        first_p = next((p['value'] for p in cue.get('properties', []) if p['name'] == 'FirstNode'), None)
        first_idx = first_p.get('index', 0) if isinstance(first_p, dict) else (first_p if isinstance(first_p, int) else 0)
        
        if first_idx <= 0 or first_idx > len(exports):
            continue
            
        visited = set()
        queue = [(first_idx, [f"Cue({cue_name})"])]
        
        while queue:
            curr_idx, path = queue.pop(0)
            if curr_idx in visited or curr_idx <= 0 or curr_idx > len(exports):
                continue
            visited.add(curr_idx)
            
            node = exports[curr_idx - 1]
            node_name = node['name']
            node_cls = node.get('class_name', 'Node')
            curr_path = path + [f"{node_cls}({node_name})"]
            
            if node_cls == 'SoundNodeWave':
                verified_w2c.setdefault(node_name, []).append(cue_name)
                verified_w2paths.setdefault(node_name, []).append(" -> ".join(curr_path))
                continue
                
            # Check properties for LinkedCue
            linked = next((p['value'] for p in node.get('properties', []) if p['name'] == 'LinkedCue'), None)
            l_idx = linked.get('index', 0) if isinstance(linked, dict) else (linked if isinstance(linked, int) else 0)
            if l_idx > 0 and l_idx <= len(exports):
                l_cue = exports[l_idx - 1]
                l_first = next((p['value'] for p in l_cue.get('properties', []) if p['name'] == 'FirstNode'), None)
                lf_idx = l_first.get('index', 0) if isinstance(l_first, dict) else (l_first if isinstance(l_first, int) else 0)
                if lf_idx > 0:
                    queue.append((lf_idx, curr_path + [f"LinkedCue({l_cue['name']})"]))
                    
            # Check properties for ChildNodes array
            c_nodes = next((p['value'] for p in node.get('properties', []) if p['name'] == 'ChildNodes'), None)
            if c_nodes and isinstance(c_nodes, list):
                for child in c_nodes:
                    ch_idx = child.get('index', 0) if isinstance(child, dict) else (child if isinstance(child, int) else 0)
                    if ch_idx > 0:
                        queue.append((ch_idx, curr_path))
                        
            # Check raw binary end of node for int32 ChildNodes
            blob = logical_data[node['offset']:node['offset']+node['size']]
            if len(blob) >= 12:
                trailing_idx = struct.unpack_from('<i', blob, len(blob)-12)[0]
                if 0 < trailing_idx <= len(exports) and trailing_idx not in visited:
                    t_node = exports[trailing_idx - 1]
                    if t_node.get('class_name') in ('SoundNodeWave', 'SoundNodeModulator', 'RSoundNodeAudioOcclusionModifier', 'RSoundNodeCue'):
                        queue.append((trailing_idx, curr_path))
                        
    # Build complete classification for all waves
    wave_to_cues = {}
    wave_to_confidence = {}
    wave_to_paths = {}
    
    for w_idx, w_name in waves.items():
        if w_name in verified_w2c:
            wave_to_cues[w_name] = list(set(verified_w2c[w_name]))
            wave_to_confidence[w_name] = 'VERIFIED_REFERENCE'
            wave_to_paths[w_name] = verified_w2paths[w_name]
        else:
            # Check for heuristic match
            matched_cues = []
            for c in cues:
                c_name = c['name']
                if (w_name.lower().startswith(c_name.lower()) or c_name.lower().startswith(w_name.lower())) and len(c_name) > 3:
                    matched_cues.append(c_name)
            if matched_cues:
                wave_to_cues[w_name] = matched_cues
                wave_to_confidence[w_name] = 'HEURISTIC_MATCH'
                wave_to_paths[w_name] = [f"NameMatch({matched_cues[0]})"]
            else:
                wave_to_cues[w_name] = []
                wave_to_confidence[w_name] = 'UNRESOLVED'
                wave_to_paths[w_name] = []
                
    return wave_to_cues, wave_to_confidence, wave_to_paths


def process_package(pkg_path: Path, out_dir: Path, tools_dir: Path, extract_audio: bool = True) -> list:
    pkg_name = pkg_path.stem
    temp_u = out_dir / 'decompressed' / f"{pkg_name}.u"
    temp_u.parent.mkdir(parents=True, exist_ok=True)
    
    if not unpack_package(pkg_path, temp_u, tools_dir):
        print(f"FAILED to unpack {pkg_path.name}")
        return []
    
    logical_data = temp_u.read_bytes()
    pkg_info = inspect(logical_data)
    full_info = parse_package_full(logical_data)
    
    wave_to_cues, wave_to_conf, wave_to_paths = analyze_sound_cues(logical_data, full_info)
    
    records = []
    waves = [e for e in pkg_info['exports'] if e.get('class_name') == 'SoundNodeWave']
    
    for w in waves:
        asset_name = w['name']
        props = {p['name']: p['value'] for p in w.get('properties', [])}
        
        num_channels = props.get('NumChannels', 1)
        sample_rate = props.get('SampleRate', 44100)
        duration = float(props.get('Duration', 0.0))
        raw_pcm_size = props.get('RawPCMDataSize', int(duration * sample_rate * num_channels * 2))
        volume_rms = props.get('VolumeRMS')
        looping = props.get('Looping', False)
        
        # Extract payload
        payload_sha = ''
        payload_len = 0
        decoding_status = 'NOT_EXTRACTED'
        
        try:
            chunks = bulks(logical_data, w)
            payload = chunks[1][1]
            payload_len = len(payload)
            payload_sha = sha256_bytes(payload)
            
            if extract_audio and payload_len > 0:
                raw_ogg_path = out_dir / 'extracted' / 'ogg' / f"{pkg_name}_{asset_name}.ogg"
                raw_wav_path = out_dir / 'extracted' / 'wav' / f"{pkg_name}_{asset_name}.wav"
                raw_ogg_path.parent.mkdir(parents=True, exist_ok=True)
                raw_wav_path.parent.mkdir(parents=True, exist_ok=True)
                
                if not raw_ogg_path.exists():
                    raw_ogg_path.write_bytes(payload)
                
                if raw_wav_path.exists() and raw_wav_path.stat().st_size > 44:
                    decoding_status = 'DECODED_PCM16'
                else:
                    # Decode to 16-bit PCM WAV with ffmpeg
                    cmd = ['ffmpeg', '-y', '-v', 'error', '-i', str(raw_ogg_path), '-c:a', 'pcm_s16le', str(raw_wav_path)]
                    res = subprocess.run(cmd, capture_output=True, text=True)
                    if res.returncode == 0 and raw_wav_path.exists():
                        decoding_status = 'DECODED_PCM16'
                    else:
                        decoding_status = f"FFMPEG_ERROR: {res.stderr.strip()[:100]}"
            else:
                decoding_status = 'AVAILABLE_NOT_EXTRACTED'
        except Exception as ex:
            decoding_status = f"PARSE_ERROR: {str(ex)[:100]}"
            
        associated_cues = wave_to_cues.get(asset_name, [])
        mapping_confidence = wave_to_conf.get(asset_name, 'UNRESOLVED')
        cue_paths = wave_to_paths.get(asset_name, [])
        classification = classify_asset(pkg_name, asset_name, associated_cues, num_channels, duration)
        
        records.append({
            'package': pkg_name,
            'asset_name': asset_name,
            'export_index': w['index'],
            'class_name': 'SoundNodeWave',
            'classification': classification,
            'mapping_confidence': mapping_confidence,
            'associated_cues': associated_cues,
            'cue_paths': cue_paths,
            'codec': 'Ogg Vorbis (PC)',
            'channels': num_channels,
            'sample_rate': sample_rate,
            'duration_sec': round(duration, 4),
            'raw_pcm_bytes': raw_pcm_size,
            'sample_count': int(duration * sample_rate),
            'volume_rms': round(float(volume_rms), 5) if volume_rms is not None else None,
            'looping': looping,
            'payload_sha256': payload_sha,
            'payload_size_bytes': payload_len,
            'decoding_status': decoding_status
        })
        
    return records


def generate_markdown_inventory(records: list, out_path: Path):
    lines = [
        "# Singularity (UE3) Original Audio Inventory & Remaster Registry",
        "",
        "## Overview & Extraction Provenance",
        "",
        "- **Engine:** Unreal Engine 3 (Version 584, Licensee 126, 2010)",
        "- **Audio Codec:** Ogg Vorbis in Native Bulk Platform Slot 1 (PC)",
        "- **Decoding Chain:** UE3 Package Parser -> Bulk Vorbis Extraction -> FFmpeg PCM16",
        "- **Verification Policy:** Level 3 (exact SHA-256 source payload, declared PCM frame budget)",
        f"- **Total Identified Assets:** {len(records)}",
        "",
        "## Classification Summary",
        "",
        "| Category | Count | Primary Storage Packages |",
        "|----------|------:|--------------------------|"
    ]
    
    categories = ['MUSIC', 'WEAPON_FIRE', 'WEAPON_MECHANICAL', 'E99_EFFECT', 'EXPLOSION', 'AMBIENCE', 'DIALOGUE', 'UNKNOWN']
    by_cat = {c: [] for c in categories}
    for r in records:
        by_cat[r['classification']].append(r)
        
    for cat in categories:
        pkgs = sorted(list(set(r['package'] for r in by_cat[cat])))
        pkg_str = ", ".join(pkgs[:4]) + (f" (+{len(pkgs)-4} more)" if len(pkgs) > 4 else "")
        lines.append(f"| {cat} | {len(by_cat[cat])} | {pkg_str} |")
        
    lines.extend([
        "",
        "---",
        "",
        "## Priority Audio Assets Detail",
        ""
    ])
    
    for cat in ['MUSIC', 'WEAPON_FIRE', 'WEAPON_MECHANICAL', 'E99_EFFECT', 'EXPLOSION', 'AMBIENCE']:
        items = by_cat[cat]
        lines.append(f"### {cat} ({len(items)} assets)")
        lines.append("")
        lines.append("| Package | Asset Name | Ch | Rate (Hz) | Duration | RMS | Confidence | SoundCue Identity | SHA-256 (Source Vorbis) | Status |")
        lines.append("|---------|------------|---:|----------:|---------:|----:|-----------:|-------------------|--------------------------|--------|")
        for r in items[:50]: # Bounded table
            cues = ", ".join(r['associated_cues'][:2]) if r['associated_cues'] else "None"
            rms = f"{r['volume_rms']:.4f}" if r['volume_rms'] is not None else "-"
            sha_short = r['payload_sha256'][:12] + "..." if r['payload_sha256'] else "-"
            conf = r.get('mapping_confidence', 'UNRESOLVED')
            lines.append(f"| {r['package']} | `{r['asset_name']}` | {r['channels']} | {r['sample_rate']} | {r['duration_sec']}s | {rms} | **{conf}** | {cues} | `{sha_short}` | {r['decoding_status']} |")
        if len(items) > 50:
            lines.append(f"| ... | *({len(items)-50} additional {cat} assets cataloged in JSON)* | | | | | | | | |")
        lines.append("")
        
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text("\n".join(lines), encoding='utf-8')
    print(f"Generated markdown inventory: {out_path}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--cooked-dir', type=Path, default=Path(r'C:\Users\Trilex\vr-mods\SingularityVR-assets-local\cooked'))
    ap.add_argument('--private-dir', type=Path, default=Path(r'C:\Users\Trilex\vr-mods\SingularityVR-audio-music-sfx-remaster\audio-remaster-private'))
    ap.add_argument('--doc-out', type=Path, default=Path(r'C:\Users\Trilex\vr-mods\SingularityVR-audio-music-sfx-remaster\docs\audio_remaster_inventory.md'))
    ap.add_argument('--json-out', type=Path, default=Path(r'C:\Users\Trilex\vr-mods\SingularityVR-audio-music-sfx-remaster\docs\audio_inventory_manifest.json'))
    ap.add_argument('--extract-all', action='store_true', help='Extract all payloads to disk')
    args = ap.parse_args()
    
    tools_dir = SCRIPT_DIR.parent
    
    packages_to_scan = [
        # Weapons
        'wp_SP_ARPistol_SF.xxx',
        'wp_SP_ARMachineGun_SF.xxx',
        'wp_SP_ARShotgun_SF.xxx',
        'wp_SP_ARSniperRifle_SF.xxx',
        'wp_SP_ARGattlingGun_SF.xxx',
        'wp_SP_ARGrenadeLauncher_SF.xxx',
        'wp_SP_Railgun_SF.xxx',
        'wp_SP_RocketLauncher_SF.xxx',
        'wp_SP_ARSteerable_SF.xxx',
        'wp_SP_2007Pistol_SF.xxx',
        # Music
        'MUSIC_exploreDrone01_XSA.xxx',
        'MUSIC_exploreDrone02_XSA.xxx',
        'MUSIC_exploreMystery01_XSA.xxx',
        'MUSIC_exploreSuspense01_XSA.xxx',
        'MUSIC_combatBossGrom01_XSA.xxx',
        'MUSIC_combatHorror01_XSA.xxx',
        'MUSIC_combatMixed01_XSA.xxx',
        'MUSIC_combatMixed02_XSA.xxx',
        'MUSIC_combatTactical01_XSA.xxx',
        'MUSIC_combatTactical02_XSA.xxx',
        # Core gameplay / TMD / Explosions
        'RvGame.xxx',
        # Ambience
        'SP_Freight_Audio_2010_XSA.xxx',
        'SP_RL1_audio01_XSA.xxx',
        'SP_SL1_audio01_XSA.xxx'
    ]
    
    all_records = []
    
    for pkg_filename in packages_to_scan:
        pkg_path = args.cooked_dir / pkg_filename
        if not pkg_path.exists():
            print(f"Skipping absent package: {pkg_filename}")
            continue
        print(f"Scanning package: {pkg_filename} ({pkg_path.stat().st_size:,} bytes)...")
        records = process_package(pkg_path, args.private_dir, tools_dir, extract_audio=True)
        print(f"  Found {len(records)} waves in {pkg_filename}")
        all_records.extend(records)
        
    generate_markdown_inventory(all_records, args.doc_out)
    
    args.json_out.parent.mkdir(parents=True, exist_ok=True)
    args.json_out.write_text(json.dumps(all_records, indent=2, ensure_ascii=False), encoding='utf-8')
    print(f"Generated JSON manifest: {args.json_out} ({len(all_records)} total records)")


if __name__ == '__main__':
    main()

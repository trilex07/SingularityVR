"""Task B step 4 — walidacja gain na KOPIACH: ponowny pomiar + pelny dekod."""
import argparse, json, os, re, subprocess, sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__))))
from measure_packs import parse_wav_header, measure_volumedetect, measure_ebur128

def decode_check(ffmpeg, path):
    p = subprocess.run([ffmpeg, '-hide_banner', '-nostats', '-v', 'error',
                        '-i', path, '-f', 'null',
                        'NUL' if os.name == 'nt' else '/dev/null'],
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    return p.returncode, p.stderr.decode('utf-8', 'replace')

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--gained-dir', required=True)
    ap.add_argument('--out', required=True)
    ap.add_argument('--ffmpeg', default='ffmpeg')
    a = ap.parse_args()
    files = sorted(f for f in os.listdir(a.gained_dir) if f.endswith('.wav'))
    rows = []
    for i, f in enumerate(files):
        p = os.path.join(a.gained_dir, f)
        h = parse_wav_header(p)
        v = measure_volumedetect(a.ffmpeg, p)
        e = measure_ebur128(a.ffmpeg, p)
        rc, derr = decode_check(a.ffmpeg, p)
        rows.append({'file': f, 'header': h,
                     'peak_after_db': v['max_volume'], 'rms_after_db': v['mean_volume'],
                     'lufs_after': e['integrated_lufs'], 'true_peak_after_db': e['true_peak_dbtp'],
                     'decode_rc': rc, 'decode_errors': derr.strip()})
        if (i + 1) % 30 == 0:
            print('validated %d/%d' % (i + 1, len(files)), flush=True)
    with open(a.out, 'w', encoding='utf-8') as fh:
        for r in rows:
            fh.write(json.dumps(r, ensure_ascii=False) + '\n')
    peaks = [float(r['peak_after_db']) for r in rows if r['peak_after_db'] not in (None, '-inf')]
    bad_peak = [r['file'] for r in rows
                if r['peak_after_db'] is None or float(r['peak_after_db']) > -0.9]
    bad_dec = [r['file'] for r in rows if r['decode_rc'] != 0 or r['decode_errors']]
    bad_fmt = [r['file'] for r in rows
               if not (r['header'].get('ok') and r['header'].get('format') == 'PCM16'
                       and r['header'].get('sample_rate') == 24000
                       and r['header'].get('channels') == 1)]
    print('n=%d peak_mean=%.2f peak_min=%.2f peak_max=%.2f' % (
        len(peaks), sum(peaks) / len(peaks), min(peaks), max(peaks)), flush=True)
    print('over_ceiling(-0.9tol): %d %s' % (len(bad_peak), bad_peak[:5]), flush=True)
    print('decode_failures: %d %s' % (len(bad_dec), bad_dec[:5]), flush=True)
    print('format_violations: %d %s' % (len(bad_fmt), bad_fmt[:5]), flush=True)

if __name__ == '__main__':
    main()

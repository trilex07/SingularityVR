"""Offline timing screen: retain complete speech, reject excessive speed changes."""
import argparse
import hashlib
import json
import subprocess
import wave
from pathlib import Path


def fit(rows, ffmpeg, out):
    if out.exists():
        raise ValueError('refuse existing output')
    out.mkdir(parents=True)
    results = []
    for row in rows:
        source = Path(row['file'])
        if hashlib.sha256(source.read_bytes()).hexdigest() != row['sha256']:
            raise ValueError('source changed: '+row['asset'])
        with wave.open(str(source)) as wav:
            if (wav.getnchannels(),wav.getsampwidth(),wav.getframerate()) != (1,2,24000):
                raise ValueError('candidate must be mono PCM16/24k')
        if (row['native_rate'] not in (24000,44100,48000) or
                not isinstance(row['native_frames'],int) or row['native_frames'] <= 0):
            raise ValueError('invalid native budget')
        trimmed = out/(row['asset']+'.trim.wav')
        subprocess.run([str(ffmpeg),'-v','error','-n','-i',str(source),'-af',
            'silenceremove=start_periods=1:start_duration=0.03:start_threshold=-50dB,'
            'areverse,silenceremove=start_periods=1:start_duration=0.03:start_threshold=-50dB,areverse',
            '-ac','1','-ar','24000','-c:a','pcm_s16le',str(trimmed)],check=True)
        with wave.open(str(trimmed)) as wav:
            duration = wav.getnframes()/wav.getframerate()
        budget = row['native_frames']/row['native_rate']
        if budget <= .02 or duration <= 0:
            raise ValueError('invalid timing')
        tempo = max(1.0,duration/(budget-.02))
        result = dict(row, trimmed_duration=duration, required_tempo=tempo)
        if tempo > 1.85:
            result['timing_status'] = 'REJECT_UNSAFE_TEMPO'
        else:
            # Output remains mono PCM16/24k. Native packaging/resampling is MAIN-owned.
            # We do not crop to the budget; silence-only trim then atempo are sufficient.
            target = out/(row['asset']+'.wav')
            subprocess.run([str(ffmpeg),'-v','error','-n','-i',str(trimmed),'-af',
                f'atempo={tempo:.9f}', '-ac','1','-ar','24000','-c:a','pcm_s16le',str(target)],check=True)
            with wave.open(str(target)) as wav:
                actual = wav.getnframes()/wav.getframerate()
                assert (wav.getnchannels(),wav.getsampwidth(),wav.getframerate()) == (1,2,24000)
            # atempo rounding may vary by a few ms; retain safety margin, never cut speech.
            if actual > budget-.005:
                result['timing_status'] = 'REJECT_FILTER_BUDGET_OVERRUN'
            else:
                result.update(file=str(target.resolve()),sha256=hashlib.sha256(target.read_bytes()).hexdigest(),
                    fitted_duration=actual,timing_status='PASS_TIMING_ONLY_NOT_AUDITIONED')
        results.append(result)
    (out/'fitted.json').write_text(json.dumps(results,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps([{k:r.get(k) for k in ('asset','required_tempo','timing_status')} for r in results]))
    return results


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('--manifest',type=Path,required=True)
    ap.add_argument('--ffmpeg',type=Path,required=True)
    ap.add_argument('--out',type=Path,required=True)
    args = ap.parse_args()
    fit(json.loads(args.manifest.read_text(encoding='utf-8')),args.ffmpeg,args.out)

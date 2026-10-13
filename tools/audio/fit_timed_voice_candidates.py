"""Offline timing screen: retain complete speech, reject excessive speed changes."""
import argparse
import array
import hashlib
import json
import subprocess
import wave
import sys
from pathlib import Path


def guarded_silence_trim(source, target, guard_ms):
    """Keep first/last audible samples plus a boundary guard, not a speech VAD."""
    with wave.open(str(source)) as wav:
        params = wav.getparams()
        pcm = array.array('h',wav.readframes(wav.getnframes()))
    if sys.byteorder!='little':
        pcm.byteswap()
    # -65dBFS. Faint consonants remain within the explicit guard. No interior cuts.
    audible = [index for index,value in enumerate(pcm) if abs(value)>=19]
    if not audible:
        raise ValueError('no audible samples')
    margin = round(params.framerate*guard_ms/1000)
    first,last = max(0,audible[0]-margin),min(len(pcm),audible[-1]+margin+1)
    output = pcm[first:last]
    if sys.byteorder!='little':
        output.byteswap()
    with wave.open(str(target),'wb') as wav:
        wav.setparams(params)
        wav.writeframes(output.tobytes())
    return dict(first_source_sample=first,last_source_sample=last,
                silence_threshold_pcm16=19,edge_guard_ms=guard_ms)


def fit(rows, ffmpeg, out, edge_guard_ms=None):
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
        trim_evidence = dict(method='legacy_ffmpeg_silenceremove')
        if edge_guard_ms is not None:
            if not 20<=edge_guard_ms<=150:
                raise ValueError('invalid edge guard')
            trim_evidence = dict(method='guarded_contiguous_pcm_trim',
                **guarded_silence_trim(source,trimmed,edge_guard_ms))
        else:
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
        result = dict(row, trimmed_duration=duration, required_tempo=tempo,
                      trim_evidence=trim_evidence,generated_source_wav_sha256=row['sha256'],
                      generated_source_file=str(source.resolve()))
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
    ap.add_argument('--edge-guard-ms',type=int)
    args = ap.parse_args()
    fit(json.loads(args.manifest.read_text(encoding='utf-8')),args.ffmpeg,args.out,args.edge_guard_ms)

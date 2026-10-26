"""Guarded single-interval trim/fitting. Never cuts speech to match a budget."""
import array
import sys
import wave
from pathlib import Path
from patch_opening_dialogue import require

MAX_DEPLOYMENT_TEMPO = 1.35


def native_tempo(guarded_duration, native_duration):
    """The input already includes trim_guarded's edge-clamped 40 ms guards."""
    require(native_duration > .02 and guarded_duration > 0, 'native budget')
    return max(1., guarded_duration / (native_duration - .02))

def trim_guarded(source:Path,out:Path):
    with wave.open(str(source)) as w:
        require(w.getnchannels()==1 and w.getsampwidth()==2,'mono PCM16 required')
        rate=w.getframerate();samples=array.array('h',w.readframes(w.getnframes()))
    if sys.byteorder!='little':samples.byteswap()
    # -65dBFS, contiguous span +40ms each side: preserve fricatives/quiet tails.
    audible=[i for i,x in enumerate(samples) if abs(x)>=18]
    require(bool(audible),'silent source')
    guard=round(rate*.04);first=max(0,audible[0]-guard);last=min(len(samples),audible[-1]+1+guard)
    kept=samples[first:last]
    if sys.byteorder!='little':kept.byteswap()
    with wave.open(str(out),'wb') as w:
        w.setnchannels(1);w.setsampwidth(2);w.setframerate(rate);w.writeframes(kept.tobytes())
    return len(kept)/rate

def fit(run,source,folder,asset,rate,frames,channels=1):
    require(channels in (1,2),'unsupported native channel count')
    trimmed=folder/(asset+'.trim.wav');duration=trim_guarded(source,trimmed)
    budget=frames/rate;require(budget>.02,'native budget')
    tempo=native_tempo(duration,budget)
    if tempo>1.85:return None,dict(reason='guarded duration requires unsafe tempo',tempo=tempo,native_duration=budget,source_duration=duration)
    spoken=folder/(asset+'.spoken.wav')
    run(['-v','error','-n','-i',str(trimmed),'-af',f'atempo={tempo:.9f},alimiter=limit=0.7:level=disabled:latency=1',
        '-ac',str(channels),'-ar',str(rate),'-c:a','pcm_s16le',str(spoken)])
    with wave.open(str(spoken)) as w:
        require(w.getframerate()==rate and w.getnchannels()==channels,'spoken format')
        if w.getnframes()>frames:return None,dict(reason='atempo output exceeds native frames; refuse speech crop',tempo=tempo,native_duration=budget,source_duration=duration)
    target=folder/(asset+'.ogg')
    # Spoken frame count was checked above: atrim can remove padding ONLY.
    run(['-v','error','-n','-i',str(spoken),'-af',f'apad=whole_len={frames},atrim=end_sample={frames}',
         '-c:a','libvorbis','-q:a','3',str(target)])
    decoded=folder/(asset+'.decoded.wav')
    run(['-v','error','-n','-i',str(target),'-c:a','pcm_s16le',str(decoded)])
    with wave.open(str(decoded)) as w:
        require(w.getframerate()==rate and w.getnchannels()==channels,'decoded format mismatch')
        decoded_frames=w.getnframes()
    padding_correction=0
    if 0 < frames-decoded_frames <= 256:
        # Measured decoder priming loss, not an assumed codec delay. Re-encode
        # the same verified speech with extra TAIL SILENCE only. A single
        # bounded retry must independently decode to the exact native budget.
        padding_correction=frames-decoded_frames
        retry=folder/(asset+'.timing-retry.ogg')
        retry_decoded=folder/(asset+'.timing-retry.wav')
        padded_frames=frames+padding_correction
        run(['-v','error','-n','-i',str(spoken),'-af',
             f'apad=whole_len={padded_frames},atrim=end_sample={padded_frames}',
             '-c:a','libvorbis','-q:a','3',str(retry)])
        run(['-v','error','-n','-i',str(retry),'-c:a','pcm_s16le',str(retry_decoded)])
        with wave.open(str(retry_decoded)) as w:
            require(w.getframerate()==rate and w.getnchannels()==channels,'retry format mismatch')
            decoded_frames=w.getnframes()
        if decoded_frames==frames:
            target=retry
    if decoded_frames!=frames:
        return None,dict(reason='encoded native PCM budget mismatch; bounded silence retry refused',tempo=tempo,native_duration=budget,source_duration=duration,decoded_frames=decoded_frames)
    return target,dict(tempo=tempo,guard_ms=40,source_duration=duration,native_frames=frames,rate=rate,channels=channels,measured_tail_padding_frames=padding_correction)

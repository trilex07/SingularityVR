"""MAIN offline native delivery of SIDE-reviewed first-area room speech.

Only2 exact zone02a banks; no accepted radio/intro/mixer/cue/metadata changes.
SIDE generates speech; this tool encodes and independently validates delivery.
"""
import argparse
import json
import subprocess
import wave
from pathlib import Path
from native_story_package import build
from patch_opening_dialogue import bulks, require
from prepare_radio_loudness import match, sha
from ue3_sound_package import inspect

BANKS = {
    'SP_VI2_zone02a_LOC_INT': ('578ad1547a69ccdc7f50d2d9c616c5ecc8c038ba0e388c8de218225f35be3c7d',
                             'd15ec25249d8a1e7dc487bbb8ab0f60be8416d59076a51ec63582b79a290ae65'),
    'SP_VI2_zone02a_2010_LOC_INT': ('d79485c60a05ba05fbdd64c61fd367bc5c8390f6226f31ebd5625d69fe712070',
                                  'f8d541723290d55c85ecfea7bb86198d66eb541bd505e77f282167cdd9728cc1')
}


def main():
    ap = argparse.ArgumentParser()
    for arg in ('bindings','cooked','ffmpeg','decoder','out'): ap.add_argument('--'+arg, type=Path, required=True)
    ap.add_argument('--bindings-sha256', required=True)
    a = ap.parse_args()
    require(not a.out.exists(), 'refuse existing output')
    binding_bytes = a.bindings.read_bytes()
    require(sha(binding_bytes) == a.bindings_sha256.lower(), 'SIDE binding changed')
    rows = json.loads(binding_bytes)
    require(len(rows)==19 and len({(r['package'],r['asset']) for r in rows})==19, 'exact19 identities required')
    require({r['package'] for r in rows}==set(BANKS), 'exact2 first-area banks required')
    for r in rows:
        require(r['semantic_status']=='SIDE_FULL_NATIVE_TEXT_REVIEW' and r['qa_status']=='PASS_AUTOMATED_SCREEN', 'unreviewed speech')
        require(sha(Path(r['file']).read_bytes()) == r['selected_wav_sha256'], 'SIDE WAV changed')
    a.out.mkdir(parents=True)
    def run(argv):
        return subprocess.run([str(a.ffmpeg), '-hide_banner', '-nostats', *argv], capture_output=True, text=True, check=True).stderr
    report = []
    for package, (bank_sha, logical_sha) in BANKS.items():
        original_path = a.cooked/(package+'.xxx')
        original = original_path.read_bytes()
        require(sha(original)==bank_sha, 'original native bank changed')
        folder = a.out/package; folder.mkdir()
        logical_path = folder/'original.u'
        subprocess.run([str(a.decoder),str(original_path),str(logical_path)], capture_output=True,check=True)
        logical = logical_path.read_bytes(); require(sha(logical)==logical_sha, 'logical bank mismatch')
        native = {e['name']:e for e in inspect(logical)['exports'] if e['class_name']=='SoundNodeWave'}
        selected = [r for r in rows if r['package']==package]
        require({r['asset'] for r in selected}==set(native), 'incomplete/unknown native target set')
        replacements = {}; proof = []
        for r in selected:
            asset=r['asset']; e=native[asset]; props={p['name']:p['value'] for p in e['properties']}
            spoken=next(p for p in e['properties'] if p['name']=='SpokenText')
            require(r['source_bank_sha256']==bank_sha and r['source_logical_sha256']==logical_sha, 'SIDE source identity mismatch')
            require(props['SpokenText']==r['source_english'] and spoken['value_offset']==r['source_property_offset'] and
                    spoken['size']==r['source_property_size'] and
                    sha(logical[spoken['value_offset']:spoken['value_offset']+spoken['size']])==r['source_property_sha256'], 'SpokenText property mismatch')
            frames=r['native_frames']; rate=r['native_rate']
            require(props['NumChannels']==1 and props['SampleRate']==rate==48000 and props['RawPCMDataSize']==frames*2, 'native PCM identity')
            source=Path(r['file']);require(sha(source.read_bytes())==r['selected_wav_sha256'],'WAV changed during preparation')
            reference=folder/(asset+'.native.ogg');reference.write_bytes(bulks(logical,e)[1][1])
            native_pcm=folder/(asset+'.native.wav')
            run(['-v','error','-n','-i',str(reference),'-c:a','pcm_s16le',str(native_pcm)])
            fitted_pcm=folder/(asset+'.selected.wav')
            # Do not trim or re-tempo the SIDE-fitted utterance. Verify the full
            # resampled payload fits BEFORE match() may remove padding only.
            run(['-v','error','-n','-i',str(source),'-ar',str(rate),'-ac','1','-c:a','pcm_s16le',str(fitted_pcm)])
            for path, exact in ((native_pcm,True),(fitted_pcm,False)):
                with wave.open(str(path)) as w:
                    require(w.getframerate()==rate and w.getnchannels()==1 and w.getsampwidth()==2,'PCM format')
                    require(w.getnframes()==frames if exact else 0<w.getnframes()<=frames, 'refuse speech crop / native duration mismatch')
            target, loudness = match(run,fitted_pcm,reference,folder,asset,rate,frames)
            replacements[asset]=target.read_bytes()
            proof.append(dict(asset=asset,source_wav_sha256=r['selected_wav_sha256'],text_pl=r['text_pl'],**loudness))
            print(package,asset,'exact frames / loudness / peak PASS',flush=True)
        for kind, clips in (('control',{}),('candidate',replacements)):
            expected, packed=build(original,logical,clips,bank_sha,logical_sha)
            dest=folder/(kind+'.xxx');dest.write_bytes(packed)
            decoded=folder/(kind+'.roundtrip.u')
            subprocess.run([str(a.decoder),str(dest),str(decoded)],capture_output=True,check=True)
            require(decoded.read_bytes()==expected,'independent LZO/absolute offset roundtrip failed')
        report.append(dict(package=package,source_sha256=bank_sha,candidate_sha256=sha((folder/'candidate.xxx').read_bytes()),
                           lines=proof,status='OFFLINE_PASS_NOT_NATIVE_LOAD_OR_AUDITORY_ACCEPTANCE'))
        (a.out/'room-banks.json').write_text(json.dumps(report,indent=2,ensure_ascii=False),encoding='utf-8')
    print('PASS:2 banks/19 native waves; exact metadata/cues/platforms; no Steam writes',flush=True)


if __name__=='__main__': main()

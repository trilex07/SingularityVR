"""Generate a reversible, offline repair candidate for failed dubbing QA rows.
Original voicepacks are never modified. Each candidate is generated as 24 kHz mono PCM16.
"""
from __future__ import annotations
import hashlib, io, json, re, sys, wave
from pathlib import Path
sys.path.insert(0,r'C:\Users\Trilex\vr-mods\.piper_tools')
sys.path.insert(0,r'C:\Users\Trilex\vr-mods\.pocket_tools')
import numpy as np
from scipy.io import wavfile
from scipy.signal import resample_poly
from piper import PiperVoice
ROOT=Path(__file__).resolve().parents[2]
QA=ROOT/'research/localization/polish_dubbing_qa_final.json'
OUT=ROOT/'audio/polish/repair_candidate_20261013/full_audit'
VOICE_ROOT=ROOT/'audio/models/piper'
VOICES={'mc_speech':PiperVoice.load(str(VOICE_ROOT/'pl_PL-mc_speech-medium/pl_PL-mc_speech-medium.onnx')),'darkman':PiperVoice.load(str(VOICE_ROOT/'pl_PL-darkman-medium/pl_PL-darkman-medium.onnx'))}
CORRECTION_FILE=ROOT/'audio/polish/repair_candidate_20261013/corrections.local.json'
_corrections=json.loads(CORRECTION_FILE.read_text(encoding='utf-8')) if CORRECTION_FILE.exists() else {}
STORY_CORRECTIONS=_corrections.get('STORY_CORRECTIONS',{})
UI_CORRECTIONS=_corrections.get('UI_CORRECTIONS',{})
TOKEN_MAP=_corrections.get('TOKEN_MAP',{})
ITEM_MAP=_corrections.get('ITEM_MAP',{})
def clean(text:str)->str:
 text=re.sub(r'<[^>]+>',' ',text or '')
 text=re.sub(r'\[([^\]]+)\]',lambda m:TOKEN_MAP.get(m.group(1).lower(),ITEM_MAP.get(m.group(1),m.group(1))),text)
 text=text.replace('`~',' ').replace('\\n',' ')
 for a,b in ITEM_MAP.items():text=re.sub(r'\b'+re.escape(a)+r'\b',b,text,flags=re.I)
 text=re.sub(r'\bE99\b','E dziewięćdziesiąt dziewięć',text,flags=re.I)
 text=re.sub(r'\bTMD\b','te em de',text,flags=re.I)
 text=re.sub(r'Katorga[- ]12','Katorga dwanaście',text,flags=re.I)
 text=re.sub(r'\bOnline\b','sieci',text,flags=re.I)
 text=re.sub(r'\bVolk S4\b','Wołk es cztery',text,flags=re.I)
 text=re.sub(r'\bKasimov\b','Kasimow',text,flags=re.I)
 text=re.sub(r'\bSNV[- ]E dziewięćdziesiąt dziewięć\b','es en we, E dziewięćdziesiąt dziewięć',text,flags=re.I)
 text=re.sub(r'\s+',' ',text).strip()
 return text

def trim_edges(a,rate):
 active=np.flatnonzero(np.abs(a)>=.002)
 if not len(active):return a
 start=max(0,int(active[0])-int(.03*rate));end=min(len(a),int(active[-1])+int(.07*rate));a=a[start:end].copy()
 n=min(int(.005*rate),len(a)//2)
 if n:a[:n]*=np.linspace(0,1,n,dtype=np.float32);a[-n:]*=np.linspace(1,0,n,dtype=np.float32)
 return a

def render(text,voice,path):
 b=io.BytesIO()
 with wave.open(b,'wb') as w:voice.synthesize_wav(clean(text),w)
 b.seek(0);rate,a=wavfile.read(b);a=np.asarray(a)
 if a.ndim==2:a=a.mean(axis=1)
 if np.issubdtype(a.dtype,np.integer):a=a.astype(np.float32)/np.iinfo(a.dtype).max
 else:a=a.astype(np.float32)
 if rate!=24000:a=resample_poly(a,24000,rate)
 peak=float(np.max(np.abs(a))) if len(a) else 0
 if peak:a*=10**(-3/20)/peak
 a=trim_edges(np.clip(a,-1,1),24000)
 wavfile.write(str(path),24000,np.asarray(np.clip(a,-1,1)*32767,dtype=np.int16))
 return len(a)/24000

def main():
 qa=json.loads(QA.read_text(encoding='utf-8'))['results']; flagged={}
 for r in qa:
  if any(n.startswith('FAIL_') for n in r.get('notes',[])):flagged[(r['pack'],r['line_id'])]=r
 story={x.get('source'):x for x in [json.loads(l) for l in (ROOT/'audio/polish/story_voicepack_nllb/manifest.jsonl').read_text(encoding='utf-8').splitlines() if l.strip()]}
 ui={x.get('id'):x for x in [json.loads(l) for l in (ROOT/'audio/polish/full_voicepack/manifest.jsonl').read_text(encoding='utf-8').splitlines() if l.strip()]}
 OUT.mkdir(parents=True,exist_ok=True); rows=[]
 for (pack,line_id),q in flagged.items():
  if pack=='story_nllb':
   src=story.get(line_id)
   if not src:continue
   asset=src.get('asset',''); text=STORY_CORRECTIONS.get(asset,src.get('text_pl_machine',''))
   profile=src.get('profile','narrator_user'); voice='darkman' if profile=='demichev_ruslan' else 'mc_speech'
   original=ROOT/'audio/polish/story_voicepack_nllb'/src['file']; outname=hashlib.sha1((pack+line_id).encode()).hexdigest()[:16]+'.wav'
  else:
   src=ui.get(line_id)
   if not src:continue
   text=UI_CORRECTIONS.get(line_id,src.get('pl',''));profile=src.get('profile','narrator_user');voice='mc_speech'
   original=ROOT/'audio/polish/full_voicepack'/src['file'];outname=hashlib.sha1((pack+line_id).encode()).hexdigest()[:16]+'.wav';asset=src.get('key','')
  if not text.strip():continue
  dest=OUT/outname
  tts=clean(text);duration=render(tts,VOICES[voice],dest)
  rows.append({'pack':pack,'line_id':line_id,'asset_or_key':asset,'source_english':q.get('text_en') or q.get('en'),'previous_polish':q.get('expected_pl'),'corrected_polish':text,'tts_text':tts,'previous_audio':str(original),'file':str(dest.resolve()),'profile_before':profile,'candidate_voice_engine':'Piper pl_PL-'+voice+'-medium','generated_duration_s':round(duration,3),'correction_applied':line_id in UI_CORRECTIONS or asset in STORY_CORRECTIONS,'baseline_qa_notes':q.get('notes',[])})
 (OUT/'manifest.jsonl').write_text('\n'.join(json.dumps(x,ensure_ascii=False) for x in rows)+'\n',encoding='utf-8')
 print(json.dumps({'baseline_failure_rows':len(flagged),'generated':len(rows),'story':sum(x['pack']=='story_nllb' for x in rows),'ui':sum(x['pack']=='full_localization' for x in rows),'manual_text_corrected':sum(x['correction_applied'] for x in rows),'output':str(OUT)},ensure_ascii=False))
if __name__=='__main__':main()

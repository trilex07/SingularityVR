"""Conservative word-error screen, never acting/pronunciation or native approval."""
import argparse
import hashlib
import json
import re
from pathlib import Path


def words(text):
    return re.findall(r'\w+',text.lower())


def word_error(expected,detected):
    a,b = words(expected),words(detected)
    previous = list(range(len(b)+1))
    for index,left in enumerate(a,1):
        current = [index]
        for other,right in enumerate(b,1):
            current.append(min(current[-1]+1,previous[other]+1,previous[other-1]+(left!=right)))
        previous = current
    return previous[-1]/max(1,len(a))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--selected',type=Path,required=True)
    ap.add_argument('--asr',type=Path,required=True)
    ap.add_argument('--out',type=Path,required=True)
    args = ap.parse_args()
    if args.out.exists():
        raise ValueError('refuse existing output')
    rows = json.loads(args.selected.read_text(encoding='utf-8'))
    asr = {r['asset']:r for r in json.loads(args.asr.read_text(encoding='utf-8'))}
    if len(asr)!=len(rows):
        raise ValueError('ASR coverage mismatch')
    for row in rows:
        if hashlib.sha256(Path(row['file']).read_bytes()).hexdigest()!=row['sha256']:
            raise ValueError('selected WAV changed')
        item = asr[row['asset']]
        error = word_error(row['text_pl'],item['detected'])
        expected,heard = words(row['text_pl']),words(item['detected'])
        flags = []
        if len(heard)<len(expected)*.65:
            flags.append('POSSIBLE_OMISSION')
        if error>.3 or (len(expected)<=4 and error>0):
            flags.append('CONTENT_OR_PRONUNCIATION_REVIEW')
        row.update(asr_detected=item['detected'],asr_word_error=error,asr_flags=flags,
                   qa_status='REVIEW_OR_REPAIR' if flags else 'PASS_AUTOMATED_SCREEN',
                   qa_limit='ASR only; no acting, fidelity, voice identity or native-playback approval')
    args.out.write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf-8')
    from collections import Counter
    print(json.dumps(dict(count=len(rows),status=dict(Counter(r['qa_status'] for r in rows)))))


if __name__ == '__main__':
    main()

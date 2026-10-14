import json, os
AUDIO = 'C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish'
OUTDIR = AUDIO + '/story_voicepack_xlingual'
with open(os.path.join(OUTDIR, 'QA_FLAGS.json'), encoding='utf-8') as f:
    flags = json.load(f)
targets = set()
for k in ('long_token', 'abrupt_tail', 'too_short'):
    for row in flags.get(k, []):
        targets.add(row[0])
targets |= {'825b608f491f738b.wav', 'db5a9bb47d436a5a.wav'}
with open(os.path.join(OUTDIR, '_asr_queue.txt'), 'w') as f:
    f.write('\n'.join(sorted(targets)))
print('queued:', len(targets))

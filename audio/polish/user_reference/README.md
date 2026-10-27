# User voice reference

Source inspected: `agresja.m4a`.

- duration: 27.307 s
- source: 48 kHz, stereo M4A
- source peak: -7.77 dBFS
- source RMS: -35.11 dBFS
- first 0.5 s RMS: -64.06 dBFS
- clipped samples: 0
- prepared reference: `agresja_reference.wav`
- prepared duration: 25.528 s
- prepared format: 48 kHz, mono, PCM 16-bit, normalized to -3 dBFS peak

The original M4A remains untouched. The WAV is trimmed with short padding and
is ready to be used as a Pocket-TTS reference clip.

Second source inspected: `Nagrywanie.m4a`.

- duration: 26.624 s
- source: 48 kHz, stereo M4A
- source peak: -18.09 dBFS
- source RMS: -40.77 dBFS
- first 0.5 s RMS: -64.15 dBFS
- clipped samples: 0
- prepared reference: `spokoj_reference.wav`
- prepared duration: 24.714 s
- prepared format: 48 kHz, mono, PCM 16-bit, normalized to -3 dBFS peak

Both references have clean starts, no clipping and enough speech for a first
Pocket-TTS comparison. Their original levels differed, so the prepared WAVs
are normalized consistently before model testing.

# First-area native NPC speech, offline SIDE handoff

The accepted opening radio and room-recording banks do not cover the next
scripted NPC voice banks. This bounded investigation covers only these three
user-owned localization packages, 32 SoundNodeWave exports in total:

- `SP_VI2_kismet02_LOC_INT`: 16 exports, source SHA-256
  `44c42e06f8c5830f1ff7deb01231b6262374957ef76aad31e8c3d814f334da56`.
- `SP_VI2_kismet03_LOC_INT`: 5 exports, source SHA-256
  `cd1d4a0565e803366d9bba59f4ad01fbd97519741b71158f1c8e5e53fc40ca17`.
- `SP_VI2_kismet04_LOC_INT`: 11 exports, source SHA-256
  `84773631379d0af88d61db8371e8d3eb17cbffdaa279410b849e3b8ca67db80b`.

## Evidence and limits

`inspect_scripted_speech.py` resolves typed `SeqAct_PlaySound.PlaySound`,
`InterpTrackSound.Sounds`, and `InterpTrackFaceFX.FaceFXSoundCueKeys` properties,
then local cue `FirstNode` and node `ChildNodes` to imported wave identity.
It preserves FName numbers, outer chains, serialized property offsets and
hashes. It refuses unknown observed array structures rather than scanning
integers for plausible references.

Ten spoken native bindings have a proven offline scripted producer-to-wave
chain. Thirteen further spoken bindings are independently proven native bank
exports but their triggering producer is unresolved; bank presence is not
runtime playback proof. The preserved MAIN session log contains no typed
speech-playback identity. The exact initial-pistol streaming boundary was not
established in this bounded investigation. No full-game coverage claim is made.

Full original source text was read from each native `SpokenText` property.
Several previous ASR-derived source transcripts and translations were
unrelated or omitted clauses. Reuse was limited to complete faithful native
meaning; duplicate native exports remain separately bound. Original coughs,
screams and death sounds were explicitly retained, not synthesized as spoken
descriptions.

## Private candidate

Private delivery is under this SIDE checkout's ignored
`deployment/hands-dubbing/npc-delivery-final/`:

- 16 selected WAVs: 14 new local-model takes and 2 reused-source takes fitted
  independently to their different native timing budgets.
- 23 spoken native bindings and 9 retained original nonverbal exports.
- 13 conservative ASR-screen passes, 3 reviews (two uncertain first-word
  consonants and one proper name). None has auditory approval.
- All selected WAV format/hash/timing checks pass: mono PCM16, 24 kHz;
  maximum tempo 1.711303; contiguous -65 dBFS span with 40 ms guards.
- No interior word cuts. Rejected incomplete EOS takes, excessive-tempo takes
  and filter-budget overruns were not promoted into final delivery.

The selected WAV identity is `sha256` / `selected_wav_sha256`, independently
checked against the delivered file. Historical source-take hashes are not
selected-output identities. Bindings contain full private EN/PL, source bank
and logical hashes, property spans, native rates/frame counts, cue routes,
producer evidence, selected WAV metadata and explicit unresolved gates.

Final manifest SHA-256:

- `native-bindings.json`:
  `dafb19fa4bcfef397fba98b5ae58047f465f72c5bab8af2c8c2dbd96f07bfc55`
- `selected.json`:
  `99cbc2a81c9d73823a17ecc18544803cf6f4bb22fd5b8e764455133d548b3fbf`
- `native-encoding-gates.json`:
  `fa0afba6fc3f28dbc68e47709ad53851943dcf9d22e49628ef211bd06acbad44`

Only MAIN may encode/repack/deploy. Native Vorbis must independently decode to
the exact original frame count, sample rate and channel count; nominal duration
is insufficient. Native timing properties, cues and accepted intro/radio/room
audio must remain unchanged. No native package encoding or Steam writes were
performed by SIDE, and no game was launched.

## Checks and source hygiene

Six synthetic tagged-graph tests and five inherited native-audit tests pass.
`git diff --check` passes. Only generic read-only tooling, synthetic tests and
this evidence summary are committed. Original assets, dialogue transcripts,
translations, WAVs, decoded packages and private manifests remain ignored.

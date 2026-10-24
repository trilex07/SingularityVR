# Feature status — combined candidate

Installed means this exact combined artifact. No row is installed by SIDE.

| Feature | SIDE source | In final branch | Built | Installed | PSVR2 grade | Missing work |
|---|---|---|---|---|---|---|
| HUD 3D | f307ad32 (5181cdc4 + 36081c3d) | INTEGRATED reader reflection + wrist probe | OFFLINE PASS 240 pixel cases | No | PENDING; old orientation failure addressed in code, hand offset open | MAIN physical acceptance |
| Reload comfort | 54bd946 | INTEGRATED | OFFLINE PASS | No | Pending new radii | 18cm Valk/Volk physical test |
| Accuracy | eade95a via 09887b1 | INTEGRATED | OFFLINE PASS | No | Proposed balance unaccepted | Separate 100% vs A/B |
| Single-owner texture bridge | PR57 via 09887b1 | INTEGRATED | OFFLINE PASS D3D9/Ex | No | Pending | Log-only then four-map replacement |
| Volk + gloves 2K | d837d68 | PREPARED private stage | OFFLINE PASS hashes/mips | No | Pending | MAIN controlled gate |
| Valkyrie 4K / Kasimov normal | d837d68 | PREPARED assets | OFFLINE PASS assets only | No | Pending | BLOCKED independent runtime scope extension/review |
| Centurion / Kasimov diffuse | d837d68 | BLOCKED | No | No | Not tested | Verified top mip/source identity |
| Detailed icons | 3223b24 | INTEGRATED | OFFLINE PASS | No | Prior user PHYSICAL PASS for source; combined pending | Preserve PL/EN atlas |
| TMD/gloves/haptics | inherited baseline | INTEGRATED unchanged | OFFLINE PASS | No | Left-hand report open | MAIN physical validation |
| WeaponLocker | inherited ABI fix | INTEGRATED unchanged | OFFLINE PASS compiled ABI | No | Combined pending | Preserve acquisition/settings |
| NIS/OLED | inherited | INTEGRATED unchanged | OFFLINE PASS pixels | No | Prior reports only | Combined physical check |
| Notes/font | existing Polish assets | INTEGRATED | Synthetic PASS; package pin BLOCKED | No | Current unverified | Reconcile accepted font hash; no repatch |
| Eight audio remasters | ba8774c | Tools INTEGRATED, WAV PREPARED | OFFLINE PASS PCM | No | Not tested | Native cue packaging/listening separate |
| XTTS | c3c911d; 602a5c historical | Existing tooling retained | OFFLINE PASS tool regressions | No | Playback unverified | Accepted bank pins and selected take manifest |
| Official Polish Bink | existing expanded routing; 47b4ef7 research | INTEGRATED unchanged | Compiled | No | Current unverified | Preserve original tracks/language config |

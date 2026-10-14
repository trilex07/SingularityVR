# libbink `binkenc` (offline)

Singularity needs **Bink 1 `BIKi`** files. FFmpeg decodes Bink but does not encode it.

Expected encoder build output:

```text
third_party/libbink-src/libbink-main/encoder/src/LibBink.Encoder/bin/Release/net8.0/binkenc.dll
```

On this SIDE host, `git clone https://github.com/jmarshall23/libbink.git` and the GitHub API return **404** (repo not reachable over HTTPS/git). A **56 KiB** Wayback snapshot zip was used instead:

```text
third_party/libbink-wayback.zip  →  Expand-Archive  →  third_party/libbink-src/
```

Build:

```powershell
.\tools\build-binkenc.ps1
```

Requires **.NET 8 SDK** and **ffmpeg/ffprobe** (WinGet Gyan FFmpeg 9.x on this machine).

`binkenc` emits mono audio; stock `SNG_logo.bik` is stereo. After encode, run `tools/title/remux_bink_stock_audio.py` so `validate-video` matches the stock layout.

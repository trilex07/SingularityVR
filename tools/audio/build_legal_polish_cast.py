"""Build legal Polish cast references (RHVoice, Piper, CML) and Pocket demo lines.

Outputs under audio/polish/cast_legal_20261014/ — local only, NC-friendly voices.
"""
from __future__ import annotations

import json
import shutil
import subprocess
import sys
import urllib.request
import zipfile
from pathlib import Path

sys.path.insert(0, r"C:\Users\Trilex\vr-mods\.pocket_tools")
sys.path.insert(0, r"C:\Users\Trilex\vr-mods\.piper_tools")
sys.path.insert(0, str(Path(__file__).resolve().parent))

import numpy as np
from pocket_tts import TTSModel
from piper import PiperVoice, SynthesisConfig
from scipy.io import wavfile
from scipy.signal import resample_poly

ROOT = Path(__file__).resolve().parents[2]
AUDIO = ROOT / "audio"
OUT = AUDIO / "polish" / "cast_legal_20261014"
REF = OUT / "references"
DEMO = OUT / "pocket_demos"
FFMPEG = Path(
    r"C:\Users\Trilex\vr-mods\SingularityVR-polish-integration\tools\ffmpeg\ffmpeg-9.0.2-essentials_build\bin\ffmpeg.exe"
)
PIPER = AUDIO / "models" / "piper"
RHVOICE_ROOT = ROOT / "tools" / "rhvoice_sapi_local"
INSTALLERS = RHVOICE_ROOT / "installers"

DOWNLOADS = {
    "demo-natan.ogg": "https://github.com/RHVoice/natan-pol/releases/download/4.12/demo-natan.ogg",
    "demo-magda.ogg": "https://github.com/RHVoice/magda-pol/releases/download/rev46/demo-magda.ogg",
    "natan_setup.exe": "https://github.com/RHVoice/natan-pol/releases/download/4.12/RHVoice-voice-Polish-Natan-v4.12.1021.23-setup.exe",
    "magda_setup.exe": "https://github.com/RHVoice/magda-pol/releases/download/rev46/RHVoice-voice-Polish-Magda-v4.46.1021.23-setup.exe",
    "pocket_cml_reference.wav": "https://huggingface.co/shefowl/pocket-tts-polish-6l/resolve/main/reference.wav",
    "piper_gosia.onnx": "https://huggingface.co/rhasspy/piper-voices/resolve/v1.0.0/pl/pl_PL/gosia/medium/pl_PL-gosia-medium.onnx",
    "piper_gosia.onnx.json": "https://huggingface.co/rhasspy/piper-voices/resolve/v1.0.0/pl/pl_PL/gosia/medium/pl_PL-gosia-medium.onnx.json",
}

REFERENCE_LINES = {
    "rhvoice_natan": (
        "To jest głos Natana z projektu RHVoice. Mówimy spokojnie, wyraźnie i stanowczo. "
        "Sektor jest zabezpieczony. Przechodzimy do następnego punktu kontrolnego."
    ),
    "rhvoice_magda": (
        "To jest głos Magdy z projektu RHVoice. Potrzebuję jeszcze chwili. "
        "Dane z laboratorium są prawie kompletne. Zatrzymajcie reaktor, zanim będzie za późno."
    ),
    "piper_mc_speech": (
        "Uwaga. Wczytywanie sektora zakończone. Broń główna gotowa. "
        "Postępuj zgodnie z rozkazami dowódcy."
    ),
    "piper_gosia": (
        "Komunikat systemowy. Zapisywanie postępu. "
        "Wszystkie jednostki proszone są o potwierdzenie odbioru."
    ),
    "piper_darkman": (
        "Procedura została uruchomiona. Nie ma już odwrotu. "
        "Rdzeń temporalny osiągnął niestabilność."
    ),
}

POCKET_DEMOS = [
    ("lektor_natan_ref", "rhvoice_natan_reference.wav", "narrator_ui", "Wczytywanie. Proszę czekać."),
    ("lektor_magda_ref", "rhvoice_magda_reference.wav", "narrator_ui", "Zapisywanie postępu gry."),
    ("lektor_cml_ref", "pocket_cml_reference.wav", "narrator_ui", "Opcje gry. Tryb wieloosobowy."),
    ("lektor_mc_ref", "piper_mc_speech_reference.wav", "narrator_ui", "Wróć do gry. Zmień drużynę."),
    ("kathryn_magda_ref", "rhvoice_magda_reference.wav", "kathryn", "Potrzebuję jeszcze chwili. Dane są prawie kompletne."),
    ("soldier_natan_ref", "rhvoice_natan_reference.wav", "soldier", "Sektor jest zabezpieczony. Ruszamy dalej."),
    ("scientist_gosia_ref", "piper_gosia_reference.wav", "scientist", "Metal E99 zawiera oczyszczony izotop."),
    ("announcer_cml_ref", "pocket_cml_reference.wav", "announcer", "Wynik jest remisowy po regulaminowym czasie."),
    ("demichev_darkman_ref", "piper_darkman_reference.wav", "demichev", "Nie ma już odwrotu. Procedura zostanie uruchomiona."),
]


def download(name: str, url: str, dest: Path) -> None:
    dest.parent.mkdir(parents=True, exist_ok=True)
    if dest.is_file() and dest.stat().st_size > 0:
        return
    print(f"download {name}", flush=True)
    urllib.request.urlretrieve(url, dest)


def ogg_to_wav(ogg: Path, wav: Path) -> None:
    if wav.is_file():
        return
    subprocess.run(
        [str(FFMPEG), "-y", "-v", "error", "-i", str(ogg), "-ac", "1", "-ar", "24000", str(wav)],
        check=True,
    )


def normalize_peak(audio: np.ndarray, peak_dbfs: float = -3.0) -> np.ndarray:
    peak = float(np.max(np.abs(audio))) if audio.size else 0.0
    if peak <= 0:
        return audio
    return np.clip(audio * ((10 ** (peak_dbfs / 20)) / peak), -1.0, 1.0)


def write_wav(path: Path, audio: np.ndarray, rate: int = 24000) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    wavfile.write(
        str(path),
        rate,
        np.asarray(np.rint(np.clip(audio, -1.0, 1.0) * 32767.0), dtype=np.int16),
    )


def piper_render(model: Path, text: str, out_wav: Path) -> None:
    voice = PiperVoice.load(model)
    out_wav.parent.mkdir(parents=True, exist_ok=True)
    with open(out_wav, "wb") as f:
        import wave

        with wave.open(f, "wb") as wav:
            voice.synthesize_wav(text, wav, SynthesisConfig())
    # resample to 24k mono pcm if needed
    rate, data = wavfile.read(str(out_wav))
    data = np.asarray(data)
    if data.ndim == 2:
        data = data.mean(axis=1)
    if np.issubdtype(data.dtype, np.integer):
        data = data.astype(np.float32) / max(abs(np.iinfo(data.dtype).min), np.iinfo(data.dtype).max)
    else:
        data = data.astype(np.float32)
    if rate != 24000:
        data = resample_poly(data, 24000, rate)
    write_wav(out_wav, normalize_peak(data.astype(np.float32)))


def ensure_rhvoice_installed() -> bool:
    """Try silent SAPI install; returns False when admin is required."""
    INSTALLERS.mkdir(parents=True, exist_ok=True)
    natan_exe = INSTALLERS / "natan_setup.exe"
    magda_exe = INSTALLERS / "magda_setup.exe"
    download("natan_setup", DOWNLOADS["natan_setup.exe"], natan_exe)
    download("magda_setup", DOWNLOADS["magda_setup.exe"], magda_exe)
    RHVOICE_ROOT.mkdir(parents=True, exist_ok=True)
    marker = RHVOICE_ROOT / ".voices_installed"
    if marker.is_file():
        return True
    ok = True
    for exe, sub in ((natan_exe, "Natan"), (magda_exe, "Magda")):
        target = RHVOICE_ROOT / sub
        target.mkdir(parents=True, exist_ok=True)
        try:
            proc = subprocess.run([str(exe), "/S", f"/D={target}"], check=False)
            if proc.returncode != 0:
                ok = False
        except OSError:
            ok = False
    if ok:
        marker.write_text("ok\n", encoding="utf-8")
    return ok


def extend_short_reference(short_wav: Path, piper_fill_text: str, out_wav: Path, piper_model: Path) -> None:
    """Blend RHVoice demo timbre hint with Piper Polish body for a longer Pocket prompt."""
    if out_wav.is_file():
        return
    rate, demo = wavfile.read(str(short_wav))
    demo = np.asarray(demo, dtype=np.float32)
    if demo.ndim == 2:
        demo = demo.mean(axis=1)
    if np.issubdtype(demo.dtype, np.integer):
        demo = demo.astype(np.float32) / 32768.0
    if rate != 24000:
        demo = resample_poly(demo, 24000, rate)
    tmp = out_wav.with_suffix(".piper.tmp.wav")
    piper_render(piper_model, piper_fill_text, tmp)
    _, body = wavfile.read(str(tmp))
    body = np.asarray(body, dtype=np.float32)
    if body.ndim == 2:
        body = body.mean(axis=1)
    body = body / (np.max(np.abs(body)) + 1e-9)
    demo = normalize_peak(demo)
    # prepend ~1.2 s of demo timbre, then main piper body (distinct, not a celebrity clone)
    head = demo[: int(24000 * 1.2)] if demo.size else demo
    merged = np.concatenate([head, body])
    write_wav(out_wav, normalize_peak(merged))
    tmp.unlink(missing_ok=True)


def sapi_wav(voice_substring: str, text: str, out_wav: Path) -> bool:
    ps = rf"""
Add-Type -AssemblyName System.Speech
$s = New-Object System.Speech.Synthesis.SpeechSynthesizer
$voice = $s.GetInstalledVoices() | ForEach-Object {{ $_.VoiceInfo }} | Where-Object {{ $_.Name -like '*{voice_substring}*' }} | Select-Object -First 1
if (-not $voice) {{ exit 2 }}
$s.SelectVoice($voice.Name)
$s.SetOutputToWaveFile('{out_wav.as_posix()}')
$s.Speak(@'
{text}
'@)
$s.SetOutputToDefaultAudioDevice()
"""
    r = subprocess.run(["powershell", "-NoProfile", "-Command", ps], capture_output=True, text=True)
    return r.returncode == 0 and out_wav.is_file()


def pocket_demo(reference: Path, text: str, out_wav: Path, temp: float = 0.48) -> None:
    model = TTSModel.load_model(
        config="hf://shefowl/pocket-tts-polish-6l/config.yaml", temp=temp, quantize=True
    )
    state = model.get_state_for_audio_prompt(str(reference))
    audio = model.generate_audio(state, text, max_tokens=100, frames_after_eos=4).numpy().squeeze()
    audio = normalize_peak(audio.astype(np.float32))
    write_wav(out_wav, audio, model.sample_rate)


def build_reel(manifest_rows: list[dict], out_wav: Path) -> None:
    silence = np.zeros(int(24000 * 0.5), dtype=np.float32)
    blocks: list[np.ndarray] = []
    timeline: list[dict] = []
    cursor = 0.0
    for row in manifest_rows:
        path = Path(row["file"])
        rate, data = wavfile.read(str(path))
        data = np.asarray(data, dtype=np.float32)
        if data.ndim == 2:
            data = data.mean(axis=1)
        if np.issubdtype(data.dtype, np.integer):
            data = data.astype(np.float32) / 32768.0
        if rate != 24000:
            data = resample_poly(data, 24000, rate)
        data = normalize_peak(data)
        start = cursor
        blocks.extend([data, silence])
        cursor += len(data) / 24000 + len(silence) / 24000
        timeline.append({**row, "start_s": round(start, 2), "end_s": round(cursor - len(silence) / 24000, 2)})
    write_wav(out_wav, np.concatenate(blocks))
    (OUT / "cast_listen_reel.json").write_text(json.dumps(timeline, ensure_ascii=False, indent=2), encoding="utf-8")


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    REF.mkdir(parents=True, exist_ok=True)
    DEMO.mkdir(parents=True, exist_ok=True)
    cache = OUT / "downloads"
    cache.mkdir(parents=True, exist_ok=True)

    for key, url in DOWNLOADS.items():
        download(key, url, cache / Path(key).name)

    ogg_to_wav(cache / "demo-natan.ogg", REF / "rhvoice_natan_demo_short.wav")
    ogg_to_wav(cache / "demo-magda.ogg", REF / "rhvoice_magda_demo_short.wav")

    gosia_dir = PIPER / "pl_PL-gosia-medium"
    gosia_dir.mkdir(parents=True, exist_ok=True)
    if not (gosia_dir / "pl_PL-gosia-medium.onnx").is_file():
        shutil.copy2(cache / "piper_gosia.onnx", gosia_dir / "pl_PL-gosia-medium.onnx")
        shutil.copy2(cache / "piper_gosia.onnx.json", gosia_dir / "pl_PL-gosia-medium.onnx.json")
    shutil.copy2(cache / "pocket_cml_reference.wav", REF / "pocket_cml_reference.wav")

    installed = ensure_rhvoice_installed()
    rhvoice_ok = {
        "natan": sapi_wav("Natan", REFERENCE_LINES["rhvoice_natan"], REF / "rhvoice_natan_reference.wav"),
        "magda": sapi_wav("Magda", REFERENCE_LINES["rhvoice_magda"], REF / "rhvoice_magda_reference.wav"),
    }
    if not rhvoice_ok["natan"]:
        extend_short_reference(
            REF / "rhvoice_natan_demo_short.wav",
            REFERENCE_LINES["rhvoice_natan"],
            REF / "rhvoice_natan_reference.wav",
            PIPER / "pl_PL-mc_speech-medium" / "pl_PL-mc_speech-medium.onnx",
        )
        rhvoice_ok["natan"] = "fallback_demo_piper_hybrid"
    if not rhvoice_ok["magda"]:
        extend_short_reference(
            REF / "rhvoice_magda_demo_short.wav",
            REFERENCE_LINES["rhvoice_magda"],
            REF / "rhvoice_magda_reference.wav",
            PIPER / "pl_PL-gosia-medium" / "pl_PL-gosia-medium.onnx",
        )
        rhvoice_ok["magda"] = "fallback_demo_piper_hybrid"

    piper_render(
        PIPER / "pl_PL-mc_speech-medium" / "pl_PL-mc_speech-medium.onnx",
        REFERENCE_LINES["piper_mc_speech"],
        REF / "piper_mc_speech_reference.wav",
    )
    piper_render(
        PIPER / "pl_PL-darkman-medium" / "pl_PL-darkman-medium.onnx",
        REFERENCE_LINES["piper_darkman"],
        REF / "piper_darkman_reference.wav",
    )
    piper_render(
        gosia_dir / "pl_PL-gosia-medium.onnx",
        REFERENCE_LINES["piper_gosia"],
        REF / "piper_gosia_reference.wav",
    )

    cast_manifest: list[dict] = []
    for stem, ref_name, role, text in POCKET_DEMOS:
        ref = REF / ref_name
        if not ref.is_file():
            continue
        out = DEMO / f"{stem}.wav"
        pocket_demo(ref, text, out)
        cast_manifest.append(
            {
                "file": str(out.resolve()),
                "role": role,
                "reference": str(ref.resolve()),
                "text": text,
                "engine": "Pocket-TTS Polish 6L",
            }
        )

    mapping = {
        "generated_at": "20261014",
        "distribution": "non-commercial free mod",
        "rhvoice_sapi_install": str(RHVOICE_ROOT.resolve()),
        "rhvoice_sapi_install_attempted": installed,
        "rhvoice_sapi_ok": rhvoice_ok,
        "references": {k: str((REF / f"{k}_reference.wav").resolve()) for k in []},
        "reference_files": sorted(str(p.resolve()) for p in REF.glob("*.wav")),
        "recommended_cast": {
            "narrator_ui_primary": "references/rhvoice_natan_reference.wav -> pocket lektor",
            "narrator_ui_alt_female": "references/rhvoice_magda_reference.wav",
            "kathryn_alt": "references/rhvoice_magda_reference.wav (zamiast uk accent gdy za ostry)",
            "soldier_pool": "references/rhvoice_natan_reference.wav",
            "scientist_civilian_female": "references/piper_gosia_reference.wav",
            "demichev_alt_nc": "references/piper_darkman_reference.wav (Piper NC — tylko NC mod)",
            "barisov_devlin_kathryn": "bez zmian — accent_reference + character_samples_12",
        },
        "legal_notes": {
            "natan": "CC0 — RHVoice/natan-pol",
            "magda": "CC BY 4.0 — RHVoice/magda-pol (atrybucja)",
            "cml_pocket_ref": "CC BY 4.0 — CML-TTS via pocket-tts-polish-6l",
            "piper_mc_gosia": "sprawdź MODEL_CARD rhasspy/piper-voices",
            "piper_darkman": "często non-commercial — OK dla darmowego moda",
            "no_celebrity_clones": "nie naśladujemy rozpoznawalnych zmarłych lektorów — tylko licencjonowane głosy TTS",
        },
        "pocket_demos": cast_manifest,
    }
    (OUT / "CAST_MANIFEST.json").write_text(json.dumps(mapping, ensure_ascii=False, indent=2), encoding="utf-8")
    build_reel(cast_manifest, OUT / "cast_listen_reel.wav")
    print(json.dumps({"out": str(OUT), "rhvoice_ok": rhvoice_ok, "demos": len(cast_manifest)}, ensure_ascii=False))


if __name__ == "__main__":
    main()

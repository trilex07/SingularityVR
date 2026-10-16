import subprocess, os
OUT = r'C:\Users\Trilex\AppData\Local\Temp\opencode\dubbing-qa\resynth_intro_piper'
PIPER = r'C:\Users\Trilex\vr-mods\.piper_tools\bin\piper.exe'
VOICE = r'C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\models\piper\pl_PL-darkman-medium\pl_PL-darkman-medium.onnx'
a = 'INTRO_1005_NIM'
r1 = subprocess.run([PIPER, '-m', VOICE, '-i', os.path.join(OUT, a + '.txt'),
                     '-f', os.path.join(OUT, a + '.raw2.wav')],
                    capture_output=True, text=True, errors='replace')
print('rc:', r1.returncode, flush=True)
print('STDERR-TAIL:', (r1.stderr or '')[-600:], flush=True)

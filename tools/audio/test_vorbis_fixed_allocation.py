"""Real Ogg encode/decode + byte-preserving native replacement regressions."""
import os, struct, subprocess, sys, tempfile, unittest, wave
from pathlib import Path
from unittest.mock import patch
from vorbis_fixed_allocation import pages, pad_to_allocation, _headers_and_audio
from build_native_remaster_candidate import ROOT, replace_payload, offline_path, whole_pack

class FixedAllocationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.ff=Path(os.environ.get('SINGULARITYVR_FFMPEG','ffmpeg'))
        try: subprocess.run([str(cls.ff),'-version'],check=True,capture_output=True)
        except (OSError,subprocess.CalledProcessError):raise unittest.SkipTest('set SINGULARITYVR_FFMPEG for real codec tests')
        base=ROOT/'build/audio-release-private/tests';base.mkdir(parents=True,exist_ok=True)
        cls.tmp=tempfile.TemporaryDirectory(dir=base);cls.dir=Path(cls.tmp.name)
        pcm=bytes((0,0,17,1,223,254,0,0))*12000
        cls.wav=cls.dir/'input.wav'
        with wave.open(str(cls.wav),'wb') as w:w.setnchannels(2);w.setsampwidth(2);w.setframerate(48000);w.writeframes(pcm)
        cls.ogg=cls.dir/'input.ogg'
        subprocess.run([str(cls.ff),'-v','error','-y','-i',str(cls.wav),'-c:a','libvorbis','-q:a','3',str(cls.ogg)],check=True,capture_output=True)
        cls.source=cls.ogg.read_bytes()
    @classmethod
    def tearDownClass(cls):cls.tmp.cleanup()
    def padded(self):
        # Some sizes are unreachable exactly due to valid page/lacing overhead.
        for size in range(len(self.source)+1000,len(self.source)+1100):
            try:return pad_to_allocation(self.source,size)
            except ValueError:pass
        self.fail('no reachable fixed allocation')
    def test_valid_crc_and_audio_packets_granules_preserved(self):
        candidate=self.padded();old,a,_=_headers_and_audio(self.source);new,b,_=_headers_and_audio(candidate)
        self.assertGreater(len(new[1]),len(old[1]));self.assertEqual(old[0],new[0]);self.assertEqual(old[2],new[2])
        self.assertEqual([p[:18]+p[26:] for p in a],[p[:18]+p[26:] for p in b]);self.assertTrue(pages(candidate)[-1][5]&4)
    def test_padding_preserves_short_vendor_and_existing_comments(self):
        old,_,_=_headers_and_audio(self.source);new,_,_=_headers_and_audio(self.padded())
        n=struct.unpack_from('<I',old[1],7)[0];m=struct.unpack_from('<I',new[1],7)[0]
        self.assertEqual(old[1][11:11+n],new[1][11:11+m])
        self.assertEqual(n,m)
        self.assertIn(b'TRILEX_PADDING=',new[1])
        self.assertEqual(struct.unpack_from('<I',new[1],11+m)[0],struct.unpack_from('<I',old[1],11+n)[0]+1)
    def test_actual_decoder_keeps_exact_timing_format(self):
        p=self.dir/'padded.ogg';p.write_bytes(self.padded());out=self.dir/'decoded.wav'
        subprocess.run([str(self.ff),'-v','error','-y','-i',str(p),'-c:a','pcm_s16le',str(out)],check=True,capture_output=True)
        reference=self.dir/'reference.wav'
        subprocess.run([str(self.ff),'-v','error','-y','-i',str(self.ogg),'-c:a','pcm_s16le',str(reference)],check=True,capture_output=True)
        with wave.open(str(out)) as w, wave.open(str(reference)) as r:
            self.assertEqual((w.getnframes(),w.getnchannels(),w.getframerate()),(r.getnframes(),r.getnchannels(),r.getframerate()))
            self.assertEqual(w.readframes(w.getnframes()),r.readframes(r.getnframes()))
    def test_crc_corruption_refused(self):
        b=bytearray(self.source);b[-10]^=1
        with self.assertRaisesRegex(ValueError,'CRC'):pages(b)
    def test_truncation_refused(self):
        with self.assertRaises(ValueError):pages(self.source[:-4])
    def test_growth_refused(self):
        with self.assertRaisesRegex(ValueError,'exceeds'):pad_to_allocation(self.source,50)
    def test_trailing_junk_refused(self):
        with self.assertRaises(ValueError):pages(self.source+b'junk')
    def test_native_replacement_preserves_properties_offsets_otherbytes(self):
        candidate=self.padded();payload=b'OggS'+b'x'*(len(candidate)-4)
        prefix=b'NATIVE PROPERTIES';chunks=bytearray(prefix)
        for i in range(4):
            p=payload if i==1 else b'';chunks+=struct.pack('<4I',0,len(p),len(p),len(chunks)+16)+p
        e=dict(name='known',class_name='SoundNodeWave',offset=0,size=len(chunks),property_end=len(prefix),properties=[])
        logical=bytes(chunks)+b'OTHER EXPORTS REMAIN HERE'
        with patch('build_native_remaster_candidate.inspect',return_value={'exports':[e]}):
            output,old,_,at=replace_payload(logical,'known',candidate)
            self.assertEqual(old,payload);self.assertEqual(output[:at],logical[:at]);self.assertEqual(output[at+len(candidate):],logical[at+len(candidate):])
            with self.assertRaisesRegex(ValueError,'allocation'):replace_payload(logical,'known',candidate+b'x')
    def test_missing_or_ambiguous_native_wave_refused(self):
        with patch('build_native_remaster_candidate.inspect',return_value={'exports':[]}):
            with self.assertRaisesRegex(ValueError,'ambiguous'):replace_payload(b'','x',b'')
    def test_steam_path_refused(self):
        with self.assertRaisesRegex(ValueError,'Steam'):offline_path(ROOT/'SteamApps/common/bank.xxx')
    def test_whole_package_independent_decoder(self):
        decoder=os.environ.get('SINGULARITYVR_LZO_DECODER')
        if not decoder:self.skipTest('set SINGULARITYVR_LZO_DECODER for independent whole-package test')
        logical=struct.pack('<I',0x9e2a83c1)+(b'example logical data'*10000)+bytes(range(256));bank=self.dir/'whole.xxx';bank.write_bytes(whole_pack(logical));result=self.dir/'whole.u'
        subprocess.run([decoder,str(bank),str(result)],check=True,capture_output=True);self.assertEqual(result.read_bytes(),logical)

if __name__=='__main__':unittest.main()

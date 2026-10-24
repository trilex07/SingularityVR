"""Structural regression checks, not Steam/OpenXR acceptance."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]
SOURCE = (ROOT / 'spikes/view_matrix/d3d9.cpp').read_text(encoding='utf-8')


class Wiring(unittest.TestCase):
    def test_gameplay_gate_and_failure_before_swapchain(self):
        body = SOURCE[SOURCE.index('const bool nisStereoSbs'):]
        self.assertIn('frame->stereo, frame->menuFlat, frame->cineFlat', body)
        # OLED selects displaySrc before NIS; testing the pre-OLED source would
        # incorrectly reject the integrated pipeline (or allow bypassing OLED).
        self.assertIn('if (nisStereoSbs && displaySrc', body)
        self.assertIn('NisBridge_ProcessStereo(&g_nis, g_ctx11, displaySrc,', body)
        self.assertLess(body.index('NisBridge_ProcessStereo'), body.index('EnsureSwapchain'))
        self.assertIn('const bool splitOutput = monoCrop || nisFrame;', body)
        self.assertIn('if (splitOutput)', body)
        self.assertIn('(stereo || splitOutput) ? i : 0', body)
        self.assertIn('if (dupNow && !splitOutput)', body)

    def test_menu_rows_and_persistence(self):
        minimal = re.search(r'kMinimalRoot\[\]\s*=\s*\{([^}]+)', SOURCE).group(1)
        self.assertIn('PR_GOTO_NIS', minimal)
        pages = re.search(r'kPageRows\[kPanelPages\]\[kPanelRowsMax\]\s*=\s*\{(.*?)\n\};',
                          SOURCE, re.S).group(1)
        pages = re.sub(r'//[^\n]*', '', pages)
        rows = re.findall(r'\{([^}]+)\}', pages)
        counts = [int(x) for x in re.search(r'kPageCount\[kPanelPages\]\s*=\s*\{([^}]+)',
                                           SOURCE).group(1).split(',')]
        self.assertEqual(len(rows), 13)
        self.assertEqual([len(re.findall(r'PR_\w+', row)) for row in rows], counts)
        self.assertLessEqual(max(counts), 13)
        self.assertIn('page != 12', SOURCE)
        save = SOURCE[SOURCE.index('void PanelSaveRow'):SOURCE.index('void PanelSave()')]
        self.assertIn('case PR_NIS_SHARPEN:', save)
        self.assertIn('case PR_NIS_STRENGTH:', save)
        self.assertIn('"Upscaling", "Mode"', save)
        self.assertIn('"Upscaling", "Sharpness"', save)

    def test_default_off_and_immutable_vendor_shaders(self):
        ini = (ROOT / 'spikes/view_matrix/SingularityVR.ini').read_text(encoding='utf-8')
        self.assertRegex(ini, r'\[Upscaling\]\s+Mode=Off')
        runtime = (ROOT / 'spikes/view_matrix/nis_runtime.h').read_text(encoding='utf-8')
        self.assertIn('useSharpen ? nis->csSharpen : nis->csScaler', runtime)
        self.assertIn('useSharpen ? 32 : 24', runtime)
        self.assertIn('D3D11_ASYNC_GETDATA_DONOTFLUSH', runtime)
        self.assertIn('if (nis->timingPending) return;', runtime)


if __name__ == '__main__':
    unittest.main()

"""Regression for source-owned diagnostic table traversal, not a visual pass."""
import re
import unittest
from pathlib import Path


class Tests(unittest.TestCase):
    def test_hashed_indices_not_count_prefix(self):
        table = [0]*2048
        table[1901] = 8
        table[2020] = 2
        self.assertEqual(sum(n > 0 for n in table), 2)
        self.assertEqual(sum(table[:2]), 0) # old prefix-by-count loses both groups
        self.assertEqual([(i,n) for i,n in enumerate(table) if n], [(1901,8),(2020,2)])
        source=(Path(__file__).resolve().parents[2]/'spikes/view_matrix/menu_draw_trace.h').read_text()
        flush=source.split('static void MenuDrawTraceFlush()',1)[1].split('static void MenuDrawTraceState()',1)[0]
        self.assertNotIn('i<g_menuGroupCount',flush)
        self.assertRegex(flush,r'for\(unsigned i=0;i<2048;\+\+i\).*?if\(!a.count\)continue;')


if __name__=='__main__': unittest.main()

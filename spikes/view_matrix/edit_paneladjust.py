with open('d3d9.cpp', 'rb') as f:
    content = f.read()

old = b'case PR_NOTESIZE: {\n            // Same 5%-a-press step as WORLD SCALE. There is a floor rather than 0 because a note you\n            // cannot read for being too small is the same bug reported from the other side.\n            //\n            // \xe2\xad\x90 It was 40, and run 230 settled on exactly 40 - AT the clamp. A value chosen at a\n            // boundary is not a preference, it is the boundary, and there is no way to tell the two\n            // apart from inside the clamp. Lowered to 25 so the comfortable setting is somewhere in\n            // the range rather than at its edge.\n            LONG p = InterlockedCompareExchange(&g_noteInsetPct, 0, 0) + dir * 5;\n            if (p < 25)  p = 25;\n            if (p > 100) p = 100;\n            InterlockedExchange(&g_noteInsetPct, p);\n            break;\n        }\n        case PR_WORLDSCALE: {'

new = b'case PR_NOTESIZE: {\n            // Same 5%-a-press step as WORLD SCALE. There is a floor rather than 0 because a note you\n            // cannot read for being too small is the same bug reported from the other side.\n            //\n            // \xe2\xad\x90 It was 40, and run 230 settled on exactly 40 - AT the clamp. A value chosen at a\n            // boundary is not a preference, it is the boundary, and there is no way to tell the two\n            // apart from inside the clamp. Lowered to 25 so the comfortable setting is somewhere in\n            // the range rather than at its edge.\n            LONG p = InterlockedCompareExchange(&g_noteInsetPct, 0, 0) + dir * 5;\n            if (p < 25)  p = 25;\n            if (p > 100) p = 100;\n            InterlockedExchange(&g_noteInsetPct, p);\n            break;\n        }\n        case PR_POLISH: {\n            PolishLocalization::PL().SetPolishLanguage(!PolishLocalization::PL().IsPolishEnabled());\n            Log("VR panel: Polish language %s", PolishLocalization::PL().IsPolishEnabled() ? "ON" : "OFF");\n            break;\n        }\n        case PR_WORLDSCALE: {'

content = content.replace(old, new)

with open('d3d9.cpp', 'wb') as f:
    f.write(content)
print('Done')
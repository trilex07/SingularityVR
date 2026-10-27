with open('d3d9.cpp', 'rb') as f:
    content = f.read()

old = b'case PR_NOTESIZE: {\n            const LONG p = InterlockedCompareExchange(&g_noteInsetPct, 0, 0);\n            if (p == 100) _snprintf_s(out, cap, _TRUNCATE, "NOTE SIZE      100%% (FULL SCREEN)");\n            else          _snprintf_s(out, cap, _TRUNCATE, "NOTE SIZE      %ld%%", p);\n            break;\n        }\n        case PR_WORLDSCALE: {'

new = b'case PR_NOTESIZE: {\n            const LONG p = InterlockedCompareExchange(&g_noteInsetPct, 0, 0);\n            if (p == 100) _snprintf_s(out, cap, _TRUNCATE, "NOTE SIZE      100%% (FULL SCREEN)");\n            else          _snprintf_s(out, cap, _TRUNCATE, "NOTE SIZE      %ld%%", p);\n            break;\n        }\n        case PR_POLISH: {\n            const LONG p = PolishLocalization::PL().IsPolishEnabled() ? 1 : 0;\n            _snprintf_s(out, cap, _TRUNCATE, "POLISH LANG    %s", p ? "ON" : "OFF");\n            break;\n        }\n        case PR_WORLDSCALE: {'

content = content.replace(old, new)

with open('d3d9.cpp', 'wb') as f:
    f.write(content)
print('Done')
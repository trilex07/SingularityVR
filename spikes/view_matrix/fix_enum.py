with open('d3d9.cpp', 'rb') as f:
    content = f.read()

# Fix the enum - add PR_POLISH after PR_NOTESIZE
old = b'PR_LASER, PR_ALIGNGUN, PR_ALIGNFWD, PR_ALIGNRIGHT, PR_ALIGNUP, PR_ALIGNYAW, PR_ALIGNPITCH,\n        PR_NOTESIZE,          // \xe2\xad\x90 run 230\n        PR_GUNSCALE,          // hub round A: first-person gun+hands size, live-tuned like world scale'

new = b'PR_LASER, PR_ALIGNGUN, PR_ALIGNFWD, PR_ALIGNRIGHT, PR_ALIGNUP, PR_ALIGNYAW, PR_ALIGNPITCH,\n        PR_NOTESIZE,          // \xe2\xad\x90 run 230\n        PR_POLISH,            // Polish localization toggle\n        PR_GUNSCALE,          // hub round A: first-person gun+hands size, live-tuned like world scale'

content = content.replace(old, new)

with open('d3d9.cpp', 'wb') as f:
    f.write(content)
print('Enum fixed')
with open('d3d9.cpp', 'rb') as f:
    content = f.read()

# Fix the strrchr usage
old = (
    b'        char* slash = strrchr(path, "\\\\");\n'
    b'        if (slash) {\n'
    b'            *slash = "\\0";\n'
    b'            _snprintf_s(polishTextPath, MAX_PATH, _TRUNCATE, "%s\\\\vr-assets\\\\localization\\\\pl\\\\polish_text.jsonl", path);\n'
    b'            _snprintf_s(polishVoicePath, MAX_PATH, _TRUNCATE, "%s\\\\vr-assets\\\\localization\\\\pl\\\\polish_voice.jsonl", path);\n'
    b'            _snprintf_s(polishStoryPath, MAX_PATH, _TRUNCATE, "%s\\\\vr-assets\\\\localization\\\\pl\\\\polish_story_voice.jsonl", path);\n'
    b'            *slash = "\\\\";\n'
    b'        }'
)

new = (
    b'        char* slash = strrchr(path, \'\\\\\');\n'
    b'        if (slash) {\n'
    b'            *slash = \'\\0\';\n'
    b'            _snprintf_s(polishTextPath, MAX_PATH, _TRUNCATE, "%s\\\\vr-assets\\\\localization\\\\pl\\\\polish_text.jsonl", path);\n'
    b'            _snprintf_s(polishVoicePath, MAX_PATH, _TRUNCATE, "%s\\\\vr-assets\\\\localization\\\\pl\\\\polish_voice.jsonl", path);\n'
    b'            _snprintf_s(polishStoryPath, MAX_PATH, _TRUNCATE, "%s\\\\vr-assets\\\\localization\\\\pl\\\\polish_story_voice.jsonl", path);\n'
    b'            *slash = \'\\\\\');\n'
    b'        }'
)

content = content.replace(old, new)

with open('d3d9.cpp', 'wb') as f:
    f.write(content)
print('strrchr fixed')
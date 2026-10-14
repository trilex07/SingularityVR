import json

# Read story voicepack manifest
with open('C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish/story_voicepack_nllb/manifest.jsonl', 'r', encoding='utf-8') as f:
    story_lines = [json.loads(line) for line in f if line.strip()]

# Read full voicepack manifest  
with open('C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish/full_voicepack/manifest.jsonl', 'r', encoding='utf-8') as f:
    full_lines = [json.loads(line) for line in f if line.strip()]

# Group by profile/speaker
story_by_profile = {}
for entry in story_lines:
    prof = entry.get('profile', 'unknown')
    if prof not in story_by_profile:
        story_by_profile[prof] = []
    story_by_profile[prof].append(entry)

full_by_profile = {}
for entry in full_lines:
    prof = entry.get('profile', 'unknown')
    if prof not in full_by_profile:
        full_by_profile[prof] = []
    full_by_profile[prof].append(entry)

# Print summary
print("=== STORY VOICEPACK (NLLB) ===")
for prof, entries in story_by_profile.items():
    print(f"\n[{prof}] - {len(entries)} lines")
    for e in entries[:3]:
        print(f"  {e['package']}|{e['asset']}: {e['text_en'][:80]}")
        print(f"    PL: {e['text_pl_machine'][:80]}")
        print(f"    File: {e['file']}")

print("\n\n=== FULL VOICEPACK (UI/TUTORIAL) ===")
for prof, entries in full_by_profile.items():
    print(f"\n[{prof}] - {len(entries)} lines")
    for e in entries[:3]:
        sec = e.get('section', '')
        key = e.get('key', '')
        print(f"  {sec}|{key}: {e['en'][:80]}")
        print(f"    PL: {e['pl'][:80]}")
        print(f"    File: {e['file']}")

# Save detailed mapping
with open('C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish/voice_mapping_report.txt', 'w', encoding='utf-8') as out:
    out.write("=== SINGULARITY VR - POLISH DUBBING CAST & MAPPING REPORT ===\n")
    out.write("Generated: 2026-10-14\n")
    out.write("Project: Singularity VR (UE3 mod)\n")
    out.write("TTS Engines: Edge TTS (pl-PL-MarekNeural, pl-PL-ZofiaNeural), Piper, CML, RHVoice\n\n")
    
    out.write("=== LEGAL VOICE REFERENCES AVAILABLE ===\n")
    out.write("Location: audio/polish/cast_legal_20261014/\n\n")
    out.write("Pocket/CML demos:\n")
    out.write("  - announcer_cml_ref.wav\n")
    out.write("  - demichev_darkman_ref.wav\n")
    out.write("  - kathryn_magda_ref.wav\n")
    out.write("  - lektor_cml_ref.wav\n")
    out.write("  - lektor_magda_ref.wav\n")
    out.write("  - lektor_mc_ref.wav\n")
    out.write("  - lektor_natan_ref.wav\n")
    out.write("  - scientist_gosia_ref.wav\n")
    out.write("  - soldier_natan_ref.wav\n\n")
    out.write("Piper references:\n")
    out.write("  - piper_darkman_reference.wav\n")
    out.write("  - piper_gosia_reference.wav\n")
    out.write("  - piper_mc_speech_reference.wav\n\n")
    out.write("RHVoice references:\n")
    out.write("  - rhvoice_natan_reference.wav\n")
    out.write("  - rhvoice_magda_reference.wav\n")
    out.write("  - rhvoice_natan_demo_short.wav\n")
    out.write("  - rhvoice_magda_demo_short.wav\n\n")
    out.write("Accent references (for voice matching):\n")
    out.write("  - ru_dmitri_reference.wav (Russian accent for Barisov)\n")
    out.write("  - ru_ruslan_reference.wav (Russian accent for Demichev)\n")
    out.write("  - uk_mykyta_reference.wav (Ukrainian accent for Kathryn)\n")
    out.write("  - uk_tetiana_reference.wav (Ukrainian accent for Kathryn)\n\n")
    
    out.write("=== CHARACTER CASTING PROPOSAL ===\n\n")
    
    # Character mapping based on profile names and speaker_guess
    char_map = {
        'barisov_dmitri': {
            'character': 'Dr. Victor Barisov',
            'role': 'Scientist, ally, mentor',
            'gender': 'Male',
            'age': '60s',
            'accent': 'Russian/Polish accent (educated)',
            'recommended_voice': 'Piper darkman or CML lektor (male, older, authoritative)',
            'rhvoice': 'Natan (lower pitch)',
            'lines_story': len(story_by_profile.get('barisov_dmitri', [])),
            'lines_ui': len(full_by_profile.get('barisov_dmitri', []))
        },
        'demichev_ruslan': {
            'character': 'Dr. Demichev',
            'role': 'Antagonist, Soviet general/scientist',
            'gender': 'Male',
            'age': '50s-60s',
            'accent': 'Russian/Polish accent (authoritarian)',
            'recommended_voice': 'Piper darkman or CML lektor (male, commanding, darker)',
            'rhvoice': 'Natan (lower pitch, slower)',
            'lines_story': len(story_by_profile.get('demichev_ruslan', [])),
            'lines_ui': len(full_by_profile.get('demichev_ruslan', []))
        },
        'kathryn_tetiana': {
            'character': 'Kathryn',
            'role': 'USMC Recon, ally, radio support',
            'gender': 'Female',
            'age': '30s',
            'accent': 'Ukrainian/Polish accent (native Polish speaker)',
            'recommended_voice': 'Piper gosia or CML magda (female, clear, professional)',
            'rhvoice': 'Magda',
            'lines_story': len(story_by_profile.get('kathryn_tetiana', [])),
            'lines_ui': len(full_by_profile.get('kathryn_tetiana', []))
        },
        'renko_user': {
            'character': 'Renko (Player)',
            'role': 'Protagonist, mostly silent',
            'gender': 'Male',
            'age': '30s',
            'accent': 'Neutral Polish',
            'recommended_voice': 'Edge TTS MarekNeural or Piper MC (male, neutral)',
            'rhvoice': 'Natan (neutral)',
            'lines_story': len(story_by_profile.get('renko_user', [])),
            'lines_ui': len(full_by_profile.get('renko_user', []))
        },
        'narrator_user': {
            'character': 'Narrator / Tutorial / UI',
            'role': 'System voice, tutorials, HUD messages',
            'gender': 'Neutral (male default)',
            'age': 'Adult',
            'accent': 'Standard Polish (polski ogólnonarodowy)',
            'recommended_voice': 'Edge TTS MarekNeural (male) or ZofiaNeural (female)',
            'rhvoice': 'Natan or Magda',
            'lines_story': len(story_by_profile.get('narrator_user', [])),
            'lines_ui': len(full_by_profile.get('narrator_user', []))
        }
    }
    
    for prof, info in char_map.items():
        out.write(f"\n--- {info['character']} ({prof}) ---\n")
        out.write(f"  Role: {info['role']}\n")
        out.write(f"  Gender: {info['gender']}, Age: {info['age']}\n")
        out.write(f"  Accent: {info['accent']}\n")
        out.write(f"  Story lines: {info['lines_story']}\n")
        out.write(f"  UI/Tutorial lines: {info['lines_ui']}\n")
        out.write(f"  Recommended: {info['recommended_voice']}\n")
        out.write(f"  RHVoice: {info['rhvoice']}\n")
    
    out.write("\n\n=== STORY VOICEPACK DETAILS (992 lines) ===\n")
    for prof, entries in story_by_profile.items():
        out.write(f"\n[{prof}] - {len(entries)} lines\n")
        for e in entries:
            out.write(f"  {e['package']}|{e['asset']}\n")
            out.write(f"    EN: {e['text_en'][:120]}\n")
            out.write(f"    PL: {e['text_pl_machine'][:120]}\n")
            out.write(f"    File: {e['file']} (from {e['source']})\n")
    
    out.write("\n\n=== FULL VOICEPACK DETAILS (894 lines) ===\n")
    for prof, entries in full_by_profile.items():
        out.write(f"\n[{prof}] - {len(entries)} lines\n")
        for e in entries:
            sec = e.get('section', '')
            key = e.get('key', '')
            out.write(f"  {sec}|{key}\n")
            out.write(f"    EN: {e['en'][:120]}\n")
            out.write(f"    PL: {e['pl'][:120]}\n")
            out.write(f"    TTS: {e.get('tts_text', '')[:120]}\n")
            out.write(f"    File: {e['file']}\n")

print("Report written to voice_mapping_report.txt")
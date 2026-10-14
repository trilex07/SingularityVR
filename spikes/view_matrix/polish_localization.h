#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <shared_mutex>
#include <cstdint>

namespace PolishLocalization {

struct LocalizedString {
    std::string pl;
    std::string en;
    std::string tts_text;
    std::string category;
    bool has_audio = false;
    std::string audio_file;
    std::string profile;
};

struct AudioEntry {
    std::string id;
    std::string package;
    std::string asset;
    std::string pl_text;
    std::string tts_text;
    std::string profile;
    std::string audio_file;
};

class PolishLocalizationManager {
public:
    static PolishLocalizationManager& Instance();
    
    // Initialization
    bool Initialize(const std::string& text_manifest_path, 
                    const std::string& voice_manifest_path,
                    const std::string& story_manifest_path = "");
    void Shutdown();
    
    // Configuration
    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }
    void SetFallbackToEnglish(bool fallback) { m_fallback_to_english = fallback; }
    
    // Text lookup
    const char* GetLocalizedString(const char* record, const char* section, const char* key);
    const char* GetLocalizedStringById(const char* id);
    
    // Audio lookup
    const AudioEntry* GetAudioEntry(const char* package, const char* asset);
    const AudioEntry* GetAudioEntryByPackageAsset(const char* package, const char* asset);
    const char* GetAudioFile(const char* package, const char* asset);
    
    // TTS text for UI
    const char* GetTTSString(const char* record, const char* section, const char* key);

    // English source text -> Polish, and the UI voice file for that same English line.
    // Copies into the caller buffer. Empty result means no replacement.
    bool PolishForEnglish(const char* english_utf8, char* out, size_t cap) const;
    bool WavForEnglish(const char* english_utf8, char* out, size_t cap) const;
    bool IsUiVoiceEnglish(const char* english_utf8) const;
    void SetNativeSpeech(bool enabled){m_native_speech=enabled;}
    
    // Configuration
    bool IsPolishEnabled() const { return m_enabled && m_polish_language; }
    void SetPolishLanguage(bool enabled) { m_polish_language = enabled; }
    
    // Stats
    size_t GetTextEntryCount() const { return m_text_entries.size(); }
    size_t GetAudioEntryCount() const { return m_audio_entries.size(); }

private:
    PolishLocalizationManager() = default;
    ~PolishLocalizationManager() = default;
    PolishLocalizationManager(const PolishLocalizationManager&) = delete;
    PolishLocalizationManager& operator=(const PolishLocalizationManager&) = delete;
    
    bool LoadTextManifest(const std::string& path);
    bool LoadVoiceManifest(const std::string& path);
    bool LoadStoryManifest(const std::string& path);
    
    static std::string MakeKey(const char* record, const char* section, const char* key);
    static std::string NormalizePath(const std::string& path);
    
    // Thread-safe access
    mutable std::shared_mutex m_mutex;
    
    // Text entries: id -> LocalizedString
    std::unordered_map<std::string, LocalizedString> m_text_entries;
    std::unordered_map<std::string, std::string> m_en_to_pl;
    std::unordered_map<std::string, std::string> m_en_to_wav;
    std::unordered_set<std::string> m_ui_voice_en;
    std::unordered_set<std::string> m_native_voice_en;
    bool m_native_speech=false; // default OFF, exact manifested lines only
    
    // Audio entries: package+asset -> AudioEntry
    std::unordered_map<std::string, AudioEntry> m_audio_entries;
    std::unordered_map<std::string, AudioEntry> m_audio_by_package_asset;
    
    // Configuration
    bool m_enabled = false;
    bool m_polish_language = false;
    bool m_fallback_to_english = true;
    bool m_initialized = false;
    
    // Paths
    std::string m_text_manifest_path;
    std::string m_voice_manifest_path;
    std::string m_story_manifest_path;
};

// Global accessor
inline PolishLocalizationManager& PL() {
    return PolishLocalizationManager::Instance();
}

// Helper macros for easy access
#define PL_TEXT(record, section, key) PolishLocalization::PL().GetLocalizedString(record, section, key)
#define PL_TTS(record, section, key) PolishLocalization::PL().GetTTSString(record, section, key)
#define PL_AUDIO(package, asset) PolishLocalization::PL().GetAudioEntry(package, asset)

} // namespace PolishLocalization

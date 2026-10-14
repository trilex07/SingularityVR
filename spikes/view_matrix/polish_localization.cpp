#include "polish_localization.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <codecvt>
#include <locale>
#include <windows.h>

namespace PolishLocalization {

PolishLocalizationManager& PolishLocalizationManager::Instance() {
    static PolishLocalizationManager instance;
    return instance;
}

std::string PolishLocalizationManager::MakeKey(const char* record, const char* section, const char* key) {
    if (!record || !section || !key) return "";
    return std::string(record) + "|" + section + "|" + key;
}

std::string PolishLocalizationManager::NormalizePath(const std::string& path) {
    std::string normalized = path;
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    return normalized;
}

bool PolishLocalizationManager::Initialize(const std::string& text_manifest_path,
                                           const std::string& voice_manifest_path,
                                           const std::string& story_manifest_path) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    
    if (m_initialized) {
        return true;
    }
    
    m_text_manifest_path = NormalizePath(text_manifest_path);
    m_voice_manifest_path = NormalizePath(voice_manifest_path);
    m_story_manifest_path = story_manifest_path.empty() ? "" : NormalizePath(story_manifest_path);
    
    bool success = true;
    success &= LoadTextManifest(m_text_manifest_path);
    success &= LoadVoiceManifest(m_voice_manifest_path);
    
    if (!m_story_manifest_path.empty() && !LoadStoryManifest(m_story_manifest_path)) {
        OutputDebugStringA("[PL] story manifest absent - UI text and UI voice stay live\n");
    }
    
    m_initialized = success;
    m_enabled = true;
    m_polish_language = false; // Default off, user enables via INI
    
    return success;
}

void PolishLocalizationManager::Shutdown() {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_text_entries.clear();
    m_audio_entries.clear();
    m_audio_by_package_asset.clear();
    m_en_to_pl.clear();m_en_to_wav.clear();m_ui_voice_en.clear();m_native_voice_en.clear();m_native_speech=false;
    m_initialized = false;
    m_enabled = false;
}

bool PolishLocalizationManager::LoadTextManifest(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        OutputDebugStringA(("[PL] Failed to open text manifest: " + path + "\n").c_str());
        return false;
    }
    
    std::string line;
    size_t loaded = 0, skipped = 0;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        try {
            // Simple JSON parsing for our specific format
            // {"id":"...","record":"...","section":"...","key":"...","category":"...","en":"...","pl":"...","status":"...","tokens":[],"markup":[],"note":"..."}
            
            std::string id, record, section, key, category, en, pl, status, tts_text;
            bool has_tts = false;
            
            // Simple field extraction
            auto extract = [&](const std::string& field) -> std::string {
                const std::string search = "\"" + field + "\"";
                size_t pos = line.find(search);
                if (pos == std::string::npos) return "";
                pos += search.size();
                while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t')) ++pos;
                if (pos >= line.size() || line[pos] != ':') return "";
                ++pos;
                while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t')) ++pos;
                if (pos >= line.size() || line[pos] != '"') return "";
                ++pos;
                size_t end = pos;
                while (end < line.size()) {
                    if (line[end] == '\\' && end + 1 < line.size()) { end += 2; continue; }
                    if (line[end] == '"') break;
                    ++end;
                }
                if (end >= line.size()) return "";
                return line.substr(pos, end - pos);
            };
            
            id = extract("id");
            record = extract("record");
            section = extract("section");
            key = extract("key");
            category = extract("category");
            en = extract("en");
            pl = extract("pl");
            status = extract("status");
            
            tts_text = extract("tts_text");
            has_tts = !tts_text.empty();
            
            if (id.empty()) {
                skipped++;
                continue;
            }
            
            // Handle escape sequences
            auto unescape = [](std::string s) -> std::string {
                size_t pos = 0;
                while ((pos = s.find("\\u", pos)) != std::string::npos) {
                    if (pos + 5 < s.length()) {
                        std::string hex = s.substr(pos + 2, 4);
                        try {
                            wchar_t wc = static_cast<wchar_t>(std::stoi(hex, nullptr, 16));
                            std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
                            std::string utf8 = conv.to_bytes(wc);
                            s.replace(pos, 6, utf8);
                            pos += utf8.length();
                        } catch (...) {
                            pos += 6;
                        }
                    } else {
                        pos += 2;
                    }
                }
                // Handle other escapes
                std::string result;
                for (size_t i = 0; i < s.length(); ++i) {
                    if (s[i] == '\\' && i + 1 < s.length()) {
                        char next = s[i + 1];
                        if (next == 'n') result += '\n';
                        else if (next == 't') result += '\t';
                        else if (next == 'r') result += '\r';
                        else if (next == '\\') result += '\\';
                        else if (next == '"') result += '"';
                        else if (next == '\\') result += '\\';
                        else result += s[i];
                        i++;
                    } else {
                        result += s[i];
                    }
                }
                return result;
            };
            
            pl = unescape(pl);
            en = unescape(en);
            if (has_tts) tts_text = unescape(tts_text);
            
            if (status != "translated") {
                skipped++;
                continue;
            }
            
            LocalizedString entry;
            entry.pl = pl;
            entry.en = en;
            entry.tts_text = has_tts ? tts_text : pl;
            entry.category = category;
            
            if (!en.empty() && !pl.empty() && pl != en && !m_en_to_pl.count(en))
                m_en_to_pl.emplace(en, pl);
            m_text_entries[id] = std::move(entry);
            loaded++;
        } catch (...) {
            skipped++;
        }
    }
    
    char buf[256];
    sprintf_s(buf, "[PL] Loaded %zu text entries, skipped %zu\n", loaded, skipped);
    OutputDebugStringA(buf);
    return loaded > 0;
}

bool PolishLocalizationManager::LoadVoiceManifest(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        OutputDebugStringA(("[PL] Failed to open voice manifest: " + path + "\n").c_str());
        return false;
    }
    
    std::string line;
    size_t loaded = 0;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        try {
            // Parse voice manifest JSONL
            // {"id":"...","record":"...","section":"...","key":"...","category":"...","en":"...","pl":"...","tts_text":"...","profile":"...","file":"...","sample_rate":24000,"peak_dbfs":-3.0}
            
            auto extract = [&](const std::string& field) -> std::string {
                const std::string search = "\"" + field + "\"";
                size_t pos = line.find(search);
                if (pos == std::string::npos) return "";
                pos += search.size();
                while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t')) ++pos;
                if (pos >= line.size() || line[pos] != ':') return "";
                ++pos;
                while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t')) ++pos;
                if (pos >= line.size() || line[pos] != '"') return "";
                ++pos;
                size_t end = pos;
                while (end < line.size()) {
                    if (line[end] == '\\' && end + 1 < line.size()) { end += 2; continue; }
                    if (line[end] == '"') break;
                    ++end;
                }
                if (end >= line.size()) return "";
                return line.substr(pos, end - pos);
            };
            
            std::string id = extract("id");
            std::string record = extract("record");
            std::string section = extract("section");
            std::string key = extract("key");
            std::string category = extract("category");
            std::string en = extract("en");
            std::string pl = extract("pl");
            std::string tts_text = extract("tts_text");
            std::string profile = extract("profile");
            std::string file = extract("file");
            
            if (id.empty() || file.empty()) continue;
            
            // Create audio entry
            AudioEntry entry;
            entry.id = id;
            entry.package = ""; // Will be extracted from record if possible
            entry.asset = "";   // Will be extracted from record if possible
            entry.pl_text = pl;
            entry.tts_text = tts_text.empty() ? pl : tts_text;
            entry.profile = profile;
            entry.audio_file = file;
            
            // Try to extract package and asset from record
            // Format: "..\\RvGame\\Localization\\INT\\RvGame.int|RvUI|NewGame"
            size_t pkg_start = record.find_last_of('\\');
            if (pkg_start != std::string::npos) {
                pkg_start++;
                size_t pkg_end = record.find('|', pkg_start);
                if (pkg_end != std::string::npos) {
                    entry.package = record.substr(pkg_start, pkg_end - pkg_start);
                    entry.asset = record.substr(pkg_end + 1);
                }
            }
            
            std::string pkgAssetKey = entry.package + "|" + entry.asset;
            if (!pkgAssetKey.empty() && pkgAssetKey != "|") {
                m_audio_by_package_asset[pkgAssetKey] = entry;
            }
            if (!en.empty() && !file.empty()) m_ui_voice_en.insert(en);
            if (!en.empty() && !file.empty() && !m_en_to_wav.count(en))
                m_en_to_wav.emplace(en, file);
            m_audio_entries[id] = std::move(entry);
            
        } catch (...) {
            // Skip malformed entries
        }
    }
    
    char buf[256];
    sprintf_s(buf, "[PL] Loaded %zu voice entries\n", m_audio_entries.size());
    OutputDebugStringA(buf);
    return true;
}

bool PolishLocalizationManager::LoadStoryManifest(const std::string& path) {
    // Same as LoadVoiceManifest but for story packages
    std::ifstream file(path);
    if (!file.is_open()) return false;
    
    std::string line;
    size_t loaded = 0;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        try {
            auto extract = [&](const std::string& field) -> std::string {
                const std::string search = "\"" + field + "\"";
                size_t pos = line.find(search);
                if (pos == std::string::npos) return "";
                pos += search.size();
                while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t')) ++pos;
                if (pos >= line.size() || line[pos] != ':') return "";
                ++pos;
                while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t')) ++pos;
                if (pos >= line.size() || line[pos] != '"') return "";
                ++pos;
                size_t end = pos;
                while (end < line.size()) {
                    if (line[end] == '\\' && end + 1 < line.size()) { end += 2; continue; }
                    if (line[end] == '"') break;
                    ++end;
                }
                if (end >= line.size()) return "";
                return line.substr(pos, end - pos);
            };
            
            std::string id = extract("id");
            std::string record = extract("record");
            std::string section = extract("section");
            std::string key = extract("key");
            std::string pl = extract("pl");
            std::string textEn = extract("text_en");
            std::string textPl = extract("text_pl");
            std::string tts_text = extract("tts_text");
            std::string profile = extract("profile");
            std::string file = extract("file");
            if (pl.empty()) pl = textPl;
            if (textEn.empty()) textEn = extract("en");
            if(!textEn.empty() && extract("native_replaced")=="1")m_native_voice_en.insert(textEn);

            if (!textEn.empty() && !pl.empty() && pl != textEn && !m_en_to_pl.count(textEn))
                m_en_to_pl.emplace(textEn, pl);
            if (!textEn.empty() && !file.empty() && !m_en_to_wav.count(textEn))
                m_en_to_wav.emplace(textEn, file);

            if (id.empty()) id = file;
            if (id.empty() || file.empty()) continue;
            
            AudioEntry entry;
            entry.id = id;
            entry.pl_text = pl;
            entry.tts_text = tts_text.empty() ? pl : tts_text;
            entry.profile = profile;
            entry.audio_file = file;
            
            size_t pkg_start = record.find_last_of('\\');
            if (pkg_start != std::string::npos) {
                pkg_start++;
                size_t pkg_end = record.find('|', pkg_start);
                if (pkg_end != std::string::npos) {
                    entry.package = record.substr(pkg_start, pkg_end - pkg_start);
                    entry.asset = record.substr(pkg_end + 1);
                }
            }
            if (entry.package.empty()) entry.package = extract("package");
            if (entry.asset.empty()) entry.asset = extract("asset");

            std::string pkgAssetKey = entry.package + "|" + entry.asset;
            if (!pkgAssetKey.empty() && pkgAssetKey != "|") {
                m_audio_by_package_asset[pkgAssetKey] = entry;
            }
            m_audio_entries[id] = std::move(entry);
            loaded++;
        } catch (...) {}
    }
    
    char buf[256];
    sprintf_s(buf, "[PL] Loaded %zu story voice entries\n", loaded);
    OutputDebugStringA(buf);
    return loaded > 0;
}

bool PolishLocalizationManager::PolishForEnglish(const char* english_utf8, char* out, size_t cap) const {
    if (!out || cap < 2) return false;
    out[0] = 0;
    if (!m_enabled || !m_polish_language || !english_utf8 || !english_utf8[0]) return false;
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    auto it = m_en_to_pl.find(english_utf8);
    if (it == m_en_to_pl.end() || it->second.empty()) return false;
    if (it->second.size() >= cap) return false; // never install a truncated note/paragraph
    strncpy_s(out, cap, it->second.c_str(), _TRUNCATE);
    return out[0] != 0;
}

bool PolishLocalizationManager::WavForEnglish(const char* english_utf8, char* out, size_t cap) const {
    if (!out || cap < 2) return false;
    out[0] = 0;
    if (!m_enabled || !m_polish_language || !english_utf8 || !english_utf8[0]) return false;
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    if(m_native_speech&&m_native_voice_en.count(english_utf8))return false; // native UE3 owns this line; no PlaySound overlay
    auto it = m_en_to_wav.find(english_utf8);
    if (it == m_en_to_wav.end() || it->second.empty()) return false;
    if (it->second.size() >= cap) return false;
    strncpy_s(out, cap, it->second.c_str(), _TRUNCATE);
    return out[0] != 0;
}

bool PolishLocalizationManager::IsUiVoiceEnglish(const char* english_utf8) const {
    if (!english_utf8 || !*english_utf8) return false;
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_ui_voice_en.count(english_utf8) != 0;
}

const char* PolishLocalizationManager::GetLocalizedString(const char* record, const char* section, const char* key) {
    if (!m_enabled || !m_polish_language) return nullptr;
    if (!record || !section || !key) return nullptr;
    
    std::string key_str = MakeKey(record, section, key);
    
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    auto it = m_text_entries.find(key_str);
    if (it != m_text_entries.end()) {
        return it->second.pl.c_str();
    }
    
    if (m_fallback_to_english) {
        auto it_en = m_text_entries.find(key_str);
        if (it_en != m_text_entries.end()) {
            return it_en->second.en.c_str();
        }
    }
    return nullptr;
}

const char* PolishLocalizationManager::GetLocalizedStringById(const char* id) {
    if (!m_enabled || !m_polish_language || !id) return nullptr;
    
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    auto it = m_text_entries.find(id);
    if (it != m_text_entries.end()) {
        return it->second.pl.c_str();
    }
    
    if (m_fallback_to_english) {
        auto it_en = m_text_entries.find(id);
        if (it_en != m_text_entries.end()) {
            return it_en->second.en.c_str();
        }
    }
    return nullptr;
}

const PolishLocalization::AudioEntry* PolishLocalizationManager::GetAudioEntry(const char* package, const char* asset) {
    if (!package || !asset) return nullptr;
    
    std::string key = std::string(package) + "|" + asset;
    
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    auto it = m_audio_by_package_asset.find(key);
    if (it != m_audio_by_package_asset.end()) {
        return &it->second;
    }
    return nullptr;
}

const PolishLocalization::AudioEntry* PolishLocalizationManager::GetAudioEntryByPackageAsset(const char* package, const char* asset) {
    return GetAudioEntry(package, asset);
}

const char* PolishLocalizationManager::GetAudioFile(const char* package, const char* asset) {
    const PolishLocalization::AudioEntry* entry = GetAudioEntry(package, asset);
    if (entry && !entry->audio_file.empty()) {
        return entry->audio_file.c_str();
    }
    return nullptr;
}

const char* PolishLocalizationManager::GetTTSString(const char* record, const char* section, const char* key) {
    if (!m_enabled || !m_polish_language) return nullptr;
    if (!record || !section || !key) return nullptr;
    
    std::string key_str = MakeKey(record, section, key);
    
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    auto it = m_text_entries.find(key_str);
    if (it != m_text_entries.end()) {
        return it->second.tts_text.c_str();
    }
    return nullptr;
}

} // namespace PolishLocalization

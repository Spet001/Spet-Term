#pragma once

#include "common.hpp"

namespace speta {

struct SnippetItem {
    std::wstring name;
    std::wstring command;
    std::wstring description;
};

struct AppSettings {
    std::wstring themeName = L"Pitch Black";
    float fontSize = 13.5f;
    std::wstring fontFamily = L"Cascadia Mono";
    bool classicFlowMode = true; // true = Windows Terminal style (default), false = Command Blocks
    std::wstring wallpaperPath = L""; // Empty or "none" means solid theme background
    int wallpaperDim = 60; // 0 to 90% (default 60%)
    bool wallpaperPauseUnfocused = false;
    bool wallpaperPauseOnBattery = true;
    std::vector<std::wstring> customWallpapers;
    int cursorStyle = 0; // 0 = Bar (|), 1 = Block (█), 2 = Underline (_)
    std::string language = "auto"; // "auto", "pt-BR", "en-US", "es-ES"
    float warmthLevel = 0.0f; // 0.0f to 1.0f (eye-care blue light filter)
    std::string promptPosition = "bottom"; // "bottom" or "top"
    std::string streamStart = "top"; // "top" = start at top of window, "bottom" = docked above prompt
    bool statusBarVisible = true;
    bool systemCursor = true; // true = native Windows cursor, false = custom Direct2D cursor (experimental)
    bool warnMultiLinePaste = true; // true = security warning on multi-line batch paste
    bool onboardingCompleted = false;
    std::vector<SnippetItem> snippets;

    static bool IsAppPathTermRegistered() {
        HKEY hKey = nullptr;
        LSTATUS st = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\term.exe", 0, KEY_READ, &hKey);
        if (st == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return true;
        }
        return false;
    }

    static bool RegisterAppPathTerm(bool enable) {
        if (!enable) {
            RegDeleteKeyW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\term.exe");
            return true;
        }
        wchar_t exePath[MAX_PATH];
        if (GetModuleFileNameW(nullptr, exePath, MAX_PATH) == 0) return false;
        wchar_t dirPath[MAX_PATH];
        wcscpy_s(dirPath, exePath);
        PathRemoveFileSpecW(dirPath);

        HKEY hKey = nullptr;
        LSTATUS st = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\term.exe",
            0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
        if (st != ERROR_SUCCESS) return false;

        RegSetValueExW(hKey, nullptr, 0, REG_SZ, reinterpret_cast<const BYTE*>(exePath), static_cast<DWORD>((wcslen(exePath) + 1) * sizeof(wchar_t)));
        RegSetValueExW(hKey, L"Path", 0, REG_SZ, reinterpret_cast<const BYTE*>(dirPath), static_cast<DWORD>((wcslen(dirPath) + 1) * sizeof(wchar_t)));
        RegCloseKey(hKey);
        return true;
    }

    AppSettings() {
        InitDefaultSnippets();
    }

    void InitDefaultSnippets() {
        snippets = {
            { L"Git: Commit & Push", L"git add . && git commit -m \"update\" && git push", L"Salva tudo e envia pro remoto" },
            { L"Git: Undo Last Commit", L"git reset --soft HEAD~1", L"Desfaz commit mantendo arquivos no stage" },
            { L"Processos: Top RAM", L"Get-Process | Sort-Object WorkingSet64 -Descending | Select-Object -First 5", L"Lista os 5 processos que mais usam memória" },
            { L"Rede: Portas Abertas", L"netstat -ano | findstr LISTENING", L"Lista todas as portas TCP em escuta" },
            { L"Build: Compilar C++", L".\\build.ps1", L"Compila Spet@term com Clang++" },
            { L"Limpeza: Cache Temp", L"Remove-Item -Path \"$env:TEMP\\*\" -Recurse -Force -ErrorAction SilentlyContinue", L"Libera espaço apagando temporários do Windows" }
        };
    }

    static std::wstring GetExeDir() {
        wchar_t exePath[MAX_PATH];
        if (GetModuleFileNameW(nullptr, exePath, MAX_PATH) > 0) {
            PathRemoveFileSpecW(exePath);
            return std::wstring(exePath);
        }
        return L".";
    }

    static std::wstring GetSettingsFilePath() {
        return GetExeDir() + L"\\settings.json";
    }

    std::wstring ToJson() const {
        std::wostringstream ss;
        ss << L"{\n";
        ss << L"  \"theme\": \"" << EscapeJson(themeName) << L"\",\n";
        ss << L"  \"fontSize\": " << fontSize << L",\n";
        ss << L"  \"fontFamily\": \"" << EscapeJson(fontFamily) << L"\",\n";
        ss << L"  \"classicFlowMode\": " << (classicFlowMode ? L"true" : L"false") << L",\n";
        ss << L"  \"wallpaperPath\": \"" << EscapeJson(wallpaperPath) << L"\",\n";
        ss << L"  \"wallpaperDim\": " << wallpaperDim << L",\n";
        ss << L"  \"wallpaperPauseUnfocused\": " << (wallpaperPauseUnfocused ? L"true" : L"false") << L",\n";
        ss << L"  \"wallpaperPauseOnBattery\": " << (wallpaperPauseOnBattery ? L"true" : L"false") << L",\n";
        ss << L"  \"customWallpapers\": [\n";
        for (size_t i = 0; i < customWallpapers.size(); ++i) {
            ss << L"    \"" << EscapeJson(customWallpapers[i]) << L"\"" << (i + 1 < customWallpapers.size() ? L"," : L"") << L"\n";
        }
        ss << L"  ],\n";
        ss << L"  \"cursorStyle\": " << cursorStyle << L",\n";
        ss << L"  \"language\": \"" << Utf8ToWide(language) << L"\",\n";
        ss << L"  \"warmthLevel\": " << warmthLevel << L",\n";
        ss << L"  \"promptPosition\": \"" << Utf8ToWide(promptPosition) << L"\",\n";
        ss << L"  \"streamStart\": \"" << Utf8ToWide(streamStart) << L"\",\n";
        ss << L"  \"systemCursor\": " << (systemCursor ? L"true" : L"false") << L",\n";
        ss << L"  \"statusBarVisible\": " << (statusBarVisible ? L"true" : L"false") << L",\n";
        ss << L"  \"warnMultiLinePaste\": " << (warnMultiLinePaste ? L"true" : L"false") << L",\n";
        ss << L"  \"onboardingCompleted\": " << (onboardingCompleted ? L"true" : L"false") << L",\n";
        ss << L"  \"snippets\": [\n";
        for (size_t i = 0; i < snippets.size(); ++i) {
            ss << L"    {\n";
            ss << L"      \"name\": \"" << EscapeJson(snippets[i].name) << L"\",\n";
            ss << L"      \"command\": \"" << EscapeJson(snippets[i].command) << L"\",\n";
            ss << L"      \"description\": \"" << EscapeJson(snippets[i].description) << L"\"\n";
            ss << L"    }" << (i + 1 < snippets.size() ? L"," : L"") << L"\n";
        }
        ss << L"  ]\n";
        ss << L"}\n";
        return ss.str();
    }

    bool SaveToFile(const std::wstring& path = L"") const {
        std::wstring target = path.empty() ? GetSettingsFilePath() : path;
        std::string utf8 = WideToUtf8(ToJson());
        std::ofstream out(target, std::ios::binary);
        if (!out.is_open()) return false;
        out.write(utf8.data(), utf8.size());
        return true;
    }

    bool LoadFromJson(std::wstring_view json) {
        if (json.empty()) return false;

        auto ExtractString = [&](std::wstring_view key) -> std::wstring {
            std::wstring search = L"\"" + std::wstring(key) + L"\"";
            size_t pos = json.find(search);
            if (pos == std::wstring_view::npos) return L"";
            size_t colon = json.find(L':', pos + search.size());
            if (colon == std::wstring_view::npos) return L"";
            size_t q1 = json.find(L'\"', colon);
            if (q1 == std::wstring_view::npos) return L"";
            size_t q2 = q1 + 1;
            while (q2 < json.size()) {
                if (json[q2] == L'\"' && json[q2 - 1] != L'\\') break;
                q2++;
            }
            if (q2 >= json.size()) return L"";
            return UnescapeJson(json.substr(q1 + 1, q2 - q1 - 1));
        };

        auto ExtractNumber = [&](std::wstring_view key, double defaultVal) -> double {
            std::wstring search = L"\"" + std::wstring(key) + L"\"";
            size_t pos = json.find(search);
            if (pos == std::wstring_view::npos) return defaultVal;
            size_t colon = json.find(L':', pos + search.size());
            if (colon == std::wstring_view::npos) return defaultVal;
            size_t start = colon + 1;
            while (start < json.size() && iswspace(json[start])) start++;
            size_t end = start;
            while (end < json.size() && (iswdigit(json[end]) || json[end] == L'.' || json[end] == L'-')) end++;
            if (end > start) {
                try {
                    return std::stod(std::wstring(json.substr(start, end - start)));
                } catch (...) {}
            }
            return defaultVal;
        };

        auto ExtractBool = [&](std::wstring_view key, bool defaultVal) -> bool {
            std::wstring search = L"\"" + std::wstring(key) + L"\"";
            size_t pos = json.find(search);
            if (pos == std::wstring_view::npos) return defaultVal;
            size_t colon = json.find(L':', pos + search.size());
            if (colon == std::wstring_view::npos) return defaultVal;
            size_t start = colon + 1;
            while (start < json.size() && iswspace(json[start])) start++;
            if (json.substr(start, 4) == L"true") return true;
            if (json.substr(start, 5) == L"false") return false;
            return defaultVal;
        };

        std::wstring t = ExtractString(L"theme");
        if (!t.empty()) themeName = t;

        fontSize = static_cast<float>(ExtractNumber(L"fontSize", fontSize));
        if (fontSize < 10.0f) fontSize = 10.0f;
        if (fontSize > 32.0f) fontSize = 32.0f;

        std::wstring f = ExtractString(L"fontFamily");
        if (!f.empty()) fontFamily = f;

        classicFlowMode = ExtractBool(L"classicFlowMode", classicFlowMode);

        std::wstring wp = ExtractString(L"wallpaperPath");
        if (!wp.empty()) wallpaperPath = wp;

        wallpaperDim = static_cast<int>(ExtractNumber(L"wallpaperDim", wallpaperDim));
        if (wallpaperDim < 0) wallpaperDim = 0;
        if (wallpaperDim > 90) wallpaperDim = 90;

        wallpaperPauseUnfocused = ExtractBool(L"wallpaperPauseUnfocused", wallpaperPauseUnfocused);
        wallpaperPauseOnBattery = ExtractBool(L"wallpaperPauseOnBattery", wallpaperPauseOnBattery);

        size_t cwPos = json.find(L"\"customWallpapers\"");
        if (cwPos != std::wstring_view::npos) {
            size_t arrStart = json.find(L'[', cwPos);
            size_t arrEnd = json.find(L']', arrStart);
            if (arrStart != std::wstring_view::npos && arrEnd != std::wstring_view::npos) {
                std::wstring_view arr = json.substr(arrStart, arrEnd - arrStart + 1);
                std::vector<std::wstring> loadedCustom;
                size_t q1 = 0;
                while ((q1 = arr.find(L'\"', q1)) != std::wstring_view::npos) {
                    size_t q2 = q1 + 1;
                    while (q2 < arr.size()) {
                        if (arr[q2] == L'\"' && arr[q2 - 1] != L'\\') break;
                        q2++;
                    }
                    if (q2 >= arr.size()) break;
                    std::wstring_view val = arr.substr(q1 + 1, q2 - q1 - 1);
                    if (!val.empty()) loadedCustom.push_back(UnescapeJson(val));
                    q1 = q2 + 1;
                }
                customWallpapers = loadedCustom;
            }
        }
        cursorStyle = static_cast<int>(ExtractNumber(L"cursorStyle", cursorStyle));
        if (cursorStyle < 0 || cursorStyle > 2) cursorStyle = 0;

        std::wstring lang = ExtractString(L"language");
        if (!lang.empty()) language = WideToUtf8(lang);

        warmthLevel = static_cast<float>(ExtractNumber(L"warmthLevel", warmthLevel));
        if (warmthLevel < 0.0f) warmthLevel = 0.0f;
        if (warmthLevel > 1.0f) warmthLevel = 1.0f;

        std::wstring pPos = ExtractString(L"promptPosition");
        if (!pPos.empty()) promptPosition = WideToUtf8(pPos);

        std::wstring sStart = ExtractString(L"streamStart");
        if (!sStart.empty()) streamStart = WideToUtf8(sStart);

        statusBarVisible = ExtractBool(L"statusBarVisible", statusBarVisible);
        systemCursor = ExtractBool(L"systemCursor", systemCursor);
        warnMultiLinePaste = ExtractBool(L"warnMultiLinePaste", warnMultiLinePaste);
        onboardingCompleted = ExtractBool(L"onboardingCompleted", onboardingCompleted);

        // Parse snippets array if present
        size_t snipPos = json.find(L"\"snippets\"");
        if (snipPos != std::wstring_view::npos) {
            size_t arrStart = json.find(L'[', snipPos);
            size_t arrEnd = json.find(L']', arrStart);
            if (arrStart != std::wstring_view::npos && arrEnd != std::wstring_view::npos) {
                std::wstring_view arr = json.substr(arrStart, arrEnd - arrStart + 1);
                std::vector<SnippetItem> loadedSnippets;

                size_t objStart = 0;
                while ((objStart = arr.find(L'{', objStart)) != std::wstring_view::npos) {
                    size_t objEnd = arr.find(L'}', objStart);
                    if (objEnd == std::wstring_view::npos) break;
                    std::wstring_view obj = arr.substr(objStart, objEnd - objStart + 1);

                    auto SubExtract = [&](std::wstring_view k) -> std::wstring {
                        std::wstring s = L"\"" + std::wstring(k) + L"\"";
                        size_t p = obj.find(s);
                        if (p == std::wstring_view::npos) return L"";
                        size_t col = obj.find(L':', p + s.size());
                        if (col == std::wstring_view::npos) return L"";
                        size_t q1 = obj.find(L'\"', col);
                        if (q1 == std::wstring_view::npos) return L"";
                        size_t q2 = q1 + 1;
                        while (q2 < obj.size()) {
                            if (obj[q2] == L'\"' && obj[q2 - 1] != L'\\') break;
                            q2++;
                        }
                        if (q2 >= obj.size()) return L"";
                        return UnescapeJson(obj.substr(q1 + 1, q2 - q1 - 1));
                    };

                    SnippetItem item;
                    item.name = SubExtract(L"name");
                    item.command = SubExtract(L"command");
                    item.description = SubExtract(L"description");
                    if (!item.name.empty() && !item.command.empty()) {
                        loadedSnippets.push_back(item);
                    }

                    objStart = objEnd + 1;
                }

                if (!loadedSnippets.empty()) {
                    snippets = std::move(loadedSnippets);
                }
            }
        }

        return true;
    }

    bool LoadFromFile(const std::wstring& path = L"") {
        std::wstring target = path.empty() ? GetSettingsFilePath() : path;
        std::ifstream in(target, std::ios::binary);
        if (!in.is_open()) {
            return false;
        }
        std::string utf8((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        return LoadFromJson(Utf8ToWide(utf8));
    }

private:
    static std::wstring EscapeJson(std::wstring_view str) {
        std::wstring out;
        out.reserve(str.size() + 8);
        for (wchar_t c : str) {
            if (c == L'\"') out += L"\\\"";
            else if (c == L'\\') out += L"\\\\";
            else if (c == L'\n') out += L"\\n";
            else if (c == L'\r') out += L"\\r";
            else if (c == L'\t') out += L"\\t";
            else out += c;
        }
        return out;
    }

    static std::wstring UnescapeJson(std::wstring_view str) {
        std::wstring out;
        out.reserve(str.size());
        for (size_t i = 0; i < str.size(); ++i) {
            if (str[i] == L'\\' && i + 1 < str.size()) {
                wchar_t next = str[i + 1];
                if (next == L'\"') { out += L'\"'; i++; }
                else if (next == L'\\') { out += L'\\'; i++; }
                else if (next == L'n') { out += L'\n'; i++; }
                else if (next == L'r') { out += L'\r'; i++; }
                else if (next == L't') { out += L'\t'; i++; }
                else { out += next; i++; }
            } else {
                out += str[i];
            }
        }
        return out;
    }
};

} // namespace speta

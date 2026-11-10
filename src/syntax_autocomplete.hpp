#pragma once

#include "common.hpp"
#include "git_engine.hpp"
#include "i18n.hpp"

namespace speta {

struct SyntaxToken {
    std::wstring text;
    size_t startCol = 0;
    size_t endCol = 0;
    D2D1_COLOR_F color{ 1.0f, 1.0f, 1.0f, 1.0f };
    bool isInvalid = false;
    bool isUnderlined = false;
};

struct AutocompleteItem {
    std::wstring text;
    std::wstring displayText;
    std::wstring type; // "[cmd]", "[dir]", "[file]", "[git]", "[py]", "[cpp]", "[img]"
    std::wstring detail;
    std::wstring fullPath;
    bool isFile = false;
    bool isDir = false;
    bool isImage = false;
    bool isCode = false;

    // Text preview cache (first 12 lines)
    std::vector<std::wstring> previewLines;
};

struct SyntaxAnalysis {
    std::vector<SyntaxToken> tokens;
    bool hasSyntaxError = false;
    std::wstring syntaxErrorMessage;
    size_t errorCol = 0;
    std::wstring ghostSuggestion;
    std::wstring characterCounter; // e.g. "git commit -m: 35/72 chars"
};

class SyntaxAndAutocomplete {
public:
    std::unordered_set<std::wstring> pathExecutables;
    std::vector<std::wstring> pathDirs;
    std::unordered_set<std::wstring> builtins;
    std::unordered_set<std::wstring> gitSubcommands;
    std::unordered_set<std::wstring> pwCmdlets;
    std::wstring lastCwd;
    bool lastCwdValid = false;
    std::vector<std::wstring> cwdFiles;
    std::vector<std::wstring> cwdDirs;

    SyntaxAndAutocomplete() {
        InitBuiltins();
        RefreshPathCache();
    }

    static std::wstring ComputeGhostSuggestion(
        const std::wstring& input,
        const std::vector<std::wstring>& history,
        const std::wstring& cwd
    ) {
        if (input.empty()) return L"";

        std::wstring inLower = ToLower(input);

        for (auto it = history.rbegin(); it != history.rend(); ++it) {
            const std::wstring& h = *it;
            if (h.size() > input.size()) {
                std::wstring hLower = ToLower(h);
                if (hLower.rfind(inLower, 0) == 0) {
                    return h.substr(input.size());
                }
            }
        }

        if (!cwd.empty() && input.find(L' ') == std::wstring::npos && input.find(L'|') == std::wstring::npos && input.find(L';') == std::wstring::npos) {
            std::wstring searchPattern = cwd;
            if (searchPattern.back() != L'\\' && searchPattern.back() != L'/') {
                searchPattern += L"\\";
            }
            searchPattern += L"*";

            WIN32_FIND_DATAW ffd;
            HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &ffd);
            if (hFind != INVALID_HANDLE_VALUE) {
                std::vector<std::wstring> candidates;
                do {
                    if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0) {
                        continue;
                    }
                    std::wstring name = ffd.cFileName;
                    if (name.size() > input.size()) {
                        std::wstring nLower = ToLower(name);
                        if (nLower.rfind(inLower, 0) == 0) {
                            candidates.push_back(name);
                        }
                    }
                } while (FindNextFileW(hFind, &ffd));
                FindClose(hFind);

                if (!candidates.empty()) {
                    std::sort(candidates.begin(), candidates.end(), [](const std::wstring& a, const std::wstring& b) {
                        return _wcsicmp(a.c_str(), b.c_str()) < 0;
                    });
                    return candidates[0].substr(input.size());
                }
            }
        }

        return L"";
    }

    void InitBuiltins() {
        std::vector<std::wstring> bList = {
            L"cd", L"ls", L"dir", L"cat", L"type", L"img", L"clear", L"cls",
            L"pwd", L"gl", L"echo", L"mkdir", L"rm", L"del", L"theme", L"mode",
            L"gstatus", L"glog", L"gdiff", L"gbranch", L"help", L"exit", L"history",
            L"split", L"quake"
        };
        for (const auto& b : bList) {
            builtins.insert(ToLower(b));
        }

        std::vector<std::wstring> gList = {
            L"status", L"commit", L"push", L"pull", L"checkout", L"switch",
            L"branch", L"diff", L"log", L"add", L"clone", L"fetch", L"rebase",
            L"merge", L"reset", L"stash", L"init", L"remote", L"tag"
        };
        for (const auto& g : gList) {
            gitSubcommands.insert(ToLower(g));
        }

        std::vector<std::wstring> pwList = {
            L"get-process", L"get-service", L"get-childitem", L"get-content",
            L"set-location", L"new-item", L"remove-item", L"copy-item", L"move-item",
            L"invoke-webrequest", L"invoke-restmethod", L"start-process", L"stop-process",
            L"clear-host", L"write-output", L"write-host", L"select-object", L"where-object",
            L"irm", L"iex", L"iwr", L"curl", L"wget", L"sl", L"gci", L"gc", L"sc", L"ni", L"ri", L"mi", L"cpi"
        };
        for (const auto& p : pwList) {
            pwCmdlets.insert(ToLower(p));
        }
    }

    void RefreshPathCache() {
        pathExecutables.clear();
        pathDirs.clear();

        wchar_t pathEnv[32768];
        DWORD len = GetEnvironmentVariableW(L"PATH", pathEnv, 32768);
        if (len == 0 || len >= 32768) return;

        std::vector<std::wstring> dirs = Split(pathEnv, L';');
        for (const auto& dir : dirs) {
            std::wstring trimmed = Trim(dir);
            if (trimmed.empty()) continue;
            if (trimmed.size() >= 2 && trimmed.front() == L'\"' && trimmed.back() == L'\"') {
                trimmed = trimmed.substr(1, trimmed.size() - 2);
            }
            while (!trimmed.empty() && (trimmed.back() == L'\\' || trimmed.back() == L'/')) {
                trimmed.pop_back();
            }
            if (trimmed.empty()) continue;
            pathDirs.push_back(trimmed);

            std::wstring searchPattern = trimmed + L"\\*";
            WIN32_FIND_DATAW fd;
            HANDLE hFind = FindFirstFileExW(searchPattern.c_str(), FindExInfoBasic, &fd, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);
            if (hFind != INVALID_HANDLE_VALUE) {
                do {
                    if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                        std::wstring fname = ToLower(fd.cFileName);
                        if (EndsWith(fname, L".exe") || EndsWith(fname, L".cmd") || EndsWith(fname, L".bat") || EndsWith(fname, L".ps1")) {
                            pathExecutables.insert(fname);
                            size_t dot = fname.rfind(L'.');
                            if (dot != std::wstring::npos) {
                                pathExecutables.insert(fname.substr(0, dot));
                            }
                        }
                    }
                } while (FindNextFileW(hFind, &fd));
                FindClose(hFind);
            }
        }
    }

    void RefreshCwdCache(const std::wstring& cwd) {
        if (lastCwdValid && cwd == lastCwd) return;
        lastCwd = cwd;
        lastCwdValid = true;
        cwdFiles.clear();
        cwdDirs.clear();

        std::wstring pattern = cwd + L"\\*";
        WIN32_FIND_DATAW fd;
        HANDLE hFind = FindFirstFileExW(pattern.c_str(), FindExInfoBasic, &fd, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    if (cwdDirs.size() < 100) cwdDirs.push_back(fd.cFileName);
                } else {
                    if (cwdFiles.size() < 100) cwdFiles.push_back(fd.cFileName);
                }
            } while (FindNextFileW(hFind, &fd));
            FindClose(hFind);
        }
    }

    static int LevenshteinDistance(std::wstring_view s1, std::wstring_view s2) {
        size_t m = s1.size();
        size_t n = s2.size();
        if (m == 0) return static_cast<int>(n);
        if (n == 0) return static_cast<int>(m);
        if (std::abs(static_cast<int>(m) - static_cast<int>(n)) >= 4) return 4;

        if (m > 63) m = 63;
        if (n > 63) n = 63;

        int dp[64][64] = {};

        for (size_t i = 0; i <= m; i++) dp[i][0] = static_cast<int>(i);
        for (size_t j = 0; j <= n; j++) dp[0][j] = static_cast<int>(j);

        for (size_t i = 1; i <= m; i++) {
            for (size_t j = 1; j <= n; j++) {
                int cost = (towlower(s1[i - 1]) == towlower(s2[j - 1])) ? 0 : 1;
                dp[i][j] = (std::min)({
                    dp[i - 1][j] + 1,     // deletion
                    dp[i][j - 1] + 1,     // insertion
                    dp[i - 1][j - 1] + cost // substitution
                });
            }
        }
        return dp[m][n];
    }

    std::wstring FindClosestCommand(const std::wstring& typo) {
        if (typo.size() < 2) return L"";

        std::wstring bestMatch;
        int minDistance = 4;

        std::wstring lowerTypo = ToLower(typo);

        for (const auto& b : builtins) {
            int dist = LevenshteinDistance(lowerTypo, b);
            if (dist < minDistance) {
                minDistance = dist;
                bestMatch = b;
            }
        }

        static const wchar_t* commonCmds[] = {
            L"python", L"node", L"npm", L"clang++", L"g++", L"git", L"cargo", L"rustc",
            L"pwsh", L"powershell", L"cmd", L"code", L"pip", L"dotnet", L"cmake"
        };
        for (const auto* c : commonCmds) {
            int dist = LevenshteinDistance(lowerTypo, c);
            if (dist < minDistance) {
                minDistance = dist;
                bestMatch = c;
            }
        }

        if (minDistance > 1) {
            for (const auto& p : pathExecutables) {
                if (p.size() < 15) {
                    int dist = LevenshteinDistance(lowerTypo, p);
                    if (dist < minDistance) {
                        minDistance = dist;
                        bestMatch = p;
                        if (minDistance <= 1) break;
                    }
                }
            }
        }

        return bestMatch;
    }

    bool IsValidExecutable(const std::wstring& cmd, const std::wstring& cwd) {
        std::wstring lower = ToLower(cmd);
        if (builtins.count(lower) > 0) return true;
        if (pwCmdlets.count(lower) > 0) return true;
        if (pathExecutables.count(lower) > 0) return true;

        // Known developer and AI CLI tools (audit to avoid false red highlights)
        static const std::unordered_set<std::wstring> knownCliTools = {
            L"claude", L"gemini", L"geminicli", L"ollama", L"cursor", L"copilot",
            L"sgpt", L"llm", L"chatgpt", L"winget", L"choco", L"scoop",
            L"docker", L"podman", L"cargo", L"rustc", L"rustup", L"bun", L"pnpm",
            L"yarn", L"npm", L"npx", L"node", L"deno", L"wsl", L"pwsh", L"powershell",
            L"cmd", L"term", L"git", L"gh", L"glab", L"kubectl", L"helm", L"terraform",
            L"az", L"aws", L"gcloud", L"pip", L"pip3", L"python", L"python3", L"py",
            L"clang", L"clang++", L"gcc", L"g++", L"cmake", L"ninja", L"make",
            L"msbuild", L"dotnet", L"go", L"vcpkg", L"code", L"wt", L"explorer",
            L"notepad", L"tasklist", L"taskkill", L"ipconfig", L"ping", L"tracert",
            L"nslookup", L"netstat", L"curl", L"ssh", L"scp", L"sftp", L"tar",
            L"findstr", L"where", L"whoami", L"hostname", L"reg", L"sfc", L"dism"
        };
        if (knownCliTools.count(lower) > 0) return true;

        // Check PowerShell Verb-Noun cmdlet pattern (e.g. Add-AppxPackage, Install-Module, Get-Process)
        size_t dash = lower.find(L'-');
        if (dash != std::wstring::npos && dash >= 2 && dash + 1 < lower.size()) {
            bool validCmdlet = true;
            for (size_t i = 0; i < dash; ++i) {
                if (!iswalnum(lower[i])) { validCmdlet = false; break; }
            }
            for (size_t i = dash + 1; i < lower.size(); ++i) {
                if (!iswalnum(lower[i])) { validCmdlet = false; break; }
            }
            if (validCmdlet) return true;
        }

        for (const auto& f : cwdFiles) {
            if (_wcsicmp(f.c_str(), cmd.c_str()) == 0) return true;
            std::wstring lowerF = ToLower(f);
            if (lowerF == lower + L".exe" || lowerF == lower + L".bat" || lowerF == lower + L".cmd" || lowerF == lower + L".ps1") {
                return true;
            }
        }

        DWORD attrs = GetFileAttributesW(cmd.c_str());
        if (attrs != INVALID_FILE_ATTRIBUTES) return true;

        return false;
    }

    static std::vector<std::pair<std::wstring, std::wstring>> ReadNpmScripts(const std::wstring& cwd) {
        std::vector<std::pair<std::wstring, std::wstring>> scripts;
        std::wstring pkgPath = cwd + L"\\package.json";
        std::ifstream f(pkgPath);
        if (!f.is_open()) return scripts;

        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        size_t sPos = content.find("\"scripts\"");
        if (sPos == std::string::npos) return scripts;

        size_t braceStart = content.find('{', sPos);
        if (braceStart == std::string::npos) return scripts;

        size_t braceEnd = content.find('}', braceStart);
        if (braceEnd == std::string::npos) return scripts;

        std::string block = content.substr(braceStart + 1, braceEnd - braceStart - 1);
        std::istringstream iss(block);
        std::string line;
        while (std::getline(iss, line)) {
            size_t firstQuote = line.find('\"');
            if (firstQuote != std::string::npos) {
                size_t secondQuote = line.find('\"', firstQuote + 1);
                if (secondQuote != std::string::npos) {
                    std::string sName = line.substr(firstQuote + 1, secondQuote - firstQuote - 1);
                    std::string sCmd;
                    size_t colon = line.find(':', secondQuote);
                    if (colon != std::string::npos) {
                        size_t cmdQuote1 = line.find('\"', colon);
                        if (cmdQuote1 != std::string::npos) {
                            size_t cmdQuote2 = line.find('\"', cmdQuote1 + 1);
                            if (cmdQuote2 != std::string::npos) {
                                sCmd = line.substr(cmdQuote1 + 1, cmdQuote2 - cmdQuote1 - 1);
                            }
                        }
                    }
                    scripts.push_back({ Utf8ToWide(sName), Utf8ToWide(sCmd) });
                }
            }
        }
        return scripts;
    }

    SyntaxAnalysis AnalyzeInput(const std::wstring& input, const std::wstring& cwd, const Theme& theme) {
        SyntaxAnalysis analysis;
        RefreshCwdCache(cwd);

        if (input.empty()) return analysis;

        std::vector<std::pair<wchar_t, size_t>> delimStack;
        wchar_t inQuote = 0;
        size_t quoteStart = 0;

        for (size_t i = 0; i < input.size(); ++i) {
            wchar_t c = input[i];

            if (inQuote != 0) {
                if (c == L'\\' && i + 1 < input.size()) {
                    ++i;
                    continue;
                }
                if (c == inQuote) {
                    inQuote = 0;
                }
                continue;
            }

            if (c == L'\"' || c == L'\'') {
                inQuote = c;
                quoteStart = i;
            } else if (c == L'(' || c == L'[' || c == L'{') {
                delimStack.push_back({ c, i });
            } else if (c == L')') {
                if (!delimStack.empty() && delimStack.back().first == L'(') delimStack.pop_back();
                else {
                    analysis.hasSyntaxError = true;
                    analysis.syntaxErrorMessage = I18n::Tr(L"Parêntese ')' inesperado sem abertura", L"Unexpected closing parenthesis ')'", L"Paréntesis ')' inesperado sin apertura");
                    analysis.errorCol = i;
                    break;
                }
            } else if (c == L']') {
                if (!delimStack.empty() && delimStack.back().first == L'[') delimStack.pop_back();
                else {
                    analysis.hasSyntaxError = true;
                    analysis.syntaxErrorMessage = I18n::Tr(L"Colchete ']' inesperado sem abertura", L"Unexpected closing bracket ']'", L"Corchete ']' inesperado sin apertura");
                    analysis.errorCol = i;
                    break;
                }
            } else if (c == L'}') {
                if (!delimStack.empty() && delimStack.back().first == L'{') delimStack.pop_back();
                else {
                    analysis.hasSyntaxError = true;
                    analysis.syntaxErrorMessage = I18n::Tr(L"Chave '}' inesperada sem abertura", L"Unexpected closing brace '}'", L"Llave '}' inesperada sin apertura");
                    analysis.errorCol = i;
                    break;
                }
            }
        }

        if (!analysis.hasSyntaxError && inQuote != 0) {
            analysis.hasSyntaxError = true;
            analysis.errorCol = quoteStart;
            analysis.syntaxErrorMessage = (inQuote == L'\"') ? I18n::Tr(L"Aspas duplas '\"' não foram fechadas", L"Unclosed double quote '\"'", L"Comillas dobles '\"' sin cerrar") : I18n::Tr(L"Aspas simples '\'' não foram fechadas", L"Unclosed single quote '\''", L"Comilla simple '\'' sin cerrar");
        } else if (!analysis.hasSyntaxError && !delimStack.empty()) {
            analysis.hasSyntaxError = true;
            wchar_t unclosed = delimStack.back().first;
            analysis.errorCol = delimStack.back().second;
            if (unclosed == L'(') analysis.syntaxErrorMessage = I18n::Tr(L"Parêntese '(' não foi fechado", L"Unclosed parenthesis '('", L"Paréntesis '(' sin cerrar");
            else if (unclosed == L'[') analysis.syntaxErrorMessage = I18n::Tr(L"Colchete '[' não foi fechado", L"Unclosed bracket '['", L"Corchete '[' sin cerrar");
            else if (unclosed == L'{') analysis.syntaxErrorMessage = I18n::Tr(L"Chave '{' não foi fechada", L"Unclosed brace '{'", L"Llave '{' sin cerrar");
        }

        size_t idx = 0;
        size_t len = input.size();
        bool isFirstToken = true;
        std::wstring firstCommand;

        while (idx < len) {
            if (iswspace(input[idx])) {
                SyntaxToken sp;
                sp.text = input.substr(idx, 1);
                sp.startCol = idx;
                sp.endCol = idx + 1;
                sp.color = theme.textPrimary;
                analysis.tokens.push_back(sp);
                idx++;
                continue;
            }

            if (input[idx] == L'\"' || input[idx] == L'\'') {
                wchar_t quote = input[idx];
                size_t start = idx;
                idx++;
                while (idx < len && input[idx] != quote) {
                    if (input[idx] == L'\\' && idx + 1 < len) idx++;
                    idx++;
                }
                if (idx < len && input[idx] == quote) idx++;

                SyntaxToken tok;
                tok.text = input.substr(start, idx - start);
                tok.startCol = start;
                tok.endCol = idx;
                tok.color = theme.syntaxString;
                analysis.tokens.push_back(tok);
                isFirstToken = false;
                continue;
            }

            if (input[idx] == L'|' || input[idx] == L'&' || input[idx] == L';' || input[idx] == L'>' || input[idx] == L'<') {
                size_t start = idx;
                wchar_t op = input[idx];
                idx++;
                if (idx < len && input[idx] == op && (op == L'|' || op == L'&' || op == L'>')) {
                    idx++;
                }
                SyntaxToken tok;
                tok.text = input.substr(start, idx - start);
                tok.startCol = start;
                tok.endCol = idx;
                tok.color = theme.accent;
                analysis.tokens.push_back(tok);
                if (tok.text == L"|" || tok.text == L"||" || tok.text == L"&&" || tok.text == L";") {
                    isFirstToken = true;
                }
                continue;
            }

            size_t start = idx;
            while (idx < len && !iswspace(input[idx]) && input[idx] != L'\"' && input[idx] != L'\'' &&
                   input[idx] != L'|' && input[idx] != L'&' && input[idx] != L';' && input[idx] != L'>' && input[idx] != L'<') {
                idx++;
            }
            if (idx == start) {
                // Safeguard against zero-advance loop
                idx++;
            }
            std::wstring word = input.substr(start, idx - start);
            if (word.empty()) continue;

            SyntaxToken tok;
            tok.text = word;
            tok.startCol = start;
            tok.endCol = idx;

            if (isFirstToken) {
                firstCommand = ToLower(word);
                bool valid = IsValidExecutable(word, cwd);
                if (valid) {
                    tok.color = theme.syntaxValidCommand;
                } else {
                    tok.color = theme.syntaxInvalidCommand;
                    tok.isInvalid = true;
                    tok.isUnderlined = true;

                    LanguageMode lang = I18n::GetResolvedLanguage();
                    std::wstring suggestion = FindClosestCommand(word);
                    if (!suggestion.empty()) {
                        if (lang == LanguageMode::PtBR) {
                            analysis.ghostSuggestion = L"\u26A0 Comando não encontrado. Você quis dizer: " + suggestion + L" ? [Tab para corrigir]";
                        } else if (lang == LanguageMode::EsES) {
                            analysis.ghostSuggestion = L"\u26A0 Comando no encontrado. ¿Quiso decir: " + suggestion + L" ? [Tab para corregir]";
                        } else {
                            analysis.ghostSuggestion = L"\u26A0 Command not found. Did you mean: " + suggestion + L" ? [Tab to fix]";
                        }
                    } else {
                        if (lang == LanguageMode::PtBR) {
                            analysis.ghostSuggestion = L"\u26A0 Comando não encontrado no PATH ou built-ins";
                        } else if (lang == LanguageMode::EsES) {
                            analysis.ghostSuggestion = L"\u26A0 Comando no encontrado en PATH o built-ins";
                        } else {
                            analysis.ghostSuggestion = L"\u26A0 Command not found in PATH or built-ins";
                        }
                    }
                }
                isFirstToken = false;
            } else {
                if (StartsWith(word, L"-") || StartsWith(word, L"/")) {
                    tok.color = theme.syntaxFlag;
                } else if (StartsWith(word, L"$") || (StartsWith(word, L"%") && EndsWith(word, L"%"))) {
                    tok.color = theme.syntaxKeyword;
                } else if (word == L"|" || word == L">" || word == L">>" || word == L"&&" || word == L"||") {
                    tok.color = theme.accent;
                    if (word == L"|" || word == L"&&" || word == L"||") {
                        isFirstToken = true;
                    }
                } else if (!word.empty() && iswdigit(word[0])) {
                    tok.color = theme.syntaxNumber;
                } else if (firstCommand == L"git" && gitSubcommands.count(ToLower(word)) > 0) {
                    tok.color = theme.syntaxKeyword;
                } else {
                    bool exists = false;
                    for (const auto& f : cwdFiles) {
                        if (_wcsicmp(f.c_str(), word.c_str()) == 0) {
                            exists = true;
                            break;
                        }
                    }
                    if (exists) {
                        tok.color = theme.accent;
                    } else {
                        tok.color = theme.textPrimary;
                    }

                    if ((firstCommand == L"python" || firstCommand == L"py") && EndsWith(ToLower(word), L".py")) {
                        if (!exists) {
                            tok.color = theme.syntaxInvalidCommand;
                            tok.isUnderlined = true;
                            analysis.ghostSuggestion = std::wstring(I18n::Tr(L"\u26A0 Arquivo '", L"\u26A0 File '", L"\u26A0 Archivo '")) + word + I18n::Tr(L"' não existe no diretório atual", L"' does not exist in current directory", L"' no existe en el directorio actual");
                        }
                    } else if ((firstCommand == L"clang++" || firstCommand == L"g++" || firstCommand == L"cl") &&
                               (EndsWith(ToLower(word), L".cpp") || EndsWith(ToLower(word), L".c") || EndsWith(ToLower(word), L".cxx"))) {
                        if (!exists) {
                            tok.color = theme.syntaxInvalidCommand;
                            tok.isUnderlined = true;
                            analysis.ghostSuggestion = std::wstring(I18n::Tr(L"\u26A0 Arquivo fonte '", L"\u26A0 Source file '", L"\u26A0 Archivo fuente '")) + word + I18n::Tr(L"' não encontrado", L"' not found", L"' no encontrado");
                        }
                    }
                }
            }

            analysis.tokens.push_back(tok);
        }

        // Python -c code checking (compound statements missing colon and unclosed delimiters)
        if (firstCommand == L"python" || firstCommand == L"py") {
            size_t cPos = input.find(L"-c");
            if (cPos != std::wstring::npos) {
                size_t codeStart = cPos + 2;
                while (codeStart < input.size() && iswspace(input[codeStart])) codeStart++;
                if (codeStart < input.size()) {
                    std::wstring pyCode = std::wstring(TrimQuotes(input.substr(codeStart)));

                    // Check delimiters inside python code
                    std::vector<wchar_t> pyDelims;
                    wchar_t pyInQuote = 0;
                    for (size_t pi = 0; pi < pyCode.size(); ++pi) {
                        wchar_t pc = pyCode[pi];
                        if (pyInQuote != 0) {
                            if (pc == L'\\' && pi + 1 < pyCode.size()) {
                                ++pi;
                                continue;
                            }
                            if (pc == pyInQuote) pyInQuote = 0;
                            continue;
                        }
                        if (pc == L'\'' || pc == L'\"') {
                            pyInQuote = pc;
                        } else if (pc == L'(' || pc == L'[' || pc == L'{') {
                            pyDelims.push_back(pc);
                        } else if (pc == L')' && !pyDelims.empty() && pyDelims.back() == L'(') {
                            pyDelims.pop_back();
                        } else if (pc == L']' && !pyDelims.empty() && pyDelims.back() == L'[') {
                            pyDelims.pop_back();
                        } else if (pc == L'}' && !pyDelims.empty() && pyDelims.back() == L'{') {
                            pyDelims.pop_back();
                        }
                    }

                    if (!analysis.hasSyntaxError && !pyDelims.empty()) {
                        analysis.hasSyntaxError = true;
                        wchar_t unclosed = pyDelims.back();
                        if (unclosed == L'(') analysis.syntaxErrorMessage = I18n::Tr(L"Sintaxe Python: Parêntese '(' não foi fechado", L"Python syntax: Unclosed parenthesis '('", L"Sintaxis Python: Paréntesis '(' sin cerrar");
                        else if (unclosed == L'[') analysis.syntaxErrorMessage = I18n::Tr(L"Sintaxe Python: Colchete '[' não foi fechado", L"Python syntax: Unclosed bracket '['", L"Sintaxis Python: Corchete '[' sin cerrar");
                        else if (unclosed == L'{') analysis.syntaxErrorMessage = I18n::Tr(L"Sintaxe Python: Chave '{' não foi fechada", L"Python syntax: Unclosed brace '{'", L"Sintaxis Python: Llave '{' sin cerrar");
                    }

                    if (!analysis.hasSyntaxError) {
                        static const wchar_t* compoundKws[] = { L"if ", L"elif ", L"for ", L"while ", L"def ", L"class ", L"with " };
                        for (const auto* kw : compoundKws) {
                            size_t kwPos = pyCode.find(kw);
                            if (kwPos != std::wstring::npos) {
                                if (pyCode.find(L':', kwPos) == std::wstring::npos) {
                                    analysis.hasSyntaxError = true;
                                    analysis.syntaxErrorMessage = I18n::Tr(L"Sintaxe Python: Faltando dois-pontos ':' após declaração de bloco", L"Python syntax: Missing colon ':' after block statement", L"Sintaxis Python: Falta dos puntos ':' tras declaración de bloque");
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }

        // Node -e code checking
        if (firstCommand == L"node") {
            size_t ePos = input.find(L"-e");
            if (ePos != std::wstring::npos) {
                size_t codeStart = ePos + 2;
                while (codeStart < input.size() && iswspace(input[codeStart])) codeStart++;
                if (codeStart < input.size()) {
                    std::wstring jsCode = std::wstring(TrimQuotes(input.substr(codeStart)));
                    std::vector<wchar_t> jsDelims;
                    wchar_t jsInQuote = 0;
                    for (size_t ji = 0; ji < jsCode.size(); ++ji) {
                        wchar_t jc = jsCode[ji];
                        if (jsInQuote != 0) {
                            if (jc == L'\\' && ji + 1 < jsCode.size()) {
                                ++ji;
                                continue;
                            }
                            if (jc == jsInQuote) jsInQuote = 0;
                            continue;
                        }
                        if (jc == L'\'' || jc == L'\"' || jc == L'`') {
                            jsInQuote = jc;
                        } else if (jc == L'(' || jc == L'[' || jc == L'{') {
                            jsDelims.push_back(jc);
                        } else if (jc == L')' && !jsDelims.empty() && jsDelims.back() == L'(') {
                            jsDelims.pop_back();
                        } else if (jc == L']' && !jsDelims.empty() && jsDelims.back() == L'[') {
                            jsDelims.pop_back();
                        } else if (jc == L'}' && !jsDelims.empty() && jsDelims.back() == L'{') {
                            jsDelims.pop_back();
                        }
                    }
                    if (!jsDelims.empty() && !analysis.hasSyntaxError) {
                        analysis.hasSyntaxError = true;
                        wchar_t unclosed = jsDelims.back();
                        if (unclosed == L'(') analysis.syntaxErrorMessage = I18n::Tr(L"Sintaxe JS: Parêntese '(' não foi fechado", L"JS syntax: Unclosed parenthesis '('", L"Sintaxis JS: Paréntesis '(' sin cerrar");
                        else if (unclosed == L'[') analysis.syntaxErrorMessage = I18n::Tr(L"Sintaxe JS: Colchete '[' não foi fechado", L"JS syntax: Unclosed bracket '['", L"Sintaxis JS: Corchete '[' sin cerrar");
                        else if (unclosed == L'{') analysis.syntaxErrorMessage = I18n::Tr(L"Sintaxe JS: Chave '{' não foi fechada", L"JS syntax: Unclosed brace '{'", L"Sintaxis JS: Llave '{' sin cerrar");
                    }
                }
            }
        }

        // NPM run validation against package.json
        if (firstCommand == L"npm") {
            size_t runPos = input.find(L"run ");
            if (runPos != std::wstring::npos) {
                size_t scriptStart = runPos + 4;
                while (scriptStart < input.size() && iswspace(input[scriptStart])) scriptStart++;
                size_t scriptEnd = scriptStart;
                while (scriptEnd < input.size() && !iswspace(input[scriptEnd])) scriptEnd++;
                if (scriptEnd > scriptStart) {
                    std::wstring scriptName = input.substr(scriptStart, scriptEnd - scriptStart);
                    auto scripts = ReadNpmScripts(cwd);
                    if (!scripts.empty()) {
                        bool found = false;
                        for (const auto& s : scripts) {
                            if (s.first == scriptName) { found = true; break; }
                        }
                        if (!found) {
                            analysis.ghostSuggestion = std::wstring(I18n::Tr(L"\u26A0 Script '", L"\u26A0 Script '", L"\u26A0 Script '")) + scriptName + I18n::Tr(L"' não encontrado no package.json", L"' not found in package.json", L"' no encontrado en package.json");
                        }
                    }
                }
            }
        }

        // Git commit -m char counter
        if (StartsWith(ToLower(input), L"git commit -m")) {
            size_t mPos = input.find(L"-m");
            size_t firstQuote = input.find_first_of(L"\"'", mPos);
            if (firstQuote != std::wstring::npos) {
                size_t secondQuote = input.find_first_of(L"\"'", firstQuote + 1);
                size_t msgLen = (secondQuote != std::wstring::npos) ? (secondQuote - firstQuote - 1) : (input.size() - firstQuote - 1);
                analysis.characterCounter = L"Commit msg: " + std::to_wstring(msgLen) + L"/72 chars";
            }
        }

        return analysis;
    }

    std::vector<AutocompleteItem> GetSuggestions(const std::wstring& input, size_t cursorPos, const std::wstring& cwd, const GitStatusInfo& gitInfo) {
        std::vector<AutocompleteItem> items;
        RefreshCwdCache(cwd);

        // Find token being typed at cursorPos
        size_t tokenStart = cursorPos;
        while (tokenStart > 0 && !iswspace(input[tokenStart - 1])) {
            tokenStart--;
        }
        std::wstring currentToken = input.substr(tokenStart, cursorPos - tokenStart);
        std::wstring lowerToken = ToLower(currentToken);

        // Check if cursor is in the first token
        bool isFirstToken = (tokenStart == 0);

        std::wstring mainCmd;
        size_t firstSpace = input.find(L' ');
        if (firstSpace != std::wstring::npos && firstSpace <= tokenStart) {
            mainCmd = ToLower(Trim(input.substr(0, firstSpace)));
        }

        if (isFirstToken) {
            for (const auto& b : builtins) {
                if (lowerToken.empty() || StartsWith(b, lowerToken)) {
                    AutocompleteItem item;
                    item.text = b;
                    item.displayText = b;
                    item.type = L"[built-in]";
                    LanguageMode lang = I18n::GetResolvedLanguage();
                    item.detail = (lang == LanguageMode::PtBR) ? L"Comando nativo rápido C++" :
                                  ((lang == LanguageMode::EsES) ? L"Comando nativo rápido C++" : L"Fast C++ native command");
                    items.push_back(item);
                    if (items.size() >= 8) return items;
                }
            }

            static const wchar_t* tools[] = { L"python", L"clang++", L"g++", L"git", L"node", L"npm", L"pwsh", L"cargo" };
            for (const auto* t : tools) {
                if (lowerToken.empty() || StartsWith(t, lowerToken)) {
                    if (std::find_if(items.begin(), items.end(), [&](const AutocompleteItem& a) { return a.text == t; }) == items.end()) {
                        AutocompleteItem item;
                        item.text = t;
                        item.displayText = t;
                        item.type = L"[cli]";
                        item.detail = I18n::Tr(L"Executável de sistema", L"System executable", L"Ejecutable del sistema");
                        items.push_back(item);
                        if (items.size() >= 8) return items;
                    }
                }
            }

            if (!lowerToken.empty()) {
                for (const auto& p : pathExecutables) {
                    if (StartsWith(p, lowerToken)) {
                        AutocompleteItem item;
                        item.text = p;
                        item.displayText = p;
                        item.type = L"[path]";
                        items.push_back(item);
                        if (items.size() >= 8) return items;
                    }
                }
            }
        } else {
            if (mainCmd == L"cd") {
                for (const auto& d : cwdDirs) {
                    if (lowerToken.empty() || StartsWith(ToLower(d), lowerToken)) {
                        AutocompleteItem item;
                        item.text = d;
                        item.displayText = d + L"/";
                        item.type = L"[dir]";
                        item.isDir = true;
                        item.fullPath = cwd + L"\\" + d;
                        items.push_back(item);
                        if (items.size() >= 8) return items;
                    }
                }
                return items;
            }

            if (mainCmd == L"git") {
                if (input.substr(0, tokenStart) == L"git " || input.substr(0, tokenStart) == L"git") {
                    for (const auto& g : gitSubcommands) {
                        if (lowerToken.empty() || StartsWith(g, lowerToken)) {
                            AutocompleteItem item;
                            item.text = g;
                            item.displayText = g;
                            item.type = L"[git]";
                            items.push_back(item);
                            if (items.size() >= 8) return items;
                        }
                    }
                    return items;
                }

                if (input.find(L"checkout ") != std::wstring::npos || input.find(L"switch ") != std::wstring::npos || input.find(L"merge ") != std::wstring::npos) {
                    for (const auto& b : gitInfo.localBranches) {
                        if (lowerToken.empty() || StartsWith(ToLower(b), lowerToken)) {
                            AutocompleteItem item;
                            item.text = b;
                            item.displayText = b;
                            item.type = L"[branch]";
                            item.detail = I18n::Tr(L"Branch local", L"Local branch", L"Rama local");
                            items.push_back(item);
                            if (items.size() >= 8) return items;
                        }
                    }
                    return items;
                }
            }

            if (mainCmd == L"npm" && (input.find(L"run ") != std::wstring::npos || input == L"npm run")) {
                auto scripts = ReadNpmScripts(cwd);
                for (const auto& s : scripts) {
                    if (lowerToken.empty() || StartsWith(ToLower(s.first), lowerToken)) {
                        AutocompleteItem item;
                        item.text = s.first;
                        item.displayText = s.first;
                        item.type = L"[npm]";
                        item.detail = s.second;
                        items.push_back(item);
                        if (items.size() >= 8) return items;
                    }
                }
                return items;
            }

            if (mainCmd == L"python" || mainCmd == L"py") {
                for (const auto& f : cwdFiles) {
                    if (EndsWith(ToLower(f), L".py") && (lowerToken.empty() || StartsWith(ToLower(f), lowerToken))) {
                        AutocompleteItem item;
                        item.text = f;
                        item.displayText = f;
                        item.type = L"[py]";
                        item.isFile = true;
                        item.isCode = true;
                        item.fullPath = cwd + L"\\" + f;
                        LoadTextPreview(item);
                        items.push_back(item);
                        if (items.size() >= 8) return items;
                    }
                }
            }

            if (mainCmd == L"clang++" || mainCmd == L"g++") {
                for (const auto& f : cwdFiles) {
                    std::wstring lf = ToLower(f);
                    if ((EndsWith(lf, L".cpp") || EndsWith(lf, L".c") || EndsWith(lf, L".hpp") || EndsWith(lf, L".h")) &&
                        (lowerToken.empty() || StartsWith(lf, lowerToken))) {
                        AutocompleteItem item;
                        item.text = f;
                        item.displayText = f;
                        item.type = L"[cpp]";
                        item.isFile = true;
                        item.isCode = true;
                        item.fullPath = cwd + L"\\" + f;
                        LoadTextPreview(item);
                        items.push_back(item);
                        if (items.size() >= 8) return items;
                    }
                }
            }

            if (mainCmd == L"img") {
                for (const auto& f : cwdFiles) {
                    std::wstring lf = ToLower(f);
                    if ((EndsWith(lf, L".png") || EndsWith(lf, L".jpg") || EndsWith(lf, L".jpeg") || EndsWith(lf, L".bmp") || EndsWith(lf, L".ico")) &&
                        (lowerToken.empty() || StartsWith(lf, lowerToken))) {
                        AutocompleteItem item;
                        item.text = f;
                        item.displayText = f;
                        item.type = L"[img]";
                        item.isFile = true;
                        item.isImage = true;
                        item.fullPath = cwd + L"\\" + f;
                        items.push_back(item);
                        if (items.size() >= 8) return items;
                    }
                }
            }

            for (const auto& d : cwdDirs) {
                if (lowerToken.empty() || StartsWith(ToLower(d), lowerToken)) {
                    AutocompleteItem item;
                    item.text = d;
                    item.displayText = d + L"/";
                    item.type = L"[dir]";
                    item.isDir = true;
                    item.fullPath = cwd + L"\\" + d;
                    items.push_back(item);
                    if (items.size() >= 8) return items;
                }
            }

            for (const auto& f : cwdFiles) {
                if (lowerToken.empty() || StartsWith(ToLower(f), lowerToken)) {
                    AutocompleteItem item;
                    item.text = f;
                    item.displayText = f;
                    item.isFile = true;
                    item.fullPath = cwd + L"\\" + f;

                    std::wstring lf = ToLower(f);
                    if (EndsWith(lf, L".png") || EndsWith(lf, L".jpg") || EndsWith(lf, L".jpeg") || EndsWith(lf, L".bmp") || EndsWith(lf, L".ico")) {
                        item.type = L"[img]";
                        item.isImage = true;
                    } else if (EndsWith(lf, L".py") || EndsWith(lf, L".cpp") || EndsWith(lf, L".hpp") || EndsWith(lf, L".h") ||
                               EndsWith(lf, L".json") || EndsWith(lf, L".md") || EndsWith(lf, L".txt") || EndsWith(lf, L".ps1") || EndsWith(lf, L".bat")) {
                        item.type = L"[code]";
                        item.isCode = true;
                        LoadTextPreview(item);
                    } else {
                        item.type = L"[file]";
                    }

                    items.push_back(item);
                    if (items.size() >= 8) return items;
                }
            }
        }

        return items;
    }

private:
    static void LoadTextPreview(AutocompleteItem& item) {
        if (item.fullPath.empty()) return;
        std::ifstream file(item.fullPath);
        if (!file.is_open()) return;

        std::string line;
        int count = 0;
        while (std::getline(file, line) && count < 12) {
            item.previewLines.push_back(Utf8ToWide(line));
            count++;
        }
    }
};

}

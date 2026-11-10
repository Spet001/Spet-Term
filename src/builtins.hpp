#pragma once

#include "common.hpp"
#include "git_engine.hpp"
#include "themes.hpp"
#include "i18n.hpp"

namespace speta {

class Builtins {
public:
    static bool IsBuiltin(const std::wstring& cmd) {
        std::wstring lower = ToLower(Trim(cmd));
        size_t space = lower.find(L' ');
        if (space != std::wstring::npos) {
            lower = lower.substr(0, space);
        }
        static const wchar_t* list[] = {
            L"cd", L"ls", L"dir", L"cat", L"type", L"img", L"clear", L"cls",
            L"pwd", L"gl", L"echo", L"mkdir", L"rm", L"del", L"theme", L"mode",
            L"gstatus", L"glog", L"gbranch", L"help", L"history", L"split", L"quake", L"exit"
        };
        for (const auto* b : list) {
            if (lower == b) return true;
        }
        return false;
    }

    static bool ExecuteBuiltin(
        const std::wstring& fullCmd,
        std::wstring& cwd,
        std::wstring& prevCwd,
        CommandBlock& block,
        const Theme& theme,
        std::function<void(const std::wstring&)> onThemeChange,
        std::function<void(ShellMode)> onModeChange,
        std::function<ID2D1Bitmap*(const std::wstring&, D2D1_SIZE_F&)> onImageLoad,
        const std::vector<std::wstring>* pHistory = nullptr,
        std::function<void(bool)> onSplit = nullptr,
        std::function<void()> onQuake = nullptr,
        std::function<void()> onClosePane = nullptr
    ) {
        auto startTime = std::chrono::high_resolution_clock::now();

        std::wstring trimmed = Trim(fullCmd);
        size_t space = trimmed.find(L' ');
        std::wstring cmd = ToLower(space == std::wstring::npos ? trimmed : trimmed.substr(0, space));
        std::wstring args = (space == std::wstring::npos) ? L"" : Trim(trimmed.substr(space + 1));

        block.commandText = fullCmd;
        block.workingDirectory = cwd;
        block.exitCode = 0;

        auto AddLine = [&](const std::wstring& text, D2D1_COLOR_F color = { 1, 1, 1, 1 }, bool bold = false) {
            StyledLine sl;
            sl.rawText = text;
            StyledSpan span;
            span.text = text;
            span.fg = color;
            span.bold = bold;
            sl.spans.push_back(span);
            block.outputLines.push_back(sl);
        };

        if (cmd == L"cd") {
            ExecuteCd(args, cwd, prevCwd, block, theme);
        } else if (cmd == L"ls" || cmd == L"dir") {
            ExecuteLs(args, cwd, block, theme);
        } else if (cmd == L"cat" || cmd == L"type") {
            ExecuteCat(args, cwd, block, theme, onImageLoad);
        } else if (cmd == L"img") {
            ExecuteImg(args, cwd, block, theme, onImageLoad);
        } else if (cmd == L"pwd" || cmd == L"gl") {
            AddLine(cwd, theme.accent, true);
        } else if (cmd == L"echo") {
            AddLine(args, theme.textPrimary);
        } else if (cmd == L"mkdir") {
            if (args.empty()) {
                AddLine(I18n::Tr(L"Uso: mkdir <nome-do-diretorio>", L"Usage: mkdir <directory-name>", L"Uso: mkdir <nombre-del-directorio>"), theme.syntaxInvalidCommand);
                block.exitCode = 1;
            } else {
                std::wstring fullPath = cwd + L"\\" + args;
                if (CreateDirectoryW(fullPath.c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS) {
                    AddLine(std::wstring(I18n::Tr(L"[OK] Diretório criado com sucesso: ", L"[OK] Directory created successfully: ", L"[OK] Directorio creado con éxito: ")) + args, theme.syntaxValidCommand);
                } else {
                    AddLine(std::wstring(I18n::Tr(L"[Erro] Falha ao criar diretório. Erro: ", L"[Error] Failed to create directory. Error: ", L"[Error] Error al crear directorio. Código: ")) + std::to_wstring(GetLastError()), theme.syntaxInvalidCommand);
                    block.exitCode = 1;
                }
            }
        } else if (cmd == L"rm" || cmd == L"del") {
            if (args.empty()) {
                AddLine(I18n::Tr(L"Uso: rm <arquivo>", L"Usage: rm <file>", L"Uso: rm <archivo>"), theme.syntaxInvalidCommand);
                block.exitCode = 1;
            } else {
                std::wstring fullPath = cwd + L"\\" + args;
                if (DeleteFileW(fullPath.c_str()) || RemoveDirectoryW(fullPath.c_str())) {
                    AddLine(std::wstring(I18n::Tr(L"[OK] Removido: ", L"[OK] Removed: ", L"[OK] Eliminado: ")) + args, theme.syntaxValidCommand);
                } else {
                    AddLine(std::wstring(I18n::Tr(L"[Erro] Falha ao remover. Erro: ", L"[Error] Failed to remove. Error: ", L"[Error] Error al eliminar. Código: ")) + std::to_wstring(GetLastError()), theme.syntaxInvalidCommand);
                    block.exitCode = 1;
                }
            }
        } else if (cmd == L"theme") {
            if (args.empty()) {
                AddLine(I18n::Tr(
                    L"Temas disponíveis: Pitch Black, Galaxy, VS Code Like, Blue, Light, Pip-Boy Green, Pip-Boy Amber, PowerShell ISE, Warm Sepia",
                    L"Available themes: Pitch Black, Galaxy, VS Code Like, Blue, Light, Pip-Boy Green, Pip-Boy Amber, PowerShell ISE, Warm Sepia",
                    L"Temas disponibles: Pitch Black, Galaxy, VS Code Like, Blue, Light, Pip-Boy Green, Pip-Boy Amber, PowerShell ISE, Warm Sepia"
                ), theme.textDim);
                AddLine(std::wstring(I18n::Tr(L"Tema atual: ", L"Current theme: ", L"Tema actual: ")) + theme.name, theme.accent, true);
            } else {
                onThemeChange(args);
                AddLine(std::wstring(I18n::Tr(L"[OK] Tema alterado para: ", L"[OK] Theme changed to: ", L"[OK] Tema cambiado a: ")) + args, theme.syntaxValidCommand);
            }
        } else if (cmd == L"mode") {
            std::wstring m = ToLower(args);
            if (m == L"cmd") {
                onModeChange(ShellMode::ModeCMD);
                AddLine(I18n::Tr(L"[OK] Modo alterado para: CMD (>_)", L"[OK] Mode changed to: CMD (>_)", L"[OK] Modo cambiado a: CMD (>_)"), theme.promptCmd, true);
            } else if (m == L"pw" || m == L"powershell") {
                onModeChange(ShellMode::ModePW);
                AddLine(I18n::Tr(L"[OK] Modo alterado para: PowerShell (PS>)", L"[OK] Mode changed to: PowerShell (PS>)", L"[OK] Modo cambiado a: PowerShell (PS>)"), theme.promptPw, true);
            } else {
                AddLine(I18n::Tr(L"Uso: mode <pw | cmd>", L"Usage: mode <pw | cmd>", L"Uso: mode <pw | cmd>"), theme.syntaxInvalidCommand);
            }
        } else if (cmd == L"gstatus") {
            ExecuteGStatus(cwd, block, theme);
        } else if (cmd == L"glog") {
            ExecuteGLog(cwd, block, theme);
        } else if (cmd == L"gbranch") {
            ExecuteGBranch(cwd, block, theme);
        } else if (cmd == L"help") {
            ExecuteHelp(block, theme);
        } else if (cmd == L"clear" || cmd == L"cls") {
            // Handled externally or clears current pane
            block.outputLines.clear();
        } else if (cmd == L"history") {
            LanguageMode lang = I18n::GetResolvedLanguage();
            StyledLine title;
            title.rawText = (lang == LanguageMode::PtBR) ? L"Histórico de Comandos" : ((lang == LanguageMode::EsES) ? L"Historial de Comandos" : L"Command History");
            title.spans.push_back({ title.rawText, theme.accent, theme.bg, true });
            block.outputLines.push_back(title);

            if (pHistory && !pHistory->empty()) {
                for (size_t i = 0; i < pHistory->size(); ++i) {
                    wchar_t numBuf[16];
                    swprintf_s(numBuf, L"%3zu | ", i + 1);
                    StyledLine sl;
                    sl.rawText = std::wstring(numBuf) + (*pHistory)[i];
                    sl.spans.push_back({ numBuf, theme.textDim });
                    sl.spans.push_back({ (*pHistory)[i], theme.textPrimary });
                    block.outputLines.push_back(sl);
                }
            } else {
                AddLine((lang == LanguageMode::PtBR) ? L"Nenhum comando no histórico recente." : ((lang == LanguageMode::EsES) ? L"Ningún comando en el historial reciente." : L"No commands in recent history."), theme.textDim);
            }
        } else if (cmd == L"split") {
            LanguageMode lang = I18n::GetResolvedLanguage();
            std::wstring a = ToLower(args);
            bool vertical = (a == L"v" || a == L"vertical");
            if (a == L"close" || a == L"w") {
                if (onClosePane) onClosePane();
                AddLine((lang == LanguageMode::PtBR) ? L"Fechando painel ativo..." : ((lang == LanguageMode::EsES) ? L"Cerrando panel activo..." : L"Closing active pane..."), theme.textDim);
            } else if (onSplit) {
                onSplit(vertical);
                std::wstring msg = (lang == LanguageMode::PtBR) ? (L"Painel dividido com sucesso (" + (vertical ? std::wstring(L"vertical") : std::wstring(L"horizontal")) + L")") :
                                   ((lang == LanguageMode::EsES) ? (L"Panel dividido con éxito (" + (vertical ? std::wstring(L"vertical") : std::wstring(L"horizontal")) + L")") :
                                   (L"Pane split successfully (" + (vertical ? std::wstring(L"vertical") : std::wstring(L"horizontal")) + L")"));
                AddLine(msg, theme.syntaxValidCommand);
            }
        } else if (cmd == L"quake") {
            LanguageMode lang = I18n::GetResolvedLanguage();
            if (onQuake) onQuake();
            AddLine((lang == LanguageMode::PtBR) ? L"Quake Mode acionado." : ((lang == LanguageMode::EsES) ? L"Modo Quake activado." : L"Quake Mode activated."), theme.accent);
        } else if (cmd == L"exit") {
            if (onClosePane) onClosePane();
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        block.executionTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
        return true;
    }

private:
    static void ExecuteCd(const std::wstring& args, std::wstring& cwd, std::wstring& prevCwd, CommandBlock& block, const Theme& theme) {
        std::wstring target = args;
        if (target.empty() || target == L"~") {
            wchar_t userProfile[MAX_PATH];
            if (GetEnvironmentVariableW(L"USERPROFILE", userProfile, MAX_PATH) > 0) {
                target = userProfile;
            }
        } else if (target == L"-") {
            if (prevCwd.empty()) {
                LanguageMode lang = I18n::GetResolvedLanguage();
                StyledLine sl;
                sl.rawText = (lang == LanguageMode::PtBR) ? L"Nenhum diretório anterior." :
                             ((lang == LanguageMode::EsES) ? L"Ningún directorio anterior." :
                             L"No previous directory.");
                sl.spans.push_back({ sl.rawText, theme.syntaxInvalidCommand });
                block.outputLines.push_back(sl);
                block.exitCode = 1;
                return;
            }
            target = prevCwd;
        }

        if (target.size() >= 2 && target.front() == L'\"' && target.back() == L'\"') {
            target = target.substr(1, target.size() - 2);
        }

        wchar_t resolved[MAX_PATH];
        if (PathIsRelativeW(target.c_str())) {
            PathCombineW(resolved, cwd.c_str(), target.c_str());
        } else {
            wcscpy_s(resolved, target.c_str());
        }

        if (SetCurrentDirectoryW(resolved)) {
            prevCwd = cwd;
            wchar_t actual[MAX_PATH];
            GetCurrentDirectoryW(MAX_PATH, actual);
            cwd = actual;
        } else {
            LanguageMode lang = I18n::GetResolvedLanguage();
            StyledLine sl;
            sl.rawText = (lang == LanguageMode::PtBR) ? (L"Não foi possível encontrar o caminho: " + target) :
                         ((lang == LanguageMode::EsES) ? (L"No se pudo encontrar la ruta: " + target) :
                         (L"Could not find path: " + target));
            sl.spans.push_back({ sl.rawText, theme.syntaxInvalidCommand });
            block.outputLines.push_back(sl);
            block.exitCode = 1;
        }
    }

    static void ExecuteLs(const std::wstring& args, const std::wstring& cwd, CommandBlock& block, const Theme& theme) {
        std::wstring searchDir = cwd;
        if (!args.empty() && args != L"-l" && args != L"-a") {
            std::wstring cleanArgs = TrimQuotes(args);
            wchar_t res[MAX_PATH];
            if (PathIsRelativeW(cleanArgs.c_str())) {
                PathCombineW(res, cwd.c_str(), cleanArgs.c_str());
            } else {
                wcscpy_s(res, cleanArgs.c_str());
            }
            searchDir = res;
        }

        std::wstring pattern = searchDir + L"\\*";
        WIN32_FIND_DATAW fd;
        HANDLE hFind = FindFirstFileExW(pattern.c_str(), FindExInfoBasic, &fd, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);

        if (hFind == INVALID_HANDLE_VALUE) {
            LanguageMode lang = I18n::GetResolvedLanguage();
            StyledLine sl;
            sl.rawText = (lang == LanguageMode::PtBR) ? (L"Diretório não encontrado: " + searchDir) :
                         ((lang == LanguageMode::EsES) ? (L"Directorio no encontrado: " + searchDir) :
                         (L"Directory not found: " + searchDir));
            sl.spans.push_back({ sl.rawText, theme.syntaxInvalidCommand });
            block.outputLines.push_back(sl);
            block.exitCode = 1;
            return;
        }

        {
            LanguageMode lang = I18n::GetResolvedLanguage();
            StyledLine header;
            header.rawText = (lang == LanguageMode::PtBR) ? L"    Tipo   Data/Hora          Tamanho   Nome" :
                             ((lang == LanguageMode::EsES) ? L"    Tipo   Fecha/Hora         Tamaño    Nombre" :
                             L"    Type   Date/Time          Size      Name");
            header.spans.push_back({ header.rawText, theme.textDim, theme.bg, true });
            block.outputLines.push_back(header);

            StyledLine div;
            div.rawText = L"    ----   ---------          -------   ----";
            div.spans.push_back({ div.rawText, theme.blockBorder });
            block.outputLines.push_back(div);
        }

        uint64_t totalBytes = 0;
        int fileCount = 0;
        int dirCount = 0;

        std::vector<WIN32_FIND_DATAW> dirs;
        std::vector<WIN32_FIND_DATAW> files;

        do {
            if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                dirs.push_back(fd);
            } else {
                files.push_back(fd);
            }
        } while (FindNextFileW(hFind, &fd));
        FindClose(hFind);

        auto comp = [](const WIN32_FIND_DATAW& a, const WIN32_FIND_DATAW& b) {
            return _wcsicmp(a.cFileName, b.cFileName) < 0;
        };
        std::sort(dirs.begin(), dirs.end(), comp);
        std::sort(files.begin(), files.end(), comp);

        auto FormatEntry = [&](const WIN32_FIND_DATAW& item, bool isDir) {
            FILETIME ftLocal;
            FileTimeToLocalFileTime(&item.ftLastWriteTime, &ftLocal);
            SYSTEMTIME st;
            FileTimeToSystemTime(&ftLocal, &st);

            wchar_t dateBuf[32];
            swprintf_s(dateBuf, L"%04d-%02d-%02d %02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);

            ULARGE_INTEGER fSize;
            fSize.LowPart = item.nFileSizeLow;
            fSize.HighPart = item.nFileSizeHigh;

            StyledLine line;
            std::wstring typeStr = isDir ? L"  [DIR]  " : L" [FILE]  ";
            D2D1_COLOR_F typeCol = isDir ? theme.accent : theme.textDim;

            line.spans.push_back({ typeStr, typeCol, theme.bg, isDir });
            line.spans.push_back({ std::wstring(dateBuf) + L"   ", theme.textDim });

            wchar_t sizeBuf[16];
            if (isDir) {
                swprintf_s(sizeBuf, L"%10s   ", L"<DIR>");
                line.spans.push_back({ sizeBuf, theme.textDim });
            } else {
                std::wstring formattedSize = FormatBytes(fSize.QuadPart);
                swprintf_s(sizeBuf, L"%10s   ", formattedSize.c_str());
                line.spans.push_back({ sizeBuf, theme.syntaxNumber });
            }

            std::wstring name = item.cFileName;
            D2D1_COLOR_F nameCol = theme.textPrimary;
            bool bold = false;

            if (isDir) {
                nameCol = theme.accent;
                name += L"/";
                bold = true;
            } else {
                std::wstring lower = ToLower(name);
                if (EndsWith(lower, L".exe") || EndsWith(lower, L".cmd") || EndsWith(lower, L".bat") || EndsWith(lower, L".ps1")) {
                    nameCol = theme.syntaxValidCommand;
                    bold = true;
                } else if (EndsWith(lower, L".py") || EndsWith(lower, L".cpp") || EndsWith(lower, L".h") || EndsWith(lower, L".hpp") || EndsWith(lower, L".js")) {
                    nameCol = theme.syntaxKeyword;
                } else if (EndsWith(lower, L".png") || EndsWith(lower, L".jpg") || EndsWith(lower, L".bmp") || EndsWith(lower, L".ico")) {
                    nameCol = theme.syntaxString;
                }
            }

            line.spans.push_back({ name, nameCol, theme.bg, bold });

            line.rawText = typeStr + dateBuf + L"   " + sizeBuf + name;
            block.outputLines.push_back(line);
        };

        for (const auto& d : dirs) {
            dirCount++;
            FormatEntry(d, true);
        }
        for (const auto& f : files) {
            fileCount++;
            ULARGE_INTEGER sz;
            sz.LowPart = f.nFileSizeLow;
            sz.HighPart = f.nFileSizeHigh;
            totalBytes += sz.QuadPart;
            FormatEntry(f, false);
        }

        {
            LanguageMode lang = I18n::GetResolvedLanguage();
            StyledLine summary;
            wchar_t sumBuf[128];
            if (lang == LanguageMode::PtBR) {
                swprintf_s(sumBuf, L"\n    Total: %d arquivo(s) (%s), %d pasta(s)", fileCount, FormatBytes(totalBytes).c_str(), dirCount);
            } else if (lang == LanguageMode::EsES) {
                swprintf_s(sumBuf, L"\n    Total: %d archivo(s) (%s), %d carpeta(s)", fileCount, FormatBytes(totalBytes).c_str(), dirCount);
            } else {
                swprintf_s(sumBuf, L"\n    Total: %d file(s) (%s), %d folder(s)", fileCount, FormatBytes(totalBytes).c_str(), dirCount);
            }
            summary.rawText = sumBuf;
            summary.spans.push_back({ summary.rawText, theme.textDim, theme.bg, false });
            block.outputLines.push_back(summary);
        }
    }

    static void ExecuteCat(
        const std::wstring& args,
        const std::wstring& cwd,
        CommandBlock& block,
        const Theme& theme,
        std::function<ID2D1Bitmap*(const std::wstring&, D2D1_SIZE_F&)> onImageLoad
    ) {
        LanguageMode lang = I18n::GetResolvedLanguage();
        if (args.empty()) {
            StyledLine sl;
            sl.rawText = (lang == LanguageMode::PtBR) ? L"Uso: cat <arquivo>" : ((lang == LanguageMode::EsES) ? L"Uso: cat <archivo>" : L"Usage: cat <file>");
            sl.spans.push_back({ sl.rawText, theme.syntaxInvalidCommand });
            block.outputLines.push_back(sl);
            block.exitCode = 1;
            return;
        }

        std::wstring target = args;
        if (target.size() >= 2 && target.front() == L'\"' && target.back() == L'\"') {
            target = target.substr(1, target.size() - 2);
        }

        wchar_t resolved[MAX_PATH];
        if (PathIsRelativeW(target.c_str())) {
            PathCombineW(resolved, cwd.c_str(), target.c_str());
        } else {
            wcscpy_s(resolved, target.c_str());
        }

        std::wstring lower = ToLower(resolved);
        if (EndsWith(lower, L".png") || EndsWith(lower, L".jpg") || EndsWith(lower, L".jpeg") || EndsWith(lower, L".bmp") || EndsWith(lower, L".ico")) {
            ExecuteImg(resolved, cwd, block, theme, onImageLoad);
            return;
        }

        std::ifstream file(resolved);
        if (!file.is_open()) {
            StyledLine sl;
            sl.rawText = (lang == LanguageMode::PtBR) ? (L"Arquivo não encontrado: " + target) :
                         ((lang == LanguageMode::EsES) ? (L"Archivo no encontrado: " + target) :
                         (L"File not found: " + target));
            sl.spans.push_back({ sl.rawText, theme.syntaxInvalidCommand });
            block.outputLines.push_back(sl);
            block.exitCode = 1;
            return;
        }

        std::string line;
        int lineNum = 1;
        while (std::getline(file, line) && lineNum <= 500) {
            std::wstring wLine = Utf8ToWide(line);
            StyledLine sl;

            wchar_t numBuf[16];
            swprintf_s(numBuf, L"%4d | ", lineNum);
            sl.spans.push_back({ numBuf, theme.textDim });

            std::wstring trimmed = Trim(wLine);
            if (StartsWith(trimmed, L"#") || StartsWith(trimmed, L"//") || StartsWith(trimmed, L"rem ")) {
                sl.spans.push_back({ wLine, theme.syntaxComment });
            } else {
                sl.spans.push_back({ wLine, theme.textPrimary });
            }

            sl.rawText = std::wstring(numBuf) + wLine;
            block.outputLines.push_back(sl);
            lineNum++;
        }

        if (lineNum > 500) {
            StyledLine sl;
            sl.rawText = (lang == LanguageMode::PtBR) ? L"... [Arquivo truncado em 500 linhas]" :
                         ((lang == LanguageMode::EsES) ? L"... [Archivo truncado en 500 líneas]" :
                         L"... [File truncated at 500 lines]");
            sl.spans.push_back({ sl.rawText, theme.textDim });
            block.outputLines.push_back(sl);
        }
    }

    static void ExecuteImg(
        const std::wstring& args,
        const std::wstring& cwd,
        CommandBlock& block,
        const Theme& theme,
        std::function<ID2D1Bitmap*(const std::wstring&, D2D1_SIZE_F&)> onImageLoad
    ) {
        LanguageMode lang = I18n::GetResolvedLanguage();
        if (args.empty()) {
            StyledLine sl;
            sl.rawText = (lang == LanguageMode::PtBR) ? L"Uso: img <caminho-da-imagem>" : ((lang == LanguageMode::EsES) ? L"Uso: img <ruta-de-imagen>" : L"Usage: img <image-path>");
            sl.spans.push_back({ sl.rawText, theme.syntaxInvalidCommand });
            block.outputLines.push_back(sl);
            block.exitCode = 1;
            return;
        }

        std::wstring target = args;
        if (target.size() >= 2 && target.front() == L'\"' && target.back() == L'\"') {
            target = target.substr(1, target.size() - 2);
        }

        wchar_t resolved[MAX_PATH];
        if (PathIsRelativeW(target.c_str())) {
            PathCombineW(resolved, cwd.c_str(), target.c_str());
        } else {
            wcscpy_s(resolved, target.c_str());
        }

        D2D1_SIZE_F size{ 0, 0 };
        if (onImageLoad) {
            block.inlineImage = onImageLoad(resolved, size);
            block.imageSize = size;
            block.imagePath = resolved;
        }

        if (block.inlineImage) {
            WIN32_FILE_ATTRIBUTE_DATA fad;
            uint64_t bytes = 0;
            if (GetFileAttributesExW(resolved, GetFileExInfoStandard, &fad)) {
                ULARGE_INTEGER sz;
                sz.LowPart = fad.nFileSizeLow;
                sz.HighPart = fad.nFileSizeHigh;
                bytes = sz.QuadPart;
            }

            StyledLine sl;
            wchar_t infoBuf[128];
            const wchar_t* imgTag = (lang == LanguageMode::PtBR) ? L"Imagem" : ((lang == LanguageMode::EsES) ? L"Imagen" : L"Image");
            swprintf_s(infoBuf, L"[IMG] %s: %s (%.0fx%.0f px, %s)", imgTag, PathFindFileNameW(resolved), size.width, size.height, FormatBytes(bytes).c_str());
            sl.rawText = infoBuf;
            sl.spans.push_back({ sl.rawText, theme.accent, theme.bg, true });
            block.outputLines.push_back(sl);
        } else {
            StyledLine sl;
            sl.rawText = (lang == LanguageMode::PtBR) ? (L"Não foi possível decodificar a imagem: " + target) :
                         ((lang == LanguageMode::EsES) ? (L"No se pudo decodificar la imagen: " + target) :
                         (L"Could not decode image: " + target));
            sl.spans.push_back({ sl.rawText, theme.syntaxInvalidCommand });
            block.outputLines.push_back(sl);
            block.exitCode = 1;
        }
    }

    static void ExecuteGStatus(const std::wstring& cwd, CommandBlock& block, const Theme& theme) {
        LanguageMode lang = I18n::GetResolvedLanguage();
        GitStatusInfo info = GitEngine::InspectRepository(cwd);
        if (!info.isGitRepo) {
            StyledLine sl;
            sl.rawText = (lang == LanguageMode::PtBR) ? L"Diretório atual não é um repositório Git." :
                         ((lang == LanguageMode::EsES) ? L"El directorio actual no es un repositorio Git." :
                         L"Current directory is not a Git repository.");
            sl.spans.push_back({ sl.rawText, theme.syntaxInvalidCommand });
            block.outputLines.push_back(sl);
            block.exitCode = 1;
            return;
        }

        auto AddRow = [&](const std::wstring& label, const std::wstring& val, D2D1_COLOR_F valCol) {
            StyledLine sl;
            sl.rawText = L"  " + label + L": " + val;
            sl.spans.push_back({ L"  " + label + L": ", theme.textDim });
            sl.spans.push_back({ val, valCol, theme.bg, true });
            block.outputLines.push_back(sl);
        };

        StyledLine title;
        title.rawText = L"\u25C6 Git Status (Zero-Process Engine)";
        title.spans.push_back({ title.rawText, theme.accent, theme.bg, true });
        block.outputLines.push_back(title);

        const wchar_t* branchLabel = (lang == LanguageMode::PtBR) ? L"Branch Atual " : ((lang == LanguageMode::EsES) ? L"Rama Actual  " : L"Branch       ");
        const wchar_t* commitLabel = (lang == LanguageMode::PtBR) ? L"Último Commit" : ((lang == LanguageMode::EsES) ? L"Último Commit" : L"Last Commit  ");
        const wchar_t* msgLabel = (lang == LanguageMode::PtBR) ? L"Mensagem     " : ((lang == LanguageMode::EsES) ? L"Mensaje      " : L"Message      ");
        const wchar_t* authorLabel = (lang == LanguageMode::PtBR) ? L"Autor        " : ((lang == LanguageMode::EsES) ? L"Autor        " : L"Author       ");
        const wchar_t* timeLabel = (lang == LanguageMode::PtBR) ? L"Tempo        " : ((lang == LanguageMode::EsES) ? L"Tiempo       " : L"Time         ");
        const wchar_t* statusLabel = (lang == LanguageMode::PtBR) ? L"Status       " : ((lang == LanguageMode::EsES) ? L"Estado       " : L"Status       ");

        AddRow(branchLabel, info.branchName + (info.isDirty ? L" *" : L""), info.isDirty ? theme.gitDirty : theme.gitClean);
        if (!info.commitHashShort.empty()) {
            AddRow(commitLabel, info.commitHashShort, theme.syntaxNumber);
        }
        if (!info.lastCommitMsg.empty()) {
            AddRow(msgLabel, info.lastCommitMsg, theme.textPrimary);
        }
        if (!info.lastCommitAuthor.empty()) {
            AddRow(authorLabel, info.lastCommitAuthor, theme.accent);
        }
        if (!info.lastCommitRelativeTime.empty()) {
            AddRow(timeLabel, info.lastCommitRelativeTime, theme.textDim);
        }

        std::wstring statusVal = info.isDirty ?
            ((lang == LanguageMode::PtBR) ? L"Modificado (arquivos alterados no disco)" : ((lang == LanguageMode::EsES) ? L"Modificado (archivos alterados en disco)" : L"Modified (files changed on disk)")) :
            ((lang == LanguageMode::PtBR) ? L"Limpo (Working tree clean)" : ((lang == LanguageMode::EsES) ? L"Limpio (Working tree clean)" : L"Clean (Working tree clean)"));
        AddRow(statusLabel, statusVal, info.isDirty ? theme.gitDirty : theme.gitClean);

        if (!info.localBranches.empty()) {
            std::wstring bList;
            for (size_t i = 0; i < info.localBranches.size(); ++i) {
                if (i > 0) bList += L", ";
                bList += info.localBranches[i];
            }
            const wchar_t* branchesLabel = (lang == LanguageMode::PtBR) ? L"Branches     " : ((lang == LanguageMode::EsES) ? L"Ramas        " : L"Branches     ");
            AddRow(branchesLabel, bList, theme.textDim);
        }
    }

    static void ExecuteGLog(const std::wstring& cwd, CommandBlock& block, const Theme& theme) {
        LanguageMode lang = I18n::GetResolvedLanguage();
        auto logs = GitEngine::GetRecentCommitLogs(cwd, 10);
        if (logs.empty()) {
            StyledLine sl;
            sl.rawText = (lang == LanguageMode::PtBR) ? L"Nenhum histórico de commit encontrado ou não é um repositório Git." :
                         ((lang == LanguageMode::EsES) ? L"No se encontró historial de commits o no es un repositorio Git." :
                         L"No commit history found or not a Git repository.");
            sl.spans.push_back({ sl.rawText, theme.textDim });
            block.outputLines.push_back(sl);
            return;
        }

        StyledLine title;
        title.rawText = (lang == LanguageMode::PtBR) ? L"Histórico Recente de Commits (Zero-Process)" :
                        ((lang == LanguageMode::EsES) ? L"Historial Reciente de Commits (Zero-Process)" :
                        L"Recent Commit History (Zero-Process)");
        title.spans.push_back({ title.rawText, theme.accent, theme.bg, true });
        block.outputLines.push_back(title);

        for (const auto& log : logs) {
            StyledLine sl;
            sl.rawText = log;
            sl.spans.push_back({ log, theme.textPrimary });
            block.outputLines.push_back(sl);
        }
    }

    static void ExecuteGBranch(const std::wstring& cwd, CommandBlock& block, const Theme& theme) {
        LanguageMode lang = I18n::GetResolvedLanguage();
        GitStatusInfo info = GitEngine::InspectRepository(cwd);
        if (!info.isGitRepo) {
            StyledLine sl;
            sl.rawText = (lang == LanguageMode::PtBR) ? L"Não é um repositório Git." :
                         ((lang == LanguageMode::EsES) ? L"No es un repositorio Git." :
                         L"Not a Git repository.");
            sl.spans.push_back({ sl.rawText, theme.syntaxInvalidCommand });
            block.outputLines.push_back(sl);
            block.exitCode = 1;
            return;
        }

        StyledLine title;
        title.rawText = (lang == LanguageMode::PtBR) ? L"Branches Locais" :
                        ((lang == LanguageMode::EsES) ? L"Ramas Locales" :
                        L"Local Branches");
        title.spans.push_back({ title.rawText, theme.accent, theme.bg, true });
        block.outputLines.push_back(title);

        for (const auto& b : info.localBranches) {
            bool current = (b == info.branchName);
            StyledLine sl;
            if (current) {
                const wchar_t* curTag = (lang == LanguageMode::PtBR) ? L" (atual)" : ((lang == LanguageMode::EsES) ? L" (actual)" : L" (current)");
                sl.rawText = L"  * " + b + curTag;
                sl.spans.push_back({ sl.rawText, theme.gitClean, theme.bg, true });
            } else {
                sl.rawText = L"    " + b;
                sl.spans.push_back({ sl.rawText, theme.textDim });
            }
            block.outputLines.push_back(sl);
        }
    }

public:
    static void ExecuteHelp(CommandBlock& block, const Theme& theme) {
        LanguageMode lang = I18n::GetResolvedLanguage();
        StyledLine title;
        if (lang == LanguageMode::PtBR) {
            title.rawText = L"Spet@term: Guia de Comandos & Atalhos";
        } else if (lang == LanguageMode::EsES) {
            title.rawText = L"Spet@term: Guía de Comandos y Atajos";
        } else {
            title.rawText = L"Spet@term: Command & Shortcut Guide";
        }
        title.spans.push_back({ title.rawText, theme.accent, theme.bg, true });
        block.outputLines.push_back(title);

        auto AddH = [&](const std::wstring& item, const std::wstring& desc) {
            StyledLine sl;
            sl.rawText = L"  " + item + L" : " + desc;
            sl.spans.push_back({ L"  " + item, theme.syntaxKeyword, theme.bg, true });
            sl.spans.push_back({ L" : ", theme.textDim });
            sl.spans.push_back({ desc, theme.textPrimary });
            block.outputLines.push_back(sl);
        };

        if (lang == LanguageMode::PtBR) {
            AddH(L"cd <dir> | cd -", L"Navegação ultrarrápida com memória do último diretório");
            AddH(L"ls / dir", L"Listagem de arquivos colorida com ícones, badges e estatísticas");
            AddH(L"cat <file>", L"Exibe arquivo de código com números de linha e sintaxe");
            AddH(L"img <arquivo>", L"Renderiza imagem (.png, .jpg, .bmp) direto dentro do terminal via GPU");
            AddH(L"gstatus / glog", L"Comandos Git Zero-Process (< 1ms sem invocar processo git.exe)");
            AddH(L"gbranch", L"Lista todas as branches locais do repositório");
            AddH(L"theme <nome>", L"Alterna entre Pitch Black, Galaxy, VS Code Like, Blue e Light");
            AddH(L"mode <pw|cmd>", L"Alterna motor entre PowerShell e CMD");
            AddH(L"clear / cls", L"Limpa os blocos do terminal ativo");
            AddH(L"Ctrl+Shift+P", L"Abre Command Palette estilo VS Code com busca fuzzy e live theme preview");
            AddH(L"Ctrl+Tab", L"Alterna instantaneamente entre Modo PowerShell e Modo CMD");
            AddH(L"Alt+Shift+D / V", L"Divide terminal em Split Panes (Horizontal / Vertical)");
            AddH(L"Alt+Shift+W", L"Fecha o painel ativo");
            AddH(L"Alt+F", L"Aplica correção sugerida pelo Smart Error Doctor com 1 clique");
            AddH(L"F12 / Win+`", L"Quake Mode: desliza terminal suavemente do topo da tela");
        } else if (lang == LanguageMode::EsES) {
            AddH(L"cd <dir> | cd -", L"Navegación ultrarrápida con memoria del directorio anterior");
            AddH(L"ls / dir", L"Listado de archivos coloreado con iconos, etiquetas y estadísticas");
            AddH(L"cat <file>", L"Muestra archivo de código con números de línea y sintaxis");
            AddH(L"img <archivo>", L"Renderiza imagen (.png, .jpg, .bmp) directamente en la terminal mediante GPU");
            AddH(L"gstatus / glog", L"Comandos Git Zero-Process (< 1ms sin invocar proceso git.exe)");
            AddH(L"gbranch", L"Lista todas las ramas locales del repositorio");
            AddH(L"theme <nombre>", L"Alterna entre Pitch Black, Galaxy, VS Code Like, Blue y Light");
            AddH(L"mode <pw|cmd>", L"Alterna motor entre PowerShell y CMD");
            AddH(L"clear / cls", L"Limpia los bloques de la terminal activa");
            AddH(L"Ctrl+Shift+P", L"Abre Command Palette estilo VS Code con búsqueda difusa y vista previa de temas");
            AddH(L"Ctrl+Tab", L"Alterna instantáneamente entre Modo PowerShell e Modo CMD");
            AddH(L"Alt+Shift+D / V", L"Divide la terminal en Paneles Divididos (Horizontal / Vertical)");
            AddH(L"Alt+Shift+W", L"Cierra el panel activo");
            AddH(L"Alt+F", L"Aplica corrección sugerida por Smart Error Doctor con 1 clic");
            AddH(L"F12 / Win+`", L"Modo Quake: desliza la terminal suavemente desde la parte superior");
        } else {
            AddH(L"cd <dir> | cd -", L"Fast navigation with memory of the previous directory");
            AddH(L"ls / dir", L"Colorized file listing with icons, badges and statistics");
            AddH(L"cat <file>", L"Display code file with line numbers and syntax styling");
            AddH(L"img <file>", L"Render image (.png, .jpg, .bmp) directly inside terminal via GPU");
            AddH(L"gstatus / glog", L"Zero-Process Git commands (< 1ms without invoking git.exe)");
            AddH(L"gbranch", L"List all local branches in the repository");
            AddH(L"theme <name>", L"Switch between Pitch Black, Galaxy, VS Code Like, Blue and Light");
            AddH(L"mode <pw|cmd>", L"Switch engine between PowerShell and CMD");
            AddH(L"clear / cls", L"Clear blocks in the active terminal");
            AddH(L"Ctrl+Shift+P", L"Open VS Code style Command Palette with fuzzy search and live theme preview");
            AddH(L"Ctrl+Tab", L"Instantly toggle between PowerShell and CMD modes");
            AddH(L"Alt+Shift+D / V", L"Split terminal into Split Panes (Horizontal / Vertical)");
            AddH(L"Alt+Shift+W", L"Close the active pane");
            AddH(L"Alt+F", L"Apply Smart Error Doctor suggestion with 1 click");
            AddH(L"F12 / Win+`", L"Quake Mode: smoothly slide terminal from the top of the screen");
        }
    }
};

}

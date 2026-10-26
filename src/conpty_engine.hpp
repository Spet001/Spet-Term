#pragma once

#include "common.hpp"
#include "error_doctor.hpp"

namespace speta {

class VT100Parser {
public:
    D2D1_COLOR_F defaultFg{ 0.9f, 0.9f, 0.9f, 1.0f };
    D2D1_COLOR_F defaultBg{ 0.0f, 0.0f, 0.0f, 0.0f };

    D2D1_COLOR_F currentFg{ 0.9f, 0.9f, 0.9f, 1.0f };
    D2D1_COLOR_F currentBg{ 0.0f, 0.0f, 0.0f, 0.0f };
    bool isBold = false;
    bool isItalic = false;
    bool isUnderline = false;

    std::wstring pendingSeq;
    bool pendingCR = false;

    StyledLine currentLine;

    VT100Parser() {
        ResetStyle();
    }

    void SetDefaultColors(D2D1_COLOR_F fg, D2D1_COLOR_F bg) {
        defaultFg = fg;
        defaultBg = bg;
        currentFg = fg;
        currentBg = bg;
    }

    void ResetStyle() {
        currentFg = defaultFg;
        currentBg = defaultBg;
        isBold = false;
        isItalic = false;
        isUnderline = false;
        pendingSeq.clear();
        pendingCR = false;
    }

    void Feed(std::string_view chunk, std::vector<StyledLine>& outLines) {
        std::wstring wchunk = Utf8ToWide(chunk);
        if (!pendingSeq.empty()) {
            wchunk = pendingSeq + wchunk;
            pendingSeq.clear();
        }

        if (pendingCR) {
            pendingCR = false;
            if (!wchunk.empty() && wchunk[0] == L'\n') {
                FlushCurrentLine(outLines);
                wchunk.erase(0, 1);
            } else {
                currentLine = StyledLine();
            }
        }

        size_t idx = 0;
        size_t len = wchunk.size();

        while (idx < len) {
            wchar_t c = wchunk[idx];

            if (c == L'\x1b') {
                // Escape sequence start
                if (idx + 1 < len && wchunk[idx + 1] == L'[') {
                    // CSI sequence
                    size_t seqStart = idx + 2;
                    size_t seqEnd = seqStart;
                    while (seqEnd < len && (wchunk[seqEnd] >= L'0' && wchunk[seqEnd] <= L'?' || wchunk[seqEnd] == L';')) {
                        seqEnd++;
                    }

                    if (seqEnd < len) {
                        wchar_t finalChar = wchunk[seqEnd];
                        std::wstring paramStr = wchunk.substr(seqStart, seqEnd - seqStart);
                        if (finalChar == L'm') {
                            // SGR color/style sequence
                            ParseSGR(paramStr);
                        } else if (finalChar == L'J') {
                            // Clear display
                            if (paramStr == L"2" || paramStr.empty()) {
                                outLines.clear();
                                currentLine = StyledLine();
                            }
                        } else if (finalChar == L'K') {
                            // Erase in line
                        }
                        idx = seqEnd + 1;
                        continue;
                    } else {
                        // Sequence cut off across chunk boundary
                        pendingSeq = wchunk.substr(idx);
                        break;
                    }
                } else if (idx + 1 < len && wchunk[idx + 1] == L']') {
                    // OSC sequence (e.g. title) - skip to BEL (\x07) or ST (\x1b\)
                    size_t endOsc = idx + 2;
                    while (endOsc < len && wchunk[endOsc] != L'\x07') {
                        if (wchunk[endOsc] == L'\x1b' && endOsc + 1 < len && wchunk[endOsc + 1] == L'\\') {
                            endOsc += 2;
                            break;
                        }
                        endOsc++;
                    }
                    if (endOsc < len) {
                        if (wchunk[endOsc] == L'\x07') endOsc++;
                        idx = endOsc;
                        continue;
                    } else {
                        pendingSeq = wchunk.substr(idx);
                        break;
                    }
                } else if (idx + 1 >= len) {
                    pendingSeq = wchunk.substr(idx);
                    break;
                }
                idx++;
                continue;
            }

            if (c == L'\r') {
                if (idx + 1 < len) {
                    if (wchunk[idx + 1] == L'\n') {
                        // CRLF
                        FlushCurrentLine(outLines);
                        idx += 2;
                        continue;
                    } else {
                        // CR alone: carriage return (overwrite line)
                        currentLine = StyledLine();
                        idx++;
                        continue;
                    }
                } else {
                    pendingCR = true;
                    idx++;
                    break;
                }
            }

            if (c == L'\n') {
                FlushCurrentLine(outLines);
                idx++;
                continue;
            }

            if (c == L'\t') {
                // Tab expand to 4 spaces
                AppendChar(L' ');
                AppendChar(L' ');
                AppendChar(L' ');
                AppendChar(L' ');
                idx++;
                continue;
            }

            if (c == L'\b') {
                if (!currentLine.rawText.empty()) {
                    currentLine.rawText.pop_back();
                    if (!currentLine.spans.empty()) {
                        if (!currentLine.spans.back().text.empty()) {
                            currentLine.spans.back().text.pop_back();
                            if (currentLine.spans.back().text.empty()) {
                                currentLine.spans.pop_back();
                            }
                        }
                    }
                }
                idx++;
                continue;
            }

            // Normal printable char
            AppendChar(c);
            idx++;
        }
    }

    void FlushCurrentLine(std::vector<StyledLine>& outLines) {
        outLines.push_back(currentLine);
        currentLine = StyledLine();
    }

private:
    void AppendChar(wchar_t c) {
        currentLine.rawText += c;
        if (currentLine.spans.empty() || !CanMergeSpan(currentLine.spans.back())) {
            StyledSpan span;
            span.text += c;
            span.fg = currentFg;
            span.bg = currentBg;
            span.bold = isBold;
            span.italic = isItalic;
            span.underline = isUnderline;
            currentLine.spans.push_back(span);
        } else {
            currentLine.spans.back().text += c;
        }
    }

    bool CanMergeSpan(const StyledSpan& span) const {
        return span.fg.r == currentFg.r && span.fg.g == currentFg.g && span.fg.b == currentFg.b &&
               span.bg.r == currentBg.r && span.bg.g == currentBg.g && span.bg.b == currentBg.b &&
               span.bold == isBold && span.italic == isItalic && span.underline == isUnderline;
    }

    static D2D1_COLOR_F GetAnsi16Color(int code, bool bright) {
        static const uint32_t standardColors[8] = {
            0x0C0C0C, // Black
            0xC50F1F, // Red
            0x13A10E, // Green
            0xC19C00, // Yellow
            0x0037DA, // Blue
            0x881798, // Magenta
            0x3A96DD, // Cyan
            0xCCCCCC  // White
        };
        static const uint32_t brightColors[8] = {
            0x767676, // Bright Black (Gray)
            0xE74856, // Bright Red
            0x16C60C, // Bright Green
            0xF9F1A5, // Bright Yellow
            0x3B78FF, // Bright Blue
            0xB4009E, // Bright Magenta
            0x61D6D6, // Bright Cyan
            0xF2F2F2  // Bright White
        };

        int idx = (code >= 0 && code <= 7) ? code : 0;
        uint32_t rgb = bright ? brightColors[idx] : standardColors[idx];
        float r = ((rgb >> 16) & 0xFF) / 255.0f;
        float g = ((rgb >> 8) & 0xFF) / 255.0f;
        float b = (rgb & 0xFF) / 255.0f;
        return D2D1::ColorF(r, g, b, 1.0f);
    }

    static D2D1_COLOR_F Get256Color(int val) {
        if (val < 16) {
            return GetAnsi16Color(val % 8, val >= 8);
        } else if (val < 232) {
            val -= 16;
            int b = val % 6;
            val /= 6;
            int g = val % 6;
            int r = val / 6;
            auto f = [](int x) { return x ? (x * 40 + 55) / 255.0f : 0.0f; };
            return D2D1::ColorF(f(r), f(g), f(b), 1.0f);
        } else {
            float gray = ((val - 232) * 10 + 8) / 255.0f;
            return D2D1::ColorF(gray, gray, gray, 1.0f);
        }
    }

    void ParseSGR(const std::wstring& paramStr) {
        if (paramStr.empty()) {
            ResetStyle();
            return;
        }

        std::vector<std::wstring> parts = Split(paramStr, L';');
        std::vector<int> codes;
        for (const auto& p : parts) {
            try {
                codes.push_back(std::stoi(p));
            } catch (...) {
                codes.push_back(0);
            }
        }

        for (size_t i = 0; i < codes.size(); ++i) {
            int code = codes[i];
            if (code == 0) {
                ResetStyle();
            } else if (code == 1) {
                isBold = true;
            } else if (code == 3) {
                isItalic = true;
            } else if (code == 4) {
                isUnderline = true;
            } else if (code == 22) {
                isBold = false;
            } else if (code == 23) {
                isItalic = false;
            } else if (code == 24) {
                isUnderline = false;
            } else if (code >= 30 && code <= 37) {
                currentFg = GetAnsi16Color(code - 30, isBold);
            } else if (code == 39) {
                currentFg = defaultFg;
            } else if (code >= 40 && code <= 47) {
                currentBg = GetAnsi16Color(code - 40, false);
            } else if (code == 49) {
                currentBg = defaultBg;
            } else if (code >= 90 && code <= 97) {
                currentFg = GetAnsi16Color(code - 90, true);
            } else if (code >= 100 && code <= 107) {
                currentBg = GetAnsi16Color(code - 100, true);
            } else if (code == 38 && i + 1 < codes.size()) {
                // 38;5;n or 38;2;r;g;b
                if (codes[i + 1] == 5 && i + 2 < codes.size()) {
                    currentFg = Get256Color(codes[i + 2]);
                    i += 2;
                } else if (codes[i + 1] == 2 && i + 4 < codes.size()) {
                    float r = (std::clamp)(codes[i + 2], 0, 255) / 255.0f;
                    float g = (std::clamp)(codes[i + 3], 0, 255) / 255.0f;
                    float b = (std::clamp)(codes[i + 4], 0, 255) / 255.0f;
                    currentFg = D2D1::ColorF(r, g, b, 1.0f);
                    i += 4;
                }
            } else if (code == 48 && i + 1 < codes.size()) {
                if (codes[i + 1] == 5 && i + 2 < codes.size()) {
                    currentBg = Get256Color(codes[i + 2]);
                    i += 2;
                } else if (codes[i + 1] == 2 && i + 4 < codes.size()) {
                    float r = (std::clamp)(codes[i + 2], 0, 255) / 255.0f;
                    float g = (std::clamp)(codes[i + 3], 0, 255) / 255.0f;
                    float b = (std::clamp)(codes[i + 4], 0, 255) / 255.0f;
                    currentBg = D2D1::ColorF(r, g, b, 1.0f);
                    i += 4;
                }
            }
        }
    }
};

typedef HRESULT (WINAPI *PFN_CreatePseudoConsole)(COORD, HANDLE, HANDLE, DWORD, HPCON*);
typedef void (WINAPI *PFN_ClosePseudoConsole)(HPCON);
typedef HRESULT (WINAPI *PFN_ResizePseudoConsole)(HPCON, COORD);

class ConPtyEngine {
public:
    std::mutex blockMutex;
    std::atomic<bool> isCommandRunning{ false };
    HANDLE hProcess = nullptr;
    HANDLE hStdInWrite = nullptr;
    HANDLE hJob = nullptr;
    HANDLE hActiveReadPipe = nullptr;
    HPCON hPC = nullptr;
    PFN_CreatePseudoConsole pfnCreate = nullptr;
    PFN_ClosePseudoConsole pfnClose = nullptr;
    PFN_ResizePseudoConsole pfnResize = nullptr;
    StyledLine activeParserCurrentLine;
    std::thread workerThread;
    HWND notifyHwnd = nullptr;
    int currentRunningPaneId = 1;

    ConPtyEngine() {
        HMODULE hKernel = GetModuleHandleW(L"kernel32.dll");
        if (hKernel) {
            pfnCreate = reinterpret_cast<PFN_CreatePseudoConsole>(GetProcAddress(hKernel, "CreatePseudoConsole"));
            pfnClose = reinterpret_cast<PFN_ClosePseudoConsole>(GetProcAddress(hKernel, "ClosePseudoConsole"));
            pfnResize = reinterpret_cast<PFN_ResizePseudoConsole>(GetProcAddress(hKernel, "ResizePseudoConsole"));
        }
    }

    ~ConPtyEngine() {
        CancelCurrentCommand();
    }

    static void EnsureFreshUserPath() {
        HKEY hKey = nullptr;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Environment", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            wchar_t regPath[8192] = {};
            DWORD dataSize = sizeof(regPath);
            DWORD type = 0;
            if (RegQueryValueExW(hKey, L"Path", nullptr, &type, reinterpret_cast<LPBYTE>(regPath), &dataSize) == ERROR_SUCCESS) {
                wchar_t curPath[32768] = {};
                DWORD curLen = GetEnvironmentVariableW(L"PATH", curPath, 32768);
                wchar_t expanded[8192] = {};
                ExpandEnvironmentStringsW(regPath, expanded, 8192);
                std::wstring expStr = expanded;
                if (!expStr.empty() && curLen > 0) {
                    std::wstring curStr = curPath;
                    if (curStr.find(expStr) == std::wstring::npos) {
                        std::wstring merged = expStr + L";" + curStr;
                        SetEnvironmentVariableW(L"PATH", merged.c_str());
                    }
                }
            }
            RegCloseKey(hKey);
        }
    }

    void SetNotificationWindow(HWND hwnd) {
        notifyHwnd = hwnd;
    }

    void CancelCurrentCommand() {
        if (isCommandRunning.load()) {
            isCommandRunning.store(false);
            if (hPC) {
                if (pfnClose) pfnClose(hPC);
                hPC = nullptr;
            }
            if (hJob) {
                TerminateJobObject(hJob, 1);
            }
            if (hProcess) {
                TerminateProcess(hProcess, 1);
            }
            if (hActiveReadPipe) {
                CancelIoEx(hActiveReadPipe, nullptr);
            }
            if (hStdInWrite) {
                CloseHandle(hStdInWrite);
                hStdInWrite = nullptr;
            }
            {
                std::lock_guard<std::mutex> lock(blockMutex);
                activeParserCurrentLine = StyledLine();
            }
            if (workerThread.joinable()) {
                workerThread.detach();
            }
            if (notifyHwnd) {
                PostMessageW(notifyHwnd, WM_USER + 102, static_cast<WPARAM>(currentRunningPaneId), 0);
            }
        }
    }

    void SendInput(const std::wstring& text) {
        if (isCommandRunning.load() && hStdInWrite) {
            std::string utf8 = WideToUtf8(text);
            DWORD written = 0;
            WriteFile(hStdInWrite, utf8.data(), static_cast<DWORD>(utf8.size()), &written, nullptr);
        }
    }

    void WriteInput(const std::string& input) {
        if (input == "\x03") {
            CancelCurrentCommand();
            return;
        }
        if (isCommandRunning.load() && hStdInWrite) {
            DWORD written = 0;
            WriteFile(hStdInWrite, input.data(), static_cast<DWORD>(input.size()), &written, nullptr);
        }
    }

    static std::wstring BuildCommandLine(ShellMode mode, const std::wstring& cmd, bool hasPwsh = false) {
        if (mode == ShellMode::ModePW) {
            std::wstring pwExe = hasPwsh ? L"pwsh.exe" : L"powershell.exe";
            return pwExe + L" -NoLogo -NoProfile -ExecutionPolicy Bypass -Command " + cmd;
        } else {
            return L"cmd.exe /q /d /c " + cmd;
        }
    }

    bool ExecuteAsync(
        const std::wstring& cmd,
        ShellMode mode,
        const std::wstring& cwd,
        CommandBlock* pBlock,
        const Theme& theme,
        int paneId = 1
    ) {
        if (isCommandRunning.load()) {
            return false;
        }

        currentRunningPaneId = paneId;
        pBlock->commandText = FormatSingleLineCommandTitle(cmd);
        pBlock->modeUsed = mode;
        pBlock->workingDirectory = cwd;
        pBlock->isRunning = true;
        pBlock->exitCode = 0;
        pBlock->outputLines.clear();

        isCommandRunning.store(true);

        if (workerThread.joinable()) {
            workerThread.join();
        }

        workerThread = std::thread([this, cmd, mode, cwd, pBlock, theme, paneId]() {
            auto startTime = std::chrono::high_resolution_clock::now();
            EnsureFreshUserPath();

            // Construct shell command line (suppress logos, banners and warnings)
            std::wstring commandLine;
            std::wstring tempBatchFile;
            if (mode == ShellMode::ModePW) {
                static bool checkedPwsh = false;
                static bool hasPwsh = false;
                if (!checkedPwsh) {
                    wchar_t testPath[MAX_PATH];
                    hasPwsh = (SearchPathW(nullptr, L"pwsh.exe", nullptr, MAX_PATH, testPath, nullptr) > 0);
                    checkedPwsh = true;
                }
                bool isMultiOrWriteHost = (cmd.find(L'\n') != std::wstring::npos || cmd.find(L'\r') != std::wstring::npos || cmd.find(L"Write-Host") != std::wstring::npos);
                std::wstring pwExe = hasPwsh ? L"pwsh.exe" : L"powershell.exe";
                if (isMultiOrWriteHost) {
                    wchar_t tempDir[MAX_PATH];
                    GetTempPathW(MAX_PATH, tempDir);
                    wchar_t tempFile[MAX_PATH];
                    if (GetTempFileNameW(tempDir, L"spt", 0, tempFile)) {
                        tempBatchFile = tempFile;
                        tempBatchFile += L".ps1";
                        MoveFileW(tempFile, tempBatchFile.c_str());
                        FILE* f = nullptr;
                        if (_wfopen_s(&f, tempBatchFile.c_str(), L"wb") == 0 && f) {
                            const unsigned char bom[3] = { 0xEF, 0xBB, 0xBF };
                            fwrite(bom, 1, 3, f);
                            std::string utf8 = WideToUtf8(cmd);
                            fwrite(utf8.data(), 1, utf8.size(), f);
                            fclose(f);
                        }
                        commandLine = pwExe + L" -NoLogo -NoProfile -ExecutionPolicy Bypass -File \"" + tempBatchFile + L"\"";
                    } else {
                        commandLine = BuildCommandLine(mode, cmd, hasPwsh);
                    }
                } else {
                    commandLine = BuildCommandLine(mode, cmd, hasPwsh);
                }
            } else {
                bool isMultiLine = (cmd.find(L'\n') != std::wstring::npos || cmd.find(L'\r') != std::wstring::npos);
                if (isMultiLine) {
                    wchar_t tempDir[MAX_PATH];
                    GetTempPathW(MAX_PATH, tempDir);
                    wchar_t tempFile[MAX_PATH];
                    if (GetTempFileNameW(tempDir, L"spt", 0, tempFile)) {
                        tempBatchFile = tempFile;
                        tempBatchFile += L".cmd";
                        MoveFileW(tempFile, tempBatchFile.c_str());
                        FILE* f = nullptr;
                        if (_wfopen_s(&f, tempBatchFile.c_str(), L"wb") == 0 && f) {
                            std::string utf8 = WideToUtf8(cmd);
                            fwrite(utf8.data(), 1, utf8.size(), f);
                            fclose(f);
                        }
                        commandLine = L"cmd.exe /q /d /c \"" + tempBatchFile + L"\"";
                    } else {
                        commandLine = BuildCommandLine(mode, cmd, false);
                    }
                } else {
                    commandLine = BuildCommandLine(mode, cmd, false);
                }
            }

            // Path 1: Native Windows ConPTY (PseudoConsole) for interactive CLIs (agy, claude, python)
            if (this->pfnCreate && this->pfnClose) {
                HANDLE hInRead = nullptr, hInWrite = nullptr;
                HANDLE hOutRead = nullptr, hOutWrite = nullptr;
                CreatePipe(&hInRead, &hInWrite, nullptr, 0);
                CreatePipe(&hOutRead, &hOutWrite, nullptr, 0);

                COORD coord = { 120, 32 };
                HPCON hPseudo = nullptr;
                HRESULT hr = this->pfnCreate(coord, hInRead, hOutWrite, 0, &hPseudo);
                if (SUCCEEDED(hr)) {
                    this->hPC = hPseudo;
                    this->hStdInWrite = hInWrite;
                    this->hActiveReadPipe = hOutRead;

                    CloseHandle(hInRead);
                    CloseHandle(hOutWrite);

                    SIZE_T attrSize = 0;
                    InitializeProcThreadAttributeList(nullptr, 1, 0, &attrSize);
                    std::vector<BYTE> attrList(attrSize);
                    auto pAttr = reinterpret_cast<PPROC_THREAD_ATTRIBUTE_LIST>(attrList.data());
                    InitializeProcThreadAttributeList(pAttr, 1, 0, &attrSize);
                    UpdateProcThreadAttribute(pAttr, 0, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE, hPseudo, sizeof(HPCON), nullptr, nullptr);

                    STARTUPINFOEXW siex = {};
                    siex.StartupInfo.cb = sizeof(STARTUPINFOEXW);
                    siex.lpAttributeList = pAttr;

                    HANDLE hJob = CreateJobObjectW(nullptr, nullptr);
                    if (hJob) {
                        JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = {};
                        jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
                        SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli));
                        this->hJob = hJob;
                    }

                    PROCESS_INFORMATION pi = {};
                    std::vector<wchar_t> cmdBuf(commandLine.begin(), commandLine.end());
                    cmdBuf.push_back(L'\0');

                    BOOL success = CreateProcessW(
                        nullptr,
                        cmdBuf.data(),
                        nullptr,
                        nullptr,
                        FALSE,
                        EXTENDED_STARTUPINFO_PRESENT,
                        nullptr,
                        cwd.c_str(),
                        &siex.StartupInfo,
                        &pi
                    );

                    if (success) {
                        if (this->hJob) {
                            AssignProcessToJobObject(this->hJob, pi.hProcess);
                        }
                        this->hProcess = pi.hProcess;

                        // Background watcher thread to close PseudoConsole when client process exits
                        std::thread waiter([this, hProc = pi.hProcess, hPseudo]() {
                            WaitForSingleObject(hProc, INFINITE);
                            if (this->hPC == hPseudo) {
                                if (this->pfnClose) this->pfnClose(hPseudo);
                                this->hPC = nullptr;
                            }
                        });
                        waiter.detach();

                        VT100Parser parser;
                        parser.SetDefaultColors(theme.textPrimary, theme.bg);

                        char buffer[4096];
                        DWORD bytesRead = 0;

                        while (ReadFile(hOutRead, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0) {
                            buffer[bytesRead] = '\0';
                            {
                                std::lock_guard<std::mutex> lock(blockMutex);
                                parser.Feed(std::string_view(buffer, bytesRead), pBlock->outputLines);
                                activeParserCurrentLine = parser.currentLine;
                            }
                            if (notifyHwnd) {
                                PostMessageW(notifyHwnd, WM_USER + 101, static_cast<WPARAM>(paneId), 0);
                            }
                        }

                        {
                            std::lock_guard<std::mutex> lock(blockMutex);
                            if (!parser.currentLine.rawText.empty()) {
                                parser.FlushCurrentLine(pBlock->outputLines);
                            }
                            activeParserCurrentLine = StyledLine();
                        }

                        WaitForSingleObject(pi.hProcess, 3000);
                        DWORD exitCode = 0;
                        GetExitCodeProcess(pi.hProcess, &exitCode);

                        CloseHandle(pi.hProcess);
                        CloseHandle(pi.hThread);
                        CloseHandle(hOutRead);
                        if (hInWrite) CloseHandle(hInWrite);
                        this->hProcess = nullptr;
                        this->hActiveReadPipe = nullptr;
                        this->hStdInWrite = nullptr;
                        if (this->hJob) {
                            CloseHandle(this->hJob);
                            this->hJob = nullptr;
                        }
                        DeleteProcThreadAttributeList(pAttr);

                        if (!tempBatchFile.empty()) {
                            _wremove(tempBatchFile.c_str());
                        }

                        auto endTime = std::chrono::high_resolution_clock::now();
                        double durationMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

                        {
                            std::lock_guard<std::mutex> lock(blockMutex);
                            pBlock->isRunning = false;
                            pBlock->exitCode = static_cast<int>(exitCode);
                            pBlock->executionTimeMs = durationMs;
                            pBlock->diagnosis = ErrorDoctor::DiagnoseOutput(cmd, pBlock->exitCode, pBlock->outputLines);
                        }

                        isCommandRunning.store(false);

                        if (notifyHwnd) {
                            PostMessageW(notifyHwnd, WM_USER + 102, static_cast<WPARAM>(paneId), 0);
                        }
                        return;
                    }
                }
            }

            // Path 2: Anonymous Pipes Fallback (for older OS or when ConPTY initialization fails)
            SECURITY_ATTRIBUTES sa;
            sa.nLength = sizeof(SECURITY_ATTRIBUTES);
            sa.bInheritHandle = TRUE;
            sa.lpSecurityDescriptor = nullptr;

            HANDLE hReadPipe = nullptr;
            HANDLE hWritePipe = nullptr;
            if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
                std::lock_guard<std::mutex> lock(blockMutex);
                StyledLine sl;
                sl.rawText = L"Falha ao criar pipes de execucao.";
                sl.spans.push_back({ sl.rawText, theme.syntaxInvalidCommand });
                pBlock->outputLines.push_back(sl);
                pBlock->isRunning = false;
                pBlock->exitCode = 1;
                isCommandRunning.store(false);
                if (notifyHwnd) {
                    PostMessageW(notifyHwnd, WM_USER + 102, static_cast<WPARAM>(paneId), 0);
                }
                return;
            }
            SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

            HANDLE hInRead = nullptr;
            HANDLE hInWrite = nullptr;
            CreatePipe(&hInRead, &hInWrite, &sa, 0);
            if (hInWrite) {
                SetHandleInformation(hInWrite, HANDLE_FLAG_INHERIT, 0);
                this->hStdInWrite = hInWrite;
            }

            STARTUPINFOEXW siex = {};
            siex.StartupInfo.cb = sizeof(STARTUPINFOEXW);
            siex.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
            siex.StartupInfo.hStdOutput = hWritePipe;
            siex.StartupInfo.hStdError = hWritePipe;
            siex.StartupInfo.hStdInput = (hInRead != nullptr) ? hInRead : INVALID_HANDLE_VALUE;

            PROCESS_INFORMATION pi = {};
            std::vector<wchar_t> cmdBuf(commandLine.begin(), commandLine.end());
            cmdBuf.push_back(L'\0');

            // Create Job Object for reliable process-tree termination
            HANDLE hJob = CreateJobObjectW(nullptr, nullptr);
            if (hJob) {
                JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = {};
                jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
                SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli));
                this->hJob = hJob;
            }

            BOOL success = CreateProcessW(
                nullptr,
                cmdBuf.data(),
                nullptr,
                nullptr,
                TRUE,
                CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT,
                nullptr,
                cwd.c_str(),
                &siex.StartupInfo,
                &pi
            );

            // Close write end in parent so ReadFile terminates on process exit
            CloseHandle(hWritePipe);
            if (hInRead) CloseHandle(hInRead);

            if (!success) {
                CloseHandle(hReadPipe);
                if (hStdInWrite) {
                    CloseHandle(hStdInWrite);
                    hStdInWrite = nullptr;
                }
                if (this->hJob) {
                    CloseHandle(this->hJob);
                    this->hJob = nullptr;
                }
                std::lock_guard<std::mutex> lock(blockMutex);
                StyledLine sl;
                sl.rawText = L"Falha ao iniciar processo: " + std::to_wstring(GetLastError());
                sl.spans.push_back({ sl.rawText, theme.syntaxInvalidCommand });
                pBlock->outputLines.push_back(sl);
                pBlock->isRunning = false;
                pBlock->exitCode = 1;
                isCommandRunning.store(false);
                if (notifyHwnd) {
                    PostMessageW(notifyHwnd, WM_USER + 102, static_cast<WPARAM>(paneId), 0);
                }
                return;
            }

            if (this->hJob) {
                AssignProcessToJobObject(this->hJob, pi.hProcess);
            }
            this->hProcess = pi.hProcess;
            this->hActiveReadPipe = hReadPipe;

            // VT100 parser instance for this execution
            VT100Parser parser;
            parser.SetDefaultColors(theme.textPrimary, theme.bg);

            char buffer[4096];
            DWORD bytesRead = 0;

            while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0) {
                buffer[bytesRead] = '\0';
                {
                    std::lock_guard<std::mutex> lock(blockMutex);
                    parser.Feed(std::string_view(buffer, bytesRead), pBlock->outputLines);
                    activeParserCurrentLine = parser.currentLine;
                }
                if (notifyHwnd) {
                    PostMessageW(notifyHwnd, WM_USER + 101, static_cast<WPARAM>(paneId), 0);
                }
            }

            // Flush any remaining characters in parser
            {
                std::lock_guard<std::mutex> lock(blockMutex);
                if (!parser.currentLine.rawText.empty()) {
                    parser.FlushCurrentLine(pBlock->outputLines);
                }
                activeParserCurrentLine = StyledLine();
            }

            WaitForSingleObject(pi.hProcess, 3000);

            DWORD exitCode = 0;
            GetExitCodeProcess(pi.hProcess, &exitCode);

            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            CloseHandle(hReadPipe);
            this->hProcess = nullptr;
            this->hActiveReadPipe = nullptr;
            if (this->hJob) {
                CloseHandle(this->hJob);
                this->hJob = nullptr;
            }

            if (!tempBatchFile.empty()) {
                _wremove(tempBatchFile.c_str());
            }

            if (hStdInWrite) {
                CloseHandle(hStdInWrite);
                hStdInWrite = nullptr;
            }

            auto endTime = std::chrono::high_resolution_clock::now();
            double durationMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

            {
                std::lock_guard<std::mutex> lock(blockMutex);
                pBlock->isRunning = false;
                pBlock->exitCode = static_cast<int>(exitCode);
                pBlock->executionTimeMs = durationMs;

                // Smart Error Doctor diagnosis
                pBlock->diagnosis = ErrorDoctor::DiagnoseOutput(cmd, pBlock->exitCode, pBlock->outputLines);
            }

            isCommandRunning.store(false);

            if (notifyHwnd) {
                PostMessageW(notifyHwnd, WM_USER + 102, static_cast<WPARAM>(paneId), 0);
            }
        });

        return true;
    }
};

} // namespace speta

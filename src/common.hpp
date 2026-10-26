#pragma once

#ifndef _ALLOW_COMPILER_AND_STL_VERSION_MISMATCH
#define _ALLOW_COMPILER_AND_STL_VERSION_MISMATCH 1
#endif

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <dwmapi.h>
#include <psapi.h>
#include <shlwapi.h>
#include <shellapi.h>

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <chrono>
#include <functional>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <iomanip>
#include <cmath>

namespace speta {

template <typename T>
inline void SafeRelease(T*& p) {
    if (p) {
        p->Release();
        p = nullptr;
    }
}

inline bool IsWindows11OrGreater() {
    typedef LONG(NTAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (hNtdll) {
        auto pRtlGetVersion = reinterpret_cast<RtlGetVersionPtr>(GetProcAddress(hNtdll, "RtlGetVersion"));
        if (pRtlGetVersion) {
            RTL_OSVERSIONINFOW rovi = { sizeof(rovi) };
            if (pRtlGetVersion(&rovi) == 0) {
                return (rovi.dwMajorVersion > 10) || (rovi.dwMajorVersion == 10 && rovi.dwBuildNumber >= 22000);
            }
        }
    }
    return false;
}

inline std::wstring Utf8ToWide(std::string_view utf8) {
    if (utf8.empty()) return L"";
    // Strip UTF-8 BOM if present
    if (utf8.size() >= 3 && static_cast<unsigned char>(utf8[0]) == 0xEF &&
        static_cast<unsigned char>(utf8[1]) == 0xBB && static_cast<unsigned char>(utf8[2]) == 0xBF) {
        utf8.remove_prefix(3);
        if (utf8.empty()) return L"";
    }

    int req = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    if (req <= 0) {
        // Fallback to OEM console code page, then ACP
        req = MultiByteToWideChar(CP_OEMCP, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
        if (req <= 0) {
            req = MultiByteToWideChar(CP_ACP, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
            if (req <= 0) return L"";
            std::wstring wide(req, 0);
            MultiByteToWideChar(CP_ACP, 0, utf8.data(), static_cast<int>(utf8.size()), wide.data(), req);
            return wide;
        }
        std::wstring wide(req, 0);
        MultiByteToWideChar(CP_OEMCP, 0, utf8.data(), static_cast<int>(utf8.size()), wide.data(), req);
        return wide;
    }
    std::wstring wide(req, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), wide.data(), req);
    return wide;
}

inline std::string WideToUtf8(std::wstring_view wide) {
    if (wide.empty()) return "";
    int req = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
    if (req <= 0) return "";
    std::string utf8(req, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), utf8.data(), req, nullptr, nullptr);
    return utf8;
}

inline std::wstring ToLower(std::wstring_view s) {
    std::wstring res(s);
    for (auto& c : res) {
        c = static_cast<wchar_t>(towlower(c));
    }
    return res;
}

inline std::string ToLower(std::string_view s) {
    std::string res(s);
    for (auto& c : res) {
        c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    }
    return res;
}

inline bool StartsWith(std::wstring_view str, std::wstring_view prefix) {
    return str.size() >= prefix.size() && str.substr(0, prefix.size()) == prefix;
}

inline bool EndsWith(std::wstring_view str, std::wstring_view suffix) {
    return str.size() >= suffix.size() && str.substr(str.size() - suffix.size()) == suffix;
}

inline bool PtInD2D1Rect(const D2D1_RECT_F& r, POINT pt) {
    return pt.x >= r.left && pt.x <= r.right && pt.y >= r.top && pt.y <= r.bottom;
}

inline std::wstring Trim(std::wstring_view s) {
    size_t start = 0;
    while (start < s.size() && iswspace(s[start])) ++start;
    size_t end = s.size();
    while (end > start && iswspace(s[end - 1])) --end;
    return std::wstring(s.substr(start, end - start));
}

inline std::wstring TrimQuotes(std::wstring_view s) {
    std::wstring t = Trim(s);
    if (t.size() >= 2) {
        if ((t.front() == L'\"' && t.back() == L'\"') || (t.front() == L'\'' && t.back() == L'\'')) {
            return t.substr(1, t.size() - 2);
        }
    }
    return t;
}

inline std::wstring FormatSingleLineCommandTitle(const std::wstring& rawCmd) {
    if (rawCmd.find(L'\n') == std::wstring::npos && rawCmd.find(L'\r') == std::wstring::npos) {
        return Trim(rawCmd);
    }
    int lineCount = 0;
    std::wstring firstNonEmpty;
    size_t start = 0;
    while (start < rawCmd.size()) {
        size_t end = rawCmd.find_first_of(L"\r\n", start);
        std::wstring line = (end == std::wstring::npos) ? rawCmd.substr(start) : rawCmd.substr(start, end - start);
        std::wstring t = Trim(line);
        if (!t.empty()) {
            lineCount++;
            if (firstNonEmpty.empty()) firstNonEmpty = t;
        }
        if (end == std::wstring::npos) break;
        if (rawCmd[end] == L'\r' && end + 1 < rawCmd.size() && rawCmd[end + 1] == L'\n') end++;
        start = end + 1;
    }
    if (firstNonEmpty.empty()) firstNonEmpty = L"batch";
    if (lineCount > 1) {
        return firstNonEmpty + L"  [Batch " + std::to_wstring(lineCount) + L" linhas]";
    }
    return firstNonEmpty;
}

inline std::wstring Base64EncodeUtf16(const std::wstring& text) {
    if (text.empty()) return L"";
    static const char b64Chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(text.data());
    size_t len = text.size() * sizeof(wchar_t);
    std::string out;
    out.reserve(((len + 2) / 3) * 4);

    for (size_t i = 0; i < len; i += 3) {
        uint32_t b0 = bytes[i];
        uint32_t b1 = (i + 1 < len) ? bytes[i + 1] : 0;
        uint32_t b2 = (i + 2 < len) ? bytes[i + 2] : 0;
        uint32_t triple = (b0 << 16) | (b1 << 8) | b2;

        out.push_back(b64Chars[(triple >> 18) & 0x3F]);
        out.push_back(b64Chars[(triple >> 12) & 0x3F]);
        out.push_back((i + 1 < len) ? b64Chars[(triple >> 6) & 0x3F] : '=');
        out.push_back((i + 2 < len) ? b64Chars[triple & 0x3F] : '=');
    }
    return Utf8ToWide(out);
}

inline std::vector<std::wstring> Split(std::wstring_view s, wchar_t delim) {
    std::vector<std::wstring> parts;
    size_t start = 0;
    while (start < s.size()) {
        size_t next = s.find(delim, start);
        if (next == std::wstring_view::npos) {
            parts.emplace_back(s.substr(start));
            break;
        }
        parts.emplace_back(s.substr(start, next - start));
        start = next + 1;
    }
    return parts;
}

inline std::wstring FormatBytes(uint64_t bytes) {
    wchar_t buf[64];
    if (bytes < 1024) {
        swprintf_s(buf, L"%llu B", bytes);
    } else if (bytes < 1024 * 1024) {
        swprintf_s(buf, L"%.1f KB", bytes / 1024.0);
    } else if (bytes < 1024ULL * 1024 * 1024) {
        swprintf_s(buf, L"%.2f MB", bytes / (1024.0 * 1024.0));
    } else {
        swprintf_s(buf, L"%.2f GB", bytes / (1024.0 * 1024.0 * 1024.0));
    }
    return buf;
}

inline std::wstring FormatRelativeTime(int64_t secondsAgo) {
    if (secondsAgo < 0) secondsAgo = 0;
    wchar_t buf[64];
    if (secondsAgo < 60) {
        swprintf_s(buf, L"há %llds", secondsAgo);
    } else if (secondsAgo < 3600) {
        swprintf_s(buf, L"há %lldmin", secondsAgo / 60);
    } else if (secondsAgo < 86400) {
        swprintf_s(buf, L"há %lldh", secondsAgo / 3600);
    } else {
        swprintf_s(buf, L"há %lldd", secondsAgo / 86400);
    }
    return buf;
}

enum class ShellMode {
    ModePW,
    ModeCMD
};

struct StyledSpan {
    std::wstring text;
    D2D1_COLOR_F fg{ 1.0f, 1.0f, 1.0f, 1.0f };
    D2D1_COLOR_F bg{ 0.0f, 0.0f, 0.0f, 0.0f };
    bool bold = false;
    bool italic = false;
    bool underline = false;
};

struct StyledLine {
    std::vector<StyledSpan> spans;
    std::wstring rawText;
};

enum class ErrorType {
    None,
    PythonTraceback,
    CompilerError,
    GitError,
    CommandNotFound,
    SyntaxUnbalanced
};

struct ErrorDiagnosis {
    bool hasError = false;
    ErrorType errorType = ErrorType::None;
    std::wstring errorTitle;
    std::wstring errorMessage;
    std::wstring suggestedFix;
    std::wstring sourceFile;
    int sourceLine = 0;
    int sourceCol = 0;
};

struct CommandBlock {
    uint64_t id = 0;
    std::wstring commandText;
    ShellMode modeUsed = ShellMode::ModePW;
    std::wstring workingDirectory;
    std::vector<StyledLine> outputLines;
    ID2D1Bitmap* inlineImage = nullptr;
    D2D1_SIZE_F imageSize{ 0, 0 };
    std::wstring imagePath;
    double executionTimeMs = 0.0;
    int exitCode = 0;
    bool isRunning = false;
    bool isCollapsed = false;
    std::wstring timestamp;
    ErrorDiagnosis diagnosis;
    DWORD startTick = 0;

    // Interactive button rects computed during render
    D2D1_RECT_F headerRect{ 0, 0, 0, 0 };
    D2D1_RECT_F blockRect{ 0, 0, 0, 0 };
    D2D1_RECT_F copyButtonRect{ 0, 0, 0, 0 };
    D2D1_RECT_F rerunButtonRect{ 0, 0, 0, 0 };
    D2D1_RECT_F fixButtonRect{ 0, 0, 0, 0 };
    D2D1_RECT_F fileLinkRect{ 0, 0, 0, 0 };
    D2D1_RECT_F collapseButtonRect{ 0, 0, 0, 0 };
    D2D1_RECT_F pinButtonRect{ 0, 0, 0, 0 };
    D2D1_RECT_F exportButtonRect{ 0, 0, 0, 0 };

    CommandBlock() = default;
    ~CommandBlock() {
        SafeRelease(inlineImage);
    }

    CommandBlock(const CommandBlock& other) {
        id = other.id;
        commandText = other.commandText;
        modeUsed = other.modeUsed;
        workingDirectory = other.workingDirectory;
        outputLines = other.outputLines;
        inlineImage = other.inlineImage;
        if (inlineImage) inlineImage->AddRef();
        imageSize = other.imageSize;
        imagePath = other.imagePath;
        executionTimeMs = other.executionTimeMs;
        exitCode = other.exitCode;
        isRunning = other.isRunning;
        isCollapsed = other.isCollapsed;
        timestamp = other.timestamp;
        diagnosis = other.diagnosis;
        startTick = other.startTick;
        headerRect = other.headerRect;
        blockRect = other.blockRect;
    }

    CommandBlock& operator=(const CommandBlock& other) {
        if (this != &other) {
            SafeRelease(inlineImage);
            id = other.id;
            commandText = other.commandText;
            modeUsed = other.modeUsed;
            workingDirectory = other.workingDirectory;
            outputLines = other.outputLines;
            inlineImage = other.inlineImage;
            if (inlineImage) inlineImage->AddRef();
            imageSize = other.imageSize;
            imagePath = other.imagePath;
            executionTimeMs = other.executionTimeMs;
            exitCode = other.exitCode;
            isRunning = other.isRunning;
            isCollapsed = other.isCollapsed;
            timestamp = other.timestamp;
            diagnosis = other.diagnosis;
            startTick = other.startTick;
            headerRect = other.headerRect;
            blockRect = other.blockRect;
        }
        return *this;
    }

    CommandBlock(CommandBlock&& other) noexcept {
        id = other.id;
        commandText = std::move(other.commandText);
        modeUsed = other.modeUsed;
        workingDirectory = std::move(other.workingDirectory);
        outputLines = std::move(other.outputLines);
        inlineImage = other.inlineImage;
        other.inlineImage = nullptr;
        imageSize = other.imageSize;
        imagePath = std::move(other.imagePath);
        executionTimeMs = other.executionTimeMs;
        exitCode = other.exitCode;
        isRunning = other.isRunning;
        isCollapsed = other.isCollapsed;
        timestamp = std::move(other.timestamp);
        diagnosis = std::move(other.diagnosis);
        headerRect = other.headerRect;
        blockRect = other.blockRect;
        copyButtonRect = other.copyButtonRect;
        rerunButtonRect = other.rerunButtonRect;
        fixButtonRect = other.fixButtonRect;
        fileLinkRect = other.fileLinkRect;
        collapseButtonRect = other.collapseButtonRect;
        pinButtonRect = other.pinButtonRect;
        exportButtonRect = other.exportButtonRect;
    }

    CommandBlock& operator=(CommandBlock&& other) noexcept {
        if (this != &other) {
            SafeRelease(inlineImage);
            id = other.id;
            commandText = std::move(other.commandText);
            modeUsed = other.modeUsed;
            workingDirectory = std::move(other.workingDirectory);
            outputLines = std::move(other.outputLines);
            inlineImage = other.inlineImage;
            other.inlineImage = nullptr;
            imageSize = other.imageSize;
            imagePath = std::move(other.imagePath);
            executionTimeMs = other.executionTimeMs;
            exitCode = other.exitCode;
            isRunning = other.isRunning;
            isCollapsed = other.isCollapsed;
            timestamp = std::move(other.timestamp);
            diagnosis = std::move(other.diagnosis);
            headerRect = other.headerRect;
            blockRect = other.blockRect;
            copyButtonRect = other.copyButtonRect;
            rerunButtonRect = other.rerunButtonRect;
            fixButtonRect = other.fixButtonRect;
            fileLinkRect = other.fileLinkRect;
            collapseButtonRect = other.collapseButtonRect;
            pinButtonRect = other.pinButtonRect;
            exportButtonRect = other.exportButtonRect;
        }
        return *this;
    }

    std::wstring GetAllOutputText() const {
        std::wstring all;
        for (const auto& line : outputLines) {
            all += line.rawText;
            all += L"\n";
        }
        return all;
    }
};

inline void CopyTextToClipboard(HWND hwnd, const std::wstring& text) {
    if (!OpenClipboard(hwnd)) return;
    EmptyClipboard();
    size_t bytes = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (hMem) {
        wchar_t* pMem = static_cast<wchar_t*>(GlobalLock(hMem));
        if (pMem) {
            memcpy(pMem, text.data(), bytes);
            GlobalUnlock(hMem);
            SetClipboardData(CF_UNICODETEXT, hMem);
        }
    }
    CloseClipboard();
}

inline std::wstring GetTextFromClipboard(HWND hwnd) {
    if (!OpenClipboard(hwnd)) return L"";
    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    std::wstring result;
    if (hData) {
        wchar_t* pText = static_cast<wchar_t*>(GlobalLock(hData));
        if (pText) {
            result = pText;
            GlobalUnlock(hData);
        }
    }
    CloseClipboard();
    return result;
}

inline bool IsRunningAsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = nullptr;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(nullptr, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin != FALSE;
}

struct DetectedLink {
    std::wstring url;
    size_t startChar = 0;
    size_t length = 0;
    bool isUrl = true;
};

inline std::vector<DetectedLink> FindLinksInText(const std::wstring& text) {
    std::vector<DetectedLink> links;
    static const wchar_t* prefixes[] = { L"http://", L"https://" };

    for (const auto* prefix : prefixes) {
        size_t pLen = wcslen(prefix);
        size_t pos = 0;
        while ((pos = text.find(prefix, pos)) != std::wstring::npos) {
            size_t end = pos + pLen;
            while (end < text.size() && !iswspace(text[end]) && text[end] != L'\"' &&
                   text[end] != L'\'' && text[end] != L'<' && text[end] != L'>' &&
                   text[end] != L'(' && text[end] != L')' && text[end] != L'[' && text[end] != L']') {
                end++;
            }
            while (end > pos + pLen && (text[end - 1] == L'.' || text[end - 1] == L',' || text[end - 1] == L';' || text[end - 1] == L':')) {
                end--;
            }
            if (end > pos + pLen) {
                links.push_back({ text.substr(pos, end - pos), pos, end - pos, true });
            }
            pos = end;
        }
    }

    std::sort(links.begin(), links.end(), [](const DetectedLink& a, const DetectedLink& b) {
        return a.startChar < b.startChar;
    });

    return links;
}

inline std::vector<StyledLine> WrapStyledLine(const StyledLine& line, int maxCols) {
    if (maxCols <= 10) maxCols = 10;

    if (static_cast<int>(line.rawText.size()) <= maxCols) {
        if (!line.spans.empty() || line.rawText.empty()) {
            return { line };
        }
        StyledLine sl = line;
        sl.spans.push_back({ sl.rawText, D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f) });
        return { sl };
    }

    struct SpanInterval {
        size_t start;
        size_t end;
        const StyledSpan* ptr;
    };
    std::vector<SpanInterval> intervals;
    intervals.reserve(line.spans.size());
    size_t spanOffset = 0;
    for (const auto& span : line.spans) {
        size_t len = span.text.size();
        intervals.push_back({ spanOffset, spanOffset + len, &span });
        spanOffset += len;
    }

    std::vector<StyledLine> result;
    const std::wstring& text = line.rawText;
    const size_t totalLen = text.size();
    size_t curIdx = 0;

    while (curIdx < totalLen) {
        size_t remaining = totalLen - curIdx;
        if (static_cast<int>(remaining) <= maxCols) {
            size_t sliceStart = curIdx;
            size_t sliceEnd = totalLen;
            StyledLine sub;
            sub.rawText = text.substr(sliceStart, sliceEnd - sliceStart);
            for (const auto& itv : intervals) {
                size_t interStart = (std::max)(itv.start, sliceStart);
                size_t interEnd = (std::min)(itv.end, sliceEnd);
                if (interStart < interEnd) {
                    StyledSpan sp = *itv.ptr;
                    sp.text = text.substr(interStart, interEnd - interStart);
                    sub.spans.push_back(std::move(sp));
                }
            }
            if (sub.spans.empty() && !sub.rawText.empty()) {
                sub.spans.push_back({ sub.rawText, D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f) });
            }
            result.push_back(std::move(sub));
            break;
        }

        size_t breakLimit = curIdx + maxCols;
        size_t breakPos = std::wstring::npos;

        for (size_t p = breakLimit; p > curIdx; --p) {
            if (iswspace(text[p])) {
                breakPos = p;
                break;
            }
        }

        size_t sliceStart = curIdx;
        size_t sliceEnd = breakLimit;
        size_t nextIdx = breakLimit;

        if (breakPos != std::wstring::npos && breakPos > curIdx) {
            sliceEnd = breakPos;
            nextIdx = breakPos + 1;
            while (sliceEnd > curIdx && text[sliceEnd - 1] == L' ') {
                sliceEnd--;
            }
            while (nextIdx < totalLen && text[nextIdx] == L' ') {
                nextIdx++;
            }
        } else {
            sliceEnd = breakLimit;
            nextIdx = breakLimit;
        }

        StyledLine sub;
        sub.rawText = text.substr(sliceStart, sliceEnd - sliceStart);
        for (const auto& itv : intervals) {
            size_t interStart = (std::max)(itv.start, sliceStart);
            size_t interEnd = (std::min)(itv.end, sliceEnd);
            if (interStart < interEnd) {
                StyledSpan sp = *itv.ptr;
                sp.text = text.substr(interStart, interEnd - interStart);
                sub.spans.push_back(std::move(sp));
            }
        }
        if (sub.spans.empty() && !sub.rawText.empty()) {
            sub.spans.push_back({ sub.rawText, D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f) });
        }
        result.push_back(std::move(sub));

        if (nextIdx <= curIdx) {
            curIdx++;
        } else {
            curIdx = nextIdx;
        }
    }

    if (result.empty()) {
        result.push_back(line);
    }
    return result;
}

} // namespace speta

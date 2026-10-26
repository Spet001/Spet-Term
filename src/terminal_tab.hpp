#pragma once

#include "common.hpp"
#include "themes.hpp"
#include "syntax_autocomplete.hpp"
#include "git_engine.hpp"

namespace speta {

struct TerminalPane {
    int id = 1;
    std::wstring cwd;
    std::wstring prevCwd;
    ShellMode mode = ShellMode::ModePW;
    std::vector<std::unique_ptr<CommandBlock>> blocks;
    std::unique_ptr<CommandBlock> activeRunningBlock;
    std::wstring inputLine;
    size_t cursorPos = 0;
    float scrollOffsetY = 0.0f;
    float totalContentHeight = 0.0f;
    bool autoScrollToBottom = true;
    std::vector<std::wstring> commandHistory;
    int historyIndex = -1;

    SyntaxAnalysis syntaxAnalysis;
    std::vector<AutocompleteItem> autocompleteSuggestions;
    int autocompleteIndex = 0;
    bool autocompleteOpen = false;

    ID2D1Bitmap* previewThumb = nullptr;
    D2D1_SIZE_F previewThumbSize{ 0, 0 };

    // Hovered command index in classic stream
    int hoveredCommandIndex = -1;
    D2D1_RECT_F hoveredCopyBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F hoveredRerunBtnRect{ 0, 0, 0, 0 };

    TerminalPane() {
        wchar_t buf[MAX_PATH] = {};
        if (GetCurrentDirectoryW(MAX_PATH, buf) == 0 || buf[0] == L'\0') {
            cwd = L"C:\\";
        } else {
            cwd = buf;
        }
        prevCwd = cwd;
    }

    ~TerminalPane() {
        SafeRelease(previewThumb);
    }

    TerminalPane(const TerminalPane&) = delete;
    TerminalPane& operator=(const TerminalPane&) = delete;

    TerminalPane(TerminalPane&& other) noexcept = default;
    TerminalPane& operator=(TerminalPane&& other) noexcept = default;
};

struct TerminalTab {
    int id = 1;
    std::wstring title;
    bool customTitle = false;
    bool hasCustomTitle = false;
    bool useProjectTitle = false;
    D2D1_COLOR_F customAccent{ 0, 0, 0, 0 }; // alpha == 0 means use theme accent
    std::vector<std::unique_ptr<TerminalPane>> panes;
    int activePaneIndex = 0;
    bool isSplitActive = false;
    bool isSplitVertical = false; // false = Horizontal (side by side), true = Vertical (top-down)
    std::wstring loadedScriptPath; // for Quick Run [Play script]

    // Render layout cache rects
    D2D1_RECT_F tabRect{ 0, 0, 0, 0 };
    D2D1_RECT_F closeBtnRect{ 0, 0, 0, 0 };

    // Mini IDE / Editor Tab integration
    bool isEditor = false;
    std::wstring editorFilePath;
    std::wstring editorFileName;
    std::wstring editorContent;
    bool isDirty = false;
    float editorScrollY = 0.0f;
    size_t editorCursorLine = 0;
    size_t editorCursorCol = 0;
    bool isImagePreview = false;
    ID2D1Bitmap* editorImageBitmap = nullptr;
    D2D1_SIZE_F editorImageSize{ 0, 0 };

    ~TerminalTab() {
        SafeRelease(editorImageBitmap);
    }

    bool SaveEditorFile() {
        if (!isEditor || editorFilePath.empty()) return false;
        std::ofstream outFile(editorFilePath, std::ios::binary | std::ios::trunc);
        if (!outFile.is_open()) return false;
        std::string utf8 = WideToUtf8(editorContent);
        outFile.write(utf8.data(), utf8.size());
        outFile.close();
        isDirty = false;
        return true;
    }

    TerminalTab() {
        title = L"Windows PowerShell";
        panes.push_back(std::make_unique<TerminalPane>());
    }

    explicit TerminalTab(ShellMode mode, int tabId = 1) : id(tabId) {
        auto p = std::make_unique<TerminalPane>();
        p->mode = mode;
        title = (mode == ShellMode::ModePW) ? L"Windows PowerShell" : L"cmd.exe";
        panes.push_back(std::move(p));
    }

    static std::wstring FormatProjectTabTitle(const std::wstring& cwd, ShellMode mode) {
        std::wstring p = cwd;
        while (p.size() > 3 && (p.back() == L'\\' || p.back() == L'/')) {
            p.pop_back();
        }
        std::wstring folderName;
        if (p.size() <= 3 && p.find(L':') != std::wstring::npos) {
            folderName = p;
            if (folderName.back() != L'\\') folderName += L'\\';
        } else {
            size_t lastSlash = p.find_last_of(L"\\/");
            if (lastSlash != std::wstring::npos) {
                folderName = p.substr(lastSlash + 1);
            } else {
                folderName = p.empty() ? L"terminal" : p;
            }
        }
        std::wstring suffix = (mode == ShellMode::ModePW) ? L" (pw)" : L" (cmd)";
        return folderName + suffix;
    }

    TerminalPane& GetActivePane() {
        if (panes.empty()) {
            panes.push_back(std::make_unique<TerminalPane>());
            activePaneIndex = 0;
        }
        if (activePaneIndex >= static_cast<int>(panes.size())) {
            activePaneIndex = static_cast<int>(panes.size()) - 1;
        }
        return *panes[activePaneIndex];
    }

    void UpdateDefaultTitle() {
        if (customTitle || hasCustomTitle) return;
        if (useProjectTitle) {
            title = FormatProjectTabTitle(GetActivePane().cwd, GetActivePane().mode);
        } else {
            title = (GetActivePane().mode == ShellMode::ModePW) ? L"Windows PowerShell" : L"cmd.exe";
        }
    }

    void UpdateProjectTitle() {
        if (customTitle || hasCustomTitle) return;
        title = FormatProjectTabTitle(GetActivePane().cwd, GetActivePane().mode);
    }
};

} // namespace speta

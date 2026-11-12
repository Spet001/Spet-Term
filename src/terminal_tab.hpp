#pragma once

#include "common.hpp"
#include "themes.hpp"
#include "syntax_autocomplete.hpp"
#include "git_engine.hpp"
#include "workfolder.hpp"
#include "compression_engine.hpp"
#include "game_file_engine.hpp"
#include "preview_3d_media.hpp"
#include "file_explorer.hpp"

namespace speta {

enum class PaneMode {
    Launcher,
    Terminal,
    FileExplorer
};

struct CustomContextMenuItem {
    int id = 0;
    std::wstring label;
    std::wstring shortcut;
    wchar_t icon = 0;
    bool isSeparator = false;
    bool isDanger = false;
    D2D1_RECT_F rect{ 0, 0, 0, 0 };
};

struct CustomContextMenuState {
    bool isOpen = false;
    POINT origin{ 0, 0 };
    D2D1_RECT_F bounds{ 0, 0, 0, 0 };
    int hoveredIndex = -1;
    std::vector<CustomContextMenuItem> items;

    void Close() noexcept {
        isOpen = false;
        hoveredIndex = -1;
        items.clear();
    }
};

struct TerminalPane {
    int id = 1;
    PaneMode paneMode = PaneMode::Terminal;
    std::wstring cwd;
    std::wstring prevCwd;
    ShellMode mode = ShellMode::ModePW;
    std::vector<std::unique_ptr<CommandBlock>> blocks;
    std::unique_ptr<CommandBlock> activeRunningBlock;
    std::wstring inputLine;
    size_t cursorPos = 0;
    float scrollOffsetY = 0.0f;
    float totalContentHeight = 0.0f;
    float maxScrollY = 0.0f;
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

    // Git and Project Task telemetry cache
    GitStatusInfo cachedGitInfo;
    std::vector<ProjectTask> cachedTasks;
    std::wstring lastInspectedCwd;
    ULONGLONG lastGitCheckTick = 0;
    bool gitInfoDirty = true;

    // Pane bounding rect & Launcher mode UI state
    D2D1_RECT_F paneBoundsRect{ 0, 0, 0, 0 };
    int hoveredLauncherCard = -1;
    int selectedLauncherCard = 0;
    D2D1_RECT_F launcherCardRects[5]{};

    // File Explorer state & Preview Inspector cache
    FileExplorerPaneState explorer;
    std::wstring previewedKey;
    int activeInspectorTab = 0; // 0 = Preview, 1 = Hex Inspector, 2 = Container/PE Info
    ID2D1Bitmap* explorerPreviewBitmap = nullptr;
    D2D1_SIZE_F explorerPreviewBitmapSize{ 0, 0 };
    std::vector<uint8_t> explorerPreviewRgba;
    uint32_t explorerPreviewRgbaW = 0;
    uint32_t explorerPreviewRgbaH = 0;
    std::wstring explorerPreviewImageFormat;
    std::vector<std::wstring> explorerPreviewTextLines;
    std::vector<uint8_t> explorerPreviewRawBytes;
    size_t hexSelectedOffset = 0;
    float explorerPreviewScrollY = 0.0f;
    bool explorerPreviewIs3D = false;
    bool explorerPreviewIsAudio = false;
    bool explorerPreviewIsVideo = false;
    bool explorerPreviewIsDoomMap = false;
    DoomMapGeometry explorerDoomMap;
    bool explorerPreviewIsExecutable = false;
    ExecutableInspectInfo explorerExeInfo;
    std::vector<LockingProcessInfo> explorerLockingProcs;
    bool explorerLockCheckDone = false;

    // Hit-test rectangles for File Explorer UI
    D2D1_RECT_F expBackBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F expUpBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F expRefreshBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F expSearchBoxRect{ 0, 0, 0, 0 };
    D2D1_RECT_F expSearchModeBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F expLinkCwdBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F expF5CopyBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F expF6MoveBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F expExtractAllBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F expTableRect{ 0, 0, 0, 0 };
    D2D1_RECT_F expPreviewRect{ 0, 0, 0, 0 };
    D2D1_RECT_F expViewport3DRect{ 0, 0, 0, 0 };
    D2D1_RECT_F expColNameRect{ 0, 0, 0, 0 };
    D2D1_RECT_F expColSizeRect{ 0, 0, 0, 0 };
    D2D1_RECT_F expColTypeRect{ 0, 0, 0, 0 };
    D2D1_RECT_F expColDateRect{ 0, 0, 0, 0 };
    D2D1_RECT_F expInspectorTabRects[3]{};
    D2D1_RECT_F expActionEditBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F expActionExportPngBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F expActionPlayAudioBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F expActionStopAudioBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F expActionAudioSeekRect{ 0, 0, 0, 0 };
    D2D1_RECT_F expActionPlayVideoBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F expActionStopVideoBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F expActionVideoSeekRect{ 0, 0, 0, 0 };
    D2D1_RECT_F exp3DWireframeBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F exp3DAutoRotBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F exp3DResetBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F exp3DFlipVBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F expUnlockBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F expCalcHashBtnRect{ 0, 0, 0, 0 };
    bool expSearchFocused = false;
    bool isDragging3D = false;
    POINT lastMouse3D{ 0, 0 };

    TerminalPane() {
        wchar_t buf[MAX_PATH] = {};
        if (GetCurrentDirectoryW(MAX_PATH, buf) == 0 || buf[0] == L'\0') {
            cwd = L"C:\\";
        } else {
            cwd = buf;
        }
        prevCwd = cwd;
        explorer.NavigateToDirectory(cwd, true);
    }

    ~TerminalPane() {
        SafeRelease(previewThumb);
        SafeRelease(explorerPreviewBitmap);
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

    // Dedicated Media Playback & 3D Tab
    bool isMediaPlayback = false;
    bool isVideoPlayback = false;
    bool isAudioPlayback = false;
    bool is3DPreviewTab = false;
    std::wstring mediaFilePath;
    std::wstring mediaTitle;
    D2D1_RECT_F tabMediaPlayBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F tabMediaStopBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F tabMediaSeekRect{ 0, 0, 0, 0 };

    // Virtual Game Archive in-place editing & repacking
    bool isVirtualArchiveEditor = false;
    std::shared_ptr<ParsedGameArchive> editorVirtualArchive;
    size_t editorVirtualEntryIndex = 0;

    ~TerminalTab() {
        SafeRelease(editorImageBitmap);
    }

    bool SaveEditorFile() {
        if (!isEditor) return false;
        if (isVirtualArchiveEditor && editorVirtualArchive && editorVirtualEntryIndex < editorVirtualArchive->entries.size()) {
            std::string utf8 = WideToUtf8(editorContent);
            auto& entry = editorVirtualArchive->entries[editorVirtualEntryIndex];
            entry.inlineData.assign(utf8.begin(), utf8.end());
            entry.uncompressedSize = entry.inlineData.size();
            entry.crc32 = ComputeCrc32(entry.inlineData);
            if (!SaveModifiedArchive(*editorVirtualArchive, editorVirtualArchive->filePath)) {
                return false;
            }
            isDirty = false;
            return true;
        }
        if (editorFilePath.empty()) return false;
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

    explicit TerminalTab(ShellMode mode, int tabId = 1, PaneMode initialPaneMode = PaneMode::Terminal) : id(tabId) {
        auto p = std::make_unique<TerminalPane>();
        p->mode = mode;
        p->paneMode = initialPaneMode;
        if (initialPaneMode == PaneMode::Launcher) {
            title = I18n::Get(StringId::Launcher_Title);
        } else if (initialPaneMode == PaneMode::FileExplorer) {
            title = FormatProjectTabTitle(p->explorer.currentDirectory, mode, initialPaneMode);
        } else {
            title = (mode == ShellMode::ModePW) ? L"Windows PowerShell" : L"cmd.exe";
        }
        panes.push_back(std::move(p));
    }

    static std::wstring FormatProjectTabTitle(const std::wstring& cwd, ShellMode mode, PaneMode pm = PaneMode::Terminal) {
        if (pm == PaneMode::Launcher) {
            return I18n::Get(StringId::Launcher_Title);
        }
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
        if (pm == PaneMode::FileExplorer) {
            return folderName + L" (files)";
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
        const auto& ap = GetActivePane();
        if (ap.paneMode == PaneMode::Launcher) {
            title = I18n::Get(StringId::Launcher_Title);
        } else if (ap.paneMode == PaneMode::FileExplorer) {
            std::wstring pathForTitle = ap.explorer.inVirtualArchive ? ap.explorer.archiveHostPath : ap.explorer.currentDirectory;
            title = FormatProjectTabTitle(pathForTitle, ap.mode, PaneMode::FileExplorer);
        } else if (useProjectTitle) {
            title = FormatProjectTabTitle(ap.cwd, ap.mode, ap.paneMode);
        } else {
            title = (ap.mode == ShellMode::ModePW) ? L"Windows PowerShell" : L"cmd.exe";
        }
    }

    void UpdateProjectTitle() {
        if (customTitle || hasCustomTitle) return;
        const auto& ap = GetActivePane();
        std::wstring pathForTitle = (ap.paneMode == PaneMode::FileExplorer)
            ? (ap.explorer.inVirtualArchive ? ap.explorer.archiveHostPath : ap.explorer.currentDirectory)
            : ap.cwd;
        title = FormatProjectTabTitle(pathForTitle, ap.mode, ap.paneMode);
    }
};

} // namespace speta

#include "common.hpp"
#include "themes.hpp"
#include "settings.hpp"
#include "terminal_tab.hpp"
#include "git_engine.hpp"
#include "error_doctor.hpp"
#include "syntax_autocomplete.hpp"
#include "builtins.hpp"
#include "conpty_engine.hpp"
#include "renderer_d2d.hpp"

#include <windowsx.h>
#include <pdh.h>
#include <map>
#include <memory>
#include <commdlg.h>

using namespace speta;

// Global Constants & Window Messages
constexpr UINT WM_TERMINAL_STREAM = WM_USER + 101;
constexpr UINT WM_TERMINAL_FINISHED = WM_USER + 102;
constexpr int HOTKEY_QUAKE = 1001;

class SpetaTermApp {
public:
    HWND hwnd = nullptr;
    RendererD2D renderer;
    SyntaxAndAutocomplete syntaxEngine;
    ConPtyEngine conPty;
    Theme currentTheme;
    AppSettings settings;
    bool isAdmin = false;

    // Tabs
    std::vector<std::unique_ptr<TerminalTab>> tabs;
    int activeTabIndex = 0;
    int nextTabId = 1;
    int nextPaneId = 1;
    uint64_t nextBlockId = 1;

    // Inline Tab Renaming
    int renamingTabIndex = -1;
    std::wstring renamingText;

    // Menus & Popovers State
    bool fileMenuOpen = false;
    float fileMenuX = 6.0f;
    bool profileMenuOpen = false;
    float profileMenuX = 0.0f;
    bool dropdownMenuOpen = false;
    bool gitPopoverOpen = false;
    bool snippetsPopoverOpen = false;
    bool settingsOpen = false;
    bool helpModalOpen = false;
    bool multiLinePasteModalOpen = false;
    std::wstring pendingPasteText;

    // Mouse Text Selection
    bool isMouseSelecting = false;
    D2D1_POINT_2F selectionStartPt{ 0.0f, 0.0f };
    D2D1_POINT_2F selectionEndPt{ 0.0f, 0.0f };
    bool hasSelection = false;
    std::wstring selectedText;
    bool workspaceSidebarOpen = false;
    OnboardingState onboarding;
    bool workfolderOpen = false;
    std::wstring workfolderFilter;
    size_t workfolderSelectedIndex = 0;
    std::vector<WorkfolderItem> workfolderItems;

    // v4.0 Tactical Features State
    bool timelineOpen = false;
    size_t timelineSelectedIndex = 0;
    bool timelineFilterSuccessOnly = false;
    std::vector<TimelineEntry> timelineHistory;
    CommandBlock* pinnedBlock = nullptr;
    std::unique_ptr<CommandBlock> pinnedBlockStorage;
    float pinnedScrollY = 0.0f;
    bool taskMenuOpen = false;
    bool breadcrumbPopoverOpen = false;
    std::wstring breadcrumbTargetDir;
    std::vector<std::wstring> breadcrumbSiblings;
    D2D1_RECT_F breadcrumbAnchorRect{ 0, 0, 0, 0 };
    bool edgePulseActive = false;
    bool edgePulseSuccess = true;
    DWORD edgePulseStartTick = 0;
    double cpuUsagePercent = 0.0;
    size_t ramUsageMb = 0;
    bool isDraggingWarmth = false;
    std::vector<ProjectTask> detectedTasks;
    ULARGE_INTEGER lastCpuIdleTime = { 0 };
    ULARGE_INTEGER lastCpuKernelTime = { 0 };
    ULARGE_INTEGER lastCpuUserTime = { 0 };

    GitStatusInfo cachedGitInfo;
    std::wstring cachedCwd;
    ULONGLONG cachedInfoTick = 0;
    TelemetryData telemetry;
    PDH_HQUERY pdhQuery = nullptr;
    PDH_HCOUNTER pdhGpuCounter = nullptr;
    PDH_HCOUNTER pdhTempCounter = nullptr;
    bool pdhReady = false;
    ULONGLONG lastProcKernelUser = 0;
    ULONGLONG lastProcWallTick = 0;

    void InitPdh() {
        if (PdhOpenQueryW(nullptr, 0, &pdhQuery) != ERROR_SUCCESS) { pdhQuery = nullptr; return; }
        PdhAddEnglishCounterW(pdhQuery, L"\\GPU Engine(*)\\Utilization Percentage", 0, &pdhGpuCounter);
        PdhAddEnglishCounterW(pdhQuery, L"\\Thermal Zone Information(*)\\Temperature", 0, &pdhTempCounter);
        PdhCollectQueryData(pdhQuery);
        pdhReady = true;
    }

    void ShutdownPdh() {
        if (pdhQuery) { PdhCloseQuery(pdhQuery); pdhQuery = nullptr; }
        pdhReady = false;
    }

    void UpdateTelemetry() {
        if (!pdhReady && !pdhQuery) InitPdh();

        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
            static ULONGLONG lastTrimTick = 0;
            ULONGLONG currentTick = GetTickCount64();
            if (currentTick - lastTrimTick > 4000) {
                SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
                lastTrimTick = currentTick;
                GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
            }
            telemetry.procRamMb = pmc.WorkingSetSize / (1024 * 1024);
            ramUsageMb = telemetry.procRamMb;
        }

        FILETIME cT, eT, pK, pU;
        if (GetProcessTimes(GetCurrentProcess(), &cT, &eT, &pK, &pU)) {
            ULARGE_INTEGER k, u;
            k.LowPart = pK.dwLowDateTime; k.HighPart = pK.dwHighDateTime;
            u.LowPart = pU.dwLowDateTime; u.HighPart = pU.dwHighDateTime;
            ULONGLONG nowTick = GetTickCount64();
            ULONGLONG used = k.QuadPart + u.QuadPart;
            if (lastProcWallTick != 0 && nowTick > lastProcWallTick) {
                SYSTEM_INFO si; GetSystemInfo(&si);
                double wall100ns = static_cast<double>(nowTick - lastProcWallTick) * 10000.0 * (si.dwNumberOfProcessors ? si.dwNumberOfProcessors : 1);
                telemetry.procCpu = (std::min)(100.0, static_cast<double>(used - lastProcKernelUser) / wall100ns * 100.0);
            }
            lastProcKernelUser = used;
            lastProcWallTick = nowTick;
        }

        MEMORYSTATUSEX ms = { sizeof(ms) };
        if (GlobalMemoryStatusEx(&ms)) {
            telemetry.sysRamTotalGb = static_cast<double>(ms.ullTotalPhys) / (1024.0 * 1024.0 * 1024.0);
            telemetry.sysRamUsedGb = static_cast<double>(ms.ullTotalPhys - ms.ullAvailPhys) / (1024.0 * 1024.0 * 1024.0);
        }

        FILETIME idleTime, kernelTime, userTime;
        if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
            ULARGE_INTEGER idle, kernel, user;
            idle.LowPart = idleTime.dwLowDateTime; idle.HighPart = idleTime.dwHighDateTime;
            kernel.LowPart = kernelTime.dwLowDateTime; kernel.HighPart = kernelTime.dwHighDateTime;
            user.LowPart = userTime.dwLowDateTime; user.HighPart = userTime.dwHighDateTime;

            if (lastCpuKernelTime.QuadPart > 0) {
                ULONGLONG total = (kernel.QuadPart - lastCpuKernelTime.QuadPart) + (user.QuadPart - lastCpuUserTime.QuadPart);
                ULONGLONG idleDiff = idle.QuadPart - lastCpuIdleTime.QuadPart;
                if (total > 0) {
                    cpuUsagePercent = (1.0 - (double)idleDiff / (double)total) * 100.0;
                    if (cpuUsagePercent < 0.0) cpuUsagePercent = 0.0;
                    if (cpuUsagePercent > 100.0) cpuUsagePercent = 100.0;
                    telemetry.sysCpu = cpuUsagePercent;
                }
            }
            lastCpuIdleTime = idle;
            lastCpuKernelTime = kernel;
            lastCpuUserTime = user;
        }

        if (pdhQuery && PdhCollectQueryData(pdhQuery) == ERROR_SUCCESS) {
            if (pdhGpuCounter) {
                DWORD bufSize = 0, count = 0;
                PDH_STATUS st = PdhGetFormattedCounterArrayW(pdhGpuCounter, PDH_FMT_DOUBLE, &bufSize, &count, nullptr);
                if (st == static_cast<PDH_STATUS>(0x800007D2L) && bufSize > 0) {
                    std::vector<uint8_t> buf(bufSize);
                    auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buf.data());
                    if (PdhGetFormattedCounterArrayW(pdhGpuCounter, PDH_FMT_DOUBLE, &bufSize, &count, items) == ERROR_SUCCESS) {
                        std::wstring pidTag = L"pid_" + std::to_wstring(GetCurrentProcessId()) + L"_";
                        std::map<std::wstring, double> sysByEngine;
                        double procSum = 0.0;
                        for (DWORD i = 0; i < count; ++i) {
                            std::wstring name = items[i].szName ? items[i].szName : L"";
                            double v = items[i].FmtValue.doubleValue;
                            if (name.find(pidTag) != std::wstring::npos) procSum += v;
                            size_t et = name.find(L"engtype_");
                            std::wstring key = et != std::wstring::npos ? name.substr(et) : name;
                            size_t luid = name.find(L"luid_");
                            if (luid != std::wstring::npos) key += name.substr(luid, name.find(L'_', luid + 5 + 11) == std::wstring::npos ? 24 : 24);
                            sysByEngine[key] += v;
                        }
                        double sysMax = 0.0;
                        for (auto& kv : sysByEngine) sysMax = (std::max)(sysMax, kv.second);
                        telemetry.procGpu = (std::min)(100.0, procSum);
                        telemetry.sysGpu = (std::min)(100.0, sysMax);
                    }
                }
            }
            if (pdhTempCounter) {
                DWORD bufSize = 0, count = 0;
                PDH_STATUS st = PdhGetFormattedCounterArrayW(pdhTempCounter, PDH_FMT_DOUBLE, &bufSize, &count, nullptr);
                if (st == static_cast<PDH_STATUS>(0x800007D2L) && bufSize > 0) {
                    std::vector<uint8_t> buf(bufSize);
                    auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buf.data());
                    if (PdhGetFormattedCounterArrayW(pdhTempCounter, PDH_FMT_DOUBLE, &bufSize, &count, items) == ERROR_SUCCESS) {
                        double best = 0.0;
                        for (DWORD i = 0; i < count; ++i) best = (std::max)(best, items[i].FmtValue.doubleValue - 273.15);
                        // Filter out dummy 30.0 C (303.15 K) ACPI sensor reported on desktops without ACPI thermal zone
                        if (best > 0.0 && !(best >= 29.95 && best <= 30.05)) {
                            telemetry.sysTempC = best;
                            telemetry.sysTempValid = true;
                        } else {
                            telemetry.sysTempValid = false;
                        }
                    }
                }
            }
        }
    }
    // Toast Notification State
    std::wstring toastMessage;
    float toastAlpha = 0.0f;
    DWORD toastStartTick = 0;

    void ShowToast(const std::wstring& msg) {
        toastMessage = msg;
        toastAlpha = 1.0f;
        toastStartTick = GetTickCount();
        Invalidate();
    }

    // Search Box State (Ctrl+F)
    bool searchOpen = false;
    std::wstring searchQuery;
    int searchMatchIndex = 1;
    int totalSearchMatches = 0;

    // Command Palette
    struct PaletteAction {
        int id;
        std::wstring title;
        std::wstring desc;
        std::wstring themeName;
    };
    bool paletteOpen = false;
    std::wstring paletteQuery;
    int paletteSelectedIndex = 0;
    int paletteScrollOffset = 0;
    Theme prePaletteTheme;
    std::vector<PaletteAction> paletteActions;

    // Quake Mode State
    bool isQuakeVisible = true;

    // Cursor Blink & Mouse
    bool cursorBlinkState = true;
    POINT mousePos{ 0, 0 };

    SpetaTermApp() {
        isAdmin = IsRunningAsAdmin();
        bool settingsLoaded = settings.LoadFromFile();
        if (!settingsLoaded || !settings.onboardingCompleted) {
            onboarding.Reset();
        }
        I18n::SetLanguage(settings.language);

        currentTheme = ThemeManager::GetThemeByName(settings.themeName);
        renderer.currentFontFamily = settings.fontFamily;
        renderer.currentFontSize = settings.fontSize;

        // Initialize First Tab with unique pane ID and project tab title
        auto firstTab = std::make_unique<TerminalTab>(ShellMode::ModePW, nextTabId++);
        firstTab->useProjectTitle = true;
        firstTab->GetActivePane().id = nextPaneId++;
        firstTab->title = firstTab->FormatProjectTabTitle(firstTab->GetActivePane().cwd, ShellMode::ModePW);
        tabs.push_back(std::move(firstTab));
        DetectLoadedScript(*tabs[0]);

        InitPaletteActions();
    }

    void InitPaletteActions() {
        bool ptbr = (settings.language == "pt-BR");
        if (ptbr) {
            paletteActions = {
                { 19, L"Ajuda: Ver Todos os Comandos e Atalhos (help)", L"Exibe documentacao de atalhos e utilitarios do Spet@term", L"" },
                { 18, L"Exibir: Alternar Barra Lateral Workspace (Ctrl+B)", L"Mostra ou oculta explorador de arquivos e pastas", L"" },
                { 1,  L"Modo: Alternar para PowerShell (\u26A1)", L"Execucao em engine PowerShell persistente", L"" },
                { 2,  L"Modo: Alternar para CMD (>_)", L"Execucao em engine CMD persistente", L"" },
                { 3,  L"Aba: Nova Aba PowerShell (Ctrl+T)", L"Abre uma nova aba com PowerShell", L"" },
                { 4,  L"Aba: Nova Aba CMD (Ctrl+Shift+T)", L"Abre uma nova aba com CMD", L"" },
                { 5,  L"Aba: Fechar Aba Atual (Ctrl+W)", L"Fecha a aba ativa", L"" },
                { 6,  L"Aba: Nova Sessao como Administrador", L"Abre terminal elevado com privilegios de Admin", L"" },
                { 7,  L"Paineis: Dividir Horizontal (Alt+Shift+D)", L"Divide aba atual lado a lado", L"" },
                { 8,  L"Paineis: Dividir Vertical (Alt+Shift+V)", L"Divide aba atual acima e abaixo", L"" },
                { 9,  L"Paineis: Fechar Painel Ativo (Alt+Shift+W)", L"Fecha divisao de tela ativa", L"" },
                { 10, L"Git: Status Detalhado (gstatus)", L"Inspeciona repositorio sem criar processo (<1ms)", L"" },
                { 11, L"Git: Ver Ultimos Commits (glog)", L"Exibe arvore de commits recentes do HEAD", L"" },
                { 12, L"Git: Push com Auto-Upstream", L"git push --set-upstream origin <branch>", L"" },
                { 13, L"Git: Pull --rebase", L"Sincroniza commits remotos com rebase", L"" },
                { 14, L"Busca: Buscar no Historico (Ctrl+F)", L"Destaca ocorrencias de texto na aba ativa", L"" },
                { 15, L"Configuracoes: Abrir Painel Visual (Ctrl+,)", L"Ajusta tema, fonte, tamanho, opacidade e cursor", L"" },
                { 16, L"Terminal: Limpar Tela (clear)", L"Limpa o historico do painel ativo", L"" },
                { 17, L"Quake Mode: Alternar Visibilidade (F12)", L"Desliza o terminal do topo da tela", L"" },
                { 101, L"Tema: Pitch Black", L"OLED puro preto absoluto (#000000)", L"Pitch Black" },
                { 102, L"Tema: VS Code Like", L"Estilo Dark+ classico do VS Code com status azul", L"VS Code Like" },
                { 103, L"Tema: Galaxy", L"Cosmico translucido com nebulosa e estrelas procedurais", L"Galaxy" },
                { 104, L"Tema: Blue", L"Deep Navy maritimo com azul eletrico", L"Blue" },
                { 105, L"Tema: Light", L"Fundo claro de alto contraste e legibilidade", L"Light" }
            };
        } else {
            paletteActions = {
                { 19, L"Help: Show All Commands and Shortcuts (help)", L"Displays keyboard shortcuts and utilities guide", L"" },
                { 18, L"View: Toggle Workspace Sidebar (Ctrl+B)", L"Shows or hides workspace file explorer", L"" },
                { 1,  L"Mode: Switch to PowerShell (\u26A1)", L"Execution in persistent PowerShell engine", L"" },
                { 2,  L"Mode: Switch to CMD (>_)", L"Execution in persistent Command Prompt engine", L"" },
                { 3,  L"Tab: New PowerShell Tab (Ctrl+T)", L"Opens a new tab running PowerShell", L"" },
                { 4,  L"Tab: New CMD Tab (Ctrl+Shift+T)", L"Opens a new tab running CMD", L"" },
                { 5,  L"Tab: Close Current Tab (Ctrl+W)", L"Closes active tab", L"" },
                { 6,  L"Tab: New Administrator Session", L"Launches elevated terminal with Admin rights", L"" },
                { 7,  L"Panes: Split Right (Alt+Shift+D)", L"Splits active pane side-by-side", L"" },
                { 8,  L"Panes: Split Down (Alt+Shift+V)", L"Splits active pane top-and-bottom", L"" },
                { 9,  L"Panes: Close Active Pane (Alt+Shift+W)", L"Closes active screen split", L"" },
                { 10, L"Git: Detailed Status (gstatus)", L"Inspects repo instantly (<1ms)", L"" },
                { 11, L"Git: View Recent Commits (glog)", L"Displays recent commit tree from HEAD", L"" },
                { 12, L"Git: Push with Auto-Upstream", L"git push --set-upstream origin <branch>", L"" },
                { 13, L"Git: Pull --rebase", L"Syncs remote commits with rebase", L"" },
                { 14, L"Search: Find in History (Ctrl+F)", L"Highlights matching text in active tab", L"" },
                { 15, L"Settings: Visual Preferences (Ctrl+,)", L"Adjust theme, font, size, wallpaper and cursor", L"" },
                { 16, L"Terminal: Clear Screen (clear)", L"Clears output blocks in active pane", L"" },
                { 17, L"Quake Mode: Toggle Dropdown (F12)", L"Slides terminal from top of screen", L"" },
                { 101, L"Theme: Pitch Black", L"Pure OLED absolute black (#000000)", L"Pitch Black" },
                { 102, L"Theme: VS Code Like", L"Classic VS Code Dark+ style with blue status", L"VS Code Like" },
                { 103, L"Theme: Galaxy", L"Translucent cosmic nebula with procedural stars", L"Galaxy" },
                { 104, L"Theme: Blue", L"Maritime Deep Navy with electric blue", L"Blue" },
                { 105, L"Theme: Light", L"High-contrast light background for crisp readability", L"Light" }
            };
        }
    }

    TerminalTab& GetActiveTab() {
        if (tabs.empty()) {
            tabs.push_back(std::make_unique<TerminalTab>(ShellMode::ModePW, nextTabId++));
            activeTabIndex = 0;
        }
        if (activeTabIndex >= static_cast<int>(tabs.size())) {
            activeTabIndex = static_cast<int>(tabs.size()) - 1;
        }
        return *tabs[activeTabIndex];
    }

    TerminalPane& GetActivePane() {
        return GetActiveTab().GetActivePane();
    }

    void DetectLoadedScript(TerminalTab& tab) {
        if (!tab.loadedScriptPath.empty()) return;
        const auto& cwd = tab.GetActivePane().cwd;
        static const wchar_t* candidateScripts[] = {
            L"build.ps1", L"run.bat", L"main.py", L"app.py", L"index.js", L"CMakeLists.txt"
        };
        for (const auto* s : candidateScripts) {
            std::wstring testP = cwd + L"\\" + s;
            if (GetFileAttributesW(testP.c_str()) != INVALID_FILE_ATTRIBUTES) {
                tab.loadedScriptPath = testP;
                break;
            }
        }
    }

    void CreateNewTab(ShellMode mode) {
        auto tab = std::make_unique<TerminalTab>(mode, nextTabId++);
        tab->useProjectTitle = true;
        tab->GetActivePane().id = nextPaneId++;
        if (!tabs.empty()) {
            tab->GetActivePane().cwd = GetActivePane().cwd;
        }
        tab->title = tab->FormatProjectTabTitle(tab->GetActivePane().cwd, mode);
        DetectLoadedScript(*tab);
        tabs.push_back(std::move(tab));
        activeTabIndex = static_cast<int>(tabs.size()) - 1;
        Invalidate();
    }

    void OpenEditorTab(const std::wstring& path) {
        for (size_t i = 0; i < tabs.size(); ++i) {
            if (tabs[i]->isEditor && _wcsicmp(tabs[i]->editorFilePath.c_str(), path.c_str()) == 0) {
                activeTabIndex = static_cast<int>(i);
                Invalidate();
                return;
            }
        }

        std::wstring fileName = path;
        size_t lastSlash = fileName.find_last_of(L"\\/");
        if (lastSlash != std::wstring::npos) fileName = fileName.substr(lastSlash + 1);

        auto newTab = std::make_unique<TerminalTab>();
        newTab->id = nextTabId++;
        newTab->isEditor = true;
        newTab->editorFilePath = path;
        newTab->editorFileName = fileName;
        newTab->title = fileName;

        std::wstring ext = fileName;
        size_t dot = ext.find_last_of(L'.');
        std::wstring e = (dot != std::wstring::npos) ? ToLower(ext.substr(dot)) : L"";

        if (e == L".png" || e == L".jpg" || e == L".jpeg" || e == L".gif" || e == L".bmp" || e == L".ico") {
            newTab->isImagePreview = true;
            newTab->editorImageBitmap = renderer.LoadBitmapFromFile(path, newTab->editorImageSize);
        } else {
            std::ifstream inFile(path, std::ios::binary);
            if (inFile.is_open()) {
                std::stringstream buffer;
                buffer << inFile.rdbuf();
                newTab->editorContent = Utf8ToWide(buffer.str());
            }
        }

        tabs.push_back(std::move(newTab));
        activeTabIndex = static_cast<int>(tabs.size()) - 1;
        Invalidate();
    }

    bool IsOverTitlebarInteractive(POINT pt) const {
        if (pt.y >= RendererD2D::TITLEBAR_HEIGHT) return true;
        if (renderer.IsOverTitlebarInteractiveElement(pt)) return true;
        for (const auto& tab : tabs) {
            if (RendererD2D::PtInD2D1Rect(tab->tabRect, pt)) {
                return true;
            }
        }
        return false;
    }

    bool IsOverInteractive(POINT pt) const {
        if (IsOverTitlebarInteractive(pt)) return true;
        if (paletteOpen && (RendererD2D::PtInD2D1Rect(renderer.paletteCloseBtnRect, pt) || RendererD2D::PtInD2D1Rect(renderer.paletteModalRect, pt))) return true;
        if (timelineOpen) {
            if (RendererD2D::PtInD2D1Rect(renderer.timelineCloseBtnRect, pt)) return true;
            if (RendererD2D::PtInD2D1Rect(renderer.timelineFilterToggleRect, pt)) return true;
            if (RendererD2D::PtInD2D1Rect(renderer.timelineFooterRunRect, pt)) return true;
            if (RendererD2D::PtInD2D1Rect(renderer.timelineFooterTabRect, pt)) return true;
            if (RendererD2D::PtInD2D1Rect(renderer.timelineFooterEscRect, pt)) return true;
            for (const auto& it : renderer.timelineItemRects) if (RendererD2D::PtInD2D1Rect(it.first, pt)) return true;
            return false;
        }
        if (fileMenuOpen && RendererD2D::PtInD2D1Rect(renderer.fileMenuRect, pt)) return true;
        if (profileMenuOpen && RendererD2D::PtInD2D1Rect(renderer.profileMenuRect, pt)) return true;
        if (gitPopoverOpen && RendererD2D::PtInD2D1Rect(renderer.gitPopoverRect, pt)) return true;
        if (snippetsPopoverOpen && RendererD2D::PtInD2D1Rect(renderer.snippetsPopoverRect, pt)) return true;
        if (settingsOpen) {
            if (RendererD2D::PtInD2D1Rect(renderer.settingsCloseBtnRect, pt)) return true;
            if (RendererD2D::PtInD2D1Rect(renderer.settingsFontMinusRect, pt)) return true;
            if (RendererD2D::PtInD2D1Rect(renderer.settingsFontPlusRect, pt)) return true;
            if (RendererD2D::PtInD2D1Rect(renderer.settingsClassicModeBtnRect, pt)) return true;
            if (RendererD2D::PtInD2D1Rect(renderer.settingsBlocksModeBtnRect, pt)) return true;
            for (const auto& b : renderer.settingsThemeBtns) if (RendererD2D::PtInD2D1Rect(b.first, pt)) return true;
            for (const auto& b : renderer.settingsPromptPositionBtns) if (RendererD2D::PtInD2D1Rect(b.first, pt)) return true;
            D2D1_RECT_F warmthHit = renderer.settingsWarmthSliderTrackRect;
            warmthHit.top -= 10.0f; warmthHit.bottom += 10.0f;
            if (RendererD2D::PtInD2D1Rect(warmthHit, pt)) return true;
            for (const auto& b : renderer.settingsFontFamilyBtns) if (RendererD2D::PtInD2D1Rect(b.first, pt)) return true;
            for (const auto& b : renderer.settingsWallpaperBtns) if (RendererD2D::PtInD2D1Rect(b.first, pt)) return true;
            if (RendererD2D::PtInD2D1Rect(renderer.settingsWallpaperAddBtnRect, pt)) return true;
            D2D1_RECT_F dimHit = renderer.settingsWallpaperDimTrackRect;
            dimHit.top -= 10.0f; dimHit.bottom += 10.0f;
            if (RendererD2D::PtInD2D1Rect(dimHit, pt)) return true;
            for (const auto& b : renderer.settingsWallpaperPauseUnfocusedBtns) if (RendererD2D::PtInD2D1Rect(b.first, pt)) return true;
            for (const auto& b : renderer.settingsWallpaperPauseOnBatteryBtns) if (RendererD2D::PtInD2D1Rect(b.first, pt)) return true;
            for (const auto& b : renderer.settingsLanguageBtns) if (RendererD2D::PtInD2D1Rect(b.first, pt)) return true;
            for (const auto& b : renderer.settingsCursorBtns) if (RendererD2D::PtInD2D1Rect(b.first, pt)) return true;
            for (const auto& b : renderer.settingsSysCursorBtns) if (RendererD2D::PtInD2D1Rect(b.first, pt)) return true;
            if (RendererD2D::PtInD2D1Rect(renderer.settingsRegisterTermBtnRect, pt)) return true;
            for (const auto& b : renderer.settingsWarnPasteBtns) if (RendererD2D::PtInD2D1Rect(b.first, pt)) return true;
            if (RendererD2D::PtInD2D1Rect(renderer.settingsViewIntroBtnRect, pt)) return true;
            return false;
        }
        if (onboarding.open) {
            if (RendererD2D::PtInD2D1Rect(onboarding.nextBtnRect, pt)) return true;
            if (RendererD2D::PtInD2D1Rect(onboarding.backBtnRect, pt)) return true;
            if (RendererD2D::PtInD2D1Rect(onboarding.skipBtnRect, pt)) return true;
            for (const auto& b : onboarding.languageBtns) if (RendererD2D::PtInD2D1Rect(b.first, pt)) return true;
            if (RendererD2D::PtInD2D1Rect(onboarding.shellPwBtnRect, pt)) return true;
            if (RendererD2D::PtInD2D1Rect(onboarding.shellCmdBtnRect, pt)) return true;
            for (const auto& b : onboarding.themeBtns) if (RendererD2D::PtInD2D1Rect(b.first, pt)) return true;
            for (const auto& b : onboarding.fontBtns) if (RendererD2D::PtInD2D1Rect(b.first, pt)) return true;
            if (RendererD2D::PtInD2D1Rect(onboarding.fontMinusBtnRect, pt)) return true;
            if (RendererD2D::PtInD2D1Rect(onboarding.fontPlusBtnRect, pt)) return true;
            for (const auto& b : onboarding.wallpaperBtns) if (RendererD2D::PtInD2D1Rect(b.first, pt)) return true;
            if (RendererD2D::PtInD2D1Rect(onboarding.wallpaperAddBtnRect, pt)) return true;
            D2D1_RECT_F oDimHit = onboarding.wallpaperDimTrackRect;
            oDimHit.top -= 10.0f; oDimHit.bottom += 10.0f;
            if (RendererD2D::PtInD2D1Rect(oDimHit, pt)) return true;
            for (const auto& b : onboarding.warnPasteBtns) if (RendererD2D::PtInD2D1Rect(b.first, pt)) return true;
            for (const auto& b : onboarding.sysCursorBtns) if (RendererD2D::PtInD2D1Rect(b.first, pt)) return true;
            if (RendererD2D::PtInD2D1Rect(onboarding.registerTermBtnRect, pt)) return true;
            return false;
        }
        if (helpModalOpen && (RendererD2D::PtInD2D1Rect(renderer.helpCloseBtnRect, pt) || RendererD2D::PtInD2D1Rect(renderer.helpModalRect, pt))) return true;
        if (multiLinePasteModalOpen && (RendererD2D::PtInD2D1Rect(renderer.multiLinePasteConfirmBtnRect, pt) || RendererD2D::PtInD2D1Rect(renderer.multiLinePasteCancelBtnRect, pt) || RendererD2D::PtInD2D1Rect(renderer.multiLinePasteModalRect, pt))) return true;
        if (breadcrumbPopoverOpen && RendererD2D::PtInD2D1Rect(renderer.breadcrumbPopoverRect, pt)) return true;
        if (taskMenuOpen && RendererD2D::PtInD2D1Rect(renderer.taskMenuRect, pt)) return true;
        if (workfolderOpen) {
            if (RendererD2D::PtInD2D1Rect(renderer.workfolderCloseBtnRect, pt)) return true;
            if (RendererD2D::PtInD2D1Rect(renderer.workfolderFooterEscRect, pt)) return true;
            if (RendererD2D::PtInD2D1Rect(renderer.workfolderRunBtnRect, pt)) return true;
            if (RendererD2D::PtInD2D1Rect(renderer.workfolderTabBtnRect, pt)) return true;
            if (RendererD2D::PtInD2D1Rect(renderer.workfolderExplorerBtnRect, pt)) return true;
            for (const auto& it : renderer.workfolderItemRects) if (RendererD2D::PtInD2D1Rect(it.first, pt)) return true;
            return false;
        }
        if (renderer.sidebarContextMenuOpen && RendererD2D::PtInD2D1Rect(renderer.sidebarContextMenuRect, pt)) return true;
        if (!workspaceSidebarOpen && RendererD2D::PtInD2D1Rect(renderer.sidebarToggleBtnRect, pt)) return true;
        if (workspaceSidebarOpen && RendererD2D::PtInD2D1Rect(renderer.sidebarCollapseBtnRect, pt)) return true;
        if (workspaceSidebarOpen && pt.y >= RendererD2D::TITLEBAR_HEIGHT && pt.x <= 220.0f) return true;
        if (pinnedBlock && RendererD2D::PtInD2D1Rect(renderer.pinnedPanelRect, pt)) return true;
        if (settings.statusBarVisible && RendererD2D::PtInD2D1Rect(renderer.statusBarRect, pt)) return true;
        if (renderer.promptFolderRect.right > renderer.promptFolderRect.left && RendererD2D::PtInD2D1Rect(renderer.promptFolderRect, pt)) return true;
        if (renderer.promptGitBadgeRect.right > renderer.promptGitBadgeRect.left && RendererD2D::PtInD2D1Rect(renderer.promptGitBadgeRect, pt)) return true;
        return false;
    }

    void CloseTab(int index) {
        if (index < 0 || index >= static_cast<int>(tabs.size())) return;
        if (tabs[index]->isEditor && tabs[index]->isDirty) {
            std::wstring msg = L"Deseja salvar as alteracoes feitas em \"" + tabs[index]->editorFileName + L"\" antes de fechar?";
            int res = MessageBoxW(hwnd, msg.c_str(), L"Salvar arquivo", MB_YESNOCANCEL | MB_ICONQUESTION);
            if (res == IDCANCEL) return;
            if (res == IDYES) {
                tabs[index]->SaveEditorFile();
            }
        }
        for (auto& pane : tabs[index]->panes) {
            if (pane && conPty.isCommandRunning.load() && conPty.currentRunningPaneId == pane->id) {
                conPty.CancelCurrentCommand();
                break;
            }
        }
        tabs.erase(tabs.begin() + index);

        // Safely adjust or reset inline renaming state
        if (renamingTabIndex == index) {
            renamingTabIndex = -1;
            renamingText.clear();
        } else if (renamingTabIndex > index) {
            renamingTabIndex--;
        }

        if (tabs.empty()) {
            auto newTab = std::make_unique<TerminalTab>(ShellMode::ModePW, nextTabId++);
            newTab->useProjectTitle = true;
            newTab->GetActivePane().id = nextPaneId++;
            newTab->title = newTab->FormatProjectTabTitle(newTab->GetActivePane().cwd, ShellMode::ModePW);
            DetectLoadedScript(*newTab);
            tabs.push_back(std::move(newTab));
            activeTabIndex = 0;
        } else if (index < activeTabIndex) {
            activeTabIndex--;
        } else if (activeTabIndex >= static_cast<int>(tabs.size())) {
            activeTabIndex = static_cast<int>(tabs.size()) - 1;
        }
        Invalidate();
    }

    void DuplicateCurrentTab() {
        const auto& cur = GetActiveTab();
        auto tab = std::make_unique<TerminalTab>(cur.panes[0]->mode, nextTabId++);
        tab->GetActivePane().id = nextPaneId++;
        tab->GetActivePane().cwd = cur.panes[0]->cwd;
        tab->title = cur.title + L" (Cópia)";
        tab->customTitle = true;
        tab->customAccent = cur.customAccent;
        tab->loadedScriptPath = cur.loadedScriptPath;
        tabs.push_back(std::move(tab));
        activeTabIndex = static_cast<int>(tabs.size()) - 1;
        Invalidate();
    }

    void LaunchAdminSession() {
        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        ShellExecuteW(hwnd, L"runas", exePath, nullptr, nullptr, SW_SHOW);
    }

    void ExportTabOutput() {
        wchar_t fileBuf[MAX_PATH] = L"spetaterm_output.txt";
        OPENFILENAMEW ofn = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = L"Text File (*.txt)\0*.txt\0Markdown File (*.md)\0*.md\0All Files (*.*)\0*.*\0";
        ofn.lpstrFile = fileBuf;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

        if (GetSaveFileNameW(&ofn)) {
            std::wofstream out(fileBuf);
            if (out.is_open()) {
                auto& pane = GetActivePane();
                out << L"# Spet@term Output Export - " << pane.cwd << L"\n\n";
                for (const auto& blk : pane.blocks) {
                    out << L"PS " << blk->workingDirectory << L"> " << blk->commandText << L"\n";
                    out << blk->GetAllOutputText() << L"\n";
                }
                out.close();
                ShowToast(I18n::Get(StringId::Toast_Exported));
            }
        }
    }

    void LoadScriptDialog(int filterType) {
        wchar_t fileBuf[MAX_PATH] = L"";
        OPENFILENAMEW ofn = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwnd;

        if (filterType == 1) {
            ofn.lpstrFilter = L"PowerShell Scripts (*.ps1)\0*.ps1\0All Files (*.*)\0*.*\0";
        } else if (filterType == 2) {
            ofn.lpstrFilter = L"Python Scripts (*.py)\0*.py\0All Files (*.*)\0*.*\0";
        } else {
            ofn.lpstrFilter = L"All Supported Files (*.ps1;*.py;*.bat;*.cmd;*.png;*.jpg;*.bmp)\0*.ps1;*.py;*.bat;*.cmd;*.png;*.jpg;*.bmp\0All Files (*.*)\0*.*\0";
        }

        ofn.lpstrFile = fileBuf;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

        if (GetOpenFileNameW(&ofn)) {
            HandleDroppedPath(fileBuf);
        }
    }

    void HandleDroppedPath(const std::wstring& path) {
        WallpaperType wt = DetectWallpaperType(path);
        if (wt != WallpaperType::None && settingsOpen) {
            settings.customWallpapers.push_back(path);
            settings.wallpaperPath = path;
            renderer.wallpaperEngine.Load(path);
            settings.SaveToFile();
            Invalidate();
            return;
        }

        auto& tab = GetActiveTab();
        auto& pane = tab.GetActivePane();

        DWORD attr = GetFileAttributesW(path.c_str());
        if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)) {
            // Dropped a folder: cd into it
            pane.inputLine = L"cd \"" + path + L"\"";
            ExecuteInput();
            return;
        }

        std::wstring lower = ToLower(path);
        if (EndsWith(lower, L".png") || EndsWith(lower, L".jpg") || EndsWith(lower, L".jpeg") || EndsWith(lower, L".bmp") || EndsWith(lower, L".ico")) {
            // Render inline
            pane.inputLine = L"img \"" + path + L"\"";
            ExecuteInput();
            return;
        }

        if (EndsWith(lower, L".ps1")) {
            tab.loadedScriptPath = path;
            pane.inputLine = L"powershell -NoLogo -ExecutionPolicy Bypass -File \"" + path + L"\"";
            pane.cursorPos = pane.inputLine.size();
            Invalidate();
            return;
        }

        if (EndsWith(lower, L".py")) {
            tab.loadedScriptPath = path;
            pane.inputLine = L"python \"" + path + L"\"";
            pane.cursorPos = pane.inputLine.size();
            Invalidate();
            return;
        }

        if (EndsWith(lower, L".bat") || EndsWith(lower, L".cmd")) {
            tab.loadedScriptPath = path;
            pane.inputLine = L"\"" + path + L"\"";
            pane.cursorPos = pane.inputLine.size();
            Invalidate();
            return;
        }

        // Default: paste file path
        if (!pane.inputLine.empty() && !EndsWith(pane.inputLine, L" ")) {
            pane.inputLine += L" ";
        }
        pane.inputLine += L"\"" + path + L"\"";
        pane.cursorPos = pane.inputLine.size();
        Invalidate();
    }

    void RunQuickScript() {
        auto& tab = GetActiveTab();
        auto& pane = tab.GetActivePane();
        if (tab.loadedScriptPath.empty()) {
            DetectLoadedScript(tab);
        }
        if (!tab.loadedScriptPath.empty()) {
            std::wstring p = tab.loadedScriptPath;
            std::wstring lower = ToLower(p);
            if (EndsWith(lower, L".ps1")) {
                pane.inputLine = L"powershell -NoLogo -ExecutionPolicy Bypass -File \"" + p + L"\"";
            } else if (EndsWith(lower, L".py")) {
                pane.inputLine = L"python \"" + p + L"\"";
            } else {
                pane.inputLine = L"\"" + p + L"\"";
            }
            ExecuteInput();
        }
    }

    void ExecuteInput() {
        auto& tab = GetActiveTab();
        auto& pane = tab.GetActivePane();
        std::wstring raw = Trim(pane.inputLine);
        if (raw.empty()) return;

        pane.commandHistory.push_back(raw);
        pane.historyIndex = -1;
        pane.inputLine.clear();
        pane.cursorPos = 0;
        pane.autocompleteOpen = false;
        pane.autoScrollToBottom = true;

        if (Builtins::IsBuiltin(raw)) {
            auto block = std::make_unique<CommandBlock>();
            block->id = nextBlockId++;
            block->modeUsed = pane.mode;

            Builtins::ExecuteBuiltin(
                raw,
                pane.cwd,
                pane.prevCwd,
                *block,
                currentTheme,
                [this](const std::wstring& tName) { SetTheme(tName); },
                [&pane, &tab](ShellMode m) {
                    pane.mode = m;
                    tab.UpdateDefaultTitle();
                },
                [this](const std::wstring& path, D2D1_SIZE_F& size) -> ID2D1Bitmap* {
                    return renderer.LoadBitmapFromFile(path, size);
                },
                &pane.commandHistory,
                [this](bool vert) { if (vert) SplitVertical(); else SplitHorizontal(); },
                [this]() { ToggleQuakeMode(); },
                [this]() { CloseActivePane(); }
            );

            TimelineEntry te;
            te.command = block->commandText;
            te.cwd = block->workingDirectory;
            te.durationMs = block->executionTimeMs;
            te.exitCode = block->exitCode;
            te.timestamp = block->timestamp;
            timelineHistory.push_back(te);

            std::wstring low = ToLower(raw);
            if (low == L"clear" || low == L"cls") {
                pane.blocks.clear();
                pane.scrollOffsetY = 0.0f;
            } else {
                pane.blocks.push_back(std::move(block));
            }
            DetectLoadedScript(tab);
        } else {
            if (conPty.isCommandRunning.load()) {
                if (conPty.currentRunningPaneId == pane.id && pane.activeRunningBlock) {
                    conPty.SendInput(raw + L"\r\n");
                    StyledLine echoSl;
                    echoSl.rawText = raw;
                    echoSl.spans.push_back({ raw, currentTheme.accent });
                    {
                        std::lock_guard<std::mutex> lock(conPty.blockMutex);
                        pane.activeRunningBlock->outputLines.push_back(echoSl);
                    }
                    Invalidate();
                    return;
                }

                auto block = std::make_unique<CommandBlock>();
                block->id = nextBlockId++;
                block->modeUsed = pane.mode;
                block->commandText = raw;
                block->workingDirectory = pane.cwd;
                block->exitCode = 1;
                StyledLine sl;
                sl.rawText = L"[Aviso] Outro comando ja esta em execucao em outra aba ou painel. Aguarde sua conclusao ou pressione Ctrl+C para cancelar.";
                sl.spans.push_back({ sl.rawText, currentTheme.syntaxInvalidCommand });
                block->outputLines.push_back(sl);
                pane.blocks.push_back(std::move(block));
                Invalidate();
                return;
            }

            auto block = std::make_unique<CommandBlock>();
            block->id = nextBlockId++;
            block->modeUsed = pane.mode;
            block->startTick = GetTickCount();
            pane.activeRunningBlock = std::move(block);

            if (!conPty.ExecuteAsync(
                raw,
                pane.mode,
                pane.cwd,
                pane.activeRunningBlock.get(),
                currentTheme,
                pane.id
            )) {
                pane.activeRunningBlock = nullptr;
            }
        }

        Invalidate();
    }

    void OpenFileInEditor(const std::wstring& file, int line) {
        if (file.empty()) return;
        std::wstring target = file;
        wchar_t resolved[MAX_PATH];
        if (PathIsRelativeW(file.c_str())) {
            PathCombineW(resolved, GetActivePane().cwd.c_str(), file.c_str());
            target = resolved;
        }

        std::wstring arg = L"-g \"" + target + L":" + std::to_wstring(line > 0 ? line : 1) + L"\"";
        HINSTANCE res = ShellExecuteW(nullptr, L"open", L"code.cmd", arg.c_str(), nullptr, SW_SHOW);
        if (reinterpret_cast<INT_PTR>(res) <= 32) {
            ShellExecuteW(nullptr, L"open", target.c_str(), nullptr, nullptr, SW_SHOW);
        }
    }

    void SetTheme(const std::wstring& name) {
        currentTheme = ThemeManager::GetThemeByName(name);
        settings.themeName = currentTheme.name;
        settings.SaveToFile();
        ShowToast(I18n::Get(StringId::Toast_ThemeChanged));
        Invalidate();
    }

    void ToggleMode() {
        auto& tab = GetActiveTab();
        auto& pane = tab.GetActivePane();
        pane.mode = (pane.mode == ShellMode::ModePW) ? ShellMode::ModeCMD : ShellMode::ModePW;
        tab.UpdateDefaultTitle();
        Invalidate();
    }

    void SplitHorizontal() {
        auto& tab = GetActiveTab();
        if (tab.panes.size() >= 2) return;
        auto newPane = std::make_unique<TerminalPane>();
        newPane->id = nextPaneId++;
        newPane->cwd = tab.GetActivePane().cwd;
        newPane->mode = (tab.GetActivePane().mode == ShellMode::ModePW) ? ShellMode::ModeCMD : ShellMode::ModePW;
        tab.panes.push_back(std::move(newPane));
        tab.isSplitActive = true;
        tab.isSplitVertical = false;
        tab.activePaneIndex = static_cast<int>(tab.panes.size()) - 1;
        Invalidate();
    }

    void SplitVertical() {
        auto& tab = GetActiveTab();
        if (tab.panes.size() >= 2) return;
        auto newPane = std::make_unique<TerminalPane>();
        newPane->id = nextPaneId++;
        newPane->cwd = tab.GetActivePane().cwd;
        newPane->mode = (tab.GetActivePane().mode == ShellMode::ModePW) ? ShellMode::ModeCMD : ShellMode::ModePW;
        tab.panes.push_back(std::move(newPane));
        tab.isSplitActive = true;
        tab.isSplitVertical = true;
        tab.activePaneIndex = static_cast<int>(tab.panes.size()) - 1;
        Invalidate();
    }

    void CloseActivePane() {
        auto& tab = GetActiveTab();
        if (tab.panes.size() <= 1) {
            CloseTab(activeTabIndex);
            return;
        }
        tab.panes.erase(tab.panes.begin() + tab.activePaneIndex);
        if (tab.activePaneIndex >= static_cast<int>(tab.panes.size())) {
            tab.activePaneIndex = static_cast<int>(tab.panes.size()) - 1;
        }
        if (tab.panes.size() == 1) {
            tab.isSplitActive = false;
        }
        Invalidate();
    }

    void ToggleQuakeMode() {
        if (isQuakeVisible) {
            ShowWindow(hwnd, SW_HIDE);
            isQuakeVisible = false;
        } else {
            HMONITOR hMon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);
            MONITORINFO mi = { sizeof(mi) };
            GetMonitorInfoW(hMon, &mi);

            int monW = mi.rcWork.right - mi.rcWork.left;
            int monH = mi.rcWork.bottom - mi.rcWork.top;
            int termH = static_cast<int>(monH * 0.58f);

            SetWindowPos(hwnd, HWND_TOPMOST, mi.rcWork.left, mi.rcWork.top, monW, termH, SWP_SHOWWINDOW);
            SetForegroundWindow(hwnd);
            SetFocus(hwnd);
            isQuakeVisible = true;
        }
    }

    void Invalidate() {
        if (hwnd) {
            InvalidateRect(hwnd, nullptr, FALSE);
        }
    }

    void Render() {
        if (!hwnd || IsIconic(hwnd)) return;
        if (!renderer.pRenderTarget) {
            if (!renderer.CreateDeviceResources(hwnd)) return;
        }

        RECT rc;
        GetClientRect(hwnd, &rc);
        float width = static_cast<float>(rc.right - rc.left);
        float height = static_cast<float>(rc.bottom - rc.top);

        renderer.BeginDraw();
        renderer.RenderBackground(currentTheme, width, height, settings.wallpaperDim);

        // Suppress hero empty state if any modal is active (prevents visual bleed-through)
        renderer.suppressEmptyState = (workfolderOpen || settingsOpen || timelineOpen || paletteOpen || helpModalOpen || multiLinePasteModalOpen || onboarding.open);

        // 1. Unified Custom Frameless Titlebar (36px)
        auto& activeTab = GetActiveTab();
        GitStatusInfo gitInfo = GitEngine::InspectRepository(activeTab.GetActivePane().cwd);
        detectedTasks = DetectProjectTasks(activeTab.GetActivePane().cwd);
        bool isMax = (IsZoomed(hwnd) != 0);

        renderer.RenderUnifiedTitleBar(
            currentTheme,
            width,
            tabs,
            activeTabIndex,
            renamingTabIndex,
            renamingText,
            isAdmin,
            mousePos,
            isMax,
            snippetsPopoverOpen,
            detectedTasks,
            workspaceSidebarOpen
        );

        bool isSidebarVisible = workspaceSidebarOpen;
        float sidebarW = isSidebarVisible ? 220.0f : 0.0f;
        float bodyX = isSidebarVisible ? sidebarW : 34.0f;
        float bodyY = RendererD2D::TITLEBAR_HEIGHT;
        float bodyH = settings.statusBarVisible ? (height - bodyY - 24.0f) : (height - bodyY);
        float bodyW = (snippetsPopoverOpen ? (width - 280.0f) : width) - bodyX;

        if (isSidebarVisible) {
            renderer.RenderWorkspaceSidebar(
                currentTheme,
                0.0f, bodyY, sidebarW, bodyH,
                activeTab.GetActivePane().cwd,
                gitInfo,
                mousePos,
                settings.language
            );
        } else {
            renderer.RenderSidebarClosedToggle(currentTheme, mousePos);
        }

        if (activeTab.isEditor) {
            renderer.RenderEditorBody(
                currentTheme,
                activeTab,
                bodyX, bodyY, bodyW, bodyH,
                mousePos,
                true,
                cursorBlinkState,
                settings.language
            );
        } else {
            // 2. Render Active Tab Panes & Pinned Reference Panel
            bool isCtrlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            float paneW = bodyW;
            if (pinnedBlock) {
                paneW = bodyW * 0.60f;
                renderer.RenderPinnedOutputPanel(currentTheme, *pinnedBlock, bodyX + paneW, bodyY, bodyW * 0.40f, bodyH, pinnedScrollY, mousePos);
            }

            if (!activeTab.isSplitActive || activeTab.panes.size() == 1) {
                RenderPaneBody(activeTab.GetActivePane(), bodyX, bodyY, paneW, bodyH, isCtrlDown, gitInfo);
            } else {
                // 2 Panes
                if (activeTab.isSplitVertical) {
                    float splitH = bodyH / 2.0f;
                    RenderPaneBody(*activeTab.panes[0], bodyX, bodyY, paneW, splitH, isCtrlDown, gitInfo);
                    renderer.DrawSplitDivider(currentTheme, D2D1::RectF(bodyX, bodyY, bodyX + paneW, bodyY + splitH), false, activeTab.activePaneIndex == 0);
                    RenderPaneBody(*activeTab.panes[1], bodyX, bodyY + splitH, paneW, splitH, isCtrlDown, gitInfo);
                } else {
                    float splitW = paneW / 2.0f;
                    RenderPaneBody(*activeTab.panes[0], bodyX, bodyY, splitW, bodyH, isCtrlDown, gitInfo);
                    renderer.DrawSplitDivider(currentTheme, D2D1::RectF(bodyX, bodyY, bodyX + splitW, bodyY + bodyH), true, activeTab.activePaneIndex == 0);
                    RenderPaneBody(*activeTab.panes[1], bodyX + splitW, bodyY, splitW, bodyH, isCtrlDown, gitInfo);
                }
            }
        }

        // Draw mouse text selection highlight overlay
        if (hasSelection && !activeTab.isEditor) {
            float sLeft = (std::min)(selectionStartPt.x, selectionEndPt.x);
            float sRight = (std::max)(selectionStartPt.x, selectionEndPt.x);
            float sTop = (std::min)(selectionStartPt.y, selectionEndPt.y);
            float sBottom = (std::max)(selectionStartPt.y, selectionEndPt.y);
            if (sRight - sLeft > 2.0f && sBottom - sTop > 2.0f) {
                D2D1_RECT_F selRect = D2D1::RectF(sLeft, sTop, sRight, sBottom);
                D2D1_COLOR_F selFill = currentTheme.accent;
                selFill.a = 0.30f;
                renderer.pSolidBrush->SetColor(selFill);
                renderer.pRenderTarget->FillRectangle(selRect, renderer.pSolidBrush);
                renderer.pSolidBrush->SetColor(currentTheme.accent);
                renderer.pRenderTarget->DrawRectangle(selRect, renderer.pSolidBrush, 1.0f);
            }
        }

        // Modular Telemetry Status Bar (24px at bottom)
        if (settings.statusBarVisible) {
            renderer.RenderModularStatusBar(currentTheme, settings, width, height, gitInfo, telemetry, mousePos);
        }

        // 3. Render Docked Snippets Sidebar if open
        if (snippetsPopoverOpen) {
            renderer.RenderSnippetsSidebar(currentTheme, width, height, settings.snippets, mousePos);
        }

        // 4. Render Search Bar (Ctrl+F)
        if (searchOpen) {
            renderer.RenderSearchBar(currentTheme, width, searchQuery, searchMatchIndex, totalSearchMatches, mousePos, snippetsPopoverOpen);
        }

        // 5. Render Popovers & Menus
        if (fileMenuOpen) {
            renderer.RenderFileMenu(currentTheme, mousePos, fileMenuX);
        } else if (profileMenuOpen) {
            renderer.RenderProfileMenu(currentTheme, mousePos, profileMenuX);
        } else if (gitPopoverOpen) {
            renderer.RenderGitPopover(currentTheme, gitInfo, mousePos);
        }

        // 6. Render Command Palette if active
        if (paletteOpen) {
            std::vector<std::pair<std::wstring, std::wstring>> filtered;
            std::wstring qLower = ToLower(paletteQuery);
            for (const auto& act : paletteActions) {
                if (qLower.empty() || ToLower(act.title).find(qLower) != std::wstring::npos || ToLower(act.desc).find(qLower) != std::wstring::npos) {
                    filtered.push_back({ act.title, act.desc });
                }
            }
            if (paletteSelectedIndex >= static_cast<int>(filtered.size())) {
                paletteSelectedIndex = (std::max)(0, static_cast<int>(filtered.size()) - 1);
            }
            renderer.RenderCommandPalette(currentTheme, width, height, paletteQuery, filtered, paletteSelectedIndex, paletteScrollOffset, mousePos, settings.language);
        }

        // 6b. Render Timeline History Modal (Ctrl+R)
        if (timelineOpen) {
            renderer.RenderTimelineHistoryModal(currentTheme, width, height, timelineHistory, timelineSelectedIndex, timelineFilterSuccessOnly, mousePos, settings.language);
        }

        // 7. Render Workfolder Modal (Telescope)
        if (workfolderOpen) {
            auto filtered = WorkfolderScanner::FuzzyFilter(workfolderItems, workfolderFilter);
            renderer.RenderWorkfolderPanel(currentTheme, width, height, activeTab.GetActivePane().cwd, filtered, workfolderSelectedIndex, mousePos, workfolderFilter, settings.language);
        }

        // 8. Render Settings Modal
        if (settingsOpen) {
            renderer.RenderSettingsPanel(currentTheme, settings, width, height, mousePos);
        }

        // 8a. Render Onboarding Walkthrough
        if (onboarding.open) {
            renderer.RenderOnboarding(currentTheme, settings, onboarding, width, height, mousePos);
        }

        // 8e. Render Help Modal
        if (helpModalOpen) {
            renderer.RenderHelpModal(currentTheme, width, height, mousePos, settings.language);
        }

        // 8f. Render Multi-line Paste Modal
        if (multiLinePasteModalOpen) {
            renderer.RenderMultiLinePasteModal(currentTheme, width, height, pendingPasteText, mousePos, settings.language);
        }

        // 8b. Render Breadcrumb Sibling Popover (#2)
        if (breadcrumbPopoverOpen) {
            renderer.RenderBreadcrumbSiblingPopover(currentTheme, breadcrumbTargetDir, breadcrumbSiblings, breadcrumbAnchorRect, mousePos);
        }

        // 8c. Render Task Runner Contextual Dropdown Menu (#3)
        if (taskMenuOpen && !detectedTasks.empty()) {
            renderer.RenderTaskMenu(currentTheme, detectedTasks, renderer.taskRunnerBtnRect, mousePos);
        }

        // 8d. Render Dark Direct2D Sidebar Context Menu
        if (renderer.sidebarContextMenuOpen) {
            renderer.RenderSidebarContextMenu(currentTheme, mousePos);
        }

        // 9. Tooltips for Titlebar and Sidebar Hover
        if (mousePos.x > 0 && mousePos.y >= 0 && !fileMenuOpen && !profileMenuOpen && !workfolderOpen && !settingsOpen && !timelineOpen && !renderer.sidebarContextMenuOpen) {
            std::wstring tip;
            if (mousePos.y < RendererD2D::TITLEBAR_HEIGHT) {
                if (RendererD2D::PtInD2D1Rect(renderer.fileBtnRect, mousePos)) tip = I18n::Get(StringId::Tooltip_Menu);
                else if (RendererD2D::PtInD2D1Rect(renderer.workfolderBtnRect, mousePos)) tip = (settings.language == "pt-BR") ? L"Workspace Explorer (Ctrl+B)" : L"Workspace Explorer (Ctrl+B)";
                else if (RendererD2D::PtInD2D1Rect(renderer.newTabBtnRect, mousePos)) tip = I18n::Get(StringId::Tooltip_NewTab);
                else if (RendererD2D::PtInD2D1Rect(renderer.dropdownBtnRect, mousePos)) tip = I18n::Get(StringId::Tooltip_Profiles);
                else if (RendererD2D::PtInD2D1Rect(renderer.snippetsBtnRect, mousePos)) tip = I18n::Get(StringId::Tooltip_Snippets);
                else if (RendererD2D::PtInD2D1Rect(renderer.searchBtnRect, mousePos)) tip = I18n::Get(StringId::Tooltip_Search);
                else if (RendererD2D::PtInD2D1Rect(renderer.settingsBtnRect, mousePos)) tip = I18n::Get(StringId::Tooltip_Settings);
            } else {
                if (!workspaceSidebarOpen && RendererD2D::PtInD2D1Rect(renderer.sidebarToggleBtnRect, mousePos)) {
                    tip = (settings.language == "pt-BR") ? L"Abrir Workspace (Ctrl+B)" : L"Open Workspace (Ctrl+B)";
                } else if (workspaceSidebarOpen && RendererD2D::PtInD2D1Rect(renderer.sidebarCollapseBtnRect, mousePos)) {
                    tip = (settings.language == "pt-BR") ? L"Recolher Workspace (Ctrl+B)" : L"Collapse Workspace (Ctrl+B)";
                }
            }

            if (!tip.empty()) {
                renderer.RenderTooltip(tip, static_cast<float>(mousePos.x), static_cast<float>(mousePos.y), width, height);
            }
        }

        // 10. Toast Notification
        if (toastAlpha > 0.0f) {
            renderer.RenderToast(toastMessage, width, height, toastAlpha);
        }

        // 11. 1px Window Border when not maximized
        if (!isMax) {
            renderer.DrawWindowBorder(width, height);
        }

        // 12. Edge Pulse Peripheral Glow
        if (edgePulseActive) {
            DWORD elapsed = GetTickCount() - edgePulseStartTick;
            if (elapsed < 1800) {
                float progress = 1.0f - (static_cast<float>(elapsed) / 1800.0f);
                renderer.RenderEdgePulse(width, height, progress, edgePulseSuccess);
            } else {
                edgePulseActive = false;
            }
        }

        // 13. Post-Processing Overlays (CRT Scanlines, Phosphor Bloom, Vignette, Warmth Filter)
        renderer.RenderPostProcessingOverlays(currentTheme, settings, width, height);

        if (!settings.systemCursor && mousePos.x >= 0 && mousePos.y >= 0) {
            int cursorMode = 0;
            if (IsOverInteractive(mousePos)) cursorMode = 1;
            else if (!(settingsOpen || workfolderOpen || timelineOpen || paletteOpen) && mousePos.y >= RendererD2D::TITLEBAR_HEIGHT) cursorMode = 2;
            renderer.RenderCustomCursor(currentTheme, mousePos, cursorMode);
        }

        HRESULT hr = renderer.EndDraw();
        if (hr == D2DERR_RECREATE_TARGET) {
            for (auto& tab : tabs) {
                for (auto& p : tab->panes) {
                    if (p) {
                        SafeRelease(p->previewThumb);
                        if (p->activeRunningBlock) {
                            SafeRelease(p->activeRunningBlock->inlineImage);
                        }
                        for (auto& blk : p->blocks) {
                            if (blk) SafeRelease(blk->inlineImage);
                        }
                    }
                }
            }
            renderer.DiscardDeviceResources();
            Invalidate();
        }
    }

    void RenderPaneBody(TerminalPane& pane, float px, float py, float pw, float ph, bool isCtrlDown, const GitStatusInfo& gitInfo) {
        // Real-time syntax analysis
        pane.syntaxAnalysis = syntaxEngine.AnalyzeInput(pane.inputLine, pane.cwd, currentTheme);

        // Classic Fluid Mode (Windows Terminal layout) vs Command Blocks Mode
        if (settings.classicFlowMode) {
            StyledLine activeCurLine;
            {
                std::lock_guard<std::mutex> lock(conPty.blockMutex);
                activeCurLine = conPty.activeParserCurrentLine;
            }
            renderer.RenderClassicStream(
                pane,
                currentTheme,
                px, py, pw, ph,
                cursorBlinkState,
                settings.cursorStyle,
                mousePos,
                isCtrlDown,
                searchQuery,
                searchMatchIndex,
                totalSearchMatches,
                gitInfo,
                conPty.blockMutex,
                settings,
                detectedTasks,
                activeCurLine
            );
        } else {
            // Command Blocks Mode
            float padding = 14.0f;
            float blockW = pw - padding * 2.0f;
            float inputLineH = 28.0f;
            float inputY = py + ph - inputLineH - 12.0f;

            float curY = py + padding - pane.scrollOffsetY;

            D2D1_RECT_F blocksClip = D2D1::RectF(px, py + 2.0f, px + pw, inputY - 8.0f);
            renderer.pRenderTarget->PushAxisAlignedClip(blocksClip, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

            float blockColsW = (std::max)(100.0f, blockW - 24.0f);
            float cwBlk = (renderer.charWidth > 1.0f) ? renderer.charWidth : 8.0f;
            int maxBlkCols = (std::max)(20, static_cast<int>(blockColsW / cwBlk));

            for (auto& blk : pane.blocks) {
                float startY = curY;
                std::vector<StyledLine> wrappedLines;
                wrappedLines.reserve(blk->outputLines.size());
                for (const auto& l : blk->outputLines) {
                    auto wLines = WrapStyledLine(l, maxBlkCols);
                    for (auto& wl : wLines) {
                        wrappedLines.push_back(std::move(wl));
                    }
                }
                float contentH = wrappedLines.size() * renderer.lineHeight + 40.0f;
                blk->blockRect = D2D1::RectF(px + padding, startY, px + padding + blockW, startY + contentH);

                renderer.pSolidBrush->SetColor(currentTheme.blockBg);
                renderer.pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(blk->blockRect, 6.0f, 6.0f), renderer.pSolidBrush);
                renderer.pSolidBrush->SetColor(currentTheme.blockBorder);
                renderer.pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(blk->blockRect, 6.0f, 6.0f), renderer.pSolidBrush, 1.0f);

                renderer.DrawText(blk->commandText, px + padding + 10.0f, curY + 6.0f, currentTheme.textPrimary, renderer.pTextFormatBold);
                curY += 28.0f;

                for (const auto& l : wrappedLines) {
                    renderer.DrawText(l.rawText, px + padding + 12.0f, curY, currentTheme.textPrimary, renderer.pTextFormat);
                    curY += renderer.lineHeight;
                }
                curY += 12.0f;
            }

            renderer.pRenderTarget->PopAxisAlignedClip();

            // Bottom Prompt
            GitStatusInfo gitInfo = GitEngine::InspectRepository(pane.cwd);
            renderer.DrawText(L"PS " + pane.cwd + L"> " + pane.inputLine, px + 10.0f, inputY, currentTheme.promptPw, renderer.pTextFormatBold);
        }

        // Right-side scrollbar with error markers
        float totalH = (settings.classicFlowMode && pane.totalContentHeight > 0.0f) ? pane.totalContentHeight : (pane.blocks.size() * 50.0f + 100.0f);
        if (!settings.classicFlowMode) {
            for (const auto& blk : pane.blocks) totalH += blk->outputLines.size() * renderer.lineHeight;
        }
        renderer.RenderScrollbarWithMarkers(currentTheme, px, py, pw, ph, pane.scrollOffsetY, totalH, pane.blocks);
    }

    void OnKeyDown(WPARAM vk, LPARAM lp) {
        bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        bool alt = (GetKeyState(VK_MENU) & 0x8000) != 0;

        // F1: Toggle Help Modal
        if (vk == VK_F1) {
            helpModalOpen = !helpModalOpen;
            Invalidate();
            return;
        }

        // Multi-line Paste Security Modal keys
        if (multiLinePasteModalOpen) {
            if (vk == VK_RETURN) {
                ExecuteMultiLinePaste();
                return;
            }
            if (vk == VK_ESCAPE) {
                multiLinePasteModalOpen = false;
                pendingPasteText.clear();
                Invalidate();
                return;
            }
            return;
        }

        // Help Modal keys
        if (helpModalOpen) {
            if (vk == VK_ESCAPE) {
                helpModalOpen = false;
                Invalidate();
                return;
            }
            return;
        }

        // Clear text selection on Escape
        if (vk == VK_ESCAPE && hasSelection) {
            hasSelection = false;
            selectedText.clear();
            Invalidate();
            return;
        }

        // Workspace Sidebar toggle Ctrl+B
        if (ctrl && (vk == 'B' || vk == 'b')) {
            workspaceSidebarOpen = !workspaceSidebarOpen;
            Invalidate();
            return;
        }

        // Editor Save Ctrl+S
        if (ctrl && (vk == 'S' || vk == 's')) {
            if (GetActiveTab().isEditor) {
                GetActiveTab().SaveEditorFile();
                Invalidate();
                return;
            }
        }

        // Editor Active Navigation & Editing
        if (GetActiveTab().isEditor) {
            auto& tab = GetActiveTab();
            if (vk == VK_UP) {
                if (tab.editorCursorLine > 0) tab.editorCursorLine--;
                Invalidate();
                return;
            } else if (vk == VK_DOWN) {
                tab.editorCursorLine++;
                Invalidate();
                return;
            } else if (vk == VK_LEFT) {
                if (tab.editorCursorCol > 0) tab.editorCursorCol--;
                Invalidate();
                return;
            } else if (vk == VK_RIGHT) {
                tab.editorCursorCol++;
                Invalidate();
                return;
            } else if (vk == VK_BACK) {
                if (!tab.editorContent.empty()) {
                    tab.editorContent.pop_back();
                    tab.isDirty = true;
                    Invalidate();
                }
                return;
            } else if (vk == VK_RETURN) {
                tab.editorContent += L"\n";
                tab.editorCursorLine++;
                tab.editorCursorCol = 0;
                tab.isDirty = true;
                Invalidate();
                return;
            }
        }

        // 1. Tab Renaming inline mode
        if (renamingTabIndex != -1) {
            if (vk == VK_RETURN) {
                if (!renamingText.empty()) {
                    tabs[renamingTabIndex]->title = renamingText;
                    tabs[renamingTabIndex]->customTitle = true;
                }
                renamingTabIndex = -1;
                Invalidate();
                return;
            } else if (vk == VK_ESCAPE) {
                renamingTabIndex = -1;
                Invalidate();
                return;
            } else if (vk == VK_BACK) {
                if (!renamingText.empty()) renamingText.pop_back();
                Invalidate();
                return;
            }
            return;
        }

        // 1b. Onboarding Modal Keyboard Navigation
        if (onboarding.open) {
            if (vk == VK_RETURN) {
                if (onboarding.step == 6) {
                    settings.onboardingCompleted = true;
                    settings.SaveToFile();
                    onboarding.Close();
                } else {
                    onboarding.Next();
                }
                Invalidate();
                return;
            }
            if (vk == VK_ESCAPE) {
                settings.onboardingCompleted = true;
                settings.SaveToFile();
                onboarding.Close();
                Invalidate();
                return;
            }
            if (vk == VK_RIGHT) {
                onboarding.Next();
                Invalidate();
                return;
            }
            if (vk == VK_LEFT) {
                onboarding.Prev();
                Invalidate();
                return;
            }
            return;
        }

        // 2. Settings Modal Esc to close
        if (settingsOpen) {
            if (vk == VK_ESCAPE) {
                settingsOpen = false;
                settings.SaveToFile();
                Invalidate();
                return;
            }
            return;
        }

        // 3. Search Bar active navigation
        if (searchOpen) {
            if (vk == VK_ESCAPE) {
                searchOpen = false;
                searchQuery.clear();
                Invalidate();
                return;
            } else if (vk == VK_RETURN) {
                if (shift) {
                    if (searchMatchIndex > 1) searchMatchIndex--;
                } else {
                    if (searchMatchIndex < totalSearchMatches) searchMatchIndex++;
                    else searchMatchIndex = 1;
                }
                Invalidate();
                return;
            } else if (vk == VK_BACK) {
                if (!searchQuery.empty()) {
                    searchQuery.pop_back();
                    searchMatchIndex = 1;
                    Invalidate();
                }
                return;
            }
        }

        if (vk == VK_ESCAPE && renderer.sidebarContextMenuOpen) {
            renderer.sidebarContextMenuOpen = false;
            Invalidate();
            return;
        }

        // 4. Command Palette
        if (paletteOpen) {
            if (vk == VK_ESCAPE) {
                paletteOpen = false;
                currentTheme = prePaletteTheme;
                Invalidate();
                return;
            } else if (vk == VK_UP) {
                if (paletteSelectedIndex > 0) {
                    paletteSelectedIndex--;
                    if (paletteSelectedIndex < paletteScrollOffset) {
                        paletteScrollOffset = paletteSelectedIndex;
                    }
                }
                Invalidate();
                return;
            } else if (vk == VK_DOWN) {
                int totalFiltered = 0;
                std::wstring qLower = ToLower(paletteQuery);
                for (const auto& act : paletteActions) {
                    if (qLower.empty() || ToLower(act.title).find(qLower) != std::wstring::npos || ToLower(act.desc).find(qLower) != std::wstring::npos) {
                        totalFiltered++;
                    }
                }
                if (paletteSelectedIndex + 1 < totalFiltered) {
                    paletteSelectedIndex++;
                    if (paletteSelectedIndex >= paletteScrollOffset + 7) {
                        paletteScrollOffset = paletteSelectedIndex - 7 + 1;
                    }
                }
                Invalidate();
                return;
            } else if (vk == VK_RETURN) {
                ExecutePaletteSelection();
                paletteOpen = false;
                Invalidate();
                return;
            } else if (vk == VK_BACK) {
                if (!paletteQuery.empty()) {
                    paletteQuery.pop_back();
                    paletteSelectedIndex = 0;
                    paletteScrollOffset = 0;
                    Invalidate();
                }
                return;
            }
            return;
        }

        auto& tab = GetActiveTab();
        auto& pane = tab.GetActivePane();

        // 4b. Timeline History Modal (Ctrl+R)
        if (timelineOpen) {
            if (vk == VK_ESCAPE) {
                timelineOpen = false;
                Invalidate();
                return;
            } else if (vk == VK_TAB) {
                timelineFilterSuccessOnly = !timelineFilterSuccessOnly;
                timelineSelectedIndex = 0;
                Invalidate();
                return;
            } else if (vk == VK_UP) {
                if (timelineSelectedIndex > 0) timelineSelectedIndex--;
                Invalidate();
                return;
            } else if (vk == VK_DOWN) {
                timelineSelectedIndex++;
                Invalidate();
                return;
            } else if (vk == VK_RETURN) {
                auto filtered = FilterTimelineHistory(timelineHistory, timelineFilterSuccessOnly);
                if (timelineSelectedIndex < filtered.size()) {
                    pane.inputLine = filtered[timelineSelectedIndex].command;
                    pane.cursorPos = pane.inputLine.size();
                    timelineOpen = false;
                    ExecuteInput();
                }
                Invalidate();
                return;
            }
            return;
        }

        // Popovers & Menus Esc to close
        if (fileMenuOpen || profileMenuOpen || gitPopoverOpen || workfolderOpen || settingsOpen || timelineOpen) {
            if (vk == VK_ESCAPE) {
                fileMenuOpen = false;
                profileMenuOpen = false;
                gitPopoverOpen = false;
                workfolderOpen = false;
                settingsOpen = false;
                timelineOpen = false;
                Invalidate();
                return;
            }
        }
        if (snippetsPopoverOpen && vk == VK_ESCAPE && pane.inputLine.empty()) {
            snippetsPopoverOpen = false;
            Invalidate();
            return;
        }

        // Workfolder Keyboard Navigation (Telescope)
        if (workfolderOpen) {
            auto filtered = WorkfolderScanner::FuzzyFilter(workfolderItems, workfolderFilter);
            if (vk == VK_UP) {
                if (workfolderSelectedIndex > 0) {
                    workfolderSelectedIndex--;
                    Invalidate();
                }
                return;
            } else if (vk == VK_DOWN) {
                if (workfolderSelectedIndex + 1 < filtered.size()) {
                    workfolderSelectedIndex++;
                    Invalidate();
                }
                return;
            } else if (vk == VK_RETURN) {
                if (workfolderSelectedIndex < filtered.size()) {
                    const auto& item = filtered[workfolderSelectedIndex];
                    if (item.isDirectory) {
                        GetActivePane().cwd = item.fullPath;
                        workfolderSelectedIndex = 0;
                        workfolderFilter.clear();
                        workfolderItems = ScanWorkfolder(item.fullPath);
                        Invalidate();
                    } else {
                        GetActivePane().inputLine = L"\"" + item.fullPath + L"\"";
                        workfolderOpen = false;
                        workfolderFilter.clear();
                        ExecuteInput();
                    }
                }
                return;
            } else if (vk == VK_TAB) {
                if (workfolderSelectedIndex < filtered.size()) {
                    pane.inputLine += L" \"" + filtered[workfolderSelectedIndex].fullPath + L"\"";
                    pane.cursorPos = pane.inputLine.size();
                    workfolderOpen = false;
                    workfolderFilter.clear();
                    Invalidate();
                }
                return;
            } else if (ctrl && vk == 'E') {
                if (workfolderSelectedIndex < filtered.size()) {
                    ShellExecuteW(nullptr, L"open", L"explorer.exe", (L"/select,\"" + filtered[workfolderSelectedIndex].fullPath + L"\"").c_str(), nullptr, SW_SHOWNORMAL);
                } else {
                    ShellExecuteW(nullptr, L"open", pane.cwd.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                }
                return;
            } else if (vk == VK_BACK) {
                if (!workfolderFilter.empty()) {
                    workfolderFilter.pop_back();
                    workfolderSelectedIndex = 0;
                    Invalidate();
                } else {
                    std::wstring cwd = GetActivePane().cwd;
                    size_t lastSlash = cwd.find_last_of(L"\\/");
                    if (lastSlash != std::wstring::npos && lastSlash > 2) {
                        GetActivePane().cwd = cwd.substr(0, lastSlash);
                        workfolderSelectedIndex = 0;
                        workfolderItems = ScanWorkfolder(GetActivePane().cwd);
                        Invalidate();
                    }
                }
                return;
            }
            return;
        }

        // 5. Global Shortcuts
        if (ctrl && shift && vk == 'P') {
            paletteOpen = true;
            prePaletteTheme = currentTheme;
            paletteQuery.clear();
            paletteSelectedIndex = 0;
            Invalidate();
            return;
        }

        // Alt+P -> Toggle Prompt Position (Bottom-Docked vs Top-Flow)
        if (alt && !ctrl && !shift && vk == 'P') {
            settings.promptPosition = (settings.promptPosition == "bottom") ? "top" : "bottom";
            settings.SaveToFile();
            ShowToast(L"Prompt: " + Utf8ToWide(settings.promptPosition));
            Invalidate();
            return;
        }

        // Alt+B -> Contextual Task Runner (Run primary detected task)
        if (alt && !ctrl && !shift && vk == 'B') {
            auto tasks = DetectProjectTasks(pane.cwd);
            if (!tasks.empty()) {
                pane.inputLine = tasks[0].command;
                pane.cursorPos = pane.inputLine.size();
                ExecuteInput();
            }
            return;
        }

        // Alt+M -> Toggle Status Bar
        if (alt && !ctrl && !shift && vk == 'M') {
            settings.statusBarVisible = !settings.statusBarVisible;
            settings.SaveToFile();
            Invalidate();
            return;
        }

        // Alt+P -> Toggle Prompt Position (Bottom / Top)
        if (alt && !ctrl && !shift && vk == 'P') {
            settings.promptPosition = (settings.promptPosition == "bottom") ? "top" : "bottom";
            settings.SaveToFile();
            ShowToast(L"Prompt: " + Utf8ToWide(settings.promptPosition));
            Invalidate();
            return;
        }

        // Alt+Shift+P -> Pin/Unpin Last Command Block
        if (alt && shift && vk == 'P') {
            if (pinnedBlock) {
                pinnedBlockStorage.reset();
                pinnedBlock = nullptr;
            } else if (!pane.blocks.empty()) {
                pinnedBlockStorage = std::make_unique<CommandBlock>(*pane.blocks.back());
                pinnedBlock = pinnedBlockStorage.get();
                pinnedScrollY = 0.0f;
            }
            Invalidate();
            return;
        }

        // Ctrl+R -> Timeline History
        if (ctrl && !shift && !alt && vk == 'R') {
            timelineOpen = !timelineOpen;
            timelineSelectedIndex = 0;
            Invalidate();
            return;
        }

        if (ctrl && shift && vk == '1') {
            CreateNewTab(ShellMode::ModePW);
            return;
        }
        if (ctrl && shift && vk == '2') {
            CreateNewTab(ShellMode::ModeCMD);
            return;
        }
        if (alt && shift && vk == 'D') {
            SplitHorizontal();
            return;
        }
        if (alt && shift && vk == 'V') {
            SplitVertical();
            return;
        }
        if (ctrl && shift && vk == 'D') {
            SplitHorizontal();
            return;
        }
        if (ctrl && shift && vk == 'E') {
            SplitVertical();
            return;
        }
        if (ctrl && !shift && !alt && vk == 'B') {
            workfolderOpen = !workfolderOpen;
            if (workfolderOpen) {
                workfolderFilter.clear();
                workfolderSelectedIndex = 0;
                workfolderItems = ScanWorkfolder(GetActivePane().cwd);
            }
            Invalidate();
            return;
        }

        if (ctrl && vk == 'T') {
            if (shift) {
                CreateNewTab(ShellMode::ModeCMD);
            } else {
                CreateNewTab(ShellMode::ModePW);
            }
            return;
        }

        auto CommitRenameIfActive = [&]() {
            if (renamingTabIndex != -1) {
                if (renamingTabIndex >= 0 && renamingTabIndex < static_cast<int>(tabs.size()) && !renamingText.empty()) {
                    tabs[renamingTabIndex]->title = renamingText;
                    tabs[renamingTabIndex]->customTitle = true;
                }
                renamingTabIndex = -1;
            }
        };

        if (ctrl && (vk == 'W' || vk == VK_F4)) {
            CloseTab(activeTabIndex);
            return;
        }

        if (ctrl && vk == 'F') {
            searchOpen = !searchOpen;
            if (searchOpen) {
                searchQuery.clear();
                searchMatchIndex = 1;
            }
            Invalidate();
            return;
        }

        if (ctrl && vk == VK_OEM_COMMA) { // Ctrl+, -> Settings
            settingsOpen = !settingsOpen;
            Invalidate();
            return;
        }

        if (alt && !ctrl && !shift && vk == 'S') { // Alt+S -> Snippets
            snippetsPopoverOpen = !snippetsPopoverOpen;
            fileMenuOpen = false;
            gitPopoverOpen = false;
            Invalidate();
            return;
        }

        // Tab switching shortcuts: Ctrl+Tab (next), Ctrl+Shift+Tab (prev)
        if (ctrl && vk == VK_TAB) {
            CommitRenameIfActive();
            if (tabs.size() > 1) {
                if (shift) {
                    activeTabIndex = (activeTabIndex - 1 + static_cast<int>(tabs.size())) % static_cast<int>(tabs.size());
                } else {
                    activeTabIndex = (activeTabIndex + 1) % static_cast<int>(tabs.size());
                }
                Invalidate();
            }
            return;
        }

        // Ctrl+PageDown / Ctrl+PageUp for tab navigation
        if (ctrl && vk == VK_NEXT) {
            CommitRenameIfActive();
            if (tabs.size() > 1) {
                activeTabIndex = (activeTabIndex + 1) % static_cast<int>(tabs.size());
                Invalidate();
            }
            return;
        }
        if (ctrl && vk == VK_PRIOR) {
            CommitRenameIfActive();
            if (tabs.size() > 1) {
                activeTabIndex = (activeTabIndex - 1 + static_cast<int>(tabs.size())) % static_cast<int>(tabs.size());
                Invalidate();
            }
            return;
        }

        // Ctrl+1 .. Ctrl+8 switches to tab 1..8, Ctrl+9 switches to last tab
        if (ctrl && !shift && !alt && vk >= '1' && vk <= '9') {
            CommitRenameIfActive();
            int targetIdx = (vk == '9') ? (static_cast<int>(tabs.size()) - 1) : (vk - '1');
            if (targetIdx >= 0 && targetIdx < static_cast<int>(tabs.size())) {
                activeTabIndex = targetIdx;
                Invalidate();
                return;
            }
        }

        // Ctrl+M to toggle shell mode (PowerShell <-> CMD)
        if (ctrl && !shift && !alt && vk == 'M') {
            ToggleMode();
            return;
        }

        if (alt && shift && vk == 'D') {
            SplitHorizontal();
            return;
        }

        if (alt && shift && vk == 'V') {
            SplitVertical();
            return;
        }

        if (alt && shift && vk == 'W') {
            CloseActivePane();
            return;
        }

        if (ctrl && vk == 'C') {
            if (hasSelection && !selectedText.empty()) {
                CopyTextToClipboard(hwnd, selectedText);
                hasSelection = false;
                ShowToast((settings.language == "pt-BR") ? L"Texto copiado!" : L"Copied to clipboard!");
                Invalidate();
                return;
            }
            if (conPty.isCommandRunning.load() || pane.activeRunningBlock) {
                conPty.CancelCurrentCommand();
                for (auto& tab : tabs) {
                    for (auto& p : tab->panes) {
                        if (p && p->activeRunningBlock) {
                            p->activeRunningBlock->exitCode = 1;
                            p->activeRunningBlock->isRunning = false;
                            StyledLine cancelSl;
                            cancelSl.rawText = L"[Processo interrompido (Ctrl+C)]";
                            cancelSl.spans.push_back({ cancelSl.rawText, currentTheme.syntaxInvalidCommand });
                            p->activeRunningBlock->outputLines.push_back(cancelSl);
                            p->blocks.push_back(std::move(p->activeRunningBlock));
                            p->activeRunningBlock = nullptr;
                            p->autoScrollToBottom = true;
                        }
                    }
                }
                ShowToast(L"Processo interrompido (SIGINT)");
            } else {
                pane.inputLine.clear();
                pane.cursorPos = 0;
                pane.autocompleteOpen = false;
            }
            Invalidate();
            return;
        }

        if (ctrl && vk == 'L') {
            pane.blocks.clear();
            pane.scrollOffsetY = 0.0f;
            Invalidate();
            return;
        }

        if (ctrl && vk == 'V') {
            std::wstring clip = GetTextFromClipboard(hwnd);
            if (!clip.empty()) {
                if (conPty.isCommandRunning.load() && conPty.currentRunningPaneId == pane.id) {
                    conPty.SendInput(clip);
                    Invalidate();
                    return;
                }
                bool isMultiLine = (clip.find(L'\n') != std::wstring::npos || clip.find(L'\r') != std::wstring::npos);
                if (isMultiLine) {
                    if (settings.warnMultiLinePaste) {
                        multiLinePasteModalOpen = true;
                        pendingPasteText = clip;
                        Invalidate();
                        return;
                    }
                    pendingPasteText = clip;
                    ExecuteMultiLinePaste();
                    return;
                }
                pane.inputLine.insert(pane.cursorPos, clip);
                pane.cursorPos += clip.size();
                Invalidate();
            }
            return;
        }

        // Direct interactive ConPTY keyboard routing for active CLIs (agy, claude, etc.)
        if (conPty.isCommandRunning.load() && conPty.currentRunningPaneId == pane.id) {
            pane.autocompleteOpen = false;
            if (vk == VK_UP) {
                conPty.WriteInput("\x1b[A");
                Invalidate();
                return;
            } else if (vk == VK_DOWN) {
                conPty.WriteInput("\x1b[B");
                Invalidate();
                return;
            } else if (vk == VK_RIGHT) {
                conPty.WriteInput("\x1b[C");
                Invalidate();
                return;
            } else if (vk == VK_LEFT) {
                conPty.WriteInput("\x1b[D");
                Invalidate();
                return;
            } else if (vk == VK_HOME) {
                conPty.WriteInput("\x1b[H");
                Invalidate();
                return;
            } else if (vk == VK_END) {
                conPty.WriteInput("\x1b[F");
                Invalidate();
                return;
            } else if (vk == VK_PRIOR) {
                conPty.WriteInput("\x1b[5~");
                Invalidate();
                return;
            } else if (vk == VK_NEXT) {
                conPty.WriteInput("\x1b[6~");
                Invalidate();
                return;
            } else if (vk == VK_TAB) {
                conPty.WriteInput("\t");
                Invalidate();
                return;
            } else if (vk == VK_SPACE) {
                conPty.WriteInput(" ");
                Invalidate();
                return;
            } else if (vk == VK_ESCAPE) {
                conPty.WriteInput("\x1b");
                Invalidate();
                return;
            } else if (vk == VK_RETURN) {
                conPty.WriteInput("\r\n");
                pane.inputLine.clear();
                pane.cursorPos = 0;
                Invalidate();
                return;
            } else if (vk == VK_BACK) {
                conPty.WriteInput("\x08");
                if (pane.cursorPos > 0 && !pane.inputLine.empty()) {
                    pane.inputLine.erase(pane.cursorPos - 1, 1);
                    pane.cursorPos--;
                }
                Invalidate();
                return;
            } else if (vk == VK_DELETE) {
                conPty.WriteInput("\x1b[3~");
                Invalidate();
                return;
            }
            return;
        }

        // Autocomplete Navigation
        if (pane.autocompleteOpen) {
            if (vk == VK_UP) {
                if (pane.autocompleteIndex > 0) pane.autocompleteIndex--;
                SafeRelease(pane.previewThumb);
                Invalidate();
                return;
            } else if (vk == VK_DOWN) {
                if (pane.autocompleteIndex + 1 < static_cast<int>(pane.autocompleteSuggestions.size())) {
                    pane.autocompleteIndex++;
                }
                SafeRelease(pane.previewThumb);
                Invalidate();
                return;
            } else if (vk == VK_ESCAPE) {
                pane.autocompleteOpen = false;
                SafeRelease(pane.previewThumb);
                Invalidate();
                return;
            } else if (vk == VK_TAB || vk == VK_RETURN) {
                if (pane.autocompleteIndex >= 0 && pane.autocompleteIndex < static_cast<int>(pane.autocompleteSuggestions.size())) {
                    AcceptAutocomplete(pane);
                    Invalidate();
                    return;
                }
            }
        }

        // Tab to trigger autocomplete or accept Levenshtein suggestion
        if (vk == VK_TAB) {
            if (!pane.syntaxAnalysis.ghostSuggestion.empty() && pane.syntaxAnalysis.ghostSuggestion.find(L"Você quis dizer: ") != std::wstring::npos) {
                size_t p = pane.syntaxAnalysis.ghostSuggestion.find(L"Você quis dizer: ");
                std::wstring sugg = pane.syntaxAnalysis.ghostSuggestion.substr(p + 17);
                size_t q = sugg.find(L" ?");
                if (q != std::wstring::npos) sugg = sugg.substr(0, q);
                pane.inputLine = sugg;
                pane.cursorPos = pane.inputLine.size();
                Invalidate();
                return;
            }

            TriggerAutocomplete(pane);
            Invalidate();
            return;
        }

        // Normal typing & navigation
        if (vk == VK_RETURN) {
            ExecuteInput();
        } else if (vk == VK_BACK) {
            if (pane.cursorPos > 0 && !pane.inputLine.empty()) {
                pane.inputLine.erase(pane.cursorPos - 1, 1);
                pane.cursorPos--;
                if (pane.autocompleteOpen) TriggerAutocomplete(pane);
                Invalidate();
            }
        } else if (vk == VK_DELETE) {
            if (pane.cursorPos < pane.inputLine.size()) {
                pane.inputLine.erase(pane.cursorPos, 1);
                Invalidate();
            }
        } else if (vk == VK_LEFT) {
            if (pane.cursorPos > 0) pane.cursorPos--;
            Invalidate();
        } else if (vk == VK_RIGHT || vk == VK_END) {
            if (pane.cursorPos == pane.inputLine.size()) {
                std::wstring ghostSuffix = SyntaxAndAutocomplete::ComputeGhostSuggestion(pane.inputLine, pane.commandHistory, pane.cwd);
                if (!ghostSuffix.empty()) {
                    pane.inputLine += ghostSuffix;
                    pane.cursorPos = pane.inputLine.size();
                    Invalidate();
                    return;
                }
            }
            if (vk == VK_RIGHT) {
                if (pane.cursorPos < pane.inputLine.size()) pane.cursorPos++;
            } else {
                pane.cursorPos = pane.inputLine.size();
            }
            Invalidate();
        } else if (vk == VK_UP) {
            if (!pane.commandHistory.empty()) {
                if (pane.historyIndex == -1) {
                    pane.historyIndex = static_cast<int>(pane.commandHistory.size()) - 1;
                } else if (pane.historyIndex > 0) {
                    pane.historyIndex--;
                }
                pane.inputLine = pane.commandHistory[pane.historyIndex];
                pane.cursorPos = pane.inputLine.size();
                Invalidate();
            }
        } else if (vk == VK_DOWN) {
            if (!pane.commandHistory.empty() && pane.historyIndex != -1) {
                if (pane.historyIndex + 1 < static_cast<int>(pane.commandHistory.size())) {
                    pane.historyIndex++;
                    pane.inputLine = pane.commandHistory[pane.historyIndex];
                } else {
                    pane.historyIndex = -1;
                    pane.inputLine.clear();
                }
                pane.cursorPos = pane.inputLine.size();
                Invalidate();
            }
        }
    }

    void OnChar(wchar_t c) {
        if (renamingTabIndex != -1) {
            if (c >= 32) {
                renamingText += c;
                Invalidate();
            }
            return;
        }

        if (onboarding.open || settingsOpen || timelineOpen) return;

        if (GetActiveTab().isEditor) {
            if (c >= 32) {
                GetActiveTab().editorContent += c;
                GetActiveTab().editorCursorCol++;
                GetActiveTab().isDirty = true;
                Invalidate();
            }
            return;
        }

        if (workfolderOpen) {
            if (c >= 32) {
                workfolderFilter += c;
                workfolderSelectedIndex = 0;
                Invalidate();
            }
            return;
        }

        if (searchOpen) {
            if (c >= 32) {
                searchQuery += c;
                searchMatchIndex = 1;
                Invalidate();
            }
            return;
        }

        if (paletteOpen) {
            if (c >= 32) {
                paletteQuery += c;
                paletteSelectedIndex = 0;
                paletteScrollOffset = 0;
                Invalidate();
            }
            return;
        }

        if (c >= 32) {
            auto& pane = GetActivePane();
            if (conPty.isCommandRunning.load() && conPty.currentRunningPaneId == pane.id) {
                std::wstring s(1, c);
                conPty.SendInput(s);
                Invalidate();
                return;
            }
            pane.inputLine.insert(pane.cursorPos, 1, c);
            pane.cursorPos++;
            if (pane.autocompleteOpen) {
                TriggerAutocomplete(pane);
            }
            Invalidate();
        }
    }

    void TriggerAutocomplete(TerminalPane& pane) {
        GitStatusInfo gitInfo = GitEngine::InspectRepository(pane.cwd);
        pane.autocompleteSuggestions = syntaxEngine.GetSuggestions(pane.inputLine, pane.cursorPos, pane.cwd, gitInfo);
        pane.autocompleteIndex = 0;
        pane.autocompleteOpen = !pane.autocompleteSuggestions.empty();
        SafeRelease(pane.previewThumb);
    }

    void AcceptAutocomplete(TerminalPane& pane) {
        if (pane.autocompleteIndex >= 0 && pane.autocompleteIndex < static_cast<int>(pane.autocompleteSuggestions.size())) {
            const auto& item = pane.autocompleteSuggestions[pane.autocompleteIndex];
            size_t tokenStart = pane.cursorPos;
            while (tokenStart > 0 && !iswspace(pane.inputLine[tokenStart - 1])) {
                tokenStart--;
            }
            std::wstring before = pane.inputLine.substr(0, tokenStart);
            std::wstring after = pane.inputLine.substr(pane.cursorPos);

            std::wstring insertText = item.text;
            if (insertText.find(L' ') != std::wstring::npos && !StartsWith(insertText, L"\"")) {
                insertText = L"\"" + insertText + L"\"";
            }
            if (item.isDir && !EndsWith(insertText, L"\\") && !EndsWith(insertText, L"/")) {
                insertText += L"\\";
            } else if (!item.isDir) {
                insertText += L" ";
            }

            pane.inputLine = before + insertText + after;
            pane.cursorPos = before.size() + insertText.size();
            pane.autocompleteOpen = false;
            SafeRelease(pane.previewThumb);
        }
    }

    void TriggerPalettePreview() {
        std::vector<PaletteAction> filtered;
        std::wstring qLower = ToLower(paletteQuery);
        for (const auto& act : paletteActions) {
            if (qLower.empty() || ToLower(act.title).find(qLower) != std::wstring::npos || ToLower(act.desc).find(qLower) != std::wstring::npos) {
                filtered.push_back(act);
            }
        }
        if (paletteSelectedIndex >= 0 && paletteSelectedIndex < static_cast<int>(filtered.size())) {
            const auto& act = filtered[paletteSelectedIndex];
            if (!act.themeName.empty()) {
                currentTheme = ThemeManager::GetThemeByName(act.themeName);
            }
        }
    }

    void ExecutePaletteSelection() {
        std::vector<PaletteAction> filtered;
        std::wstring qLower = ToLower(paletteQuery);
        for (const auto& act : paletteActions) {
            if (qLower.empty() || ToLower(act.title).find(qLower) != std::wstring::npos || ToLower(act.desc).find(qLower) != std::wstring::npos) {
                filtered.push_back(act);
            }
        }
        if (paletteSelectedIndex >= 0 && paletteSelectedIndex < static_cast<int>(filtered.size())) {
            const auto& act = filtered[paletteSelectedIndex];
            if (!act.themeName.empty()) {
                SetTheme(act.themeName);
                prePaletteTheme = currentTheme;
            } else {
                switch (act.id) {
                case 1:
                    GetActivePane().mode = ShellMode::ModePW;
                    GetActiveTab().UpdateDefaultTitle();
                    break;
                case 2:
                    GetActivePane().mode = ShellMode::ModeCMD;
                    GetActiveTab().UpdateDefaultTitle();
                    break;
                case 3:
                    CreateNewTab(ShellMode::ModePW);
                    break;
                case 4:
                    CreateNewTab(ShellMode::ModeCMD);
                    break;
                case 5:
                    CloseTab(activeTabIndex);
                    break;
                case 6:
                    LaunchAdminSession();
                    break;
                case 7:
                    SplitHorizontal();
                    break;
                case 8:
                    SplitVertical();
                    break;
                case 9:
                    CloseActivePane();
                    break;
                case 10:
                    GetActivePane().inputLine = L"gstatus";
                    ExecuteInput();
                    break;
                case 11:
                    GetActivePane().inputLine = L"glog";
                    ExecuteInput();
                    break;
                case 12: {
                    GitStatusInfo gi = GitEngine::InspectRepository(GetActivePane().cwd);
                    GetActivePane().inputLine = L"git push --set-upstream origin " + gi.branchName;
                    ExecuteInput();
                    break;
                }
                case 13:
                    GetActivePane().inputLine = L"git pull --rebase";
                    ExecuteInput();
                    break;
                case 14:
                    searchOpen = true;
                    searchQuery.clear();
                    break;
                case 15:
                    settingsOpen = true;
                    break;
                case 16:
                    GetActivePane().blocks.clear();
                    GetActivePane().scrollOffsetY = 0.0f;
                    break;
                case 17:
                    ToggleQuakeMode();
                    break;
                case 18:
                    workspaceSidebarOpen = !workspaceSidebarOpen;
                    break;
                case 19:
                    helpModalOpen = true;
                    break;
                }
            }
        }
    }

    void ExecuteMultiLinePaste() {
        if (pendingPasteText.empty()) {
            multiLinePasteModalOpen = false;
            Invalidate();
            return;
        }
        auto& tab = GetActiveTab();
        auto& pane = tab.GetActivePane();
        std::wstring textToRun = pendingPasteText;
        pendingPasteText.clear();
        multiLinePasteModalOpen = false;

        if (conPty.isCommandRunning.load()) {
            ShowToast((settings.language == "pt-BR") ? L"Comando anterior ainda em execução." : L"Previous command still running.");
            Invalidate();
            return;
        }

        auto block = std::make_unique<CommandBlock>();
        block->id = nextBlockId++;
        block->modeUsed = pane.mode;
        block->startTick = GetTickCount();

        std::wstring firstLine = textToRun;
        size_t nl = firstLine.find_first_of(L"\r\n");
        if (nl != std::wstring::npos) {
            firstLine = Trim(firstLine.substr(0, nl));
        }
        int lineCount = static_cast<int>(std::count(textToRun.begin(), textToRun.end(), L'\n')) + 1;
        block->commandText = firstLine + L"  [Batch " + std::to_wstring(lineCount) + L" linhas]";
        block->workingDirectory = pane.cwd;

        pane.activeRunningBlock = std::move(block);
        pane.autoScrollToBottom = true;
        pane.inputLine.clear();
        pane.cursorPos = 0;

        conPty.ExecuteAsync(textToRun, pane.mode, pane.cwd, pane.activeRunningBlock.get(), currentTheme, pane.id);
        Invalidate();
    }

    void UpdateSelectedTextFromRect() {
        if (!hasSelection) { selectedText.clear(); return; }
        float sTop = (std::min)(selectionStartPt.y, selectionEndPt.y);
        float sBottom = (std::max)(selectionStartPt.y, selectionEndPt.y);
        float sLeft = (std::min)(selectionStartPt.x, selectionEndPt.x);
        float sRight = (std::max)(selectionStartPt.x, selectionEndPt.x);
        if (sRight - sLeft < 4.0f || sBottom - sTop < 4.0f) {
            hasSelection = false;
            selectedText.clear();
            return;
        }

        auto& pane = GetActivePane();
        std::wostringstream ss;

        // Bottom prompt check
        RECT rc; GetClientRect(hwnd, &rc);
        float height = static_cast<float>(rc.bottom - rc.top);
        float promptY = height - 24.0f - 30.0f;
        if (sBottom >= promptY && sTop <= promptY + 30.0f && !pane.inputLine.empty()) {
            ss << pane.inputLine << L"\n";
        }

        // Output lines in blocks
        for (const auto& blk : pane.blocks) {
            if (!blk) continue;
            if (sBottom >= blk->headerRect.top && sTop <= blk->headerRect.bottom) {
                if (!blk->commandText.empty()) ss << blk->commandText << L"\n";
            }
            if (blk->isCollapsed) continue;
            for (const auto& line : blk->outputLines) {
                if (!line.rawText.empty()) {
                    ss << line.rawText << L"\n";
                }
            }
        }
        selectedText = ss.str();
        while (!selectedText.empty() && (selectedText.back() == L'\n' || selectedText.back() == L'\r')) {
            selectedText.pop_back();
        }
    }

    void OnMouseDown(POINT pt) {
        // Multi-line Paste Security Modal Clicks
        if (multiLinePasteModalOpen) {
            if (RendererD2D::PtInD2D1Rect(renderer.multiLinePasteConfirmBtnRect, pt)) {
                ExecuteMultiLinePaste();
                return;
            }
            if (RendererD2D::PtInD2D1Rect(renderer.multiLinePasteCancelBtnRect, pt) ||
                !RendererD2D::PtInD2D1Rect(renderer.multiLinePasteModalRect, pt)) {
                multiLinePasteModalOpen = false;
                pendingPasteText.clear();
                Invalidate();
                return;
            }
            return;
        }

        // Help Modal Clicks
        if (helpModalOpen) {
            if (RendererD2D::PtInD2D1Rect(renderer.helpCloseBtnRect, pt) ||
                !RendererD2D::PtInD2D1Rect(renderer.helpModalRect, pt)) {
                helpModalOpen = false;
                Invalidate();
                return;
            }
            return;
        }

        // Command Palette Clicks (Ctrl+Shift+P)
        if (paletteOpen) {
            if (RendererD2D::PtInD2D1Rect(renderer.paletteCloseBtnRect, pt)) {
                paletteOpen = false;
                currentTheme = prePaletteTheme;
                Invalidate();
                return;
            }
            for (const auto& itRect : renderer.paletteItemRects) {
                if (RendererD2D::PtInD2D1Rect(itRect.first, pt)) {
                    paletteSelectedIndex = itRect.second;
                    ExecutePaletteSelection();
                    paletteOpen = false;
                    Invalidate();
                    return;
                }
            }
            if (!RendererD2D::PtInD2D1Rect(renderer.paletteModalRect, pt)) {
                paletteOpen = false;
                currentTheme = prePaletteTheme;
                Invalidate();
            }
            return;
        }

        // Dark Direct2D Sidebar Context Menu Clicks
        if (renderer.sidebarContextMenuOpen) {
            for (const auto& it : renderer.sidebarContextMenuItems) {
                if (it.id > 0 && RendererD2D::PtInD2D1Rect(it.rect, pt)) {
                    ExecuteSidebarContextAction(it.id, renderer.contextMenuFilePath);
                    renderer.sidebarContextMenuOpen = false;
                    Invalidate();
                    return;
                }
            }
            renderer.sidebarContextMenuOpen = false;
            Invalidate();
            return;
        }

        // Timeline History Modal Clicks (Ctrl+R)
        if (timelineOpen) {
            if (RendererD2D::PtInD2D1Rect(renderer.timelineCloseBtnRect, pt) ||
                RendererD2D::PtInD2D1Rect(renderer.timelineFooterEscRect, pt)) {
                timelineOpen = false;
                Invalidate();
                return;
            }
            if (RendererD2D::PtInD2D1Rect(renderer.timelineFilterToggleRect, pt) ||
                RendererD2D::PtInD2D1Rect(renderer.timelineFooterTabRect, pt)) {
                timelineFilterSuccessOnly = !timelineFilterSuccessOnly;
                timelineSelectedIndex = 0;
                Invalidate();
                return;
            }
            auto filtered = FilterTimelineHistory(timelineHistory, timelineFilterSuccessOnly);
            if (RendererD2D::PtInD2D1Rect(renderer.timelineFooterRunRect, pt)) {
                if (timelineSelectedIndex < filtered.size()) {
                    GetActivePane().inputLine = filtered[timelineSelectedIndex].command;
                    GetActivePane().cursorPos = GetActivePane().inputLine.size();
                    timelineOpen = false;
                    ExecuteInput();
                }
                return;
            }
            for (const auto& itRect : renderer.timelineItemRects) {
                if (RendererD2D::PtInD2D1Rect(itRect.first, pt)) {
                    if (itRect.second < filtered.size()) {
                        GetActivePane().inputLine = filtered[itRect.second].command;
                        GetActivePane().cursorPos = GetActivePane().inputLine.size();
                        timelineOpen = false;
                        ExecuteInput();
                    }
                    return;
                }
            }
            if (!RendererD2D::PtInD2D1Rect(renderer.timelineModalRect, pt)) {
                timelineOpen = false;
                Invalidate();
            }
            return;
        }

        // Breadcrumb Popover Clicks
        if (breadcrumbPopoverOpen) {
            for (const auto& it : renderer.breadcrumbPopoverItems) {
                if (RendererD2D::PtInD2D1Rect(it.first, pt)) {
                    GetActivePane().cwd = it.second;
                    breadcrumbPopoverOpen = false;
                    Invalidate();
                    return;
                }
            }
            breadcrumbPopoverOpen = false;
            Invalidate();
            return;
        }

        // Task Runner Dropdown Menu Clicks
        if (taskMenuOpen) {
            for (const auto& it : renderer.taskMenuItems) {
                if (RendererD2D::PtInD2D1Rect(it.first, pt)) {
                    if (it.second < detectedTasks.size()) {
                        GetActivePane().inputLine = detectedTasks[it.second].command;
                        GetActivePane().cursorPos = GetActivePane().inputLine.size();
                        taskMenuOpen = false;
                        ExecuteInput();
                        Invalidate();
                        return;
                    }
                }
            }
            taskMenuOpen = false;
            Invalidate();
            return;
        }

        // Pinned Output Panel Close Button
        if (pinnedBlock && RendererD2D::PtInD2D1Rect(renderer.pinnedCloseBtnRect, pt)) {
            pinnedBlockStorage.reset();
            pinnedBlock = nullptr;
            Invalidate();
            return;
        }

        // Editor Save Button Click
        if (GetActiveTab().isEditor && RendererD2D::PtInD2D1Rect(renderer.editorSaveBtnRect, pt)) {
            GetActiveTab().SaveEditorFile();
            Invalidate();
            return;
        }

        // Onboarding Walkthrough Clicks
        if (onboarding.open) {
            if (RendererD2D::PtInD2D1Rect(onboarding.nextBtnRect, pt)) {
                if (onboarding.step == 6) {
                    settings.onboardingCompleted = true;
                    settings.SaveToFile();
                    onboarding.Close();
                } else {
                    onboarding.Next();
                }
                Invalidate();
                return;
            }
            if (RendererD2D::PtInD2D1Rect(onboarding.backBtnRect, pt)) {
                onboarding.Prev();
                Invalidate();
                return;
            }
            if (RendererD2D::PtInD2D1Rect(onboarding.skipBtnRect, pt)) {
                settings.onboardingCompleted = true;
                settings.SaveToFile();
                onboarding.Close();
                Invalidate();
                return;
            }
            // Step 0: Language
            for (const auto& lb : onboarding.languageBtns) {
                if (RendererD2D::PtInD2D1Rect(lb.first, pt)) {
                    settings.language = lb.second;
                    I18n::SetLanguage(settings.language);
                    Invalidate();
                    return;
                }
            }
            // Step 1: Shell
            if (RendererD2D::PtInD2D1Rect(onboarding.shellPwBtnRect, pt)) {
                settings.classicFlowMode = true;
                Invalidate();
                return;
            }
            if (RendererD2D::PtInD2D1Rect(onboarding.shellCmdBtnRect, pt)) {
                settings.classicFlowMode = false;
                Invalidate();
                return;
            }
            // Step 2: Themes
            for (const auto& tb : onboarding.themeBtns) {
                if (RendererD2D::PtInD2D1Rect(tb.first, pt)) {
                    SetTheme(tb.second);
                    Invalidate();
                    return;
                }
            }
            // Step 2: Fonts
            for (const auto& fb : onboarding.fontBtns) {
                if (RendererD2D::PtInD2D1Rect(fb.first, pt)) {
                    settings.fontFamily = fb.second;
                    renderer.SetFont(settings.fontFamily, settings.fontSize);
                    Invalidate();
                    return;
                }
            }
            if (RendererD2D::PtInD2D1Rect(onboarding.fontMinusBtnRect, pt)) {
                if (settings.fontSize > 10.0f) {
                    settings.fontSize -= 1.0f;
                    renderer.SetFont(settings.fontFamily, settings.fontSize);
                    Invalidate();
                }
                return;
            }
            if (RendererD2D::PtInD2D1Rect(onboarding.fontPlusBtnRect, pt)) {
                if (settings.fontSize < 32.0f) {
                    settings.fontSize += 1.0f;
                    renderer.SetFont(settings.fontFamily, settings.fontSize);
                    Invalidate();
                }
                return;
            }
            // Step 3: Wallpaper
            for (const auto& wb : onboarding.wallpaperBtns) {
                if (RendererD2D::PtInD2D1Rect(wb.first, pt)) {
                    settings.wallpaperPath = wb.second;
                    std::wstring exeDir = AppSettings::GetExeDir();
                    std::wstring resolved = ResolveWallpaperPath(settings.wallpaperPath, exeDir);
                    renderer.wallpaperEngine.Load(resolved);
                    Invalidate();
                    return;
                }
            }
            if (RendererD2D::PtInD2D1Rect(onboarding.wallpaperAddBtnRect, pt)) {
                wchar_t fileBuf[MAX_PATH] = L"";
                OPENFILENAMEW ofn = {};
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hwnd;
                ofn.lpstrFilter = L"Wallpapers (*.png;*.jpg;*.jpeg;*.bmp;*.webp;*.gif;*.mp4;*.webm;*.mov)\0*.png;*.jpg;*.jpeg;*.bmp;*.webp;*.gif;*.mp4;*.webm;*.mov\0All Files (*.*)\0*.*\0";
                ofn.lpstrFile = fileBuf;
                ofn.nMaxFile = MAX_PATH;
                ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
                if (GetOpenFileNameW(&ofn)) {
                    settings.customWallpapers.push_back(fileBuf);
                    settings.wallpaperPath = fileBuf;
                    renderer.wallpaperEngine.Load(fileBuf);
                    Invalidate();
                }
                return;
            }
            D2D1_RECT_F oDimHit = onboarding.wallpaperDimTrackRect;
            oDimHit.top -= 10.0f; oDimHit.bottom += 10.0f;
            if (RendererD2D::PtInD2D1Rect(oDimHit, pt)) {
                float trackW = onboarding.wallpaperDimTrackRect.right - onboarding.wallpaperDimTrackRect.left;
                if (trackW > 0.0f) {
                    float frac = (pt.x - onboarding.wallpaperDimTrackRect.left) / trackW;
                    settings.wallpaperDim = (std::clamp)(static_cast<int>(frac * 90.0f + 0.5f), 0, 90);
                    Invalidate();
                }
                return;
            }
            // Step 5: Quick Prefs
            for (const auto& wb : onboarding.warnPasteBtns) {
                if (RendererD2D::PtInD2D1Rect(wb.first, pt)) {
                    settings.warnMultiLinePaste = wb.second;
                    Invalidate();
                    return;
                }
            }
            for (const auto& sb : onboarding.sysCursorBtns) {
                if (RendererD2D::PtInD2D1Rect(sb.first, pt)) {
                    settings.systemCursor = sb.second;
                    Invalidate();
                    return;
                }
            }
            if (RendererD2D::PtInD2D1Rect(onboarding.registerTermBtnRect, pt)) {
                bool termReg = AppSettings::IsAppPathTermRegistered();
                AppSettings::RegisterAppPathTerm(!termReg);
                Invalidate();
                return;
            }
            return;
        }

        // Settings Modal Clicks
        if (settingsOpen) {
            if (RendererD2D::PtInD2D1Rect(renderer.settingsCloseBtnRect, pt)) {
                settingsOpen = false;
                settings.SaveToFile();
                Invalidate();
                return;
            }
            // Theme buttons
            for (const auto& tb : renderer.settingsThemeBtns) {
                if (RendererD2D::PtInD2D1Rect(tb.first, pt)) {
                    SetTheme(tb.second);
                    return;
                }
            }
            // Prompt Position buttons
            for (const auto& pb : renderer.settingsPromptPositionBtns) {
                if (RendererD2D::PtInD2D1Rect(pb.first, pt)) {
                    settings.promptPosition = pb.second;
                    settings.SaveToFile();
                    Invalidate();
                    return;
                }
            }
            // Warmth continuous slider click
            D2D1_RECT_F warmthHit = renderer.settingsWarmthSliderTrackRect;
            warmthHit.top -= 10.0f;
            warmthHit.bottom += 10.0f;
            if (RendererD2D::PtInD2D1Rect(warmthHit, pt)) {
                float trackW = renderer.settingsWarmthSliderTrackRect.right - renderer.settingsWarmthSliderTrackRect.left;
                if (trackW > 1.0f) {
                    float ratio = (pt.x - renderer.settingsWarmthSliderTrackRect.left) / trackW;
                    settings.warmthLevel = (std::clamp)(ratio, 0.0f, 1.0f);
                    settings.SaveToFile();
                    isDraggingWarmth = true;
                    SetCapture(hwnd);
                    Invalidate();
                    return;
                }
            }
            // Font minus / plus
            if (RendererD2D::PtInD2D1Rect(renderer.settingsFontMinusRect, pt)) {
                if (settings.fontSize > 10.0f) {
                    settings.fontSize -= 1.0f;
                    renderer.SetFont(settings.fontFamily, settings.fontSize);
                    settings.SaveToFile();
                    Invalidate();
                }
                return;
            }
            if (RendererD2D::PtInD2D1Rect(renderer.settingsFontPlusRect, pt)) {
                if (settings.fontSize < 32.0f) {
                    settings.fontSize += 1.0f;
                    renderer.SetFont(settings.fontFamily, settings.fontSize);
                    settings.SaveToFile();
                    Invalidate();
                }
                return;
            }
            // Font family
            for (const auto& fb : renderer.settingsFontFamilyBtns) {
                if (RendererD2D::PtInD2D1Rect(fb.first, pt)) {
                    settings.fontFamily = fb.second;
                    renderer.SetFont(settings.fontFamily, settings.fontSize);
                    settings.SaveToFile();
                    Invalidate();
                    return;
                }
            }
            // Wallpaper buttons
            for (const auto& wb : renderer.settingsWallpaperBtns) {
                if (RendererD2D::PtInD2D1Rect(wb.first, pt)) {
                    settings.wallpaperPath = wb.second;
                    std::wstring exeDir = AppSettings::GetExeDir();
                    std::wstring resolved = ResolveWallpaperPath(settings.wallpaperPath, exeDir);
                    renderer.wallpaperEngine.Load(resolved);
                    settings.SaveToFile();
                    Invalidate();
                    return;
                }
            }
            // Wallpaper ADD+
            if (RendererD2D::PtInD2D1Rect(renderer.settingsWallpaperAddBtnRect, pt)) {
                wchar_t fileBuf[MAX_PATH] = L"";
                OPENFILENAMEW ofn = {};
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hwnd;
                ofn.lpstrFilter = L"Wallpapers (*.png;*.jpg;*.jpeg;*.bmp;*.webp;*.gif;*.mp4;*.webm;*.mov)\0*.png;*.jpg;*.jpeg;*.bmp;*.webp;*.gif;*.mp4;*.webm;*.mov\0All Files (*.*)\0*.*\0";
                ofn.lpstrFile = fileBuf;
                ofn.nMaxFile = MAX_PATH;
                ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
                if (GetOpenFileNameW(&ofn)) {
                    settings.customWallpapers.push_back(fileBuf);
                    settings.wallpaperPath = fileBuf;
                    renderer.wallpaperEngine.Load(fileBuf);
                    settings.SaveToFile();
                    Invalidate();
                }
                return;
            }
            // Wallpaper Dim Slider
            D2D1_RECT_F dimHit = renderer.settingsWallpaperDimTrackRect;
            dimHit.top -= 10.0f; dimHit.bottom += 10.0f;
            if (RendererD2D::PtInD2D1Rect(dimHit, pt)) {
                float trackW = renderer.settingsWallpaperDimTrackRect.right - renderer.settingsWallpaperDimTrackRect.left;
                if (trackW > 0.0f) {
                    float frac = (pt.x - renderer.settingsWallpaperDimTrackRect.left) / trackW;
                    settings.wallpaperDim = (std::clamp)(static_cast<int>(frac * 90.0f + 0.5f), 0, 90);
                    settings.SaveToFile();
                    Invalidate();
                }
                return;
            }
            // Wallpaper Pause Unfocused
            for (const auto& pb : renderer.settingsWallpaperPauseUnfocusedBtns) {
                if (RendererD2D::PtInD2D1Rect(pb.first, pt)) {
                    settings.wallpaperPauseUnfocused = pb.second;
                    settings.SaveToFile();
                    Invalidate();
                    return;
                }
            }
            // Wallpaper Pause on Battery
            for (const auto& pb : renderer.settingsWallpaperPauseOnBatteryBtns) {
                if (RendererD2D::PtInD2D1Rect(pb.first, pt)) {
                    settings.wallpaperPauseOnBattery = pb.second;
                    settings.SaveToFile();
                    Invalidate();
                    return;
                }
            }
            // Classic mode vs Blocks mode
            if (RendererD2D::PtInD2D1Rect(renderer.settingsClassicModeBtnRect, pt)) {
                settings.classicFlowMode = true;
                settings.SaveToFile();
                Invalidate();
                return;
            }
            if (RendererD2D::PtInD2D1Rect(renderer.settingsBlocksModeBtnRect, pt)) {
                settings.classicFlowMode = false;
                settings.SaveToFile();
                Invalidate();
                return;
            }
            // System vs custom cursor
            for (const auto& sb : renderer.settingsSysCursorBtns) {
                if (RendererD2D::PtInD2D1Rect(sb.first, pt)) {
                    settings.systemCursor = (sb.second != 0);
                    settings.SaveToFile();
                    Invalidate();
                    return;
                }
            }
            // Cursor style
            for (const auto& cb : renderer.settingsCursorBtns) {
                if (RendererD2D::PtInD2D1Rect(cb.first, pt)) {
                    settings.cursorStyle = cb.second;
                    settings.SaveToFile();
                    Invalidate();
                    return;
                }
            }
            // Language selector
            for (const auto& lb : renderer.settingsLanguageBtns) {
                if (RendererD2D::PtInD2D1Rect(lb.first, pt)) {
                    settings.language = lb.second;
                    I18n::SetLanguage(settings.language);
                    InitPaletteActions();
                    settings.SaveToFile();
                    ShowToast(I18n::Get(StringId::Toast_LanguageChanged));
                    Invalidate();
                    return;
                }
            }
            // Register 'term' in Win+R / Explorer
            if (RendererD2D::PtInD2D1Rect(renderer.settingsRegisterTermBtnRect, pt)) {
                bool termReg = AppSettings::IsAppPathTermRegistered();
                AppSettings::RegisterAppPathTerm(!termReg);
                bool ptbr = (settings.language == "pt-BR");
                if (!termReg) {
                    ShowToast(ptbr ? L"Atalho 'term' registrado no Win+R e Explorer!" : L"'term' shortcut registered in Win+R & Explorer!");
                } else {
                    ShowToast(ptbr ? L"Atalho 'term' desativado." : L"'term' shortcut removed.");
                }
                Invalidate();
                return;
            }
            // Multi-line paste security warning toggle
            for (const auto& wb : renderer.settingsWarnPasteBtns) {
                if (RendererD2D::PtInD2D1Rect(wb.first, pt)) {
                    settings.warnMultiLinePaste = wb.second;
                    settings.SaveToFile();
                    Invalidate();
                    return;
                }
            }
            // View intro again button
            if (RendererD2D::PtInD2D1Rect(renderer.settingsViewIntroBtnRect, pt)) {
                settingsOpen = false;
                onboarding.Reset();
                Invalidate();
                return;
            }
            // Click outside modal closes it
            if (!RendererD2D::PtInD2D1Rect(renderer.settingsModalRect, pt)) {
                settingsOpen = false;
                settings.SaveToFile();
                Invalidate();
            }
            return;
        }

        // Workfolder Modal Clicks (Telescope)
        if (workfolderOpen) {
            auto filtered = WorkfolderScanner::FuzzyFilter(workfolderItems, workfolderFilter);
            if (RendererD2D::PtInD2D1Rect(renderer.workfolderCloseBtnRect, pt) ||
                RendererD2D::PtInD2D1Rect(renderer.workfolderFooterEscRect, pt)) {
                workfolderOpen = false;
                Invalidate();
                return;
            }
            if (RendererD2D::PtInD2D1Rect(renderer.workfolderRunBtnRect, pt)) {
                if (workfolderSelectedIndex < filtered.size()) {
                    const auto& item = filtered[workfolderSelectedIndex];
                    if (item.isDirectory) {
                        GetActivePane().cwd = item.fullPath;
                        workfolderItems = ScanWorkfolder(GetActivePane().cwd);
                        workfolderSelectedIndex = 0;
                        workfolderFilter.clear();
                        Invalidate();
                        return;
                    } else if (item.extension == L".ps1") {
                        GetActivePane().inputLine = L"powershell -ExecutionPolicy Bypass -File \"" + item.fullPath + L"\"";
                        workfolderOpen = false;
                        ExecuteInput();
                        return;
                    } else if (item.extension == L".py") {
                        GetActivePane().inputLine = L"python \"" + item.fullPath + L"\"";
                        workfolderOpen = false;
                        ExecuteInput();
                        return;
                    } else if (item.extension == L".bat" || item.extension == L".cmd" || item.extension == L".exe") {
                        GetActivePane().inputLine = L"\"" + item.fullPath + L"\"";
                        workfolderOpen = false;
                        ExecuteInput();
                        return;
                    } else {
                        GetActivePane().inputLine += L" \"" + item.fullPath + L"\"";
                        GetActivePane().cursorPos = GetActivePane().inputLine.size();
                        workfolderOpen = false;
                        Invalidate();
                        return;
                    }
                }
            }
            if (RendererD2D::PtInD2D1Rect(renderer.workfolderTabBtnRect, pt)) {
                if (workfolderSelectedIndex < filtered.size()) {
                    const auto& item = filtered[workfolderSelectedIndex];
                    GetActivePane().inputLine += L" \"" + item.fullPath + L"\"";
                    GetActivePane().cursorPos = GetActivePane().inputLine.size();
                    workfolderOpen = false;
                    Invalidate();
                    return;
                }
            }
            if (RendererD2D::PtInD2D1Rect(renderer.workfolderExplorerBtnRect, pt)) {
                if (workfolderSelectedIndex < filtered.size()) {
                    const auto& item = filtered[workfolderSelectedIndex];
                    if (!item.isDirectory) {
                        ShellExecuteW(nullptr, L"open", L"explorer.exe", (L"/select,\"" + item.fullPath + L"\"").c_str(), nullptr, SW_SHOWNORMAL);
                        return;
                    }
                }
                ShellExecuteW(nullptr, L"open", GetActivePane().cwd.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                return;
            }
            for (const auto& itRect : renderer.workfolderItemRects) {
                if (RendererD2D::PtInD2D1Rect(itRect.first, pt)) {
                    workfolderSelectedIndex = itRect.second;
                    Invalidate();
                    return;
                }
            }
            if (!RendererD2D::PtInD2D1Rect(renderer.workfolderModalRect, pt)) {
                workfolderOpen = false;
                Invalidate();
            }
            return;
        }

        // Profile Menu clicks
        if (profileMenuOpen) {
            for (const auto& mi : renderer.profileMenuItems) {
                if (RendererD2D::PtInD2D1Rect(mi.first, pt)) {
                    profileMenuOpen = false;
                    switch (mi.second) {
                    case 1: CreateNewTab(ShellMode::ModePW); break;
                    case 2: CreateNewTab(ShellMode::ModeCMD); break;
                    case 3: LaunchAdminSession(); break;
                    case 4: SplitHorizontal(); break; // Split Right
                    case 5: SplitVertical(); break;   // Split Down
                    }
                    Invalidate();
                    return;
                }
            }
            profileMenuOpen = false;
            Invalidate();
            return;
        }

        // File Menu clicks
        if (fileMenuOpen) {
            for (const auto& mi : renderer.fileMenuItems) {
                if (RendererD2D::PtInD2D1Rect(mi.first, pt)) {
                    fileMenuOpen = false;
                    switch (mi.second) {
                    case 12: {
                        workspaceSidebarOpen = !workspaceSidebarOpen;
                        Invalidate();
                        return;
                    }
                    case 1: LoadScriptDialog(1); break; // .ps1
                    case 2: LoadScriptDialog(2); break; // .py
                    case 3: LoadScriptDialog(3); break; // image / any file
                    case 8: ExportTabOutput(); break;
                    case 10: settingsOpen = true; break;
                    case 9: CloseTab(activeTabIndex); break;
                    case 11: PostMessage(hwnd, WM_CLOSE, 0, 0); break;
                    }
                    Invalidate();
                    return;
                }
            }
            fileMenuOpen = false;
            Invalidate();
            return;
        }

        // Git Popover clicks
        if (gitPopoverOpen) {
            for (const auto& gi : renderer.gitPopoverItems) {
                if (RendererD2D::PtInD2D1Rect(gi.first, pt)) {
                    gitPopoverOpen = false;
                    GitStatusInfo info = GitEngine::InspectRepository(GetActivePane().cwd);
                    if (gi.second == 1) { // Push auto-upstream
                        GetActivePane().inputLine = L"git push --set-upstream origin " + info.branchName;
                        ExecuteInput();
                    } else if (gi.second == 2) { // Pull rebase
                        GetActivePane().inputLine = L"git pull --rebase";
                        ExecuteInput();
                    } else if (gi.second == 3) { // gstatus
                        GetActivePane().inputLine = L"gstatus";
                        ExecuteInput();
                    } else if (gi.second == 4) { // glog
                        GetActivePane().inputLine = L"glog";
                        ExecuteInput();
                    }
                    Invalidate();
                    return;
                }
            }
            for (const auto& br : renderer.gitBranchSwitchItems) {
                if (RendererD2D::PtInD2D1Rect(br.first, pt)) {
                    gitPopoverOpen = false;
                    GetActivePane().inputLine = L"git checkout " + br.second;
                    ExecuteInput();
                    Invalidate();
                    return;
                }
            }
            gitPopoverOpen = false;
            Invalidate();
            return;
        }

        // Snippets Sidebar clicks
        if (snippetsPopoverOpen) {
            RECT rcClient;
            GetClientRect(hwnd, &rcClient);
            float clientW = static_cast<float>(rcClient.right - rcClient.left);
            float sidebarX = clientW - 280.0f;

            if (pt.x >= sidebarX) {
                if (RendererD2D::PtInD2D1Rect(renderer.snippetsAddCurrentBtnRect, pt)) {
                    std::wstring cmd = GetActivePane().inputLine;
                    if (cmd.empty() && !GetActivePane().blocks.empty()) {
                        cmd = GetActivePane().blocks.back()->commandText;
                    }
                    if (!cmd.empty()) {
                        SnippetItem sn;
                        sn.name = L"Snippet " + std::to_wstring(settings.snippets.size() + 1);
                        sn.command = cmd;
                        sn.description = L"Comando salvo";
                        settings.snippets.push_back(sn);
                        settings.SaveToFile();
                    }
                    Invalidate();
                    return;
                }
                for (const auto& si : renderer.snippetsItemBtns) {
                    if (RendererD2D::PtInD2D1Rect(si.first, pt)) {
                        if (si.second < settings.snippets.size()) {
                            GetActivePane().inputLine = settings.snippets[si.second].command;
                            GetActivePane().cursorPos = GetActivePane().inputLine.size();
                        }
                        Invalidate();
                        return;
                    }
                }
                return;
            }
        }

        // Search Bar Clicks
        if (searchOpen && RendererD2D::PtInD2D1Rect(renderer.searchBoxRect, pt)) {
            if (RendererD2D::PtInD2D1Rect(renderer.searchCloseBtnRect, pt)) {
                searchOpen = false;
                searchQuery.clear();
                Invalidate();
                return;
            }
            if (RendererD2D::PtInD2D1Rect(renderer.searchPrevBtnRect, pt)) {
                if (searchMatchIndex > 1) searchMatchIndex--;
                Invalidate();
                return;
            }
            if (RendererD2D::PtInD2D1Rect(renderer.searchNextBtnRect, pt)) {
                if (searchMatchIndex < totalSearchMatches) searchMatchIndex++;
                else searchMatchIndex = 1;
                Invalidate();
                return;
            }
            return;
        }

        // Prompt Line Git Badge Click
        if (renderer.statusGitRect.right > renderer.statusGitRect.left &&
            RendererD2D::PtInD2D1Rect(renderer.statusGitRect, pt)) {
            gitPopoverOpen = !gitPopoverOpen;
            fileMenuOpen = false;
            Invalidate();
            return;
        }

        // Prompt Line Shell Mode Click (PowerShell <-> CMD)
        if (renderer.statusModeRect.right > renderer.statusModeRect.left &&
            RendererD2D::PtInD2D1Rect(renderer.statusModeRect, pt)) {
            ToggleMode();
            return;
        }

        // Prompt Folder icon click (opens Workfolder)
        if (renderer.promptFolderRect.right > renderer.promptFolderRect.left &&
            RendererD2D::PtInD2D1Rect(renderer.promptFolderRect, pt)) {
            workfolderOpen = !workfolderOpen;
            if (workfolderOpen) {
                workfolderSelectedIndex = 0;
                workfolderItems = ScanWorkfolder(GetActivePane().cwd);
            }
            Invalidate();
            return;
        }

        // Prompt Git badge click
        if (renderer.promptGitBadgeRect.right > renderer.promptGitBadgeRect.left &&
            RendererD2D::PtInD2D1Rect(renderer.promptGitBadgeRect, pt)) {
            gitPopoverOpen = !gitPopoverOpen;
            Invalidate();
            return;
        }

        // Titlebar element clicks (y < TITLEBAR_HEIGHT)
        if (pt.y < RendererD2D::TITLEBAR_HEIGHT) {
            // Window control buttons
            if (RendererD2D::PtInD2D1Rect(renderer.closeBtnRect, pt)) {
                PostMessage(hwnd, WM_CLOSE, 0, 0);
                return;
            }
            if (RendererD2D::PtInD2D1Rect(renderer.maxBtnRect, pt)) {
                if (IsZoomed(hwnd)) ShowWindow(hwnd, SW_RESTORE);
                else ShowWindow(hwnd, SW_MAXIMIZE);
                return;
            }
            if (RendererD2D::PtInD2D1Rect(renderer.minBtnRect, pt)) {
                ShowWindow(hwnd, SW_MINIMIZE);
                return;
            }

            // Global navigation menu button (far left \uE700)
            if (RendererD2D::PtInD2D1Rect(renderer.fileBtnRect, pt)) {
                fileMenuOpen = !fileMenuOpen;
                fileMenuX = 6.0f;
                profileMenuOpen = false;
                gitPopoverOpen = false;
                Invalidate();
                return;
            }

            // New Tab button [+] creates tab with active pane mode
            if (RendererD2D::PtInD2D1Rect(renderer.newTabBtnRect, pt)) {
                CreateNewTab(GetActivePane().mode);
                return;
            }

            // Dropdown button [v] next to [+] opens Profile Menu
            if (renderer.dropdownBtnRect.right > renderer.dropdownBtnRect.left &&
                RendererD2D::PtInD2D1Rect(renderer.dropdownBtnRect, pt)) {
                profileMenuOpen = !profileMenuOpen;
                profileMenuX = renderer.dropdownBtnRect.left;
                fileMenuOpen = false;
                gitPopoverOpen = false;
                Invalidate();
                return;
            }

            // Contextual Task Runner Button
            if (renderer.taskRunnerBtnRect.right > renderer.taskRunnerBtnRect.left &&
                RendererD2D::PtInD2D1Rect(renderer.taskRunnerBtnRect, pt)) {
                if (pt.x >= renderer.taskRunnerBtnRect.right - 22.0f || detectedTasks.size() > 1) {
                    taskMenuOpen = !taskMenuOpen;
                } else if (!detectedTasks.empty()) {
                    GetActivePane().inputLine = detectedTasks[0].command;
                    GetActivePane().cursorPos = GetActivePane().inputLine.size();
                    ExecuteInput();
                }
                Invalidate();
                return;
            }

            // Workfolder Button (toggles Workspace Sidebar)
            if (renderer.workfolderBtnRect.right > renderer.workfolderBtnRect.left &&
                RendererD2D::PtInD2D1Rect(renderer.workfolderBtnRect, pt)) {
                workspaceSidebarOpen = !workspaceSidebarOpen;
                Invalidate();
                return;
            }

            // Quick Run Script Button (ghost button)
            if (renderer.quickRunBtnRect.right > renderer.quickRunBtnRect.left &&
                RendererD2D::PtInD2D1Rect(renderer.quickRunBtnRect, pt)) {
                RunQuickScript();
                return;
            }

            // Snippets Button (code book)
            if (renderer.snippetsBtnRect.right > renderer.snippetsBtnRect.left &&
                RendererD2D::PtInD2D1Rect(renderer.snippetsBtnRect, pt)) {
                snippetsPopoverOpen = !snippetsPopoverOpen;
                fileMenuOpen = false;
                gitPopoverOpen = false;
                Invalidate();
                return;
            }

            // Search Button
            if (RendererD2D::PtInD2D1Rect(renderer.searchBtnRect, pt)) {
                searchOpen = !searchOpen;
                if (searchOpen) { searchQuery.clear(); searchMatchIndex = 1; }
                Invalidate();
                return;
            }

            // Settings Button
            if (RendererD2D::PtInD2D1Rect(renderer.settingsBtnRect, pt)) {
                settingsOpen = true;
                Invalidate();
                return;
            }

            // Tab clicks
            for (size_t i = 0; i < tabs.size(); ++i) {
                if (RendererD2D::PtInD2D1Rect(tabs[i]->closeBtnRect, pt)) {
                    CloseTab(static_cast<int>(i));
                    return;
                }
                if (RendererD2D::PtInD2D1Rect(tabs[i]->tabRect, pt)) {
                    if (renamingTabIndex != -1) {
                        if (renamingTabIndex >= 0 && renamingTabIndex < static_cast<int>(tabs.size()) && !renamingText.empty()) {
                            tabs[renamingTabIndex]->title = renamingText;
                            tabs[renamingTabIndex]->customTitle = true;
                            tabs[renamingTabIndex]->hasCustomTitle = true;
                        }
                        renamingTabIndex = -1;
                    }
                    activeTabIndex = static_cast<int>(i);
                    Invalidate();
                    return;
                }
            }
            return;
        }

        // Live Process Pill Kill Button
        if (renderer.liveProcessKillBtnRect.right > renderer.liveProcessKillBtnRect.left &&
            RendererD2D::PtInD2D1Rect(renderer.liveProcessKillBtnRect, pt)) {
            conPty.CancelCurrentCommand();
            for (auto& tab : tabs) {
                for (auto& p : tab->panes) {
                    if (p && p->activeRunningBlock) {
                        p->activeRunningBlock->exitCode = 1;
                        p->activeRunningBlock->isRunning = false;
                        StyledLine cancelSl;
                        cancelSl.rawText = L"[Processo interrompido pelo usuario (Kill)]";
                        cancelSl.spans.push_back({ cancelSl.rawText, currentTheme.syntaxInvalidCommand });
                        p->activeRunningBlock->outputLines.push_back(cancelSl);
                        p->blocks.push_back(std::move(p->activeRunningBlock));
                        p->activeRunningBlock = nullptr;
                        p->autoScrollToBottom = true;
                    }
                }
            }
            ShowToast(L"Processo interrompido (SIGINT)");
            Invalidate();
            return;
        }

        // Empty State Badges Clicks
        for (const auto& eb : renderer.emptyStateBadges) {
            if (RendererD2D::PtInD2D1Rect(eb.first, pt)) {
                switch (eb.second) {
                case 0:
                    workfolderOpen = true;
                    workfolderFilter.clear();
                    workfolderSelectedIndex = 0;
                    workfolderItems = ScanWorkfolder(GetActivePane().cwd);
                    break;
                case 1:
                    timelineOpen = true;
                    timelineSelectedIndex = 0;
                    break;
                case 2:
                    if (!detectedTasks.empty()) {
                        GetActivePane().inputLine = detectedTasks[0].command;
                        GetActivePane().cursorPos = GetActivePane().inputLine.size();
                        ExecuteInput();
                    }
                    break;
                case 3:
                    settingsOpen = true;
                    break;
                case 4:
                    paletteOpen = true;
                    prePaletteTheme = currentTheme;
                    paletteQuery.clear();
                    paletteSelectedIndex = 0;
                    break;
                }
                Invalidate();
                return;
            }
        }

        // Breadcrumb Segments Click
        for (const auto& bc : renderer.breadcrumbSegments) {
            if (RendererD2D::PtInD2D1Rect(bc.first, pt)) {
                breadcrumbTargetDir = bc.second;
                breadcrumbAnchorRect = bc.first;
                breadcrumbSiblings.clear();

                std::wstring parentDir = bc.second;
                size_t lastSlash = parentDir.find_last_of(L"\\/");
                if (lastSlash != std::wstring::npos) {
                    parentDir = parentDir.substr(0, lastSlash);
                    if (parentDir.size() == 2 && parentDir[1] == L':') parentDir += L"\\";
                }
                WIN32_FIND_DATAW fd;
                std::wstring pattern = parentDir + L"\\*";
                HANDLE hFind = FindFirstFileW(pattern.c_str(), &fd);
                if (hFind != INVALID_HANDLE_VALUE) {
                    do {
                        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                            if (wcscmp(fd.cFileName, L".") != 0 && wcscmp(fd.cFileName, L"..") != 0) {
                                std::wstring sibPath = parentDir + (parentDir.back() == L'\\' ? L"" : L"\\") + fd.cFileName;
                                breadcrumbSiblings.push_back(sibPath);
                            }
                        }
                    } while (FindNextFileW(hFind, &fd));
                    FindClose(hFind);
                }

                breadcrumbPopoverOpen = true;
                Invalidate();
                return;
            }
        }

        // Modular Status Bar Clicks
        if (settings.statusBarVisible) {
            if (renderer.statusBarThemeRect.right > renderer.statusBarThemeRect.left &&
                RendererD2D::PtInD2D1Rect(renderer.statusBarThemeRect, pt)) {
                auto all = ThemeManager::GetAllThemes();
                size_t idx = 0;
                for (size_t t = 0; t < all.size(); ++t) {
                    if (all[t].name == currentTheme.name) { idx = (t + 1) % all.size(); break; }
                }
                SetTheme(all[idx].name);
                return;
            }
            if (renderer.statusBarWarmthRect.right > renderer.statusBarWarmthRect.left &&
                RendererD2D::PtInD2D1Rect(renderer.statusBarWarmthRect, pt)) {
                static const float warmthLevels[] = { 0.0f, 0.25f, 0.50f, 0.75f, 1.0f };
                size_t wIdx = 0;
                for (size_t w = 0; w < 5; ++w) {
                    if (std::abs(settings.warmthLevel - warmthLevels[w]) < 0.10f) {
                        wIdx = (w + 1) % 5;
                        break;
                    }
                }
                settings.warmthLevel = warmthLevels[wIdx];
                settings.SaveToFile();
                Invalidate();
                return;
            }
            if (renderer.statusBarPromptPosRect.right > renderer.statusBarPromptPosRect.left &&
                RendererD2D::PtInD2D1Rect(renderer.statusBarPromptPosRect, pt)) {
                settings.promptPosition = (settings.promptPosition == "bottom") ? "top" : "bottom";
                settings.SaveToFile();
                ShowToast(L"Prompt: " + Utf8ToWide(settings.promptPosition));
                Invalidate();
                return;
            }
            if (renderer.statusBarCpuRamRect.right > renderer.statusBarCpuRamRect.left &&
                RendererD2D::PtInD2D1Rect(renderer.statusBarCpuRamRect, pt)) {
                gitPopoverOpen = !gitPopoverOpen;
                Invalidate();
                return;
            }
        }

        // Sidebar Collapse Button [◀]
        if (workspaceSidebarOpen && RendererD2D::PtInD2D1Rect(renderer.sidebarCollapseBtnRect, pt)) {
            workspaceSidebarOpen = false;
            Invalidate();
            return;
        }

        // Sidebar Closed Toggle Button (below FILE)
        if (!workspaceSidebarOpen && RendererD2D::PtInD2D1Rect(renderer.sidebarToggleBtnRect, pt)) {
            workspaceSidebarOpen = true;
            Invalidate();
            return;
        }

        // Workspace Sidebar Clicks (Ctrl+B)
        if (workspaceSidebarOpen && pt.y >= RendererD2D::TITLEBAR_HEIGHT && pt.x <= 220.0f) {
            if (RendererD2D::PtInD2D1Rect(renderer.workspaceFilesTabRect, pt)) {
                renderer.workspaceSidebarActiveTab = 0;
                Invalidate();
                return;
            }
            if (RendererD2D::PtInD2D1Rect(renderer.workspaceGitTabRect, pt)) {
                renderer.workspaceSidebarActiveTab = 1;
                Invalidate();
                return;
            }
            for (const auto& fi : renderer.workspaceFileItemRects) {
                if (RendererD2D::PtInD2D1Rect(fi.first, pt)) {
                    OpenEditorTab(fi.second);
                    return;
                }
            }
            for (const auto& fld : renderer.workspaceFolderItemRects) {
                if (RendererD2D::PtInD2D1Rect(fld.first, pt)) {
                    GetActivePane().cwd = fld.second;
                    Invalidate();
                    return;
                }
            }
            return;
        }

        // Ctrl+Click URL detection
        bool isCtrlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        if (isCtrlDown) {
            for (const auto& lk : renderer.frameLinks) {
                if (RendererD2D::PtInD2D1Rect(lk.rect, pt)) {
                    ShellExecuteW(nullptr, L"open", lk.url.c_str(), nullptr, nullptr, SW_SHOW);
                    return;
                }
            }
        }

        // Interactive buttons inside blocks: Copy, Rerun, Quick Fix, Open File, Collapse, Pin, Export
        auto& pane = GetActivePane();
        for (auto& blk : pane.blocks) {
            if (blk->copyButtonRect.right > blk->copyButtonRect.left &&
                RendererD2D::PtInD2D1Rect(blk->copyButtonRect, pt)) {
                CopyTextToClipboard(hwnd, blk->GetAllOutputText());
                ShowToast(I18n::Get(StringId::Toast_Copied));
                return;
            }
            if (blk->collapseButtonRect.right > blk->collapseButtonRect.left &&
                RendererD2D::PtInD2D1Rect(blk->collapseButtonRect, pt)) {
                blk->isCollapsed = !blk->isCollapsed;
                Invalidate();
                return;
            }
            if (blk->pinButtonRect.right > blk->pinButtonRect.left &&
                RendererD2D::PtInD2D1Rect(blk->pinButtonRect, pt)) {
                if (pinnedBlock && pinnedBlockStorage && pinnedBlockStorage->commandText == blk->commandText) {
                    pinnedBlockStorage.reset();
                    pinnedBlock = nullptr;
                } else {
                    pinnedBlockStorage = std::make_unique<CommandBlock>(*blk);
                    pinnedBlock = pinnedBlockStorage.get();
                }
                pinnedScrollY = 0.0f;
                Invalidate();
                return;
            }
            if (blk->exportButtonRect.right > blk->exportButtonRect.left &&
                RendererD2D::PtInD2D1Rect(blk->exportButtonRect, pt)) {
                wchar_t fName[MAX_PATH] = L"command_log.txt";
                OPENFILENAMEW ofn = {};
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hwnd;
                ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
                ofn.lpstrFile = fName;
                ofn.nMaxFile = MAX_PATH;
                ofn.Flags = OFN_OVERWRITEPROMPT;
                if (GetSaveFileNameW(&ofn)) {
                    std::ofstream out(fName, std::ios::binary);
                    if (out.is_open()) {
                        std::string text = WideToUtf8(blk->GetAllOutputText());
                        out.write(text.data(), text.size());
                        ShowToast(L"Log exportado!");
                    }
                }
                return;
            }
            if (blk->rerunButtonRect.right > blk->rerunButtonRect.left &&
                RendererD2D::PtInD2D1Rect(blk->rerunButtonRect, pt)) {
                pane.inputLine = blk->commandText;
                pane.cursorPos = pane.inputLine.size();
                ExecuteInput();
                return;
            }
            if (blk->fixButtonRect.right > blk->fixButtonRect.left &&
                RendererD2D::PtInD2D1Rect(blk->fixButtonRect, pt)) {
                pane.inputLine = blk->diagnosis.suggestedFix;
                pane.cursorPos = pane.inputLine.size();
                ExecuteInput();
                return;
            }
            if (blk->fileLinkRect.right > blk->fileLinkRect.left &&
                RendererD2D::PtInD2D1Rect(blk->fileLinkRect, pt)) {
                OpenFileInEditor(blk->diagnosis.sourceFile, blk->diagnosis.sourceLine);
                return;
            }
        }

        // Start Mouse Text Selection in Terminal Pane
        RECT clRc; GetClientRect(hwnd, &clRc);
        float clientH = static_cast<float>(clRc.bottom - clRc.top);
        if (pt.y >= RendererD2D::TITLEBAR_HEIGHT && pt.y < clientH - 24.0f) {
            if (!workspaceSidebarOpen || pt.x > 220.0f) {
                isMouseSelecting = true;
                selectionStartPt = D2D1::Point2F(static_cast<float>(pt.x), static_cast<float>(pt.y));
                selectionEndPt = selectionStartPt;
                hasSelection = false;
                selectedText.clear();
                SetCapture(hwnd);
                Invalidate();
            }
        }
    }

    void OnDoubleClick(POINT pt) {
        if (workspaceSidebarOpen && pt.x <= 220.0f) {
            for (const auto& fi : renderer.workspaceFileItemRects) {
                if (RendererD2D::PtInD2D1Rect(fi.first, pt)) {
                    OpenEditorTab(fi.second);
                    return;
                }
            }
            for (const auto& fld : renderer.workspaceFolderItemRects) {
                if (RendererD2D::PtInD2D1Rect(fld.first, pt)) {
                    GetActivePane().cwd = fld.second;
                    Invalidate();
                    return;
                }
            }
        }

        if (workfolderOpen) {
            for (const auto& itRect : renderer.workfolderItemRects) {
                if (RendererD2D::PtInD2D1Rect(itRect.first, pt)) {
                    size_t idx = itRect.second;
                    if (idx < workfolderItems.size()) {
                        const auto& item = workfolderItems[idx];
                        if (item.isDirectory) {
                            GetActivePane().cwd = item.fullPath;
                            workfolderSelectedIndex = 0;
                            workfolderItems = ScanWorkfolder(item.fullPath);
                            Invalidate();
                            return;
                        } else if (item.isScript) {
                            GetActivePane().inputLine = L"\"" + item.fullPath + L"\"";
                            workfolderOpen = false;
                            ExecuteInput();
                            return;
                        }
                    }
                }
            }
            return;
        }

        // Tab double click: inline rename
        if (pt.y < RendererD2D::TITLEBAR_HEIGHT) {
            for (size_t i = 0; i < tabs.size(); ++i) {
                if (RendererD2D::PtInD2D1Rect(tabs[i]->tabRect, pt) && !RendererD2D::PtInD2D1Rect(tabs[i]->closeBtnRect, pt)) {
                    renamingTabIndex = static_cast<int>(i);
                    renamingText = tabs[i]->title;
                    Invalidate();
                    return;
                }
            }
        }
    }

    void OpenSidebarContextMenu(POINT pt, const std::wstring& filePath) {
        bool ptbr = (settings.language == "pt-BR");
        bool es = (settings.language == "es-ES");

        std::wstring ext = filePath;
        size_t dot = ext.find_last_of(L'.');
        std::wstring e = (dot != std::wstring::npos) ? ToLower(ext.substr(dot)) : L"";

        renderer.sidebarContextMenuOpen = true;
        renderer.contextMenuFilePath = filePath;
        renderer.sidebarContextMenuItems.clear();

        std::vector<std::pair<int, std::pair<std::wstring, std::string>>> items;
        items.push_back({ 1, { ptbr ? L"Abrir no Editor" : (es ? L"Abrir en Editor" : L"Open in Editor"), "file_text_16.png" } });
        items.push_back({ 2, { ptbr ? L"Executar no terminal ativo" : (es ? L"Ejecutar en terminal activo" : L"Run in active terminal"), "run_16.png" } });
        items.push_back({ 3, { ptbr ? L"Executar em nova aba" : (es ? L"Ejecutar en nueva pestaña" : L"Run in new tab"), "terminal_16.png" } });

        if (e == L".html" || e == L".htm") {
            items.push_back({ -1, { L"", "" } });
            items.push_back({ 4, { L"Run on Localhost (Python http.server)", "radar_sonar.png" } });
            items.push_back({ 5, { L"Run on Localhost (NPM serve)", "radar_sonar.png" } });
        }

        items.push_back({ -1, { L"", "" } });
        items.push_back({ 6, { ptbr ? L"Abrir no Explorer" : (es ? L"Abrir en Explorer" : L"Open in Explorer"), "folder_16.png" } });

        float menuW = 230.0f;
        float itemH = 26.0f;
        float menuH = 12.0f;
        for (const auto& it : items) {
            menuH += (it.first == -1) ? 8.0f : itemH;
        }

        float menuX = static_cast<float>(pt.x);
        float menuY = static_cast<float>(pt.y);
        RECT rc = {};
        GetClientRect(hwnd, &rc);
        float width = static_cast<float>(rc.right - rc.left);
        float height = static_cast<float>(rc.bottom - rc.top);
        if (menuX + menuW > width - 10.0f) menuX = width - menuW - 10.0f;
        if (menuY + menuH > height - 30.0f) menuY = height - menuH - 30.0f;

        renderer.sidebarContextMenuRect = D2D1::RectF(menuX, menuY, menuX + menuW, menuY + menuH);

        float curY = menuY + 6.0f;
        for (const auto& it : items) {
            float h = (it.first == -1) ? 8.0f : itemH;
            D2D1_RECT_F r = D2D1::RectF(menuX + 4.0f, curY, menuX + menuW - 4.0f, curY + h);
            renderer.sidebarContextMenuItems.push_back({ r, it.first, it.second.first, it.second.second });
            curY += h;
        }
        Invalidate();
    }

    void ExecuteSidebarContextAction(int cmd, const std::wstring& filePath) {
        std::wstring ext = filePath;
        size_t dot = ext.find_last_of(L'.');
        std::wstring e = (dot != std::wstring::npos) ? ToLower(ext.substr(dot)) : L"";

        if (cmd == 1) {
            OpenEditorTab(filePath);
        } else if (cmd == 2) {
            if (e == L".py") GetActivePane().inputLine = L"python \"" + filePath + L"\"";
            else if (e == L".ps1") GetActivePane().inputLine = L"& \"" + filePath + L"\"";
            else GetActivePane().inputLine = L"\"" + filePath + L"\"";
            GetActivePane().cursorPos = GetActivePane().inputLine.size();
            ExecuteInput();
        } else if (cmd == 3) {
            CreateNewTab(ShellMode::ModePW);
            if (e == L".py") GetActivePane().inputLine = L"python \"" + filePath + L"\"";
            else if (e == L".ps1") GetActivePane().inputLine = L"& \"" + filePath + L"\"";
            else GetActivePane().inputLine = L"\"" + filePath + L"\"";
            GetActivePane().cursorPos = GetActivePane().inputLine.size();
            ExecuteInput();
        } else if (cmd == 4) {
            GetActivePane().inputLine = L"python -m http.server 8000";
            GetActivePane().cursorPos = GetActivePane().inputLine.size();
            ExecuteInput();
        } else if (cmd == 5) {
            GetActivePane().inputLine = L"npx serve .";
            GetActivePane().cursorPos = GetActivePane().inputLine.size();
            ExecuteInput();
        } else if (cmd == 6) {
            ShellExecuteW(nullptr, L"open", L"explorer.exe", (L"/select,\"" + filePath + L"\"").c_str(), nullptr, SW_SHOWNORMAL);
        }
    }

    void OnRightClick(POINT pt) {
        // Sidebar item right-click context menu
        if (workspaceSidebarOpen && pt.x <= 220.0f) {
            for (const auto& fi : renderer.workspaceFileItemRects) {
                if (RendererD2D::PtInD2D1Rect(fi.first, pt)) {
                    OpenSidebarContextMenu(pt, fi.second);
                    return;
                }
            }
        }

        // Tab right click: cycle custom accent color
        if (pt.y < RendererD2D::TITLEBAR_HEIGHT) {
            for (size_t i = 0; i < tabs.size(); ++i) {
                if (RendererD2D::PtInD2D1Rect(tabs[i]->tabRect, pt)) {
                    static const uint32_t colors[] = {
                        0x00E5FF, // Cyan
                        0x00FF9C, // Green
                        0xBD93F9, // Purple
                        0xFF5555, // Red
                        0xFFB86C, // Amber
                        0x000000  // Reset to default
                    };
                    static int colorIdx = 0;
                    colorIdx = (colorIdx + 1) % 6;
                    if (colors[colorIdx] == 0) {
                        tabs[i]->customAccent = ColorRgba(0, 0, 0, 0);
                    } else {
                        tabs[i]->customAccent = ColorHex(colors[colorIdx]);
                    }
                    Invalidate();
                    return;
                }
            }
        }

        // Terminal Pane Right Click: Copy selection if any, else Paste from clipboard
        RECT rc; GetClientRect(hwnd, &rc);
        float clientH = static_cast<float>(rc.bottom - rc.top);
        if (pt.y >= RendererD2D::TITLEBAR_HEIGHT && pt.y < clientH - 24.0f) {
            if (hasSelection && !selectedText.empty()) {
                CopyTextToClipboard(hwnd, selectedText);
                hasSelection = false;
                ShowToast((settings.language == "pt-BR") ? L"Texto copiado!" : L"Copied to clipboard!");
                Invalidate();
                return;
            }

            // Paste from clipboard
            std::wstring clip = GetTextFromClipboard(hwnd);
            if (!clip.empty()) {
                bool isMultiLine = (clip.find(L'\n') != std::wstring::npos || clip.find(L'\r') != std::wstring::npos);
                if (isMultiLine) {
                    if (settings.warnMultiLinePaste) {
                        multiLinePasteModalOpen = true;
                        pendingPasteText = clip;
                        Invalidate();
                        return;
                    }
                    pendingPasteText = clip;
                    ExecuteMultiLinePaste();
                    return;
                }
                auto& pane = GetActivePane();
                pane.inputLine.insert(pane.cursorPos, clip);
                pane.cursorPos += clip.size();
                Invalidate();
            }
        }
    }

    void OnMouseWheel(short delta) {
        if (settingsOpen) {
            renderer.settingsScrollY = (std::clamp)(
                renderer.settingsScrollY - (delta / 120.0f) * 45.0f,
                0.0f, renderer.settingsMaxScrollY
            );
            Invalidate();
            return;
        }
        if (paletteOpen) {
            int totalFiltered = 0;
            std::wstring qLower = ToLower(paletteQuery);
            for (const auto& act : paletteActions) {
                if (qLower.empty() || ToLower(act.title).find(qLower) != std::wstring::npos || ToLower(act.desc).find(qLower) != std::wstring::npos) {
                    totalFiltered++;
                }
            }
            if (delta > 0) {
                if (paletteScrollOffset > 0) paletteScrollOffset--;
                if (paletteSelectedIndex > paletteScrollOffset + 6) paletteSelectedIndex = paletteScrollOffset + 6;
            } else {
                if (paletteScrollOffset + 7 < totalFiltered) paletteScrollOffset++;
                if (paletteSelectedIndex < paletteScrollOffset) paletteSelectedIndex = paletteScrollOffset;
            }
            Invalidate();
            return;
        }
        auto& activeTab = GetActiveTab();
        if (activeTab.isEditor) {
            activeTab.editorScrollY -= (delta / 120.0f) * 40.0f;
            if (activeTab.editorScrollY < 0.0f) activeTab.editorScrollY = 0.0f;
            Invalidate();
            return;
        }
        bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        if (ctrl) {
            // Zoom font size
            if (delta > 0 && settings.fontSize < 30.0f) {
                settings.fontSize += 1.0f;
            } else if (delta < 0 && settings.fontSize > 10.0f) {
                settings.fontSize -= 1.0f;
            }
            renderer.SetFont(settings.fontFamily, settings.fontSize);
            settings.SaveToFile();
            Invalidate();
            return;
        }

        // Pinned Reference Panel scroll
        if (pinnedBlock && RendererD2D::PtInD2D1Rect(renderer.pinnedPanelRect, mousePos)) {
            pinnedScrollY -= (delta / 120.0f) * 45.0f;
            if (pinnedScrollY < 0.0f) pinnedScrollY = 0.0f;
            Invalidate();
            return;
        }

        auto& pane = GetActivePane();
        pane.autoScrollToBottom = false;
        pane.scrollOffsetY -= (delta / 120.0f) * 45.0f;
        if (pane.scrollOffsetY < 0.0f) {
            pane.scrollOffsetY = 0.0f;
            pane.autoScrollToBottom = true;
        }
        Invalidate();
    }
};

static SpetaTermApp* g_App = nullptr;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_NCCALCSIZE: {
        if (IsIconic(hwnd)) return 0;
        // Custom Frameless Window: Returning 0 for both wParam == TRUE and wParam == FALSE
        // completely eliminates the native Windows titlebar while retaining DWM drop shadow and resizing borders!
        if (wParam == TRUE) {
            NCCALCSIZE_PARAMS* pParams = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
            if (IsZoomed(hwnd)) {
                HMONITOR hMon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
                MONITORINFO mi = { sizeof(mi) };
                if (GetMonitorInfoW(hMon, &mi)) {
                    pParams->rgrc[0] = mi.rcWork;
                }
            }
        }
        return 0;
    }

    case WM_NCACTIVATE:
        // Prevent Windows from repainting standard non-client caption/titlebar on focus change
        return DefWindowProcW(hwnd, msg, wParam, -1);

    case WM_NCHITTEST: {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        RECT wr;
        GetWindowRect(hwnd, &wr);

        bool isMax = (IsZoomed(hwnd) != 0);
        if (!isMax) {
            const int border = 6;
            if (pt.y < wr.top + border) {
                if (pt.x < wr.left + border) return HTTOPLEFT;
                if (pt.x > wr.right - border) return HTTOPRIGHT;
                return HTTOP;
            }
            if (pt.y > wr.bottom - border) {
                if (pt.x < wr.left + border) return HTBOTTOMLEFT;
                if (pt.x > wr.right - border) return HTBOTTOMRIGHT;
                return HTBOTTOM;
            }
            if (pt.x < wr.left + border) return HTLEFT;
            if (pt.x > wr.right - border) return HTRIGHT;
        }

        POINT clientPt = pt;
        ScreenToClient(hwnd, &clientPt);

        if (clientPt.y < RendererD2D::TITLEBAR_HEIGHT) {
            if (g_App && g_App->IsOverTitlebarInteractive(clientPt)) {
                return HTCLIENT;
            }
            return HTCAPTION; // Allows dragging window and double clicking to maximize!
        }
        return HTCLIENT;
    }

    case WM_SETCURSOR: {
        if (LOWORD(lParam) == HTCLIENT) {
            if (g_App && !g_App->settings.systemCursor) {
                SetCursor(nullptr);
                return TRUE;
            }
            if (g_App) {
                if (g_App->IsOverInteractive(g_App->mousePos)) {
                    SetCursor(LoadCursorW(nullptr, IDC_HAND));
                } else if (!g_App->settingsOpen && !g_App->workfolderOpen && !g_App->timelineOpen && !g_App->paletteOpen && g_App->mousePos.y >= RendererD2D::TITLEBAR_HEIGHT) {
                    SetCursor(LoadCursorW(nullptr, IDC_IBEAM));
                } else {
                    SetCursor(LoadCursorW(nullptr, IDC_ARROW));
                }
                return TRUE;
            }
            return TRUE;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    case WM_DROPFILES: {
        HDROP hDrop = reinterpret_cast<HDROP>(wParam);
        UINT fileCount = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
        if (fileCount > 0 && g_App) {
            wchar_t droppedPath[MAX_PATH];
            if (DragQueryFileW(hDrop, 0, droppedPath, MAX_PATH)) {
                g_App->HandleDroppedPath(droppedPath);
            }
        }
        DragFinish(hDrop);
        return 0;
    }

    case WM_SIZE:
        if (g_App) {
            if (wParam == SIZE_MINIMIZED) {
                g_App->renderer.wallpaperEngine.Pause();
                return 0;
            }
            g_App->renderer.wallpaperEngine.Resume();
            UINT w = LOWORD(lParam);
            UINT h = HIWORD(lParam);
            if (w > 0 && h > 0) {
                g_App->renderer.Resize(w, h);
                g_App->Invalidate();
            }
        }
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        if (g_App) {
            g_App->Render();
        }
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_MOUSEMOVE:
        if (g_App) {
            TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, hwnd, 0 };
            TrackMouseEvent(&tme);

            POINT newPt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            bool wasInteractive = g_App->IsOverInteractive(g_App->mousePos);
            bool isInteractive = g_App->IsOverInteractive(newPt);

            bool needInvalidate = false;
            if (g_App->isMouseSelecting) {
                g_App->selectionEndPt = D2D1::Point2F(static_cast<float>(newPt.x), static_cast<float>(newPt.y));
                float dx = fabsf(g_App->selectionEndPt.x - g_App->selectionStartPt.x);
                float dy = fabsf(g_App->selectionEndPt.y - g_App->selectionStartPt.y);
                if (dx > 4.0f || dy > 4.0f) {
                    g_App->hasSelection = true;
                }
                needInvalidate = true;
            }
            if (g_App->isDraggingWarmth) {
                float trackW = g_App->renderer.settingsWarmthSliderTrackRect.right - g_App->renderer.settingsWarmthSliderTrackRect.left;
                if (trackW > 1.0f) {
                    float ratio = (newPt.x - g_App->renderer.settingsWarmthSliderTrackRect.left) / trackW;
                    g_App->settings.warmthLevel = (std::clamp)(ratio, 0.0f, 1.0f);
                    needInvalidate = true;
                }
            }
            if (g_App && !g_App->settings.systemCursor) {
                needInvalidate = true;
            }
            if (g_App->settingsOpen || g_App->workfolderOpen || g_App->fileMenuOpen ||
                g_App->profileMenuOpen || g_App->gitPopoverOpen || g_App->snippetsPopoverOpen ||
                g_App->paletteOpen || g_App->searchOpen) {
                needInvalidate = true;
            } else if (wasInteractive != isInteractive) {
                needInvalidate = true;
            } else if (newPt.y < RendererD2D::TITLEBAR_HEIGHT || g_App->mousePos.y < RendererD2D::TITLEBAR_HEIGHT) {
                needInvalidate = true;
            }

            g_App->mousePos = newPt;
            if (needInvalidate) {
                g_App->Invalidate();
            }
        }
        return 0;

    case WM_MOUSELEAVE:
        if (g_App) {
            g_App->mousePos.x = -1;
            g_App->mousePos.y = -1;
            g_App->Invalidate();
        }
        return 0;

    case WM_ACTIVATE:
        if (g_App) {
            if (LOWORD(wParam) == WA_INACTIVE) {
                if (g_App->settings.wallpaperPauseUnfocused) {
                    g_App->renderer.wallpaperEngine.Pause();
                }
            } else {
                if (!IsIconic(hwnd)) {
                    g_App->renderer.wallpaperEngine.Resume();
                }
            }
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);

    case WM_POWERBROADCAST:
        if (wParam == PBT_APMPOWERSTATUSCHANGE && g_App) {
            SYSTEM_POWER_STATUS sps;
            if (GetSystemPowerStatus(&sps)) {
                bool onBattery = (sps.ACLineStatus == 0);
                if (g_App->settings.wallpaperPauseOnBattery && onBattery) {
                    g_App->renderer.wallpaperEngine.Pause();
                } else if (!IsIconic(hwnd)) {
                    g_App->renderer.wallpaperEngine.Resume();
                }
            }
        }
        return TRUE;

    case WM_SETFOCUS:
        if (g_App) {
            g_App->Invalidate();
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);

    case WM_LBUTTONDOWN:
        if (GetFocus() != hwnd) {
            SetFocus(hwnd);
        }
        if (g_App) {
            POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            g_App->OnMouseDown(pt);
        }
        return 0;

    case WM_LBUTTONUP:
        if (g_App) {
            if (g_App->isMouseSelecting) {
                g_App->isMouseSelecting = false;
                ReleaseCapture();
                float dx = fabsf(g_App->selectionEndPt.x - g_App->selectionStartPt.x);
                float dy = fabsf(g_App->selectionEndPt.y - g_App->selectionStartPt.y);
                if (dx > 4.0f || dy > 4.0f) {
                    g_App->hasSelection = true;
                    g_App->UpdateSelectedTextFromRect();
                } else {
                    g_App->hasSelection = false;
                    g_App->selectedText.clear();
                }
                g_App->Invalidate();
            }
            if (g_App->isDraggingWarmth) {
                g_App->isDraggingWarmth = false;
                ReleaseCapture();
                g_App->settings.SaveToFile();
                g_App->Invalidate();
            }
        }
        return 0;

    case WM_LBUTTONDBLCLK:
        if (g_App) {
            POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            g_App->OnDoubleClick(pt);
        }
        return 0;

    case WM_RBUTTONDOWN:
        if (GetFocus() != hwnd) {
            SetFocus(hwnd);
        }
        if (g_App) {
            POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            g_App->OnRightClick(pt);
        }
        return 0;

    case WM_MBUTTONDOWN:
        if (g_App) {
            POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            if (pt.y < RendererD2D::TITLEBAR_HEIGHT) {
                for (size_t i = 0; i < g_App->tabs.size(); ++i) {
                    if (RendererD2D::PtInD2D1Rect(g_App->tabs[i]->tabRect, pt)) {
                        g_App->CloseTab(static_cast<int>(i));
                        return 0;
                    }
                }
            }
        }
        return 0;

    case WM_MOUSEWHEEL:
        if (g_App) {
            g_App->OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
        }
        return 0;

    case WM_KEYDOWN:
        if (g_App) {
            g_App->OnKeyDown(wParam, lParam);
        }
        return 0;

    case WM_CHAR:
        if (g_App) {
            g_App->OnChar(static_cast<wchar_t>(wParam));
        }
        return 0;

    case WM_HOTKEY:
        if ((wParam == HOTKEY_QUAKE || wParam == HOTKEY_QUAKE + 1) && g_App) {
            g_App->ToggleQuakeMode();
        }
        return 0;

    case WM_TIMER:
        if (wParam == 1 && g_App) {
            if (IsIconic(hwnd)) return 0;
            g_App->cursorBlinkState = !g_App->cursorBlinkState;
            g_App->UpdateTelemetry();
            if (g_App->edgePulseActive) {
                DWORD elapsed = GetTickCount() - g_App->edgePulseStartTick;
                if (elapsed >= 1800) g_App->edgePulseActive = false;
            }
            if (g_App->toastAlpha > 0.0f) {
                DWORD elapsed = GetTickCount() - g_App->toastStartTick;
                if (elapsed > 2000) {
                    float fade = 1.0f - (elapsed - 2000) / 500.0f;
                    if (fade <= 0.0f) {
                        g_App->toastAlpha = 0.0f;
                    } else {
                        g_App->toastAlpha = fade;
                    }
                }
            }
            g_App->Invalidate();
        }
        return 0;

    case WM_TERMINAL_STREAM:
        if (g_App) {
            g_App->conPty.isStreamPending.store(false);
            int paneId = static_cast<int>(wParam);
            bool found = false;
            for (auto& tab : g_App->tabs) {
                for (auto& p : tab->panes) {
                    if (p && (p->id == paneId || paneId == 0)) {
                        p->autoScrollToBottom = true;
                        found = true;
                        break;
                    }
                }
                if (found && paneId != 0) break;
            }
            g_App->Invalidate();
        }
        return 0;

    case WM_TERMINAL_FINISHED:
        if (g_App) {
            int paneId = static_cast<int>(wParam);
            bool found = false;
            for (auto& tab : g_App->tabs) {
                for (auto& p : tab->panes) {
                    if (p && (p->id == paneId || paneId == 0)) {
                        if (p->activeRunningBlock) {
                            double dur = p->activeRunningBlock->executionTimeMs;
                            int exitCode = p->activeRunningBlock->exitCode;
                            if (dur >= 5000.0) {
                                g_App->edgePulseActive = true;
                                g_App->edgePulseSuccess = (exitCode == 0);
                                g_App->edgePulseStartTick = GetTickCount();
                            }

                            TimelineEntry te;
                            te.command = FormatSingleLineCommandTitle(p->activeRunningBlock->commandText);
                            te.cwd = p->activeRunningBlock->workingDirectory;
                            te.durationMs = dur;
                            te.exitCode = exitCode;
                            te.timestamp = p->activeRunningBlock->timestamp;
                            g_App->timelineHistory.push_back(te);

                            p->blocks.push_back(std::move(p->activeRunningBlock));
                            p->activeRunningBlock = nullptr;
                            p->autoScrollToBottom = true;
                        }
                        found = true;
                        break;
                    }
                }
                if (found && paneId != 0) break;
            }
            g_App->Invalidate();
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) return 1;

    HICON hAppIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(1));
    HICON hAppIconSm = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(1), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = hAppIcon;
    wc.hIconSm = hAppIconSm;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = L"SpetaTermWindowClass";

    if (!RegisterClassExW(&wc)) return 1;

    auto appPtr = std::make_unique<SpetaTermApp>();
    SpetaTermApp& app = *appPtr;
    g_App = &app;

    HWND hwnd = CreateWindowExW(
        WS_EX_APPWINDOW,
        wc.lpszClassName,
        L"Spet@term: Ultra-Fast Native Terminal v2.0",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1150, 720,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hwnd) return 1;
    if (hAppIcon) SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hAppIcon);
    if (hAppIconSm) SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hAppIconSm);
    app.hwnd = hwnd;
    app.conPty.SetNotificationWindow(hwnd);

    // Modern Frameless Window Styling with per-pixel alpha composition
    MARGINS margins = { -1, -1, -1, -1 };
    DwmExtendFrameIntoClientArea(hwnd, &margins);
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

    DWM_WINDOW_CORNER_PREFERENCE corner = DWMWCP_ROUND;
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(corner));

    BOOL useDarkMode = TRUE;
    DwmSetWindowAttribute(hwnd, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &useDarkMode, sizeof(useDarkMode));

    // Enable Drag and Drop
    DragAcceptFiles(hwnd, TRUE);

    // Register Quake Mode global hotkeys (F12 and Alt+`)
    RegisterHotKey(hwnd, HOTKEY_QUAKE, MOD_NOREPEAT, VK_F12);
    RegisterHotKey(hwnd, HOTKEY_QUAKE + 1, MOD_ALT | MOD_NOREPEAT, VK_OEM_3);

    if (!app.renderer.Initialize(hwnd)) {
        MessageBoxW(hwnd, L"Falha ao inicializar Direct2D / DirectWrite / WIC.", L"Erro Spet@term", MB_ICONERROR);
        return 1;
    }

    if (!app.settings.wallpaperPath.empty()) {
        std::wstring exeDir = AppSettings::GetExeDir();
        std::wstring resolved = ResolveWallpaperPath(app.settings.wallpaperPath, exeDir);
        app.renderer.wallpaperEngine.Load(resolved);
    }

    // Cursor Blink Timer (500ms)
    SetTimer(hwnd, 1, 500, nullptr);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    SetFocus(hwnd);

    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    UnregisterHotKey(hwnd, HOTKEY_QUAKE);
    UnregisterHotKey(hwnd, HOTKEY_QUAKE + 1);
    KillTimer(hwnd, 1);
    int exitCode = static_cast<int>(msg.wParam);
    app.ShutdownPdh();
    g_App = nullptr;
    appPtr.reset();
    SpriteAtlas::Instance().Shutdown();
    CoUninitialize();
    return exitCode;
}

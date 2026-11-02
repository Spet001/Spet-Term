#pragma once

#include "common.hpp"
#include "themes.hpp"
#include "settings.hpp"
#include "terminal_tab.hpp"
#include "syntax_autocomplete.hpp"
#include "git_engine.hpp"
#include "i18n.hpp"
#include "workfolder.hpp"
#include "sprite_atlas.hpp"
#include "wallpaper.hpp"
#include "onboarding.hpp"

#include <d3d11.h>
#include <dxgi1_4.h>
#include <dcomp.h>
#include <d2d1_1.h>

namespace speta {

struct TelemetryData {
    double procCpu = 0.0;
    size_t procRamMb = 0;
    double procGpu = 0.0;
    double sysCpu = 0.0;
    double sysRamUsedGb = 0.0;
    double sysRamTotalGb = 0.0;
    double sysGpu = 0.0;
    double sysTempC = 0.0;
    bool sysTempValid = false;
    double gpuTempC = 0.0;
    bool gpuTempValid = false;
};

class RendererD2D {
public:
    ID2D1Factory* pD2DFactory = nullptr;
    ID2D1Factory1* pD2DFactory1 = nullptr;
    ID2D1RenderTarget* pRenderTarget = nullptr;

    // DirectComposition & D3D11 / DXGI pipeline
    ID3D11Device* pD3DDevice = nullptr;
    ID3D11DeviceContext* pD3DContext = nullptr;
    IDXGISwapChain1* pSwapChain = nullptr;
    ID2D1Device* pD2DDevice = nullptr;
    ID2D1DeviceContext* pD2DContext = nullptr;
    ID2D1Bitmap1* pTargetBitmap = nullptr;
    IDCompositionDevice* pDCompDevice = nullptr;
    IDCompositionTarget* pDCompTarget = nullptr;
    IDCompositionVisual* pDCompVisual = nullptr;
    ID2D1HwndRenderTarget* pHwndRenderTarget = nullptr; // fallback if DComp unavailable
    bool usingDComp = false;
    bool suppressEmptyState = false;
    WallpaperEngine wallpaperEngine;

    IDWriteFactory* pDWriteFactory = nullptr;

    // UI Chrome text formats (proportional fonts for chrome, tabs, menus, modals)
    IDWriteTextFormat* pUIFormat = nullptr;
    IDWriteTextFormat* pUIFormatMedium = nullptr;
    IDWriteTextFormat* pUIFormatBold = nullptr;
    IDWriteTextFormat* pUIFormatSmall = nullptr;
    IDWriteTextFormat* pUIFormatTitle = nullptr;

    // Terminal text formats (monospace fonts for terminal buffer, commands, output)
    IDWriteTextFormat* pTermFormat = nullptr;
    IDWriteTextFormat* pTermFormatBold = nullptr;
    IDWriteTextFormat* pTermFormatSmall = nullptr;

    // Native Windows 11 Fluent Icon text formats (Segoe Fluent Icons / Segoe MDL2 Assets)
    IDWriteTextFormat* pIconFormat = nullptr;
    IDWriteTextFormat* pIconFormatSmall = nullptr;
    IDWriteTextFormat* pIconFormatWindow = nullptr;

    // Compatibility aliases pointing to active formats
    IDWriteTextFormat* pTextFormat = nullptr;
    IDWriteTextFormat* pTextFormatBold = nullptr;
    IDWriteTextFormat* pTextFormatSmall = nullptr;
    IDWriteTextFormat* pTextFormatTitle = nullptr;

    IWICImagingFactory* pWICFactory = nullptr;
    ID2D1SolidColorBrush* pSolidBrush = nullptr;
    ID2D1SolidColorBrush* pBorderBrush = nullptr;

    // Current font settings
    std::wstring currentFontFamily = L"Cascadia Mono";
    float currentFontSize = 13.5f;

    // Galaxy effect resources
    std::vector<Star> stars;
    float galaxyAnimTime = 0.0f;

    // Font metrics
    float charWidth = 9.0f;
    float lineHeight = 20.0f;

    // Cached Rects for Top Bar & Hit Testing
    static constexpr float TITLEBAR_HEIGHT = 36.0f;
    static constexpr float STATUSBAR_HEIGHT = 0.0f;

    D2D1_RECT_F fileBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F newTabBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F dropdownBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F quickRunBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F gitBadgeRect{ 0, 0, 0, 0 };
    D2D1_RECT_F snippetsBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F searchBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F settingsBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F workfolderBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F sidebarToggleBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F sidebarCollapseBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F minBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F maxBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F closeBtnRect{ 0, 0, 0, 0 };

    // Dark Direct2D Sidebar Context Menu
    struct ContextMenuItem {
        D2D1_RECT_F rect;
        int id;
        std::wstring label;
        std::string icon;
    };
    bool sidebarContextMenuOpen = false;
    D2D1_RECT_F sidebarContextMenuRect{ 0, 0, 0, 0 };
    std::wstring contextMenuFilePath;
    std::vector<ContextMenuItem> sidebarContextMenuItems;

    // Prompt Context Rects
    D2D1_RECT_F promptFolderRect{ 0, 0, 0, 0 };
    D2D1_RECT_F promptGitBadgeRect{ 0, 0, 0, 0 };

    // Status Bar Rects
    D2D1_RECT_F statusBarRect{ 0, 0, 0, 0 };
    D2D1_RECT_F statusGitRect{ 0, 0, 0, 0 };
    D2D1_RECT_F statusQuickRunRect{ 0, 0, 0, 0 };
    D2D1_RECT_F statusModeRect{ 0, 0, 0, 0 };
    D2D1_RECT_F statusSnippetsRect{ 0, 0, 0, 0 };

    // Search Box Rects
    D2D1_RECT_F searchBoxRect{ 0, 0, 0, 0 };
    D2D1_RECT_F searchPrevBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F searchNextBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F searchCloseBtnRect{ 0, 0, 0, 0 };

    // Settings Modal Rects
    D2D1_RECT_F settingsModalRect{ 0, 0, 0, 0 };
    D2D1_RECT_F settingsCloseBtnRect{ 0, 0, 0, 0 };
    std::vector<std::pair<D2D1_RECT_F, std::wstring>> settingsThemeBtns;
    D2D1_RECT_F settingsWarmthSliderTrackRect{ 0, 0, 0, 0 };
    D2D1_RECT_F settingsWarmthThumbRect{ 0, 0, 0, 0 };
    D2D1_RECT_F settingsFontMinusRect{ 0, 0, 0, 0 };
    D2D1_RECT_F settingsFontPlusRect{ 0, 0, 0, 0 };
    std::vector<std::pair<D2D1_RECT_F, std::wstring>> settingsFontFamilyBtns;
    std::vector<std::pair<D2D1_RECT_F, std::wstring>> settingsWallpaperBtns;
    D2D1_RECT_F settingsWallpaperAddBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F settingsWallpaperDimTrackRect{ 0, 0, 0, 0 };
    D2D1_RECT_F settingsWallpaperDimThumbRect{ 0, 0, 0, 0 };
    std::vector<std::pair<D2D1_RECT_F, bool>> settingsWallpaperPauseUnfocusedBtns;
    std::vector<std::pair<D2D1_RECT_F, bool>> settingsWallpaperPauseOnBatteryBtns;
    float settingsScrollY = 0.0f;
    float settingsMaxScrollY = 0.0f;
    D2D1_RECT_F settingsClassicModeBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F settingsBlocksModeBtnRect{ 0, 0, 0, 0 };
    std::vector<std::pair<D2D1_RECT_F, std::string>> settingsLanguageBtns;
    std::vector<std::pair<D2D1_RECT_F, int>> settingsCursorBtns;
    std::vector<std::pair<D2D1_RECT_F, int>> settingsSysCursorBtns;
    D2D1_RECT_F settingsRegisterTermBtnRect{ 0, 0, 0, 0 };
    std::vector<std::pair<D2D1_RECT_F, bool>> settingsWarnPasteBtns;
    D2D1_RECT_F settingsViewIntroBtnRect{ 0, 0, 0, 0 };

    // Help Cheat-Sheet Modal Rects
    D2D1_RECT_F helpModalRect{ 0, 0, 0, 0 };
    D2D1_RECT_F helpCloseBtnRect{ 0, 0, 0, 0 };

    // Multi-line Paste Security Modal Rects
    D2D1_RECT_F multiLinePasteModalRect{ 0, 0, 0, 0 };
    D2D1_RECT_F multiLinePasteConfirmBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F multiLinePasteCancelBtnRect{ 0, 0, 0, 0 };

    // Popover Rects & Items
    D2D1_RECT_F fileMenuRect{ 0, 0, 0, 0 };
    std::vector<std::pair<D2D1_RECT_F, int>> fileMenuItems; // item rect & command id

    D2D1_RECT_F profileMenuRect{ 0, 0, 0, 0 };
    std::vector<std::pair<D2D1_RECT_F, int>> profileMenuItems; // item rect & profile id

    D2D1_RECT_F gitPopoverRect{ 0, 0, 0, 0 };
    std::vector<std::pair<D2D1_RECT_F, int>> gitPopoverItems; // action id
    std::vector<std::pair<D2D1_RECT_F, std::wstring>> gitBranchSwitchItems;

    D2D1_RECT_F snippetsPopoverRect{ 0, 0, 0, 0 };
    D2D1_RECT_F snippetsAddCurrentBtnRect{ 0, 0, 0, 0 };
    std::vector<std::pair<D2D1_RECT_F, size_t>> snippetsItemBtns;

    // Workfolder Explorer Modal Rects & Items
    D2D1_RECT_F workfolderModalRect{ 0, 0, 0, 0 };
    D2D1_RECT_F workfolderCloseBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F workfolderRunBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F workfolderExplorerBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F workfolderTabBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F workfolderFooterEscRect{ 0, 0, 0, 0 };
    std::vector<std::pair<D2D1_RECT_F, size_t>> workfolderItemRects;

    // Workspace Explorer Sidebar & Mini IDE Editor
    D2D1_RECT_F workspaceSidebarRect{ 0, 0, 0, 0 };
    D2D1_RECT_F workspaceFilesTabRect{ 0, 0, 0, 0 };
    D2D1_RECT_F workspaceGitTabRect{ 0, 0, 0, 0 };
    int workspaceSidebarActiveTab = 0; // 0 = Files, 1 = Git
    std::vector<std::pair<D2D1_RECT_F, std::wstring>> workspaceFileItemRects;
    std::vector<std::pair<D2D1_RECT_F, std::wstring>> workspaceFolderItemRects;
    D2D1_RECT_F editorSaveBtnRect{ 0, 0, 0, 0 };

    // Contextual Task Runner
    D2D1_RECT_F taskRunnerBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F taskMenuRect{ 0, 0, 0, 0 };
    std::vector<std::pair<D2D1_RECT_F, size_t>> taskMenuItems;

    // Empty State HUD Badges
    std::vector<std::pair<D2D1_RECT_F, int>> emptyStateBadges;

    // Breadcrumbs
    std::vector<std::pair<D2D1_RECT_F, std::wstring>> breadcrumbSegments;
    D2D1_RECT_F breadcrumbPopoverRect{ 0, 0, 0, 0 };
    std::vector<std::pair<D2D1_RECT_F, std::wstring>> breadcrumbPopoverItems;

    // Live Process Pill
    D2D1_RECT_F liveProcessPillRect{ 0, 0, 0, 0 };
    D2D1_RECT_F liveProcessKillBtnRect{ 0, 0, 0, 0 };

    // Output Pinning Split Panel
    D2D1_RECT_F pinnedPanelRect{ 0, 0, 0, 0 };
    D2D1_RECT_F pinnedCloseBtnRect{ 0, 0, 0, 0 };

    // Modular Telemetry Status Bar
    D2D1_RECT_F statusBarThemeRect{ 0, 0, 0, 0 };
    D2D1_RECT_F statusBarWarmthRect{ 0, 0, 0, 0 };
    D2D1_RECT_F statusBarPromptPosRect{ 0, 0, 0, 0 };
    D2D1_RECT_F statusBarCpuRamRect{ 0, 0, 0, 0 };

    // Settings v4 Rects
    std::vector<std::pair<D2D1_RECT_F, std::string>> settingsPromptPositionBtns;
    std::vector<std::pair<D2D1_RECT_F, std::string>> settingsStreamStartBtns;
    std::vector<std::pair<D2D1_RECT_F, float>> settingsWarmthBtns;

    // Timeline History
    D2D1_RECT_F timelineModalRect{ 0, 0, 0, 0 };
    D2D1_RECT_F timelineCloseBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F timelineFilterToggleRect{ 0, 0, 0, 0 };
    D2D1_RECT_F timelineFooterRunRect{ 0, 0, 0, 0 };
    D2D1_RECT_F timelineFooterTabRect{ 0, 0, 0, 0 };
    D2D1_RECT_F timelineFooterEscRect{ 0, 0, 0, 0 };
    std::vector<std::pair<D2D1_RECT_F, size_t>> timelineItemRects;

    // Command Palette
    D2D1_RECT_F paletteModalRect{ 0, 0, 0, 0 };
    D2D1_RECT_F paletteCloseBtnRect{ 0, 0, 0, 0 };
    std::vector<std::pair<D2D1_RECT_F, int>> paletteItemRects;

    // Detected Link Rects in current frame
    struct LinkHitRect {
        D2D1_RECT_F rect;
        std::wstring url;
        bool isUrl;
    };
    std::vector<LinkHitRect> frameLinks;

    RendererD2D() {
        InitGalaxyStars();
    }

    ~RendererD2D() {
        DiscardDeviceResources();
        SafeRelease(pUIFormat);
        SafeRelease(pUIFormatMedium);
        SafeRelease(pUIFormatBold);
        SafeRelease(pUIFormatSmall);
        SafeRelease(pUIFormatTitle);
        SafeRelease(pTermFormat);
        SafeRelease(pTermFormatBold);
        SafeRelease(pTermFormatSmall);
        SafeRelease(pIconFormat);
        SafeRelease(pIconFormatSmall);
        SafeRelease(pIconFormatWindow);
        pTextFormat = nullptr;
        pTextFormatBold = nullptr;
        pTextFormatSmall = nullptr;
        pTextFormatTitle = nullptr;
        SafeRelease(pDWriteFactory);
        SafeRelease(pD2DFactory1);
        SafeRelease(pD2DFactory);
        SafeRelease(pWICFactory);
    }

    bool Initialize(HWND hwnd) {
        MARGINS margins = { -1, -1, -1, -1 };
        DwmExtendFrameIntoClientArea(hwnd, &margins);

        DWM_BLURBEHIND bb = {};
        bb.dwFlags = DWM_BB_ENABLE;
        bb.fEnable = TRUE;
        bb.hRgnBlur = nullptr;
        bb.fTransitionOnMaximized = FALSE;
        DwmEnableBlurBehindWindow(hwnd, &bb);

        HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory);
        if (FAILED(hr)) return false;

        hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&pDWriteFactory));
        if (FAILED(hr)) return false;

        hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pWICFactory));
        if (FAILED(hr)) return false;

        SpriteAtlas::Instance().Initialize(pWICFactory);

        CreateTextFormats(currentFontFamily, currentFontSize);
        return CreateDeviceResources(hwnd);
    }

    bool SetFont(const std::wstring& family, float size) {
        currentFontFamily = family;
        currentFontSize = size;
        CreateTextFormats(family, size);
        MeasureFontMetrics();
        return true;
    }

    void CreateTextFormats(const std::wstring& family, float size) {
        SafeRelease(pUIFormat);
        SafeRelease(pUIFormatMedium);
        SafeRelease(pUIFormatBold);
        SafeRelease(pUIFormatSmall);
        SafeRelease(pUIFormatTitle);
        SafeRelease(pTermFormat);
        SafeRelease(pTermFormatBold);
        SafeRelease(pTermFormatSmall);
        SafeRelease(pIconFormat);
        SafeRelease(pIconFormatSmall);
        SafeRelease(pIconFormatWindow);

        if (!pDWriteFactory) return;

        // 1. Proportional UI Fonts (Segoe UI Variable Text -> Segoe UI -> Arial)
        static const wchar_t* uiFallbacks[] = { L"Segoe UI Variable Text", L"Segoe UI", L"system-ui", L"Arial" };
        const wchar_t* chosenUIFont = L"Segoe UI";
        for (const auto* uif : uiFallbacks) {
            HRESULT hr = pDWriteFactory->CreateTextFormat(
                uif, nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL, 12.0f, L"en-us", &pUIFormat
            );
            if (SUCCEEDED(hr)) {
                chosenUIFont = uif;
                break;
            }
        }
        if (!pUIFormat) {
            pDWriteFactory->CreateTextFormat(
                L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL, 12.0f, L"en-us", &pUIFormat
            );
            chosenUIFont = L"Segoe UI";
        }

        pDWriteFactory->CreateTextFormat(
            chosenUIFont, nullptr, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 12.0f, L"en-us", &pUIFormatMedium
        );
        pDWriteFactory->CreateTextFormat(
            chosenUIFont, nullptr, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 12.0f, L"en-us", &pUIFormatBold
        );
        pDWriteFactory->CreateTextFormat(
            chosenUIFont, nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 11.0f, L"en-us", &pUIFormatSmall
        );
        pDWriteFactory->CreateTextFormat(
            chosenUIFont, nullptr, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 15.0f, L"en-us", &pUIFormatTitle
        );

        // 2. Monospace Terminal Fonts (Requested family -> Cascadia Mono -> Cascadia Code -> Consolas -> Courier New)
        static const wchar_t* termFallbacks[] = { L"Cascadia Mono", L"Cascadia Code", L"Consolas", L"Courier New" };
        const wchar_t* chosenTermFont = family.c_str();

        HRESULT hr = pDWriteFactory->CreateTextFormat(
            chosenTermFont, nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, size, L"en-us", &pTermFormat
        );
        if (FAILED(hr)) {
            for (const auto* f : termFallbacks) {
                hr = pDWriteFactory->CreateTextFormat(
                    f, nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                    DWRITE_FONT_STRETCH_NORMAL, size, L"en-us", &pTermFormat
                );
                if (SUCCEEDED(hr)) {
                    chosenTermFont = f;
                    break;
                }
            }
        }
        if (!pTermFormat) {
            pDWriteFactory->CreateTextFormat(
                L"Cascadia Mono", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL, size, L"en-us", &pTermFormat
            );
            chosenTermFont = L"Cascadia Mono";
        }

        pDWriteFactory->CreateTextFormat(
            chosenTermFont, nullptr, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, size, L"en-us", &pTermFormatBold
        );
        pDWriteFactory->CreateTextFormat(
            chosenTermFont, nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, (std::max)(10.0f, size - 2.5f), L"en-us", &pTermFormatSmall
        );

        // 3. Native Windows 11 Fluent Icon Text Formats (Segoe Fluent Icons -> Segoe MDL2 Assets)
        static const wchar_t* iconFallbacks[] = { L"Segoe Fluent Icons", L"Segoe MDL2 Assets" };
        const wchar_t* chosenIconFont = L"Segoe Fluent Icons";
        for (const auto* icf : iconFallbacks) {
            hr = pDWriteFactory->CreateTextFormat(
                icf, nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL, 10.5f, L"en-us", &pIconFormat
            );
            if (SUCCEEDED(hr)) {
                chosenIconFont = icf;
                break;
            }
        }
        if (!pIconFormat) {
            chosenIconFont = L"Segoe MDL2 Assets";
            pDWriteFactory->CreateTextFormat(
                chosenIconFont, nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL, 10.5f, L"en-us", &pIconFormat
            );
        }
        if (pIconFormat) {
            pIconFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            pIconFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }

        pDWriteFactory->CreateTextFormat(
            chosenIconFont, nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 8.5f, L"en-us", &pIconFormatSmall
        );
        if (pIconFormatSmall) {
            pIconFormatSmall->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            pIconFormatSmall->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }

        pDWriteFactory->CreateTextFormat(
            chosenIconFont, nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"en-us", &pIconFormatWindow
        );
        if (pIconFormatWindow) {
            pIconFormatWindow->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            pIconFormatWindow->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }

        // Set compatibility pointers
        pTextFormat = pTermFormat;
        pTextFormatBold = pTermFormatBold;
        pTextFormatSmall = pTermFormatSmall;
        pTextFormatTitle = pUIFormatTitle;
    }

    bool CreateDeviceResources(HWND hwnd) {
        if (pRenderTarget) return true;

        RECT rc;
        GetClientRect(hwnd, &rc);
        UINT width = static_cast<UINT>((std::max)(1L, rc.right - rc.left));
        UINT height = static_cast<UINT>((std::max)(1L, rc.bottom - rc.top));

        // Attempt DirectComposition with D3D11 & DXGI first for genuine per-pixel alpha blur
        do {
            UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
            D3D_FEATURE_LEVEL featureLevels[] = {
                D3D_FEATURE_LEVEL_11_1,
                D3D_FEATURE_LEVEL_11_0,
                D3D_FEATURE_LEVEL_10_1,
                D3D_FEATURE_LEVEL_10_0
            };
            D3D_FEATURE_LEVEL fl;
            HRESULT hr = D3D11CreateDevice(
                nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, creationFlags,
                featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION,
                &pD3DDevice, &fl, &pD3DContext
            );
            if (FAILED(hr)) break;

            ID3D10Multithread* pMultithread = nullptr;
            if (SUCCEEDED(pD3DContext->QueryInterface(__uuidof(ID3D10Multithread), reinterpret_cast<void**>(&pMultithread))) ||
                SUCCEEDED(pD3DDevice->QueryInterface(__uuidof(ID3D10Multithread), reinterpret_cast<void**>(&pMultithread)))) {
                if (pMultithread) {
                    pMultithread->SetMultithreadProtected(TRUE);
                    pMultithread->Release();
                }
            }

            IDXGIDevice* pDxgiDevice = nullptr;
            hr = pD3DDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&pDxgiDevice));
            if (FAILED(hr)) break;

            IDXGIAdapter* pAdapter = nullptr;
            hr = pDxgiDevice->GetAdapter(&pAdapter);
            if (FAILED(hr)) { SafeRelease(pDxgiDevice); break; }

            IDXGIFactory2* pDxgiFactory = nullptr;
            hr = pAdapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&pDxgiFactory));
            SafeRelease(pAdapter);
            if (FAILED(hr)) { SafeRelease(pDxgiDevice); break; }

            DXGI_SWAP_CHAIN_DESC1 scDesc = {};
            scDesc.Width = width;
            scDesc.Height = height;
            scDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            scDesc.Stereo = FALSE;
            scDesc.SampleDesc.Count = 1;
            scDesc.SampleDesc.Quality = 0;
            scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            scDesc.BufferCount = 2;
            scDesc.Scaling = DXGI_SCALING_STRETCH;
            scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
            scDesc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

            hr = pDxgiFactory->CreateSwapChainForComposition(pD3DDevice, &scDesc, nullptr, &pSwapChain);
            SafeRelease(pDxgiFactory);
            if (FAILED(hr)) { SafeRelease(pDxgiDevice); break; }

            if (!pD2DFactory1) {
                pD2DFactory->QueryInterface(__uuidof(ID2D1Factory1), reinterpret_cast<void**>(&pD2DFactory1));
            }
            if (!pD2DFactory1) { SafeRelease(pDxgiDevice); break; }

            hr = pD2DFactory1->CreateDevice(pDxgiDevice, &pD2DDevice);
            if (FAILED(hr)) { SafeRelease(pDxgiDevice); break; }

            hr = pD2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &pD2DContext);
            if (FAILED(hr)) { SafeRelease(pDxgiDevice); break; }

            IDXGISurface* pDxgiBackBuffer = nullptr;
            hr = pSwapChain->GetBuffer(0, __uuidof(IDXGISurface), reinterpret_cast<void**>(&pDxgiBackBuffer));
            if (FAILED(hr)) { SafeRelease(pDxgiDevice); break; }

            D2D1_BITMAP_PROPERTIES1 bp = D2D1::BitmapProperties1(
                D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
            );
            hr = pD2DContext->CreateBitmapFromDxgiSurface(pDxgiBackBuffer, &bp, &pTargetBitmap);
            SafeRelease(pDxgiBackBuffer);
            if (FAILED(hr) || !pTargetBitmap) { SafeRelease(pDxgiDevice); break; }

            pD2DContext->SetTarget(pTargetBitmap);

            hr = DCompositionCreateDevice(pDxgiDevice, __uuidof(IDCompositionDevice), reinterpret_cast<void**>(&pDCompDevice));
            SafeRelease(pDxgiDevice);
            if (FAILED(hr)) break;

            hr = pDCompDevice->CreateTargetForHwnd(hwnd, TRUE, &pDCompTarget);
            if (FAILED(hr)) break;

            hr = pDCompDevice->CreateVisual(&pDCompVisual);
            if (FAILED(hr)) break;

            pDCompVisual->SetContent(pSwapChain);
            pDCompTarget->SetRoot(pDCompVisual);
            pDCompDevice->Commit();

            pRenderTarget = pD2DContext;
            usingDComp = true;
            wallpaperEngine.Initialize(pD3DDevice, pD3DContext, pD2DContext, pWICFactory, hwnd);
            pRenderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
            hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), &pSolidBrush);
            if (FAILED(hr)) break;
            hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), &pBorderBrush);
            if (FAILED(hr)) break;

            MeasureFontMetrics();
            return true;
        } while (false);

        // Fallback to standard ID2D1HwndRenderTarget
        DiscardDeviceResources();
        D2D1_SIZE_U size = D2D1::SizeU(width, height);
        HRESULT hr = pD2DFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(
                D2D1_RENDER_TARGET_TYPE_DEFAULT,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
            ),
            D2D1::HwndRenderTargetProperties(hwnd, size, D2D1_PRESENT_OPTIONS_IMMEDIATELY),
            &pHwndRenderTarget
        );
        if (FAILED(hr)) return false;

        pRenderTarget = pHwndRenderTarget;
        usingDComp = false;
        pRenderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
        hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), &pSolidBrush);
        if (FAILED(hr)) return false;
        hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), &pBorderBrush);
        if (FAILED(hr)) return false;

        MeasureFontMetrics();
        return true;
    }

    void DiscardDeviceResources() {
        SpriteAtlas::Instance().DiscardDeviceResources();
        SafeRelease(pSolidBrush);
        SafeRelease(pBorderBrush);
        wallpaperEngine.Shutdown();
        if (usingDComp) {
            if (pD2DContext) pD2DContext->SetTarget(nullptr);
            SafeRelease(pTargetBitmap);
            SafeRelease(pD2DContext);
            SafeRelease(pD2DDevice);
            SafeRelease(pDCompVisual);
            SafeRelease(pDCompTarget);
            SafeRelease(pDCompDevice);
            SafeRelease(pSwapChain);
            SafeRelease(pD3DContext);
            SafeRelease(pD3DDevice);
        } else {
            SafeRelease(pHwndRenderTarget);
        }
        pRenderTarget = nullptr;
        usingDComp = false;
    }

    void Resize(UINT width, UINT height) {
        if (width == 0 || height == 0) return;
        if (usingDComp && pSwapChain && pD2DContext) {
            pD2DContext->SetTarget(nullptr);
            SafeRelease(pTargetBitmap);
            HRESULT hr = pSwapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_B8G8R8A8_UNORM, 0);
            if (SUCCEEDED(hr)) {
                IDXGISurface* pDxgiBackBuffer = nullptr;
                hr = pSwapChain->GetBuffer(0, __uuidof(IDXGISurface), reinterpret_cast<void**>(&pDxgiBackBuffer));
                if (SUCCEEDED(hr) && pDxgiBackBuffer) {
                    D2D1_BITMAP_PROPERTIES1 bp = D2D1::BitmapProperties1(
                        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
                    );
                    pD2DContext->CreateBitmapFromDxgiSurface(pDxgiBackBuffer, &bp, &pTargetBitmap);
                    pD2DContext->SetTarget(pTargetBitmap);
                    SafeRelease(pDxgiBackBuffer);
                }
            }
        } else if (pHwndRenderTarget) {
            pHwndRenderTarget->Resize(D2D1::SizeU(width, height));
        }
    }

    ID2D1Bitmap* LoadBitmapFromFile(const std::wstring& path, D2D1_SIZE_F& outSize) {
        if (!pRenderTarget || !pWICFactory) return nullptr;

        IWICBitmapDecoder* pDecoder = nullptr;
        HRESULT hr = pWICFactory->CreateDecoderFromFilename(
            path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &pDecoder
        );
        if (FAILED(hr)) return nullptr;

        IWICBitmapFrameDecode* pFrame = nullptr;
        hr = pDecoder->GetFrame(0, &pFrame);
        if (FAILED(hr)) {
            SafeRelease(pDecoder);
            return nullptr;
        }

        IWICFormatConverter* pConverter = nullptr;
        hr = pWICFactory->CreateFormatConverter(&pConverter);
        if (FAILED(hr)) {
            SafeRelease(pFrame);
            SafeRelease(pDecoder);
            return nullptr;
        }

        hr = pConverter->Initialize(
            pFrame, GUID_WICPixelFormat32bppPBGRA,
            WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom
        );
        if (FAILED(hr)) {
            SafeRelease(pConverter);
            SafeRelease(pFrame);
            SafeRelease(pDecoder);
            return nullptr;
        }

        ID2D1Bitmap* pBitmap = nullptr;
        hr = pRenderTarget->CreateBitmapFromWicBitmap(pConverter, nullptr, &pBitmap);

        SafeRelease(pConverter);
        SafeRelease(pFrame);
        SafeRelease(pDecoder);

        if (SUCCEEDED(hr) && pBitmap) {
            outSize = pBitmap->GetSize();
            return pBitmap;
        }
        return nullptr;
    }

    void BeginDraw() {
        if (usingDComp && pD2DContext && pTargetBitmap) {
            pD2DContext->SetTarget(pTargetBitmap);
        }
        if (pRenderTarget) {
            pRenderTarget->BeginDraw();
            frameLinks.clear();
        }
    }

    HRESULT EndDraw() {
        if (!pRenderTarget) return S_OK;
        HRESULT hr = pRenderTarget->EndDraw();
        if (SUCCEEDED(hr) && usingDComp && pSwapChain) {
            HRESULT phr = pSwapChain->Present(1, 0);
            if (phr == DXGI_ERROR_DEVICE_REMOVED || phr == DXGI_ERROR_DEVICE_RESET) return phr;
        }
        return hr;
    }

    void FillAdaptiveBox(
        const D2D1_RECT_F& r,
        const Theme& theme,
        ID2D1Brush* fillBrush,
        ID2D1Brush* borderBrush,
        float radiusOrChamfer = 6.0f,
        bool drawCornerBrackets = false
    ) {
        if (!pRenderTarget) return;

        if (theme.geometry == GeometryStyle::BrutalistChamfer) {
            float c = (std::min)({ radiusOrChamfer, (r.right - r.left) * 0.45f, (r.bottom - r.top) * 0.45f });
            if (c < 1.0f) c = 1.0f;

            ID2D1PathGeometry* path = nullptr;
            if (pD2DFactory && SUCCEEDED(pD2DFactory->CreatePathGeometry(&path)) && path) {
                ID2D1GeometrySink* sink = nullptr;
                if (SUCCEEDED(path->Open(&sink)) && sink) {
                    sink->BeginFigure(D2D1::Point2F(r.left + c, r.top), D2D1_FIGURE_BEGIN_FILLED);
                    sink->AddLine(D2D1::Point2F(r.right - c, r.top));
                    sink->AddLine(D2D1::Point2F(r.right, r.top + c));
                    sink->AddLine(D2D1::Point2F(r.right, r.bottom - c));
                    sink->AddLine(D2D1::Point2F(r.right - c, r.bottom));
                    sink->AddLine(D2D1::Point2F(r.left + c, r.bottom));
                    sink->AddLine(D2D1::Point2F(r.left, r.bottom - c));
                    sink->AddLine(D2D1::Point2F(r.left, r.top + c));
                    sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                    sink->Close();
                    sink->Release();

                    if (fillBrush) pRenderTarget->FillGeometry(path, fillBrush);
                    if (borderBrush) pRenderTarget->DrawGeometry(path, borderBrush, 1.0f);
                }
                path->Release();
            }

            if (drawCornerBrackets) {
                float bSize = 6.0f;
                pSolidBrush->SetColor(theme.accent);
                pRenderTarget->DrawLine(D2D1::Point2F(r.left, r.top), D2D1::Point2F(r.left + bSize, r.top), pSolidBrush, 1.5f);
                pRenderTarget->DrawLine(D2D1::Point2F(r.left, r.top), D2D1::Point2F(r.left, r.top + bSize), pSolidBrush, 1.5f);
                pRenderTarget->DrawLine(D2D1::Point2F(r.right, r.top), D2D1::Point2F(r.right - bSize, r.top), pSolidBrush, 1.5f);
                pRenderTarget->DrawLine(D2D1::Point2F(r.right, r.top), D2D1::Point2F(r.right, r.top + bSize), pSolidBrush, 1.5f);
                pRenderTarget->DrawLine(D2D1::Point2F(r.left, r.bottom), D2D1::Point2F(r.left + bSize, r.bottom), pSolidBrush, 1.5f);
                pRenderTarget->DrawLine(D2D1::Point2F(r.left, r.bottom), D2D1::Point2F(r.left, r.bottom - bSize), pSolidBrush, 1.5f);
                pRenderTarget->DrawLine(D2D1::Point2F(r.right, r.bottom), D2D1::Point2F(r.right - bSize, r.bottom), pSolidBrush, 1.5f);
                pRenderTarget->DrawLine(D2D1::Point2F(r.right, r.bottom), D2D1::Point2F(r.right, r.bottom - bSize), pSolidBrush, 1.5f);
            }
        } else {
            D2D1_ROUNDED_RECT rr = D2D1::RoundedRect(r, radiusOrChamfer, radiusOrChamfer);
            if (fillBrush) pRenderTarget->FillRoundedRectangle(rr, fillBrush);
            if (borderBrush) pRenderTarget->DrawRoundedRectangle(rr, borderBrush, 1.0f);

            pSolidBrush->SetColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.14f));
            pRenderTarget->DrawLine(
                D2D1::Point2F(r.left + radiusOrChamfer, r.top + 0.5f),
                D2D1::Point2F(r.right - radiusOrChamfer, r.top + 0.5f),
                pSolidBrush, 1.0f
            );
        }
    }

    void FillAdaptiveBox(
        const D2D1_RECT_F& r,
        const Theme& theme,
        const D2D1_COLOR_F& fillColor,
        const D2D1_COLOR_F& borderColor,
        float radiusOrChamfer = 6.0f,
        bool drawCornerBrackets = false
    ) {
        if (!pRenderTarget) return;
        if (!pBorderBrush) {
            pRenderTarget->CreateSolidColorBrush(borderColor, &pBorderBrush);
        }
        if (pSolidBrush && pBorderBrush) {
            pSolidBrush->SetColor(fillColor);
            pBorderBrush->SetColor(borderColor);
            FillAdaptiveBox(r, theme, pSolidBrush, pBorderBrush, radiusOrChamfer, drawCornerBrackets);
        }
    }

    void FillAdaptiveBox(
        const Theme& theme,
        const D2D1_RECT_F& r,
        const D2D1_COLOR_F& fillColor,
        const D2D1_COLOR_F& borderColor,
        float radiusOrChamfer = 6.0f,
        bool drawCornerBrackets = false
    ) {
        FillAdaptiveBox(r, theme, fillColor, borderColor, radiusOrChamfer, drawCornerBrackets);
    }

    void FillAdaptiveBox(
        const Theme& theme,
        const D2D1_RECT_F& r,
        ID2D1Brush* fillBrush,
        ID2D1Brush* borderBrush,
        float radiusOrChamfer = 6.0f,
        bool drawCornerBrackets = false
    ) {
        FillAdaptiveBox(r, theme, fillBrush, borderBrush, radiusOrChamfer, drawCornerBrackets);
    }

    void RenderCustomCursor(const Theme& theme, POINT mp, int mode) {
        if (!pRenderTarget) return;
        float x = static_cast<float>(mp.x), y = static_cast<float>(mp.y);
        D2D1_COLOR_F dark = ColorHex(0x08080C);
        if (mode == 2) {
            // I-beam
            pSolidBrush->SetColor(dark);
            pRenderTarget->FillRectangle(D2D1::RectF(x - 3.0f, y - 9.0f, x + 4.0f, y - 7.0f), pSolidBrush);
            pRenderTarget->FillRectangle(D2D1::RectF(x - 3.0f, y + 7.0f, x + 4.0f, y + 9.0f), pSolidBrush);
            pRenderTarget->FillRectangle(D2D1::RectF(x - 0.5f - 1.0f, y - 9.0f, x + 2.0f, y + 9.0f), pSolidBrush);
            pSolidBrush->SetColor(theme.accent);
            pRenderTarget->FillRectangle(D2D1::RectF(x - 2.0f, y - 8.0f, x + 3.0f, y - 7.0f), pSolidBrush);
            pRenderTarget->FillRectangle(D2D1::RectF(x - 2.0f, y + 7.0f, x + 3.0f, y + 8.0f), pSolidBrush);
            pRenderTarget->FillRectangle(D2D1::RectF(x, y - 8.0f, x + 1.0f, y + 8.0f), pSolidBrush);
            return;
        }
        // Tactical arrow, chamfered tail
        ID2D1PathGeometry* geo = nullptr;
        if (pD2DFactory && SUCCEEDED(pD2DFactory->CreatePathGeometry(&geo)) && geo) {
            ID2D1GeometrySink* sink = nullptr;
            if (SUCCEEDED(geo->Open(&sink)) && sink) {
                sink->BeginFigure(D2D1::Point2F(x, y), D2D1_FIGURE_BEGIN_FILLED);
                sink->AddLine(D2D1::Point2F(x, y + 16.0f));
                sink->AddLine(D2D1::Point2F(x + 4.0f, y + 12.0f));
                sink->AddLine(D2D1::Point2F(x + 7.5f, y + 19.0f));
                sink->AddLine(D2D1::Point2F(x + 10.0f, y + 18.0f));
                sink->AddLine(D2D1::Point2F(x + 6.5f, y + 11.0f));
                sink->AddLine(D2D1::Point2F(x + 12.0f, y + 11.0f));
                sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                sink->Close();
                sink->Release();
                pSolidBrush->SetColor(mode == 1 ? theme.accent : ColorHex(0xF2F2FA));
                pRenderTarget->FillGeometry(geo, pSolidBrush);
                pSolidBrush->SetColor(dark);
                pRenderTarget->DrawGeometry(geo, pSolidBrush, 1.2f);
            }
            geo->Release();
        }
        if (mode == 1) {
            // Reticle brackets around the hotspot signal a clickable target
            pSolidBrush->SetColor(theme.accent);
            float bx = x + 14.0f, by = y + 14.0f;
            pRenderTarget->DrawLine(D2D1::Point2F(bx, by), D2D1::Point2F(bx + 5.0f, by), pSolidBrush, 1.5f);
            pRenderTarget->DrawLine(D2D1::Point2F(bx, by), D2D1::Point2F(bx, by + 5.0f), pSolidBrush, 1.5f);
        }
    }
    void RenderPostProcessingOverlays(const Theme& theme, const AppSettings& settings, float width, float height) {
        if (!pRenderTarget) return;

        // 1. CRT Scanlines
        if (theme.crtScanlines) {
            pSolidBrush->SetColor(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.22f));
            for (float y = 0.0f; y < height; y += 3.0f) {
                pRenderTarget->DrawLine(D2D1::Point2F(0.0f, y), D2D1::Point2F(width, y), pSolidBrush, 1.0f);
            }
        }

        // 2. CRT Vignette
        if (theme.crtVignette) {
            float vSize = 48.0f;
            pSolidBrush->SetColor(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.12f));
            pRenderTarget->FillRectangle(D2D1::RectF(0, 0, width, vSize * 0.5f), pSolidBrush);
            pRenderTarget->FillRectangle(D2D1::RectF(0, height - vSize * 0.5f, width, height), pSolidBrush);
            pRenderTarget->FillRectangle(D2D1::RectF(0, 0, vSize * 0.5f, height), pSolidBrush);
            pRenderTarget->FillRectangle(D2D1::RectF(width - vSize * 0.5f, 0, width, height), pSolidBrush);

            pSolidBrush->SetColor(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.24f));
            pRenderTarget->FillRectangle(D2D1::RectF(0, 0, vSize, vSize * 0.5f), pSolidBrush);
            pRenderTarget->FillRectangle(D2D1::RectF(width - vSize, 0, width, vSize * 0.5f), pSolidBrush);
            pRenderTarget->FillRectangle(D2D1::RectF(0, height - vSize * 0.5f, vSize, height), pSolidBrush);
            pRenderTarget->FillRectangle(D2D1::RectF(width - vSize, height - vSize * 0.5f, width, height), pSolidBrush);
        }

        // 3. Global Anti-Blue-Light Warmth Filter (Calibrated smooth amber curve)
        if (settings.warmthLevel > 0.005f) {
            float alpha = settings.warmthLevel * 0.08f;
            pSolidBrush->SetColor(D2D1::ColorF(1.0f, 0.72f, 0.38f, alpha));
            pRenderTarget->FillRectangle(D2D1::RectF(0.0f, 0.0f, width, height), pSolidBrush);
        }
    }

    void DrawFluentIcon(wchar_t glyph, const D2D1_RECT_F& rect, D2D1_COLOR_F color, IDWriteTextFormat* fmt = nullptr) {
        if (!pRenderTarget) return;
        if (!fmt) fmt = pIconFormat;
        color.a = 1.0f;
        pSolidBrush->SetColor(color);
        wchar_t buf[2] = { glyph, 0 };
        pRenderTarget->DrawTextW(buf, 1, fmt, rect, pSolidBrush);
    }

    void DrawFluentIcon(const wchar_t* glyphStr, const D2D1_RECT_F& rect, D2D1_COLOR_F color, IDWriteTextFormat* fmt = nullptr) {
        if (!glyphStr || !glyphStr[0]) return;
        DrawFluentIcon(glyphStr[0], rect, color, fmt);
    }

    void DrawFluentIcon(wchar_t glyph, float x, float y, float size, D2D1_COLOR_F color, IDWriteTextFormat* fmt = nullptr) {
        DrawFluentIcon(glyph, D2D1::RectF(x, y, x + size, y + size), color, fmt);
    }

    void DrawFluentIcon(const wchar_t* glyphStr, float x, float y, float size, D2D1_COLOR_F color, IDWriteTextFormat* fmt = nullptr) {
        if (!glyphStr || !glyphStr[0]) return;
        DrawFluentIcon(glyphStr[0], D2D1::RectF(x, y, x + size, y + size), color, fmt);
    }

    void DrawFluentIcon(wchar_t glyph, float x, float y, float w, float h, D2D1_COLOR_F color, IDWriteTextFormat* fmt = nullptr) {
        DrawFluentIcon(glyph, D2D1::RectF(x, y, x + w, y + h), color, fmt);
    }

    void DrawFluentIcon(const wchar_t* glyphStr, float x, float y, float w, float h, D2D1_COLOR_F color, IDWriteTextFormat* fmt = nullptr) {
        if (!glyphStr || !glyphStr[0]) return;
        DrawFluentIcon(glyphStr[0], D2D1::RectF(x, y, x + w, y + h), color, fmt);
    }

    void DrawWindowBorder(float width, float height) {
        if (!pRenderTarget) return;
        pSolidBrush->SetColor(ColorHex(0x26262E));
        pRenderTarget->DrawRectangle(D2D1::RectF(0.5f, 0.5f, width - 0.5f, height - 0.5f), pSolidBrush, 1.0f);
    }

    void DrawShieldIcon(float x, float y, float size, D2D1_COLOR_F color, bool fill = false) {
        pSolidBrush->SetColor(color);
        float w = size;
        float h = size * 1.15f;

        // Draw shield contour using clean line segments & arcs
        D2D1_POINT_2F p1 = D2D1::Point2F(x, y + 2.0f);
        D2D1_POINT_2F p2 = D2D1::Point2F(x + w * 0.5f, y);
        D2D1_POINT_2F p3 = D2D1::Point2F(x + w, y + 2.0f);
        D2D1_POINT_2F p4 = D2D1::Point2F(x + w, y + h * 0.55f);
        D2D1_POINT_2F p5 = D2D1::Point2F(x + w * 0.5f, y + h);
        D2D1_POINT_2F p6 = D2D1::Point2F(x, y + h * 0.55f);

        pRenderTarget->DrawLine(p1, p2, pSolidBrush, 1.4f);
        pRenderTarget->DrawLine(p2, p3, pSolidBrush, 1.4f);
        pRenderTarget->DrawLine(p3, p4, pSolidBrush, 1.4f);
        pRenderTarget->DrawLine(p4, p5, pSolidBrush, 1.4f);
        pRenderTarget->DrawLine(p5, p6, pSolidBrush, 1.4f);
        pRenderTarget->DrawLine(p6, p1, pSolidBrush, 1.4f);

        // Inner vertical line for heraldic emblem look
        pRenderTarget->DrawLine(p2, p5, pSolidBrush, 1.0f);
    }

    void DrawGitBranchIcon(float x, float y, float size, D2D1_COLOR_F color) {
        pSolidBrush->SetColor(color);
        float r = 2.4f;

        // Root circle
        pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x + 4.0f, y + size - 3.0f), r, r), pSolidBrush);
        // Top circle
        pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x + 4.0f, y + 3.0f), r, r), pSolidBrush);
        // Branch circle
        pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x + size - 2.0f, y + 5.0f), r, r), pSolidBrush);

        // Trunk line
        pRenderTarget->DrawLine(D2D1::Point2F(x + 4.0f, y + size - 3.0f), D2D1::Point2F(x + 4.0f, y + 3.0f), pSolidBrush, 1.5f);
        // Branch diagonal
        pRenderTarget->DrawLine(D2D1::Point2F(x + 4.0f, y + size * 0.60f), D2D1::Point2F(x + size - 2.0f, y + 5.0f), pSolidBrush, 1.5f);
    }

    void DrawLightningIcon(float x, float y, float size, D2D1_COLOR_F color) {
        pSolidBrush->SetColor(color);
        D2D1_POINT_2F p1 = D2D1::Point2F(x + size * 0.65f, y);
        D2D1_POINT_2F p2 = D2D1::Point2F(x + size * 0.20f, y + size * 0.50f);
        D2D1_POINT_2F p3 = D2D1::Point2F(x + size * 0.55f, y + size * 0.50f);
        D2D1_POINT_2F p4 = D2D1::Point2F(x + size * 0.35f, y + size);
        D2D1_POINT_2F p5 = D2D1::Point2F(x + size * 0.85f, y + size * 0.40f);
        D2D1_POINT_2F p6 = D2D1::Point2F(x + size * 0.50f, y + size * 0.40f);

        pRenderTarget->DrawLine(p1, p2, pSolidBrush, 1.5f);
        pRenderTarget->DrawLine(p2, p3, pSolidBrush, 1.5f);
        pRenderTarget->DrawLine(p3, p4, pSolidBrush, 1.5f);
        pRenderTarget->DrawLine(p4, p5, pSolidBrush, 1.5f);
        pRenderTarget->DrawLine(p5, p6, pSolidBrush, 1.5f);
        pRenderTarget->DrawLine(p6, p1, pSolidBrush, 1.5f);
    }

    void DrawPromptIcon(float x, float y, float size, D2D1_COLOR_F color) {
        pSolidBrush->SetColor(color);
        // Chevron >
        pRenderTarget->DrawLine(D2D1::Point2F(x + 2.0f, y + 2.0f), D2D1::Point2F(x + size * 0.55f, y + size * 0.5f), pSolidBrush, 1.6f);
        pRenderTarget->DrawLine(D2D1::Point2F(x + size * 0.55f, y + size * 0.5f), D2D1::Point2F(x + 2.0f, y + size - 2.0f), pSolidBrush, 1.6f);
        // Underscore _
        pRenderTarget->DrawLine(D2D1::Point2F(x + size * 0.65f, y + size - 2.0f), D2D1::Point2F(x + size, y + size - 2.0f), pSolidBrush, 1.6f);
    }

    void DrawPlayIcon(float x, float y, float size, D2D1_COLOR_F color) {
        pSolidBrush->SetColor(color);
        D2D1_POINT_2F p1 = D2D1::Point2F(x + 2.0f, y + 1.5f);
        D2D1_POINT_2F p2 = D2D1::Point2F(x + size - 1.0f, y + size * 0.5f);
        D2D1_POINT_2F p3 = D2D1::Point2F(x + 2.0f, y + size - 1.5f);

        ID2D1PathGeometry* path = nullptr;
        if (pD2DFactory && SUCCEEDED(pD2DFactory->CreatePathGeometry(&path)) && path) {
            ID2D1GeometrySink* sink = nullptr;
            if (SUCCEEDED(path->Open(&sink)) && sink) {
                sink->BeginFigure(p1, D2D1_FIGURE_BEGIN_FILLED);
                sink->AddLine(p2);
                sink->AddLine(p3);
                sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                sink->Close();
                SafeRelease(sink);
                pRenderTarget->FillGeometry(path, pSolidBrush);
            }
            SafeRelease(path);
        } else {
            pRenderTarget->DrawLine(p1, p2, pSolidBrush, 1.4f);
            pRenderTarget->DrawLine(p2, p3, pSolidBrush, 1.4f);
            pRenderTarget->DrawLine(p3, p1, pSolidBrush, 1.4f);
        }
    }

    void DrawHamburgerIcon(float x, float y, float size, D2D1_COLOR_F color) {
        pSolidBrush->SetColor(color);
        float lineW = size * 0.85f;
        float startX = x + (size - lineW) * 0.5f;
        float y1 = y + size * 0.22f;
        float y2 = y + size * 0.50f;
        float y3 = y + size * 0.78f;
        pRenderTarget->DrawLine(D2D1::Point2F(startX, y1), D2D1::Point2F(startX + lineW, y1), pSolidBrush, 1.4f);
        pRenderTarget->DrawLine(D2D1::Point2F(startX, y2), D2D1::Point2F(startX + lineW, y2), pSolidBrush, 1.4f);
        pRenderTarget->DrawLine(D2D1::Point2F(startX, y3), D2D1::Point2F(startX + lineW, y3), pSolidBrush, 1.4f);
    }

    void DrawChevronDownIcon(float x, float y, float size, D2D1_COLOR_F color) {
        pSolidBrush->SetColor(color);
        float cx = x + size * 0.5f;
        float cy = y + size * 0.52f;
        float w = size * 0.30f;
        float h = size * 0.18f;
        pRenderTarget->DrawLine(D2D1::Point2F(cx - w, cy - h), D2D1::Point2F(cx, cy + h), pSolidBrush, 1.4f);
        pRenderTarget->DrawLine(D2D1::Point2F(cx, cy + h), D2D1::Point2F(cx + w, cy - h), pSolidBrush, 1.4f);
    }

    void DrawChevronLeftIcon(float x, float y, float size, D2D1_COLOR_F color) {
        pSolidBrush->SetColor(color);
        float cx = x + size * 0.5f;
        float cy = y + size * 0.5f;
        float w = size * 0.22f;
        float h = size * 0.32f;
        pRenderTarget->DrawLine(D2D1::Point2F(cx + w, cy - h), D2D1::Point2F(cx - w, cy), pSolidBrush, 1.4f);
        pRenderTarget->DrawLine(D2D1::Point2F(cx - w, cy), D2D1::Point2F(cx + w, cy + h), pSolidBrush, 1.4f);
    }

    void DrawChevronRightIcon(float x, float y, float size, D2D1_COLOR_F color) {
        pSolidBrush->SetColor(color);
        float cx = x + size * 0.5f;
        float cy = y + size * 0.5f;
        float w = size * 0.22f;
        float h = size * 0.32f;
        pRenderTarget->DrawLine(D2D1::Point2F(cx - w, cy - h), D2D1::Point2F(cx + w, cy), pSolidBrush, 1.4f);
        pRenderTarget->DrawLine(D2D1::Point2F(cx + w, cy), D2D1::Point2F(cx - w, cy + h), pSolidBrush, 1.4f);
    }

    void DrawBookmarkIcon(float x, float y, float size, D2D1_COLOR_F color) {
        pSolidBrush->SetColor(color);
        float w = size * 0.65f;
        float h = size * 0.85f;
        float sx = x + (size - w) * 0.5f;
        float sy = y + (size - h) * 0.5f;
        D2D1_POINT_2F p1 = D2D1::Point2F(sx, sy);
        D2D1_POINT_2F p2 = D2D1::Point2F(sx + w, sy);
        D2D1_POINT_2F p3 = D2D1::Point2F(sx + w, sy + h);
        D2D1_POINT_2F p4 = D2D1::Point2F(sx + w * 0.5f, sy + h - w * 0.38f);
        D2D1_POINT_2F p5 = D2D1::Point2F(sx, sy + h);
        pRenderTarget->DrawLine(p1, p2, pSolidBrush, 1.3f);
        pRenderTarget->DrawLine(p2, p3, pSolidBrush, 1.3f);
        pRenderTarget->DrawLine(p3, p4, pSolidBrush, 1.3f);
        pRenderTarget->DrawLine(p4, p5, pSolidBrush, 1.3f);
        pRenderTarget->DrawLine(p5, p1, pSolidBrush, 1.3f);
    }

    void DrawSearchIcon(float x, float y, float size, D2D1_COLOR_F color) {
        pSolidBrush->SetColor(color);
        float r = size * 0.34f;
        pRenderTarget->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(x + r + 2.0f, y + r + 2.0f), r, r), pSolidBrush, 1.5f);
        pRenderTarget->DrawLine(
            D2D1::Point2F(x + r * 1.9f, y + r * 1.9f),
            D2D1::Point2F(x + size, y + size),
            pSolidBrush, 1.8f
        );
    }

    void DrawGearIcon(float x, float y, float size, D2D1_COLOR_F color) {
        pSolidBrush->SetColor(color);
        float cx = x + size * 0.5f;
        float cy = y + size * 0.5f;
        float rOuter = size * 0.45f;
        float rRoot = size * 0.33f;
        float rHole = size * 0.16f;

        // Central cutout hole (empty center, authentic bore)
        pRenderTarget->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), rHole, rHole), pSolidBrush, 1.3f);
        // Rim body
        pRenderTarget->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), rRoot, rRoot), pSolidBrush, 1.2f);

        // 6 cog teeth evenly spaced around perimeter
        for (int i = 0; i < 6; ++i) {
            float angle = i * 1.04719755f; // 60 deg in radians (2 * PI / 6)
            float cosA = cosf(angle);
            float sinA = sinf(angle);
            D2D1_POINT_2F pFrom = D2D1::Point2F(cx + rRoot * cosA, cy + rRoot * sinA);
            D2D1_POINT_2F pTo   = D2D1::Point2F(cx + rOuter * cosA, cy + rOuter * sinA);
            pRenderTarget->DrawLine(pFrom, pTo, pSolidBrush, 2.4f);
        }
    }

    std::wstring FormatPromptPath(const std::wstring& fullPath, float maxPixelWidth) {
        if (fullPath.empty()) return fullPath;
        std::wstring p = fullPath;
        wchar_t userProf[MAX_PATH] = { 0 };
        DWORD len = GetEnvironmentVariableW(L"USERPROFILE", userProf, MAX_PATH);
        if (len > 0 && StartsWith(ToLower(p), ToLower(std::wstring(userProf)))) {
            p = L"~" + p.substr(len);
        }
        if (maxPixelWidth > 0.0f) {
            float w = MeasureStringWidth(p, pTermFormat);
            if (w > maxPixelWidth) {
                size_t firstSlash = p.find_first_of(L"\\/");
                size_t lastSlash = p.find_last_of(L"\\/");
                if (firstSlash != std::wstring::npos && lastSlash != std::wstring::npos && lastSlash > firstSlash) {
                    std::wstring compact = p.substr(0, firstSlash + 1) + L"...\\" + p.substr(lastSlash + 1);
                    if (MeasureStringWidth(compact, pTermFormat) <= maxPixelWidth) {
                        return compact;
                    }
                    std::wstring base = p.substr(lastSlash + 1);
                    return L"...\\" + base;
                }
            }
        }
        return p;
    }

    void RenderUnifiedTitleBar(
        const Theme& theme,
        float width,
        const std::vector<std::unique_ptr<TerminalTab>>& tabs,
        int activeTabIndex,
        int renamingTabIndex,
        const std::wstring& renamingText,
        bool isAdmin,
        POINT mousePos,
        bool isMaximized,
        bool snippetsOpen = false,
        const std::vector<ProjectTask>& detectedTasks = {},
        bool sidebarOpen = false
    ) {
        D2D1_RECT_F barRect = D2D1::RectF(0, 0, width, TITLEBAR_HEIGHT);

        pSolidBrush->SetColor(ColorHex(0x16161E));
        pRenderTarget->FillRectangle(barRect, pSolidBrush);

        pSolidBrush->SetColor(ColorHex(0x282836));
        pRenderTarget->DrawLine(D2D1::Point2F(0, TITLEBAR_HEIGHT - 0.5f), D2D1::Point2F(width, TITLEBAR_HEIGHT - 0.5f), pSolidBrush, 1.0f);

        fileBtnRect = D2D1::RectF(6.0f, 5.0f, 36.0f, 31.0f);
        bool fileHover = PtInD2D1Rect(fileBtnRect, mousePos);
        if (fileHover) {
            pSolidBrush->SetColor(ColorRgba(1.0f, 1.0f, 1.0f, 0.08f));
            pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(fileBtnRect, 4.0f, 4.0f), pSolidBrush);
        }
        SpriteAtlas::Instance().DrawSpriteCentered(pRenderTarget, SpriteId::TerminalWindow, fileBtnRect, 18.0f, 18.0f);

        workfolderBtnRect = D2D1::RectF(40.0f, 5.0f, 70.0f, 31.0f);
        bool wfHover = PtInD2D1Rect(workfolderBtnRect, mousePos);
        if (sidebarOpen || wfHover) {
            pSolidBrush->SetColor(sidebarOpen ? ColorRgba(1.0f, 1.0f, 1.0f, 0.14f) : ColorRgba(1.0f, 1.0f, 1.0f, 0.08f));
            pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(workfolderBtnRect, 4.0f, 4.0f), pSolidBrush);
            if (sidebarOpen) {
                pSolidBrush->SetColor(theme.accent);
                pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(workfolderBtnRect, 4.0f, 4.0f), pSolidBrush, 1.0f);
            }
        }
        SpriteAtlas::Instance().DrawSpriteCentered(pRenderTarget, "folder_16.png", workfolderBtnRect, 16.0f, 16.0f);

        minBtnRect = D2D1::RectF(width - 138.0f, 0, width - 92.0f, TITLEBAR_HEIGHT);
        maxBtnRect = D2D1::RectF(width - 92.0f, 0, width - 46.0f, TITLEBAR_HEIGHT);
        closeBtnRect = D2D1::RectF(width - 46.0f, 0, width, TITLEBAR_HEIGHT);

        bool minHover = PtInD2D1Rect(minBtnRect, mousePos);
        bool maxHover = PtInD2D1Rect(maxBtnRect, mousePos);
        bool closeHover = PtInD2D1Rect(closeBtnRect, mousePos);

        D2D1_RECT_F minBox = D2D1::RectF(minBtnRect.left + 4.0f, minBtnRect.top + 5.0f, minBtnRect.right - 4.0f, minBtnRect.bottom - 5.0f);
        if (minHover) {
            FillAdaptiveBox(minBox, theme, ColorHex(0x22222E), ColorHex(0x38384A), 4.0f, false);
        }
        float minBarW = 10.0f;
        float minBarH = 2.0f;
        float minX = minBtnRect.left + (minBtnRect.right - minBtnRect.left - minBarW) * 0.5f;
        float minY = minBtnRect.top + (minBtnRect.bottom - minBtnRect.top) * 0.5f + 4.0f;
        pSolidBrush->SetColor(minHover ? theme.accent : ColorHex(0xC0C0D0));
        pRenderTarget->FillRectangle(D2D1::RectF(minX, minY, minX + minBarW, minY + minBarH), pSolidBrush);

        // Maximize [⌜⌟]: Technical crosshair / corner frames that expand on hover
        D2D1_RECT_F maxBox = D2D1::RectF(maxBtnRect.left + 4.0f, maxBtnRect.top + 5.0f, maxBtnRect.right - 4.0f, maxBtnRect.bottom - 5.0f);
        if (maxHover) {
            FillAdaptiveBox(maxBox, theme, ColorHex(0x22222E), ColorHex(0x38384A), 4.0f, false);
        }
        pSolidBrush->SetColor(maxHover ? theme.accent : ColorHex(0xC0C0D0));
        float mcx = (maxBtnRect.left + maxBtnRect.right) * 0.5f;
        float mcy = (maxBtnRect.top + maxBtnRect.bottom) * 0.5f;
        if (isMaximized) {
            pRenderTarget->DrawRectangle(D2D1::RectF(mcx - 5.0f, mcy - 3.0f, mcx + 3.0f, mcy + 5.0f), pSolidBrush, 1.2f);
            pRenderTarget->DrawRectangle(D2D1::RectF(mcx - 3.0f, mcy - 5.0f, mcx + 5.0f, mcy + 3.0f), pSolidBrush, 1.2f);
        } else {
            float b = maxHover ? 5.5f : 4.5f;
            float l = maxHover ? 3.0f : 2.5f;
            pRenderTarget->DrawLine(D2D1::Point2F(mcx - b, mcy - b), D2D1::Point2F(mcx - b + l, mcy - b), pSolidBrush, 1.4f);
            pRenderTarget->DrawLine(D2D1::Point2F(mcx - b, mcy - b), D2D1::Point2F(mcx - b, mcy - b + l), pSolidBrush, 1.4f);
            pRenderTarget->DrawLine(D2D1::Point2F(mcx + b, mcy - b), D2D1::Point2F(mcx + b - l, mcy - b), pSolidBrush, 1.4f);
            pRenderTarget->DrawLine(D2D1::Point2F(mcx + b, mcy - b), D2D1::Point2F(mcx + b, mcy - b + l), pSolidBrush, 1.4f);
            pRenderTarget->DrawLine(D2D1::Point2F(mcx - b, mcy + b), D2D1::Point2F(mcx - b + l, mcy + b), pSolidBrush, 1.4f);
            pRenderTarget->DrawLine(D2D1::Point2F(mcx - b, mcy + b), D2D1::Point2F(mcx - b, mcy + b - l), pSolidBrush, 1.4f);
            pRenderTarget->DrawLine(D2D1::Point2F(mcx + b, mcy + b), D2D1::Point2F(mcx + b - l, mcy + b), pSolidBrush, 1.4f);
            pRenderTarget->DrawLine(D2D1::Point2F(mcx + b, mcy + b), D2D1::Point2F(mcx + b, mcy + b - l), pSolidBrush, 1.4f);
        }

        // Close [✕]: Crisp chamfered close button that lights up red #FF2A55
        D2D1_RECT_F closeBox = D2D1::RectF(closeBtnRect.left + 4.0f, closeBtnRect.top + 5.0f, closeBtnRect.right - 4.0f, closeBtnRect.bottom - 5.0f);
        if (closeHover) {
            FillAdaptiveBox(closeBox, theme, ColorHex(0xFF2A55), ColorHex(0xFF5577), 4.0f, false);
            pSolidBrush->SetColor(ColorHex(0xFFFFFF));
        } else {
            pSolidBrush->SetColor(ColorHex(0xC0C0D0));
        }
        float ccx = (closeBtnRect.left + closeBtnRect.right) * 0.5f;
        float ccy = (closeBtnRect.top + closeBtnRect.bottom) * 0.5f;
        float cSize = 4.5f;
        pRenderTarget->DrawLine(D2D1::Point2F(ccx - cSize, ccy - cSize), D2D1::Point2F(ccx + cSize, ccy + cSize), pSolidBrush, 1.5f);
        pRenderTarget->DrawLine(D2D1::Point2F(ccx + cSize, ccy - cSize), D2D1::Point2F(ccx - cSize, ccy + cSize), pSolidBrush, 1.5f);

        float rightCur = width - 138.0f;

        // Settings Button (32x26px, Win98 Control Panel settings_16.png)
        settingsBtnRect = D2D1::RectF(rightCur - 34.0f, 5.0f, rightCur - 2.0f, 31.0f);
        bool setHover = PtInD2D1Rect(settingsBtnRect, mousePos);
        if (setHover) {
            pSolidBrush->SetColor(ColorRgba(1.0f, 1.0f, 1.0f, 0.08f));
            pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(settingsBtnRect, 4.0f, 4.0f), pSolidBrush);
        }
        SpriteAtlas::Instance().DrawSpriteCentered(pRenderTarget, "settings_16.png", settingsBtnRect, 16.0f, 16.0f);
        rightCur -= 34.0f;

        // Search Button (32x26px, Win98 search_16.png)
        searchBtnRect = D2D1::RectF(rightCur - 34.0f, 5.0f, rightCur - 2.0f, 31.0f);
        bool srchHover = PtInD2D1Rect(searchBtnRect, mousePos);
        if (srchHover) {
            pSolidBrush->SetColor(ColorRgba(1.0f, 1.0f, 1.0f, 0.08f));
            pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(searchBtnRect, 4.0f, 4.0f), pSolidBrush);
        }
        SpriteAtlas::Instance().DrawSpriteCentered(pRenderTarget, "search_16.png", searchBtnRect, 16.0f, 16.0f);
        rightCur -= 34.0f;

        // Snippets Sidebar Button (32x26px, Win98 snippets_16.png Notepad)
        snippetsBtnRect = D2D1::RectF(rightCur - 34.0f, 5.0f, rightCur - 2.0f, 31.0f);
        bool snipHover = PtInD2D1Rect(snippetsBtnRect, mousePos);
        if (snippetsOpen || snipHover) {
            pSolidBrush->SetColor(snippetsOpen ? ColorRgba(1.0f, 1.0f, 1.0f, 0.10f) : ColorRgba(1.0f, 1.0f, 1.0f, 0.06f));
            pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(snippetsBtnRect, 4.0f, 4.0f), pSolidBrush);
        }
        SpriteAtlas::Instance().DrawSpriteCentered(pRenderTarget, "snippets_16.png", snippetsBtnRect, 16.0f, 16.0f);
        rightCur -= 34.0f;

        // Contextual Task Runner on Top Bar (Win98 build_16.png)
        if (!detectedTasks.empty()) {
            std::wstring taskLabel = detectedTasks[0].title;
            float taskTxtW = MeasureStringWidth(taskLabel, pUIFormatSmall);
            float taskW = 28.0f + taskTxtW + 16.0f;
            taskRunnerBtnRect = D2D1::RectF(rightCur - taskW - 8.0f, 5.0f, rightCur - 8.0f, 31.0f);
            bool trHover = PtInD2D1Rect(taskRunnerBtnRect, mousePos);
            pSolidBrush->SetColor(trHover ? ColorRgba(1.0f, 1.0f, 1.0f, 0.09f) : ColorRgba(1.0f, 1.0f, 1.0f, 0.04f));
            pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(taskRunnerBtnRect, 4.0f, 4.0f), pSolidBrush);
            pSolidBrush->SetColor(trHover ? theme.accent : ColorHex(0x323242));
            pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(taskRunnerBtnRect, 4.0f, 4.0f), pSolidBrush, 1.0f);

            D2D1_RECT_F icR = D2D1::RectF(taskRunnerBtnRect.left + 4.0f, taskRunnerBtnRect.top + 2.0f, taskRunnerBtnRect.left + 22.0f, taskRunnerBtnRect.bottom - 2.0f);
            SpriteAtlas::Instance().DrawSpriteCentered(pRenderTarget, "build_16.png", icR, 16.0f, 16.0f);
            DrawText(taskLabel, taskRunnerBtnRect.left + 24.0f, taskRunnerBtnRect.top + 4.5f, trHover ? theme.accent : ColorHex(0xE0E0EC), pUIFormatSmall);
            DrawChevronDownIcon(taskRunnerBtnRect.right - 14.0f, taskRunnerBtnRect.top + 6.0f, 10.0f, trHover ? theme.accent : ColorHex(0x78788C));
            rightCur -= (taskW + 8.0f);
        } else {
            taskRunnerBtnRect = D2D1::RectF(0, 0, 0, 0);
        }

        // Quick Run Action Button (only if no detected tasks to eliminate duplicate buttons!)
        std::wstring loadedScript;
        if (detectedTasks.empty() && activeTabIndex >= 0 && activeTabIndex < static_cast<int>(tabs.size())) {
            loadedScript = tabs[activeTabIndex]->loadedScriptPath;
        }

        if (!loadedScript.empty()) {
            std::wstring shortName = loadedScript;
            size_t lastSlash = shortName.find_last_of(L"\\/");
            if (lastSlash != std::wstring::npos) shortName = shortName.substr(lastSlash + 1);

            float scTxtW = MeasureStringWidth(shortName, pUIFormatSmall);
            float qkW = 24.0f + scTxtW + 12.0f;
            quickRunBtnRect = D2D1::RectF(rightCur - qkW - 8.0f, 6.0f, rightCur - 8.0f, 30.0f);
            statusQuickRunRect = quickRunBtnRect;

            bool qkHover = PtInD2D1Rect(quickRunBtnRect, mousePos);
            pSolidBrush->SetColor(qkHover ? ColorRgba(1.0f, 1.0f, 1.0f, 0.09f) : ColorRgba(1.0f, 1.0f, 1.0f, 0.04f));
            pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(quickRunBtnRect, 4.0f, 4.0f), pSolidBrush);

            D2D1_RECT_F qkIconRect = D2D1::RectF(quickRunBtnRect.left + 4.0f, 6.0f, quickRunBtnRect.left + 22.0f, 30.0f);
            DrawFluentIcon(0xE768, qkIconRect, ColorHex(0x66FFD9), pIconFormatSmall);
            DrawText(shortName, quickRunBtnRect.left + 24.0f, 8.5f, ColorHex(0xD4D4D8), pUIFormatSmall);

            rightCur -= (qkW + 8.0f);
        } else {
            quickRunBtnRect = D2D1::RectF(0, 0, 0, 0);
            statusQuickRunRect = D2D1::RectF(0, 0, 0, 0);
        }

        float maxTabsRight = rightCur - 60.0f;

        // 3. Render Tabs
        float tabX = 76.0f;
        float tabAvailableWidth = (std::max)(120.0f, maxTabsRight - tabX);
        float tabWidth = (std::min)(180.0f, tabAvailableWidth / (std::max)(1ULL, tabs.size()));

        for (size_t i = 0; i < tabs.size(); ++i) {
            auto& tab = *tabs[i];
            bool isActive = (static_cast<int>(i) == activeTabIndex);
            tab.tabRect = D2D1::RectF(tabX, 0.0f, tabX + tabWidth, TITLEBAR_HEIGHT);

            bool tabHover = PtInD2D1Rect(tab.tabRect, mousePos);
            float yTop = isMaximized ? 0.0f : 4.0f;

            if (isActive) {
                D2D1_COLOR_F activeTabBg = (theme.bg.r == 0.0f && theme.bg.g == 0.0f && theme.bg.b == 0.0f) ? ColorHex(0x252532) : ColorHex(0x282836);
                pSolidBrush->SetColor(activeTabBg);
                D2D1_RECT_F tabPill = D2D1::RectF(tabX, yTop, tabX + tabWidth, TITLEBAR_HEIGHT - 2.0f);
                pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(tabPill, 5.0f, 5.0f), pSolidBrush);

                pSolidBrush->SetColor(ColorHex(0x3A3A4C));
                pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(tabPill, 5.0f, 5.0f), pSolidBrush, 1.0f);

                D2D1_COLOR_F accCol = (tab.customAccent.a > 0.0f) ? tab.customAccent : theme.accent;
                pSolidBrush->SetColor(accCol);
                D2D1_RECT_F stripRect = D2D1::RectF(tabX + 4.0f, yTop + 0.5f, tabX + tabWidth - 4.0f, yTop + 2.5f);
                pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(stripRect, 1.0f, 1.0f), pSolidBrush);
            } else {
                if (tabHover) {
                    pSolidBrush->SetColor(ColorHex(0x1E1E26));
                    D2D1_RECT_F inactHoverRect = D2D1::RectF(tabX, yTop + 2.0f, tabX + tabWidth, TITLEBAR_HEIGHT - 3.0f);
                    pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(inactHoverRect, 4.0f, 4.0f), pSolidBrush);
                }

                if (i > 0 && static_cast<int>(i - 1) != activeTabIndex) {
                    pSolidBrush->SetColor(ColorHex(0x282830));
                    pRenderTarget->DrawLine(D2D1::Point2F(tabX - 1.0f, 11.0f), D2D1::Point2F(tabX - 1.0f, 25.0f), pSolidBrush, 1.0f);
                }
            }

            float iconX = tabX + 8.0f;
            if (isAdmin) {
                D2D1_RECT_F shieldRect = D2D1::RectF(iconX, 8.0f, iconX + 16.0f, 28.0f);
                DrawFluentIcon(0xEA18, shieldRect, ColorHex(0x66FFD9), pIconFormatSmall);
                iconX += 18.0f;
            }

            D2D1_RECT_F termIconRect = D2D1::RectF(iconX, 8.0f, iconX + 18.0f, 28.0f);
            if (tab.isEditor) {
                std::string iconName = "file.png";
                if (tab.isImagePreview) iconName = "image.png";
                else {
                    std::wstring ext = tab.editorFilePath;
                    size_t dot = ext.find_last_of(L'.');
                    if (dot != std::wstring::npos) {
                        std::wstring e = ToLower(ext.substr(dot));
                        if (e == L".py" || e == L".ps1" || e == L".bat" || e == L".cmd") iconName = "file_script.png";
                        else if (e == L".txt" || e == L".md" || e == L".json" || e == L".ini") iconName = "file_text.png";
                        else if (e == L".png" || e == L".jpg" || e == L".jpeg" || e == L".gif" || e == L".bmp" || e == L".ico") iconName = "image.png";
                        else if (e == L".exe") iconName = "exe.png";
                    }
                }
                SpriteAtlas::Instance().DrawSpriteCentered(pRenderTarget, iconName, termIconRect, 16.0f, 16.0f);
            } else {
                SpriteAtlas::Instance().DrawSpriteCentered(
                    pRenderTarget,
                    SpriteId::TerminalWindow,
                    termIconRect,
                    16.0f, 16.0f
                );
            }
            iconX += 20.0f;

            float titleMaxW = tabWidth - (iconX - tabX) - 26.0f;
            if (static_cast<int>(i) == renamingTabIndex) {
                D2D1_RECT_F editR = D2D1::RectF(iconX, 6.0f, iconX + titleMaxW, 30.0f);
                pSolidBrush->SetColor(ColorHex(0x28282E));
                pRenderTarget->FillRectangle(editR, pSolidBrush);
                pSolidBrush->SetColor(theme.accent);
                pRenderTarget->DrawRectangle(editR, pSolidBrush, 1.0f);
                DrawText(renamingText + L"|", iconX + 4.0f, 8.5f, theme.textPrimary, pUIFormatSmall);
            } else {
                std::wstring tabLabel = tab.isEditor ? (tab.isDirty ? (L"*" + tab.editorFileName) : tab.editorFileName) : tab.title;
                std::wstring dispTitle = tabLabel;
                while (MeasureStringWidth(dispTitle, pUIFormat) > titleMaxW && dispTitle.size() > 4) {
                    dispTitle.pop_back();
                }
                if (dispTitle != tabLabel) dispTitle += L"...";

                D2D1_COLOR_F titleCol = isActive ? ColorHex(0xFFFFFF) : ColorHex(0x8A8A98);
                if (tab.isEditor && tab.isDirty) titleCol = ColorHex(0xF59E0B);
                DrawText(dispTitle, iconX, 9.0f, titleCol, pUIFormat);
            }

            tab.closeBtnRect = D2D1::RectF(tabX + tabWidth - 24.0f, 8.0f, tabX + tabWidth - 4.0f, 28.0f);
            bool closeHover = PtInD2D1Rect(tab.closeBtnRect, mousePos);
            if (closeHover) {
                pSolidBrush->SetColor(ColorRgba(1.0f, 1.0f, 1.0f, 0.08f));
                pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(tab.closeBtnRect, 4.0f, 4.0f), pSolidBrush);
            }
            D2D1_COLOR_F clrIcon = closeHover ? ColorHex(0xFFFFFF) : (isActive ? ColorHex(0xD4D4D8) : ColorHex(0x606068));
            DrawFluentIcon(0xE711, tab.closeBtnRect, clrIcon, pIconFormatSmall);

            tabX += tabWidth + 2.0f;
        }

        newTabBtnRect = D2D1::RectF(tabX + 4.0f, 5.0f, tabX + 32.0f, 31.0f);
        bool newTabHover = PtInD2D1Rect(newTabBtnRect, mousePos);
        if (newTabHover) {
            pSolidBrush->SetColor(ColorRgba(1.0f, 1.0f, 1.0f, 0.08f));
            pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(newTabBtnRect, 4.0f, 4.0f), pSolidBrush);
        }
        SpriteAtlas::Instance().DrawSpriteCentered(pRenderTarget, SpriteId::SaturnFolder24, newTabBtnRect, 18.0f, 18.0f);

        dropdownBtnRect = D2D1::RectF(tabX + 33.0f, 5.0f, tabX + 55.0f, 31.0f);
        bool dropHover = PtInD2D1Rect(dropdownBtnRect, mousePos);
        if (dropHover) {
            pSolidBrush->SetColor(ColorRgba(1.0f, 1.0f, 1.0f, 0.06f));
            pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(dropdownBtnRect, 4.0f, 4.0f), pSolidBrush);
        }
        DrawFluentIcon(0xE70D, dropdownBtnRect, dropHover ? theme.textPrimary : theme.textDim, pIconFormatSmall);
    }

    void RenderSidebarClosedToggle(const Theme& theme, POINT mousePos) {
        if (!pRenderTarget) return;
        sidebarToggleBtnRect = D2D1::RectF(6.0f, TITLEBAR_HEIGHT + 6.0f, 30.0f, TITLEBAR_HEIGHT + 30.0f);
        bool stbHover = PtInD2D1Rect(sidebarToggleBtnRect, mousePos);
        pSolidBrush->SetColor(stbHover ? ColorHex(0x282838) : ColorHex(0x181824));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(sidebarToggleBtnRect, 4.0f, 4.0f), pSolidBrush);
        pSolidBrush->SetColor(stbHover ? theme.accent : ColorHex(0x38384C));
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(sidebarToggleBtnRect, 4.0f, 4.0f), pSolidBrush, 1.0f);
        DrawChevronRightIcon(sidebarToggleBtnRect.left + 7.0f, sidebarToggleBtnRect.top + 7.0f, 10.0f, stbHover ? theme.accent : ColorHex(0x9090A4));
    }

    bool IsOverTitlebarInteractiveElement(POINT pt) const {
        if (pt.y >= TITLEBAR_HEIGHT) return false;
        if (PtInD2D1Rect(fileBtnRect, pt)) return true;
        if (PtInD2D1Rect(newTabBtnRect, pt)) return true;
        if (PtInD2D1Rect(dropdownBtnRect, pt)) return true;
        if (taskRunnerBtnRect.right > taskRunnerBtnRect.left && PtInD2D1Rect(taskRunnerBtnRect, pt)) return true;
        if (quickRunBtnRect.right > quickRunBtnRect.left && PtInD2D1Rect(quickRunBtnRect, pt)) return true;
        if (PtInD2D1Rect(workfolderBtnRect, pt)) return true;
        if (PtInD2D1Rect(snippetsBtnRect, pt)) return true;
        if (PtInD2D1Rect(searchBtnRect, pt)) return true;
        if (PtInD2D1Rect(settingsBtnRect, pt)) return true;
        if (PtInD2D1Rect(minBtnRect, pt)) return true;
        if (PtInD2D1Rect(maxBtnRect, pt)) return true;
        if (PtInD2D1Rect(closeBtnRect, pt)) return true;
        return false;
    }

    void RenderModularStatusBar(
        const Theme& theme,
        const AppSettings& settings,
        float width,
        float height,
        const GitStatusInfo& gitInfo,
        const TelemetryData& tm,
        POINT mousePos
    ) {
        (void)gitInfo; (void)mousePos; (void)theme;
        statusGitRect = statusBarThemeRect = statusBarWarmthRect = statusBarPromptPosRect = statusBarCpuRamRect = D2D1::RectF(0, 0, 0, 0);
        if (!settings.statusBarVisible || !pRenderTarget) {
            statusBarRect = D2D1::RectF(0, 0, 0, 0);
            return;
        }

        float barH = 24.0f;
        statusBarRect = D2D1::RectF(0.0f, height - barH, width, height);

        pSolidBrush->SetColor(ColorHex(0x101014));
        pRenderTarget->FillRectangle(statusBarRect, pSolidBrush);
        pSolidBrush->SetColor(ColorHex(0x202028));
        pRenderTarget->DrawLine(D2D1::Point2F(0.0f, height - barH), D2D1::Point2F(width, height - barH), pSolidBrush, 1.0f);

        float textY = height - barH + 4.0f;
        D2D1_COLOR_F dim = ColorHex(0x9A9AB0);

        auto DrawStatusBadge = [&](const D2D1_RECT_F& r, const std::wstring& txt, D2D1_COLOR_F bgCol, D2D1_COLOR_F borderCol, D2D1_COLOR_F textCol) {
            pSolidBrush->SetColor(bgCol);
            pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(r, 3.0f, 3.0f), pSolidBrush);
            pSolidBrush->SetColor(borderCol);
            pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(r, 3.0f, 3.0f), pSolidBrush, 1.0f);

            IDWriteTextLayout* pLayout = nullptr;
            float rw = r.right - r.left;
            float rh = r.bottom - r.top;
            if (pDWriteFactory && SUCCEEDED(pDWriteFactory->CreateTextLayout(txt.c_str(), static_cast<UINT32>(txt.size()), pUIFormatBold, rw, rh, &pLayout)) && pLayout) {
                pLayout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
                pLayout->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
                pSolidBrush->SetColor(textCol);
                pRenderTarget->DrawTextLayout(D2D1::Point2F(r.left, r.top + 0.5f), pLayout, pSolidBrush);
                SafeRelease(pLayout);
            } else {
                float tw = MeasureStringWidth(txt, pUIFormatBold);
                DrawText(txt, r.left + (rw - tw) * 0.5f, r.top + 0.5f, textCol, pUIFormatBold);
            }
        };

        // PROC badge capsule
        float badgeH = 16.0f;
        float badgeY = height - barH + (barH - badgeH) * 0.5f;
        float procBadgeW = 40.0f;
        D2D1_RECT_F procBadgeR = D2D1::RectF(10.0f, badgeY, 10.0f + procBadgeW, badgeY + badgeH);
        DrawStatusBadge(procBadgeR, L"PROC", ColorHex(0x182032), ColorHex(0x384E7A), ColorHex(0x80AAFF));

        wchar_t procBuf[96];
        swprintf_s(procBuf, L"CPU %.1f%%  |  RAM %zu MB  |  GPU %.1f%%", tm.procCpu, tm.procRamMb, tm.procGpu);
        DrawText(procBuf, procBadgeR.right + 8.0f, textY, dim, pUIFormatSmall);

        // SYS metrics and badge
        bool ptbr = (settings.language == "pt-BR");
        wchar_t sysBuf[160];
        if (tm.sysTempValid && tm.sysTempC > 0.0) {
            if (tm.gpuTempValid && tm.gpuTempC > 0.0) {
                swprintf_s(sysBuf, L"CPU %.1f%%  |  RAM %.1f / %.1f GB  |  GPU %.1f%%  |  CPU %.0f\u00B0C  GPU %.0f\u00B0C",
                           tm.sysCpu, tm.sysRamUsedGb, tm.sysRamTotalGb, tm.sysGpu, tm.sysTempC, tm.gpuTempC);
            } else {
                swprintf_s(sysBuf, L"CPU %.1f%%  |  RAM %.1f / %.1f GB  |  GPU %.1f%%  |  TEMP %.0f\u00B0C",
                           tm.sysCpu, tm.sysRamUsedGb, tm.sysRamTotalGb, tm.sysGpu, tm.sysTempC);
            }
        } else {
            swprintf_s(sysBuf, ptbr ? L"CPU %.1f%%  |  RAM %.1f / %.1f GB  |  GPU %.1f%%  |  TEMP: n/d"
                                    : L"CPU %.1f%%  |  RAM %.1f / %.1f GB  |  GPU %.1f%%  |  TEMP: N/A",
                       tm.sysCpu, tm.sysRamUsedGb, tm.sysRamTotalGb, tm.sysGpu);
        }

        float sw = MeasureStringWidth(sysBuf, pUIFormatSmall);
        float sysBadgeW = 34.0f;
        float totalSysW = sysBadgeW + 8.0f + sw;
        float sysStartX = width - 12.0f - totalSysW;

        // SYS badge capsule
        D2D1_RECT_F sysBadgeR = D2D1::RectF(sysStartX, badgeY, sysStartX + sysBadgeW, badgeY + badgeH);
        DrawStatusBadge(sysBadgeR, L"SYS", ColorHex(0x281832), ColorHex(0x6A3880), ColorHex(0xBD93F9));

        DrawText(sysBuf, sysBadgeR.right + 8.0f, textY, dim, pUIFormatSmall);
    }
    bool IsOverStatusbarInteractiveElement(POINT pt) const {
        if (statusBarRect.right <= statusBarRect.left) return false;
        if (PtInD2D1Rect(statusGitRect, pt)) return true;
        if (PtInD2D1Rect(statusBarThemeRect, pt)) return true;
        if (PtInD2D1Rect(statusBarWarmthRect, pt)) return true;
        if (PtInD2D1Rect(statusBarPromptPosRect, pt)) return true;
        if (PtInD2D1Rect(statusBarCpuRamRect, pt)) return true;
        return false;
    }

    float DrawKeycapVec(const std::wstring& label, float x, float y, float h = 22.0f) {
        float tw = MeasureStringWidth(label, pUIFormatBold);
        float w = (std::max)(22.0f, tw + 14.0f);
        D2D1_RECT_F lip = D2D1::RectF(x, y + 1.0f, x + w, y + h);
        D2D1_RECT_F cap = D2D1::RectF(x, y, x + w, y + h - 2.0f);
        pSolidBrush->SetColor(ColorHex(0x14141C));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(lip, 4.0f, 4.0f), pSolidBrush);
        pSolidBrush->SetColor(ColorHex(0x2E2E40));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(cap, 4.0f, 4.0f), pSolidBrush);
        pSolidBrush->SetColor(ColorHex(0x50506A));
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(cap, 4.0f, 4.0f), pSolidBrush, 1.0f);

        IDWriteTextLayout* pLayout = nullptr;
        if (pDWriteFactory && SUCCEEDED(pDWriteFactory->CreateTextLayout(label.c_str(), static_cast<UINT32>(label.size()), pUIFormatBold, w, h - 2.0f, &pLayout)) && pLayout) {
            pLayout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            pLayout->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            float yShift = (label == L"," || label == L"." || label == L";") ? -2.0f : 1.0f;
            pSolidBrush->SetColor(ColorHex(0xF2F2FA));
            pRenderTarget->DrawTextLayout(D2D1::Point2F(x, y + yShift), pLayout, pSolidBrush);
            SafeRelease(pLayout);
        } else {
            float yShift = (label == L"," || label == L"." || label == L";") ? -2.0f : 1.0f;
            DrawText(label, x + (w - tw) * 0.5f, y + 2.0f + yShift, ColorHex(0xF2F2FA), pUIFormatBold);
        }
        return w;
    }

    float DrawKeyComboVec(const std::vector<std::wstring>& keys, float x, float y, float h = 22.0f) {
        float cur = x;
        for (size_t i = 0; i < keys.size(); ++i) {
            cur += DrawKeycapVec(keys[i], cur, y, h);
            if (i + 1 < keys.size()) cur += 4.0f;
        }
        return cur - x;
    }

    void RenderEmptyStateHud(
        const Theme& theme,
        const std::wstring& cwd,
        const GitStatusInfo& gitInfo,
        const std::vector<ProjectTask>& detectedTasks,
        float px, float py, float pw, float ph,
        POINT mousePos,
        const std::string& language = "en-US"
    ) {
        emptyStateBadges.clear();
        if (!pRenderTarget || suppressEmptyState) return;

        float cardW = (std::min)(560.0f, pw - 40.0f);
        float cardH = 270.0f;
        float cardX = px + (pw - cardW) * 0.5f;
        float cardY = py + (ph - cardH) * 0.40f;
        if (cardY < py + 12.0f) cardY = py + 12.0f;

        std::string hero = theme.heroSprite.empty() ? "saturn_folder_clean.png" : theme.heroSprite;
        D2D1_RECT_F heroRect = D2D1::RectF(cardX + (cardW - 96.0f) * 0.5f, cardY, cardX + (cardW + 96.0f) * 0.5f, cardY + 84.0f);
        SpriteAtlas::Instance().DrawSpriteCentered(pRenderTarget, hero, heroRect, 96.0f, 84.0f);

        std::wstring projName = cwd;
        size_t lastSlash = projName.find_last_of(L"\\/");
        if (lastSlash != std::wstring::npos && lastSlash + 1 < projName.size()) projName = projName.substr(lastSlash + 1);
        float nW = MeasureStringWidth(projName, pUIFormatTitle);
        DrawText(projName, cardX + (cardW - nW) * 0.5f, cardY + 94.0f, ColorHex(0xE6E6F2), pUIFormatTitle);

        std::wstring sub;
        if (gitInfo.isGitRepo) sub = gitInfo.branchName + (gitInfo.isDirty ? L" (modified)" : L"");
        if (!sub.empty()) {
            float sW = MeasureStringWidth(sub, pUIFormatSmall);
            DrawText(sub, cardX + (cardW - sW) * 0.5f, cardY + 120.0f, ColorHex(0x9A9AB0), pUIFormatSmall);
        }

        // Shortcut reference
        bool ptbr = (language == "pt-BR");
        struct Row { int id; std::vector<std::wstring> keys; std::wstring label; };
        std::vector<Row> rows = {
            { 0, { L"Ctrl", L"B" },  ptbr ? L"Explorador" : L"Workspace" },
            { 1, { L"Ctrl", L"R" },  ptbr ? L"Historico" : L"History" },
            { 3, { L"Ctrl", L"," },  ptbr ? L"Configuracoes" : L"Settings" },
            { 4, { L"Ctrl", L"Shift", L"P" }, ptbr ? L"Comandos" : L"Commands" }
        };
        float rowW = 265.0f, rowH = 36.0f, gap = 14.0f;
        float startX = cardX + (cardW - rowW * 2.0f - gap) * 0.5f;
        float y0 = cardY + 154.0f;
        for (size_t i = 0; i < rows.size(); ++i) {
            float bx = startX + (i % 2) * (rowW + gap);
            float by = y0 + (i / 2) * (rowH + 10.0f);
            D2D1_RECT_F r = D2D1::RectF(bx, by, bx + rowW, by + rowH);
            emptyStateBadges.push_back({ r, rows[i].id });
            bool hov = PtInD2D1Rect(r, mousePos);
            pSolidBrush->SetColor(hov ? ColorHex(0x1C1C28) : ColorHex(0x121218));
            pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(r, 6.0f, 6.0f), pSolidBrush);
            pSolidBrush->SetColor(hov ? theme.accent : ColorHex(0x28283C));
            pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(r, 6.0f, 6.0f), pSolidBrush, 1.0f);

            // Vertically center label
            float textY = by + (rowH - 16.0f) * 0.5f;
            DrawText(rows[i].label, bx + 14.0f, textY, hov ? theme.accent : ColorHex(0xD6D6E6), pUIFormatMedium);

            // Compute total keycap width and align right with 12px margin
            float keyH = 22.0f;
            float keyY = by + (rowH - keyH) * 0.5f;
            float totalKw = 0.0f;
            for (size_t k = 0; k < rows[i].keys.size(); ++k) {
                float tw = MeasureStringWidth(rows[i].keys[k], pUIFormatBold);
                totalKw += (std::max)(22.0f, tw + 14.0f);
                if (k + 1 < rows[i].keys.size()) totalKw += 4.0f;
            }
            DrawKeyComboVec(rows[i].keys, bx + rowW - 12.0f - totalKw, keyY, keyH);
        }
    }

    void RenderBreadcrumbSiblingPopover(
        const Theme& theme,
        const std::wstring& targetDir,
        const std::vector<std::wstring>& siblings,
        const D2D1_RECT_F& anchorRect,
        POINT mousePos
    ) {
        if (!pRenderTarget) return;
        breadcrumbPopoverItems.clear();

        float popW = 240.0f;
        float itemH = 26.0f;
        float maxVisibleItems = (std::min)(static_cast<size_t>(10), siblings.size());
        float popH = (maxVisibleItems > 0 ? (maxVisibleItems * itemH + 12.0f) : 40.0f);

        float popX = anchorRect.left;
        float popY = anchorRect.bottom + 4.0f;
        D2D1_SIZE_F sz = pRenderTarget->GetSize();
        if (popY + popH > sz.height - 30.0f) {
            popY = anchorRect.top - popH - 4.0f;
        }
        if (popX + popW > sz.width - 20.0f) {
            popX = sz.width - popW - 20.0f;
        }
        if (popX < 10.0f) popX = 10.0f;

        breadcrumbPopoverRect = D2D1::RectF(popX, popY, popX + popW, popY + popH);
        FillAdaptiveBox(breadcrumbPopoverRect, theme, ColorHex(0x13131A), ColorHex(0x2C2C3C), 4.0f, false);

        if (siblings.empty()) {
            DrawText(L"(Nenhuma pasta encontrada)", popX + 12.0f, popY + 10.0f, ColorHex(0x78788C), pUIFormatSmall);
            return;
        }

        float curY = popY + 6.0f;
        for (size_t i = 0; i < maxVisibleItems; ++i) {
            const auto& sib = siblings[i];
            D2D1_RECT_F itR = D2D1::RectF(popX + 4.0f, curY, popX + popW - 4.0f, curY + itemH);
            breadcrumbPopoverItems.push_back({ itR, sib });

            bool isHov = PtInD2D1Rect(itR, mousePos);
            if (isHov) {
                pSolidBrush->SetColor(ColorHex(0x222234));
                pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(itR, 3.0f, 3.0f), pSolidBrush);
            }

            std::wstring fName = sib;
            size_t slash = fName.find_last_of(L"\\/");
            if (slash != std::wstring::npos && slash + 1 < fName.size()) {
                fName = fName.substr(slash + 1);
            }

            D2D1_RECT_F folR = D2D1::RectF(itR.left + 6.0f, curY + 5.0f, itR.left + 22.0f, curY + 21.0f);
            SpriteAtlas::Instance().DrawSpriteCentered(pRenderTarget, SpriteId::BlueFolder, folR, 14.0f, 14.0f);

            DrawText(fName, itR.left + 26.0f, curY + 4.5f, isHov ? theme.accent : ColorHex(0xD8D8E6), pUIFormatSmall);
            curY += itemH;
        }
    }

    void RenderTaskMenu(
        const Theme& theme,
        const std::vector<ProjectTask>& tasks,
        const D2D1_RECT_F& anchorRect,
        POINT mousePos
    ) {
        if (!pRenderTarget) return;
        taskMenuItems.clear();

        float maxTextW = 200.0f;
        for (const auto& t : tasks) {
            float tw = MeasureStringWidth(t.title, pUIFormatSmall);
            float cw = MeasureStringWidth(t.command, pTermFormatSmall);
            if (tw > maxTextW) maxTextW = tw;
            if (cw > maxTextW) maxTextW = cw;
        }
        float menuW = (std::clamp)(maxTextW + 48.0f, 320.0f, 520.0f);
        float itemH = 36.0f;
        float menuH = tasks.size() * itemH + 12.0f;

        float menuX = anchorRect.left;
        float menuY = anchorRect.bottom + 4.0f;
        D2D1_SIZE_F sz = pRenderTarget->GetSize();
        if (menuX + menuW > sz.width - 20.0f) {
            menuX = sz.width - menuW - 20.0f;
        }

        taskMenuRect = D2D1::RectF(menuX, menuY, menuX + menuW, menuY + menuH);
        FillAdaptiveBox(taskMenuRect, theme, ColorHex(0x13131A), ColorHex(0x2C2C3C), 4.0f, false);

        float curY = menuY + 6.0f;
        for (size_t i = 0; i < tasks.size(); ++i) {
            const auto& t = tasks[i];
            D2D1_RECT_F itR = D2D1::RectF(menuX + 4.0f, curY, menuX + menuW - 4.0f, curY + itemH);
            taskMenuItems.push_back({ itR, i });

            bool isHov = PtInD2D1Rect(itR, mousePos);
            if (isHov) {
                pSolidBrush->SetColor(ColorHex(0x222234));
                pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(itR, 3.0f, 3.0f), pSolidBrush);
            }

            D2D1_RECT_F chipR = D2D1::RectF(itR.left + 6.0f, curY + 6.0f, itR.left + 22.0f, curY + 22.0f);
            SpriteAtlas::Instance().DrawSpriteCentered(pRenderTarget, SpriteId::GreenChip, chipR, 14.0f, 14.0f);

            DrawText(t.title, itR.left + 26.0f, curY + 3.0f, isHov ? theme.accent : ColorHex(0xF0F0FF), pUIFormatSmall);

            std::wstring dispCmd = t.command;
            float maxCmdW = menuW - 40.0f;
            while (MeasureStringWidth(dispCmd, pTermFormatSmall) > maxCmdW && dispCmd.size() > 4) {
                dispCmd.pop_back();
            }
            if (dispCmd != t.command) dispCmd += L"...";
            DrawText(dispCmd, itR.left + 26.0f, curY + 18.0f, ColorHex(0x78788C), pTermFormatSmall);

            curY += itemH;
        }
    }

    void RenderPinnedOutputPanel(
        const Theme& theme,
        const CommandBlock& blk,
        float panelX, float panelY, float panelW, float panelH,
        float scrollY,
        POINT mousePos
    ) {
        if (!pRenderTarget || panelW < 100.0f) return;

        pinnedPanelRect = D2D1::RectF(panelX, panelY, panelX + panelW, panelY + panelH);

        // Panel background (#0E0E14) with left specular accent border
        pSolidBrush->SetColor(ColorHex(0x0E0E14));
        pRenderTarget->FillRectangle(pinnedPanelRect, pSolidBrush);
        pSolidBrush->SetColor(theme.accent);
        pRenderTarget->DrawLine(D2D1::Point2F(panelX, panelY), D2D1::Point2F(panelX, panelY + panelH), pSolidBrush, 1.5f);

        // Header (30px)
        float headH = 30.0f;
        D2D1_RECT_F headRect = D2D1::RectF(panelX, panelY, panelX + panelW, panelY + headH);
        pSolidBrush->SetColor(ColorHex(0x161622));
        pRenderTarget->FillRectangle(headRect, pSolidBrush);
        pSolidBrush->SetColor(ColorHex(0x262636));
        pRenderTarget->DrawLine(D2D1::Point2F(panelX, panelY + headH), D2D1::Point2F(panelX + panelW, panelY + headH), pSolidBrush, 1.0f);

        D2D1_RECT_F icR = D2D1::RectF(panelX + 8.0f, panelY + 6.0f, panelX + 26.0f, panelY + 24.0f);
        SpriteAtlas::Instance().DrawSpriteCentered(pRenderTarget, SpriteId::FloppyDisk, icR, 16.0f, 16.0f);

        std::wstring headTitle = L"PINNED: " + blk.commandText;
        while (MeasureStringWidth(headTitle, pUIFormatBold) > panelW - 90.0f && headTitle.size() > 8) {
            headTitle.pop_back();
        }
        if (headTitle != L"PINNED: " + blk.commandText) headTitle += L"...";
        DrawText(headTitle, panelX + 30.0f, panelY + 6.0f, ColorHex(0xF0F0FF), pUIFormatBold);

        // Unpin button on right
        pinnedCloseBtnRect = D2D1::RectF(panelX + panelW - 54.0f, panelY + 4.0f, panelX + panelW - 6.0f, panelY + 26.0f);
        bool unpinHov = PtInD2D1Rect(pinnedCloseBtnRect, mousePos);
        pSolidBrush->SetColor(unpinHov ? ColorHex(0x282838) : ColorHex(0x1A1A24));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(pinnedCloseBtnRect, 3.0f, 3.0f), pSolidBrush);
        pSolidBrush->SetColor(unpinHov ? theme.accent : ColorHex(0x323246));
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(pinnedCloseBtnRect, 3.0f, 3.0f), pSolidBrush, 1.0f);
        DrawText(L"Unpin", pinnedCloseBtnRect.left + 8.0f, panelY + 6.0f, unpinHov ? theme.accent : ColorHex(0xD0D0E0), pUIFormatSmall);

        // Output lines clip
        D2D1_RECT_F outClip = D2D1::RectF(panelX, panelY + headH + 2.0f, panelX + panelW, panelY + panelH - 2.0f);
        pRenderTarget->PushAxisAlignedClip(outClip, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        float availPinW = (std::max)(100.0f, panelW - 24.0f);
        float cwPin = (charWidth > 1.0f) ? charWidth : 8.0f;
        int maxPinCols = (std::max)(20, static_cast<int>(availPinW / cwPin));

        float curY = panelY + headH + 8.0f - scrollY;
        for (const auto& rawLine : blk.outputLines) {
            std::vector<StyledLine> wrappedLines = WrapStyledLine(rawLine, maxPinCols);
            for (const auto& line : wrappedLines) {
                float spanX = panelX + 12.0f;
                for (const auto& span : line.spans) {
                    IDWriteTextFormat* tf = span.bold ? pTermFormatBold : pTermFormat;
                    DrawText(span.text, spanX, curY, span.fg, tf);
                    spanX += MeasureStringWidth(span.text, tf);
                }
                curY += lineHeight;
            }
        }

        pRenderTarget->PopAxisAlignedClip();
    }

    void RenderTimelineHistoryModal(
        const Theme& theme,
        float screenW, float screenH,
        const std::vector<TimelineEntry>& history,
        size_t selectedIndex,
        bool successOnly,
        POINT mousePos,
        const std::string& language = "en-US"
    ) {
        if (!pRenderTarget) return;
        timelineItemRects.clear();

        // Backdrop
        pSolidBrush->SetColor(ColorRgba(0.0f, 0.0f, 0.0f, 0.60f));
        pRenderTarget->FillRectangle(D2D1::RectF(0, 0, screenW, screenH), pSolidBrush);

        float mw = (std::min)(680.0f, screenW - 40.0f);
        float mh = 420.0f;
        float mx = (screenW - mw) * 0.5f;
        float my = (screenH - mh) * 0.45f;
        timelineModalRect = D2D1::RectF(mx, my, mx + mw, my + mh);

        FillAdaptiveBox(timelineModalRect, theme, ColorHex(0x101016), ColorHex(0x34344A), 6.0f, true);

        // Header (44px)
        D2D1_RECT_F rdrRect = D2D1::RectF(mx + 14.0f, my + 10.0f, mx + 36.0f, my + 32.0f);
        SpriteAtlas::Instance().DrawSpriteCentered(pRenderTarget, SpriteId::RadarSonar, rdrRect, 20.0f, 20.0f);
        DrawText(L"TIMELINE HISTORY (Ctrl+R)", mx + 42.0f, my + 11.0f, theme.accent, pUIFormatBold);

        // Success Only filter toggle pill
        timelineFilterToggleRect = D2D1::RectF(mx + mw - 180.0f, my + 8.0f, mx + mw - 46.0f, my + 34.0f);
        bool fltHov = PtInD2D1Rect(timelineFilterToggleRect, mousePos);
        pSolidBrush->SetColor(successOnly ? ColorHex(0x13381B) : (fltHov ? ColorHex(0x22222E) : ColorHex(0x181822)));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(timelineFilterToggleRect, 4.0f, 4.0f), pSolidBrush);
        pSolidBrush->SetColor(successOnly ? ColorHex(0x22C55E) : ColorHex(0x323244));
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(timelineFilterToggleRect, 4.0f, 4.0f), pSolidBrush, 1.0f);
        DrawText(successOnly ? L"[Tab] OK Only *" : L"[Tab] All Cmds", timelineFilterToggleRect.left + 8.0f, my + 11.0f, successOnly ? ColorHex(0x4ADE80) : ColorHex(0xA0A0B0), pUIFormatSmall);

        // Close button
        timelineCloseBtnRect = D2D1::RectF(mx + mw - 38.0f, my + 8.0f, mx + mw - 10.0f, my + 34.0f);
        bool clHov = PtInD2D1Rect(timelineCloseBtnRect, mousePos);
        pSolidBrush->SetColor(clHov ? ColorHex(0xE81123) : ColorHex(0x1E1E26));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(timelineCloseBtnRect, 4.0f, 4.0f), pSolidBrush);
        DrawFluentIcon(0xE8BB, timelineCloseBtnRect, ColorHex(0xFFFFFF), pIconFormatSmall);

        // Divider
        pSolidBrush->SetColor(ColorHex(0x222230));
        pRenderTarget->DrawLine(D2D1::Point2F(mx, my + 44.0f), D2D1::Point2F(mx + mw, my + 44.0f), pSolidBrush, 1.0f);

        // List items
        std::vector<TimelineEntry> filtered = FilterTimelineHistory(history, successOnly);
        float itemY = my + 50.0f;
        float itemH = 34.0f;

        for (size_t i = 0; i < filtered.size() && itemY + itemH <= my + mh - 44.0f; ++i) {
            const auto& ent = filtered[i];
            D2D1_RECT_F itR = D2D1::RectF(mx + 10.0f, itemY, mx + mw - 10.0f, itemY + itemH);
            timelineItemRects.push_back({ itR, i });

            bool isSel = (i == selectedIndex);
            bool isHov = PtInD2D1Rect(itR, mousePos);

            if (isSel) {
                pSolidBrush->SetColor(ColorHex(0x222232));
                pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(itR, 4.0f, 4.0f), pSolidBrush);
                pSolidBrush->SetColor(theme.accent);
                pRenderTarget->FillRectangle(D2D1::RectF(itR.left, itR.top, itR.left + 3.0f, itR.bottom), pSolidBrush);
            } else if (isHov) {
                pSolidBrush->SetColor(ColorHex(0x181822));
                pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(itR, 4.0f, 4.0f), pSolidBrush);
            }

            // Status Badge [OK] vs [FAIL]
            std::wstring stBadge = (ent.exitCode == 0) ? L"OK" : L"FAIL";
            D2D1_COLOR_F stCol = (ent.exitCode == 0) ? ColorHex(0x4ADE80) : ColorHex(0xEF4444);
            DrawText(stBadge, itR.left + 10.0f, itemY + 8.0f, stCol, pUIFormatBold);

            // Command Text
            float maxCmdW = mw - 240.0f;
            std::wstring cmdText = TruncateTextToWidth(ent.command, maxCmdW, pTermFormatBold);
            DrawText(cmdText, itR.left + 54.0f, itemY + 7.0f, isSel ? ColorHex(0xFFFFFF) : theme.textPrimary, pTermFormatBold);

            // Duration & Timestamp
            wchar_t durBuf[32];
            if (ent.durationMs < 1000.0) {
                swprintf_s(durBuf, L"%.0fms", ent.durationMs);
            } else {
                swprintf_s(durBuf, L"%.2fs", ent.durationMs / 1000.0);
            }
            std::wstring meta = std::wstring(durBuf) + L"  " + ent.timestamp;
            float metaW = MeasureStringWidth(meta, pUIFormatSmall);
            DrawText(meta, itR.right - metaW - 10.0f, itemY + 8.5f, ColorHex(0x78788C), pUIFormatSmall);

            itemY += itemH + 2.0f;
        }

        bool ptbr = (language == "pt-BR");
        if (filtered.empty()) {
            std::wstring emptyMsg = ptbr ? L"Nenhum comando registrado no historico." : L"No commands recorded in history.";
            DrawText(emptyMsg, mx + 24.0f, my + 70.0f, ColorHex(0x78788C), pUIFormatSmall);
        }

        // Footer (40px) with tactical vector keycaps
        float footY = my + mh - 40.0f;
        pSolidBrush->SetColor(ColorHex(0x222230));
        pRenderTarget->DrawLine(D2D1::Point2F(mx, footY), D2D1::Point2F(mx + mw, footY), pSolidBrush, 1.0f);

        float footCurX = mx + 12.0f;
        auto DrawTimelinePill = [&](D2D1_RECT_F& outRect, const std::vector<std::wstring>& keys, const std::wstring& label) {
            float txtW = MeasureStringWidth(label, pUIFormatSmall);
            float keysW = 0.0f;
            for (size_t i = 0; i < keys.size(); ++i) {
                float tw = MeasureStringWidth(keys[i], pUIFormatBold);
                keysW += (std::max)(18.0f, tw + 12.0f);
                if (i + 1 < keys.size()) keysW += 4.0f;
            }
            float pillW = 8.0f + keysW + 8.0f + txtW + 12.0f;
            outRect = D2D1::RectF(footCurX, footY + 6.0f, footCurX + pillW, footY + 34.0f);
            bool hov = PtInD2D1Rect(outRect, mousePos);
            pSolidBrush->SetColor(hov ? ColorHex(0x242434) : ColorHex(0x161620));
            pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(outRect, 4.0f, 4.0f), pSolidBrush);
            pSolidBrush->SetColor(hov ? theme.accent : ColorHex(0x2E2E3E));
            pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(outRect, 4.0f, 4.0f), pSolidBrush, 1.0f);

            float keyH = 18.0f;
            float keyY = footY + 6.0f + (28.0f - keyH) * 0.5f; // footY + 11.0f
            float used = DrawKeyComboVec(keys, footCurX + 8.0f, keyY, keyH);
            float labelY = footY + 6.0f + (28.0f - 14.0f) * 0.5f; // footY + 13.0f
            DrawText(label, footCurX + 8.0f + used + 8.0f, labelY, hov ? theme.accent : ColorHex(0xD4D4E0), pUIFormatSmall);
            footCurX += (pillW + 8.0f);
        };

        DrawTimelinePill(timelineFooterRunRect, { L"Enter" }, ptbr ? L"Executar" : L"Run");
        DrawTimelinePill(timelineFooterTabRect, { L"Tab" }, ptbr ? L"Alternar Filtro" : L"Toggle Filter");
        DrawTimelinePill(timelineFooterEscRect, { L"Esc" }, ptbr ? L"Fechar" : L"Close");
    }

    void RenderEdgePulse(float width, float height, float progress, bool isSuccess) {
        if (!pRenderTarget || progress <= 0.001f) return;

        float alpha = (std::clamp)(progress * 0.70f, 0.0f, 0.70f);
        D2D1_COLOR_F glowCol = isSuccess ? ColorHex(0x00FF9C, alpha) : ColorHex(0xFF2A55, alpha);
        pSolidBrush->SetColor(glowCol);

        float strokeW = (std::max)(2.0f, 5.0f * progress);
        pRenderTarget->DrawRectangle(D2D1::RectF(0.0f, 0.0f, width, height), pSolidBrush, strokeW);

        // Centered audio alert pill at top of screen (#9 Feedback Periférico com audio_speaker.png)
        float pillW = 160.0f;
        float pillH = 26.0f;
        float pillX = (width - pillW) * 0.5f;
        float pillY = 42.0f;
        D2D1_RECT_F pillR = D2D1::RectF(pillX, pillY, pillX + pillW, pillY + pillH);
        pSolidBrush->SetColor(ColorHex(0x101016, (std::min)(0.90f, alpha * 1.5f)));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(pillR, 4.0f, 4.0f), pSolidBrush);
        pSolidBrush->SetColor(glowCol);
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(pillR, 4.0f, 4.0f), pSolidBrush, 1.0f);
        D2D1_RECT_F spkR = D2D1::RectF(pillX + 6.0f, pillY + 3.0f, pillX + 26.0f, pillY + 23.0f);
        SpriteAtlas::Instance().DrawSpriteCentered(pRenderTarget, SpriteId::AudioSpeaker, spkR, 16.0f, 16.0f);
        std::wstring alertTxt = isSuccess ? L"TASK FINISHED" : L"TASK FAILED";
        DrawText(alertTxt, pillX + 30.0f, pillY + 4.5f, isSuccess ? ColorHex(0x00FF9C) : ColorHex(0xFF2A55), pUIFormatBold);
    }

    static std::mutex& GetFallbackMutex() {
        static std::mutex s_mtx;
        return s_mtx;
    }

    void RenderClassicStream(
        TerminalPane& pane,
        const Theme& theme,
        float px,
        float py,
        float pw,
        float ph,
        bool cursorBlink,
        int cursorStyle,
        POINT mousePos,
        bool isCtrlDown,
        const std::wstring& searchQuery,
        int searchMatchIndex,
        int& outTotalMatches,
        const GitStatusInfo& gitInfo = GitStatusInfo{},
        std::mutex& blockMutex = GetFallbackMutex(),
        const AppSettings& settings = AppSettings{},
        const std::vector<ProjectTask>& detectedTasks = {},
        const StyledLine& activeCurrentLine = StyledLine{}
    ) {
        outTotalMatches = 0;
        breadcrumbSegments.clear();

        float paddingX = 18.0f;
        float paddingY = 12.0f;
        float streamX = px + paddingX;
        float streamW = pw - paddingX * 2.0f;

        bool isBottomDocked = (settings.promptPosition == "bottom");
        float promptCardH = 56.0f;
        float scrollClipH = isBottomDocked ? (ph - promptCardH - 8.0f) : ph;

        D2D1_RECT_F streamClip = D2D1::RectF(px, py, px + pw, py + scrollClipH);
        pRenderTarget->PushAxisAlignedClip(streamClip, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        // Pre-calculate content height and layout geometry before drawing
        float availStreamW = (std::max)(100.0f, streamW - 20.0f);
        float cw = (charWidth > 1.0f) ? charWidth : 8.0f;
        int maxStreamCols = (std::max)(20, static_cast<int>(availStreamW / cw));

        float measuredContentH = 0.0f;
        for (const auto& blkPtr : pane.blocks) {
            if (!blkPtr) continue;
            const auto& blk = *blkPtr;
            float blockH = 26.0f + 4.0f;
            if (blk.isCollapsed) {
                blockH += lineHeight;
            } else {
                for (const auto& rawLine : blk.outputLines) {
                    blockH += CountWrappedLines(rawLine, maxStreamCols) * lineHeight;
                }
                if (blk.inlineImage) {
                    float maxW = (std::min)(streamW - 40.0f, 650.0f);
                    float aspect = (blk.imageSize.height > 0) ? (blk.imageSize.width / blk.imageSize.height) : 1.0f;
                    float imgW = maxW;
                    float imgH = imgW / aspect;
                    if (imgH > 400.0f) { imgH = 400.0f; imgW = imgH * aspect; }
                    blockH += imgH + 12.0f;
                }
                if (blk.diagnosis.hasError) {
                    blockH += 4.0f + 28.0f + 8.0f;
                }
            }
            blockH += 8.0f;
            measuredContentH += blockH;
        }

        if (pane.activeRunningBlock) {
            float runH = lineHeight;
            std::lock_guard<std::mutex> lock(blockMutex);
            const auto& srcLines = pane.activeRunningBlock->outputLines;
            size_t count = srcLines.size();
            size_t maxSnap = pane.activeRunningBlock->isAltBuffer ? 50 : 150;
            size_t startI = (count > maxSnap) ? (count - maxSnap) : 0;
            if (pane.activeRunningBlock->isAltBuffer) {
                runH += (count - startI) * lineHeight;
            } else {
                for (size_t i = startI; i < count; ++i) {
                    runH += CountWrappedLines(srcLines[i], maxStreamCols) * lineHeight;
                }
            }
            if (!activeCurrentLine.rawText.empty()) {
                runH += CountWrappedLines(activeCurrentLine, maxStreamCols) * lineHeight;
            }
            runH += 8.0f;
            measuredContentH += runH;
        }

        pane.totalContentHeight = measuredContentH;
        float streamTop = py + paddingY;
        float streamBottom = py + scrollClipH - 4.0f;
        float visibleH = (std::max)(50.0f, streamBottom - streamTop);
        float maxScrollY = (measuredContentH > visibleH) ? (measuredContentH - visibleH) : 0.0f;

        if (pane.autoScrollToBottom) {
            pane.scrollOffsetY = maxScrollY;
        } else {
            pane.scrollOffsetY = (std::clamp)(pane.scrollOffsetY, 0.0f, maxScrollY);
        }

        float curY = 0.0f;
        if (isBottomDocked && settings.streamStart == "bottom") {
            if (measuredContentH <= visibleH) {
                curY = streamBottom - measuredContentH;
            } else {
                curY = streamTop - pane.scrollOffsetY;
            }
        } else {
            curY = streamTop - pane.scrollOffsetY;
        }

        // Render Empty State HUD if there are no blocks and no active command running
        if (pane.blocks.empty() && !pane.activeRunningBlock) {
            RenderEmptyStateHud(theme, pane.cwd, gitInfo, detectedTasks, px, py, pw, scrollClipH, mousePos);
        }

        // Render Command Blocks (#1)
        for (size_t bIdx = 0; bIdx < pane.blocks.size(); ++bIdx) {
            auto& blk = *pane.blocks[bIdx];
            float blkStartY = curY;
            float headerH = 26.0f;

            blk.headerRect = D2D1::RectF(streamX, curY, streamX + streamW, curY + headerH);
            bool isHdrHover = PtInD2D1Rect(blk.headerRect, mousePos);

            // Block Header background & subtle border
            pSolidBrush->SetColor(isHdrHover ? ColorHex(0x181822) : ColorHex(0x121218));
            pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(blk.headerRect, 4.0f, 4.0f), pSolidBrush);
            pSolidBrush->SetColor(ColorHex(0x262634));
            pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(blk.headerRect, 4.0f, 4.0f), pSolidBrush, 1.0f);

            // Command text directly without redundant icon or shell badge
            float maxHdrCmdW = (streamW > 160.0f) ? (streamW - 160.0f) : 40.0f;
            std::wstring safeHdrCmd = TruncateTextToWidth(blk.commandText, maxHdrCmdW, pTermFormatBold);
            DrawText(safeHdrCmd, streamX + 10.0f, curY + 5.0f, theme.textPrimary, pTermFormatBold);

            // Exit Code Badge: EXIT 0 or EXIT 1
            float rightCur = streamX + streamW - 6.0f;
            std::wstring exitBadge = (blk.exitCode == 0) ? L"EXIT 0" : (L"EXIT " + std::to_wstring(blk.exitCode));
            float exitW = MeasureStringWidth(exitBadge, pUIFormatSmall);
            D2D1_COLOR_F exitCol = (blk.exitCode == 0) ? ColorHex(0x22C55E) : ColorHex(0xEF4444);
            DrawText(exitBadge, rightCur - exitW - 4.0f, curY + 5.5f, exitCol, pUIFormatSmall);
            rightCur -= (exitW + 16.0f);

            // Duration badge
            if (blk.executionTimeMs > 0.0) {
                wchar_t durBuf[32];
                if (blk.executionTimeMs < 1000.0) swprintf_s(durBuf, L"%.0fms", blk.executionTimeMs);
                else swprintf_s(durBuf, L"%.2fs", blk.executionTimeMs / 1000.0);
                float durW = MeasureStringWidth(durBuf, pUIFormatSmall);
                DrawText(durBuf, rightCur - durW - 4.0f, curY + 5.5f, ColorHex(0x78788C), pUIFormatSmall);
                rightCur -= (durW + 14.0f);
            }

            // Action Pills on Header Hover: [Copy], [Fold], [Rerun], [Pin], [Export]
            if (isHdrHover) {
                auto DrawActionPill = [&](D2D1_RECT_F& outRect, const std::wstring& label, float w) {
                    outRect = D2D1::RectF(rightCur - w, curY + 3.0f, rightCur, curY + headerH - 3.0f);
                    bool hov = PtInD2D1Rect(outRect, mousePos);
                    pSolidBrush->SetColor(hov ? ColorHex(0x2A2A3A) : ColorHex(0x1E1E28));
                    pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(outRect, 3.0f, 3.0f), pSolidBrush);
                    pSolidBrush->SetColor(ColorHex(0x36364A));
                    pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(outRect, 3.0f, 3.0f), pSolidBrush, 1.0f);
                    DrawText(label, outRect.left + 5.0f, curY + 4.5f, hov ? theme.accent : ColorHex(0xD4D4E0), pUIFormatSmall);
                    rightCur -= (w + 6.0f);
                };

                DrawActionPill(blk.copyButtonRect, L"Copy", 36.0f);
                DrawActionPill(blk.collapseButtonRect, blk.isCollapsed ? L"Expand" : L"Fold", 44.0f);
                DrawActionPill(blk.rerunButtonRect, L"Rerun", 38.0f);
                DrawActionPill(blk.pinButtonRect, L"Pin", 30.0f);
                DrawActionPill(blk.exportButtonRect, L"Export", 42.0f);
            } else {
                blk.copyButtonRect = D2D1::RectF(0, 0, 0, 0);
                blk.collapseButtonRect = D2D1::RectF(0, 0, 0, 0);
                blk.rerunButtonRect = D2D1::RectF(0, 0, 0, 0);
                blk.pinButtonRect = D2D1::RectF(0, 0, 0, 0);
                blk.exportButtonRect = D2D1::RectF(0, 0, 0, 0);
            }

            curY += headerH + 4.0f;

            // Render Output Lines if not collapsed
            if (blk.isCollapsed) {
                wchar_t foldBuf[64];
                swprintf_s(foldBuf, L"[Output folded - %zu lines]", blk.outputLines.size());
                DrawText(foldBuf, streamX + 10.0f, curY, ColorHex(0x606074), pUIFormatSmall);
                curY += lineHeight;
            } else {
                float availStreamW = (std::max)(100.0f, streamW - 20.0f);
                float cw = (charWidth > 1.0f) ? charWidth : 8.0f;
                int maxStreamCols = (std::max)(20, static_cast<int>(availStreamW / cw));

                for (const auto& rawLine : blk.outputLines) {
                    std::vector<StyledLine> wrappedLines = WrapStyledLine(rawLine, maxStreamCols);
                    for (const auto& line : wrappedLines) {
                        // Search Highlight
                        if (!searchQuery.empty()) {
                            std::wstring lowerLine = ToLower(line.rawText);
                            std::wstring lowerQuery = ToLower(searchQuery);
                            size_t sPos = 0;
                            while ((sPos = lowerLine.find(lowerQuery, sPos)) != std::wstring::npos) {
                                outTotalMatches++;
                                bool isCurrentMatch = (outTotalMatches == searchMatchIndex);

                                std::wstring beforeMatch = line.rawText.substr(0, sPos);
                                float hlX = streamX + MeasureStringWidth(beforeMatch, pTermFormat);
                                std::wstring matchText = line.rawText.substr(sPos, searchQuery.size());
                                float hlW = MeasureStringWidth(matchText, pTermFormat);

                                D2D1_RECT_F hlRect = D2D1::RectF(hlX, curY, hlX + hlW, curY + lineHeight);
                                pSolidBrush->SetColor(isCurrentMatch ? ColorRgba(1.0f, 0.60f, 0.10f, 0.70f) : ColorRgba(1.0f, 0.85f, 0.20f, 0.35f));
                                pRenderTarget->FillRectangle(hlRect, pSolidBrush);

                                sPos += lowerQuery.size();
                            }
                        }

                        float lineBottom = curY + lineHeight;
                        if (lineBottom >= py - 4.0f && curY <= py + ph + 4.0f) {
                            float spanX = streamX;
                            for (const auto& span : line.spans) {
                                IDWriteTextFormat* tf = span.bold ? pTermFormatBold : pTermFormat;
                                DrawText(span.text, spanX, curY, span.fg, tf);
                                spanX += MeasureStringWidth(span.text, tf);
                            }

                            // Detect Clickable URLs
                            auto links = FindLinksInText(line.rawText);
                            for (const auto& lk : links) {
                                std::wstring beforeLk = line.rawText.substr(0, lk.startChar);
                                float lkX = streamX + MeasureStringWidth(beforeLk, pTermFormat);
                                float lkW = MeasureStringWidth(lk.url, pTermFormat);
                                D2D1_RECT_F lkRect = D2D1::RectF(lkX, curY, lkX + lkW, curY + lineHeight);
                                frameLinks.push_back({ lkRect, lk.url, lk.isUrl });

                                if (PtInD2D1Rect(lkRect, mousePos) && isCtrlDown) {
                                    pSolidBrush->SetColor(theme.accent);
                                    pRenderTarget->DrawLine(D2D1::Point2F(lkX, curY + lineHeight - 2.0f), D2D1::Point2F(lkX + lkW, curY + lineHeight - 2.0f), pSolidBrush, 1.2f);
                                }
                            }
                        }

                        curY += lineHeight;
                    }
                }

                // Inline Image
                if (blk.inlineImage) {
                    float maxW = (std::min)(streamW - 40.0f, 650.0f);
                    float aspect = (blk.imageSize.height > 0) ? (blk.imageSize.width / blk.imageSize.height) : 1.0f;
                    float imgW = maxW;
                    float imgH = imgW / aspect;
                    if (imgH > 400.0f) { imgH = 400.0f; imgW = imgH * aspect; }

                    D2D1_RECT_F imgRect = D2D1::RectF(streamX, curY + 4.0f, streamX + imgW, curY + 4.0f + imgH);
                    pRenderTarget->DrawBitmap(blk.inlineImage, imgRect);
                    pSolidBrush->SetColor(theme.blockBorder);
                    pRenderTarget->DrawRectangle(imgRect, pSolidBrush, 1.0f);
                    curY += imgH + 12.0f;
                }

                // Smart Error Doctor Inline Callout
                if (blk.diagnosis.hasError) {
                    curY += 4.0f;
                    float barH = 28.0f;
                    D2D1_RECT_F errBarRect = D2D1::RectF(streamX, curY, streamX + streamW, curY + barH);
                    pSolidBrush->SetColor(ColorRgba(1.0f, 0.50f, 0.67f, 0.07f));
                    pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(errBarRect, 4.0f, 4.0f), pSolidBrush);

                    pSolidBrush->SetColor(ColorHex(0xFF80AA));
                    pRenderTarget->FillRectangle(D2D1::RectF(streamX, curY, streamX + 2.0f, curY + barH), pSolidBrush);

                    D2D1_RECT_F errIconRect = D2D1::RectF(streamX + 8.0f, curY + 4.0f, streamX + 26.0f, curY + 24.0f);
                    DrawFluentIcon(0xE783, errIconRect, ColorHex(0xFF80AA), pIconFormatSmall);

                    DrawText(blk.diagnosis.errorTitle, streamX + 30.0f, curY + 6.0f, ColorHex(0xFF99BB), pUIFormatSmall);

                    float fixRight = streamX + streamW - 8.0f;
                    if (!blk.diagnosis.suggestedFix.empty()) {
                        LanguageMode lang = I18n::GetResolvedLanguage();
                        std::wstring fixPrefix = (lang == LanguageMode::PtBR) ? L"Aplicar: " : ((lang == LanguageMode::EsES) ? L"Aplicar: " : L"Apply: ");
                        std::wstring fixBtn = fixPrefix + blk.diagnosis.suggestedFix;
                        float fixW = MeasureStringWidth(fixBtn, pUIFormatSmall) + 26.0f;
                        blk.fixButtonRect = D2D1::RectF(fixRight - fixW, curY + 3.0f, fixRight, curY + barH - 3.0f);

                        bool fxHov = PtInD2D1Rect(blk.fixButtonRect, mousePos);
                        pSolidBrush->SetColor(fxHov ? ColorHex(0x282834) : ColorHex(0x1E1E26));
                        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(blk.fixButtonRect, 4.0f, 4.0f), pSolidBrush);
                        pSolidBrush->SetColor(ColorHex(0x343442));
                        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(blk.fixButtonRect, 4.0f, 4.0f), pSolidBrush, 1.0f);

                        D2D1_RECT_F playR = D2D1::RectF(fixRight - fixW + 4.0f, curY + 4.0f, fixRight - fixW + 18.0f, curY + barH - 4.0f);
                        DrawFluentIcon(0xE768, playR, ColorHex(0x66FFD9), pIconFormatSmall);
                        DrawText(fixBtn, fixRight - fixW + 20.0f, curY + 5.0f, theme.textPrimary, pUIFormatSmall);
                        fixRight -= (fixW + 8.0f);
                    }

                    if (!blk.diagnosis.sourceFile.empty()) {
                        LanguageMode lang = I18n::GetResolvedLanguage();
                        std::wstring openPrefix = (lang == LanguageMode::PtBR) ? L"Abrir " : ((lang == LanguageMode::EsES) ? L"Abrir " : L"Open ");
                        std::wstring fileLink = openPrefix + blk.diagnosis.sourceFile + L":" + std::to_wstring(blk.diagnosis.sourceLine);
                        float flW = MeasureStringWidth(fileLink, pUIFormatSmall) + 16.0f;
                        blk.fileLinkRect = D2D1::RectF(fixRight - flW, curY + 3.0f, fixRight, curY + barH - 3.0f);

                        bool flHov = PtInD2D1Rect(blk.fileLinkRect, mousePos);
                        pSolidBrush->SetColor(flHov ? ColorHex(0x282834) : ColorHex(0x1E1E26));
                        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(blk.fileLinkRect, 4.0f, 4.0f), pSolidBrush);
                        pSolidBrush->SetColor(ColorHex(0x343442));
                        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(blk.fileLinkRect, 4.0f, 4.0f), pSolidBrush, 1.0f);
                        DrawText(fileLink, fixRight - flW + 8.0f, curY + 5.0f, theme.accent, pUIFormatSmall);
                    }

                    curY += barH + 8.0f;
                }
            }

            curY += 8.0f;
            blk.blockRect = D2D1::RectF(streamX, blkStartY, streamX + streamW, curY);
        }

        // Active running command if any (thread-safe snapshot under blockMutex)
        if (pane.activeRunningBlock) {
            float cmdLineY = curY;
            std::wstring promptTag;
            std::wstring runCmdText;
            std::wstring runWorkDir;
            std::vector<StyledLine> runLinesSnapshot;
            bool isAltMode = false;
            {
                std::lock_guard<std::mutex> lock(blockMutex);
                if (pane.activeRunningBlock) {
                    isAltMode = pane.activeRunningBlock->isAltBuffer;
                    promptTag = (pane.activeRunningBlock->modeUsed == ShellMode::ModePW) ? L"PS " : L"";
                    runCmdText = pane.activeRunningBlock->commandText;
                    runWorkDir = pane.activeRunningBlock->workingDirectory;
                    const auto& srcLines = pane.activeRunningBlock->outputLines;
                    size_t count = srcLines.size();
                    size_t maxSnap = isAltMode ? 50 : 150;
                    size_t startI = (count > maxSnap) ? (count - maxSnap) : 0;
                    runLinesSnapshot.reserve(count - startI + 1);
                    for (size_t i = startI; i < count; ++i) {
                        runLinesSnapshot.push_back(srcLines[i]);
                    }
                    if (!activeCurrentLine.rawText.empty()) {
                        runLinesSnapshot.push_back(activeCurrentLine);
                    }
                }
            }

            DrawText(promptTag, streamX, cmdLineY, theme.promptPw, pTermFormatBold);
            float tagW = MeasureStringWidth(promptTag, pTermFormatBold);

            DrawText(runWorkDir, streamX + tagW, cmdLineY, theme.textDim, pTermFormat);
            float dirW = MeasureStringWidth(runWorkDir, pTermFormat);

            std::wstring promptSep = L"> ";
            DrawText(promptSep, streamX + tagW + dirW, cmdLineY, theme.promptPw, pTermFormatBold);
            float sepW = MeasureStringWidth(promptSep, pTermFormatBold);

            std::wstring runStatus = std::wstring(L"  ") + I18n::Get(StringId::Prompt_Running);
            float runStatusW = MeasureStringWidth(runStatus, pTermFormatBold);
            float cmdStartX = streamX + tagW + dirW + sepW;
            float maxRunCmdW = (streamX + streamW) - cmdStartX - runStatusW - 10.0f;
            if (maxRunCmdW < 40.0f) maxRunCmdW = 40.0f;
            std::wstring truncatedRunCmd = TruncateTextToWidth(runCmdText, maxRunCmdW, pTermFormatBold);

            DrawText(truncatedRunCmd + runStatus, cmdStartX, cmdLineY, theme.accent, pTermFormatBold);
            curY += lineHeight;

            float availRunW = (std::max)(100.0f, streamW - 20.0f);
            float cwRun = (charWidth > 1.0f) ? charWidth : 8.0f;
            int maxRunCols = (std::max)(20, static_cast<int>(availRunW / cwRun));

            if (isAltMode) {
                for (const auto& line : runLinesSnapshot) {
                    float lineBottom = curY + lineHeight;
                    if (lineBottom >= py - 4.0f && curY <= py + ph + 4.0f) {
                        float spanX = streamX;
                        for (const auto& span : line.spans) {
                            IDWriteTextFormat* tf = span.bold ? pTermFormatBold : pTermFormat;
                            DrawText(span.text, spanX, curY, span.fg, tf);
                            spanX += MeasureStringWidth(span.text, tf);
                        }
                    }
                    curY += lineHeight;
                }
            } else {
                for (const auto& rawLine : runLinesSnapshot) {
                    std::vector<StyledLine> wrappedLines = WrapStyledLine(rawLine, maxRunCols);
                    for (const auto& line : wrappedLines) {
                        float lineBottom = curY + lineHeight;
                        if (lineBottom >= py - 4.0f && curY <= py + ph + 4.0f) {
                            float spanX = streamX;
                            for (const auto& span : line.spans) {
                                IDWriteTextFormat* tf = span.bold ? pTermFormatBold : pTermFormat;
                                DrawText(span.text, spanX, curY, span.fg, tf);
                                spanX += MeasureStringWidth(span.text, tf);
                            }
                        }
                        curY += lineHeight;
                    }
                }
            }
            curY += 8.0f;
        }

        pRenderTarget->PopAxisAlignedClip();

        float promptCardY = isBottomDocked ? (py + ph - promptCardH - 4.0f) : curY;
        D2D1_RECT_F promptCardRect = D2D1::RectF(streamX - 6.0f, promptCardY - 2.0f, streamX + streamW + 6.0f, promptCardY + promptCardH);

        // Card surface with subtle border
        FillAdaptiveBox(promptCardRect, theme, theme.blockBg, theme.blockBorder, 4.0f, false);

        // Prompt Line 1: blue_folder.png + Clickable Breadcrumbs + radio_tower.png Git + Duration + Live Process Pill
        float line1Y = promptCardY + 4.0f;
        D2D1_RECT_F folR = D2D1::RectF(streamX, line1Y + 1.0f, streamX + 16.0f, line1Y + 17.0f);
        SpriteAtlas::Instance().DrawSpriteCentered(pRenderTarget, SpriteId::BlueFolder, folR, 16.0f, 16.0f);

        // Clickable Breadcrumbs (#2)
        auto segments = SplitPathBreadcrumbs(pane.cwd);
        float bcCurX = streamX + 22.0f;
        promptFolderRect = D2D1::RectF(streamX, line1Y, bcCurX, line1Y + 18.0f);

        for (size_t s = 0; s < segments.size(); ++s) {
            float segW = MeasureStringWidth(segments[s].name, pUIFormatSmall);
            D2D1_RECT_F segRect = D2D1::RectF(bcCurX, line1Y, bcCurX + segW, line1Y + 18.0f);
            breadcrumbSegments.push_back({ segRect, segments[s].fullPath });

            bool segHov = PtInD2D1Rect(segRect, mousePos);
            DrawText(segments[s].name, bcCurX, line1Y + 1.0f, segHov ? theme.accent : ColorHex(0x7AA2F7), pUIFormatSmall);
            bcCurX += segW;

            if (s + 1 < segments.size()) {
                DrawText(L" > ", bcCurX, line1Y + 1.0f, ColorHex(0x4A4A60), pUIFormatSmall);
                bcCurX += 16.0f;
            }
        }
        promptFolderRect.right = bcCurX;

        float ctxCurX = bcCurX + 16.0f;

        // Git badge in Line 1
        if (gitInfo.isGitRepo) {
            std::wstring gitTxt = gitInfo.branchName + (gitInfo.isDirty ? L" *" : L"");
            float gitTxtW = MeasureStringWidth(gitTxt, pUIFormatSmall);
            promptGitBadgeRect = D2D1::RectF(ctxCurX, line1Y - 1.0f, ctxCurX + 22.0f + gitTxtW, line1Y + 18.0f);
            statusGitRect = promptGitBadgeRect;

            bool gitHov = PtInD2D1Rect(promptGitBadgeRect, mousePos);
            D2D1_RECT_F radR = D2D1::RectF(ctxCurX, line1Y + 1.0f, ctxCurX + 16.0f, line1Y + 17.0f);
            SpriteAtlas::Instance().DrawSpriteCentered(pRenderTarget, SpriteId::RadioTower, radR, 14.0f, 14.0f);
            DrawText(gitTxt, ctxCurX + 20.0f, line1Y + 1.0f, gitHov ? ColorHex(0xFFFFFF) : (gitInfo.isDirty ? ColorHex(0xFACC15) : ColorHex(0x4ADE80)), pUIFormatSmall);
            ctxCurX += (22.0f + gitTxtW + 14.0f);
        } else {
            promptGitBadgeRect = D2D1::RectF(0, 0, 0, 0);
            statusGitRect = D2D1::RectF(0, 0, 0, 0);
        }

        // Live Process Pill (#6) if running
        if (pane.activeRunningBlock) {
            std::wstring runCmd = pane.activeRunningBlock->commandText;
            if (runCmd.size() > 18) runCmd = runCmd.substr(0, 15) + L"...";

            DWORD elapsedMs = (pane.activeRunningBlock->startTick > 0) ? (GetTickCount() - pane.activeRunningBlock->startTick) : 0;
            int totalSec = static_cast<int>(elapsedMs / 1000);
            int mins = totalSec / 60;
            int secs = totalSec % 60;
            int tenths = static_cast<int>((elapsedMs % 1000) / 100);
            wchar_t timeBuf[32];
            swprintf_s(timeBuf, L"%02d:%02d.%1ds", mins, secs, tenths);
            std::wstring procTxt = runCmd + L" (" + timeBuf + L")";

            float pTxtW = MeasureStringWidth(procTxt, pUIFormatSmall);
            float killBtnW = 92.0f;

            liveProcessPillRect = D2D1::RectF(ctxCurX, line1Y - 1.0f, ctxCurX + 26.0f + pTxtW + killBtnW + 12.0f, line1Y + 19.0f);
            pSolidBrush->SetColor(ColorHex(0x1F2A18));
            pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(liveProcessPillRect, 3.0f, 3.0f), pSolidBrush);
            pSolidBrush->SetColor(ColorHex(0x22C55E));
            pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(liveProcessPillRect, 3.0f, 3.0f), pSolidBrush, 1.0f);

            D2D1_RECT_F monR = D2D1::RectF(ctxCurX + 4.0f, line1Y + 1.0f, ctxCurX + 20.0f, line1Y + 17.0f);
            SpriteAtlas::Instance().DrawSpriteCentered(pRenderTarget, SpriteId::CrtMonitor, monR, 14.0f, 14.0f);
            DrawText(procTxt, ctxCurX + 24.0f, line1Y + 1.5f, ColorHex(0x4ADE80), pUIFormatSmall);

            // [KILL • Ctrl+C] button inside pill
            liveProcessKillBtnRect = D2D1::RectF(liveProcessPillRect.right - killBtnW - 4.0f, line1Y + 1.0f, liveProcessPillRect.right - 4.0f, line1Y + 17.0f);
            bool kHov = PtInD2D1Rect(liveProcessKillBtnRect, mousePos);
            pSolidBrush->SetColor(kHov ? ColorHex(0xDC2626) : ColorHex(0x7F1D1D));
            pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(liveProcessKillBtnRect, 2.0f, 2.0f), pSolidBrush);
            std::wstring killLabel = L"KILL \x2022 Ctrl+C";
            float killTxtW = MeasureStringWidth(killLabel, pUIFormatSmall);
            DrawText(killLabel, liveProcessKillBtnRect.left + (killBtnW - killTxtW) * 0.5f, line1Y + 1.5f, ColorHex(0xFFFFFF), pUIFormatSmall);

            ctxCurX = liveProcessPillRect.right + 14.0f;
        } else {
            liveProcessPillRect = D2D1::RectF(0, 0, 0, 0);
            liveProcessKillBtnRect = D2D1::RectF(0, 0, 0, 0);
        }

        // Prompt Line 2: Chevron > + Input Tokens + Ghost Autocomplete + Cursor
        float line2Y = line1Y + 24.0f;
        bool hasLastExitError = (!pane.blocks.empty() && pane.blocks.back()->exitCode != 0);
        D2D1_COLOR_F symbolCol = hasLastExitError ? ColorHex(0xEF4444) : ColorHex(0x2AC3DE);

        std::wstring promptSymbol = L"> ";
        DrawText(promptSymbol, streamX, line2Y, symbolCol, pTermFormatBold, theme.phosphorBloom);
        float symbolW = MeasureStringWidth(promptSymbol, pTermFormatBold);
        float inputX = streamX + symbolW;

        if (pane.syntaxAnalysis.tokens.empty() && !pane.inputLine.empty()) {
            DrawText(pane.inputLine, inputX, line2Y, theme.textPrimary, pTermFormat, theme.phosphorBloom);
        } else {
            float tokX = inputX;
            for (const auto& tok : pane.syntaxAnalysis.tokens) {
                DrawText(tok.text, tokX, line2Y, tok.color, pTermFormat, theme.phosphorBloom);
                float tokW = MeasureStringWidth(tok.text, pTermFormat);
                if (tok.isUnderlined) {
                    pSolidBrush->SetColor(theme.syntaxInvalidCommand);
                    pRenderTarget->DrawLine(
                        D2D1::Point2F(tokX, line2Y + lineHeight - 2.0f),
                        D2D1::Point2F(tokX + tokW, line2Y + lineHeight - 2.0f),
                        pSolidBrush, 1.5f
                    );
                }
                tokX += tokW;
            }
        }

        // Inline Ghost Autocomplete
        std::wstring ghostSuffix = SyntaxAndAutocomplete::ComputeGhostSuggestion(pane.inputLine, pane.commandHistory, pane.cwd);
        if (!ghostSuffix.empty()) {
            float typedW = MeasureStringWidth(pane.inputLine, pTermFormat);
            DrawText(ghostSuffix, inputX + typedW, line2Y, ColorHex(0x565668), pTermFormat);
        }

        // Cursor
        std::wstring beforeCursor = pane.inputLine.substr(0, (std::min)(pane.cursorPos, pane.inputLine.size()));
        float cursorX = inputX + MeasureStringWidth(beforeCursor, pTermFormat);
        if (cursorBlink) {
            pSolidBrush->SetColor(theme.accent);
            if (cursorStyle == 0) {
                pRenderTarget->FillRectangle(D2D1::RectF(cursorX, line2Y + 2.0f, cursorX + 2.0f, line2Y + lineHeight - 2.0f), pSolidBrush);
            } else if (cursorStyle == 1) {
                pSolidBrush->SetColor(ColorRgba(theme.accent.r, theme.accent.g, theme.accent.b, 0.45f));
                pRenderTarget->FillRectangle(D2D1::RectF(cursorX, line2Y + 2.0f, cursorX + charWidth, line2Y + lineHeight - 2.0f), pSolidBrush);
            } else {
                pRenderTarget->FillRectangle(D2D1::RectF(cursorX, line2Y + lineHeight - 3.0f, cursorX + charWidth, line2Y + lineHeight - 1.0f), pSolidBrush);
            }
        }

        // Keep scroll offset clamped within valid bounds
        if (pane.autoScrollToBottom) {
            pane.scrollOffsetY = maxScrollY;
        } else {
            pane.scrollOffsetY = (std::clamp)(pane.scrollOffsetY, 0.0f, maxScrollY);
        }

        // Render Autocomplete Popup right below input line
        if (pane.autocompleteOpen && !pane.autocompleteSuggestions.empty()) {
            RenderAutocompletePopup(
                theme,
                inputX,
                line2Y + lineHeight + 6.0f,
                pane.autocompleteSuggestions,
                pane.autocompleteIndex,
                pane.previewThumb,
                pane.previewThumbSize,
                pw
            );
        }
    }

    void RenderScrollbarWithMarkers(
        const Theme& theme,
        float px, float py, float pw, float ph,
        float scrollOffsetY,
        float totalContentHeight,
        const std::vector<std::unique_ptr<CommandBlock>>& blocks
    ) {
        if (totalContentHeight <= ph) return;

        float barW = 5.0f;
        float barX = px + pw - 7.0f;

        // Thumb
        float thumbH = (std::max)(24.0f, (ph / totalContentHeight) * ph);
        float thumbY = py + (scrollOffsetY / totalContentHeight) * ph;
        if (thumbY + thumbH > py + ph) thumbY = py + ph - thumbH;

        D2D1_RECT_F thumbRect = D2D1::RectF(barX, thumbY, barX + barW, thumbY + thumbH);
        pSolidBrush->SetColor(ColorRgba(0.94f, 0.94f, 1.0f, 0.20f));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(thumbRect, 2.5f, 2.5f), pSolidBrush);

        // Mini-Map Markers: Cyan block starts, Red errors, Yellow warnings
        for (const auto& blk : blocks) {
            float blockContentY = (blk->blockRect.top > 0.0f) ? (blk->blockRect.top - (py + 16.0f) + scrollOffsetY) : 0.0f;
            float markerRatio = (std::clamp)(blockContentY / (totalContentHeight > 0.0f ? totalContentHeight : 1.0f), 0.0f, 1.0f);
            float markerY = py + markerRatio * ph;

            if (markerY >= py && markerY <= py + ph - 3.0f) {
                if (blk->exitCode != 0) {
                    pSolidBrush->SetColor(ColorHex(0xFF2A55));
                    pRenderTarget->FillRectangle(D2D1::RectF(barX - 1.0f, markerY, barX + barW, markerY + 3.0f), pSolidBrush);
                } else if (blk->diagnosis.hasError) {
                    pSolidBrush->SetColor(ColorHex(0xFFD480));
                    pRenderTarget->FillRectangle(D2D1::RectF(barX - 0.5f, markerY, barX + barW, markerY + 2.0f), pSolidBrush);
                } else {
                    pSolidBrush->SetColor(ColorHex(0x00FFBF));
                    pRenderTarget->FillRectangle(D2D1::RectF(barX, markerY, barX + barW, markerY + 2.0f), pSolidBrush);
                }
            }
        }
    }

    void RenderFileMenu(const Theme& theme, POINT mousePos, float menuX = 6.0f) {
        float mx = menuX;
        if (pRenderTarget) {
            D2D1_SIZE_F sz = pRenderTarget->GetSize();
            if (mx + 260.0f > sz.width - 10.0f) {
                mx = (std::max)(6.0f, sz.width - 260.0f - 10.0f);
            }
        }
        float my = TITLEBAR_HEIGHT + 4.0f;
        float mw = 260.0f;
        float itemH = 30.0f;

        struct ItemDef {
            int id;
            wchar_t glyph;
            std::wstring label;
            std::wstring shortcut;
            bool dividerAfter;
        };

        std::vector<ItemDef> items = {
            { 12, 0xE8B7, I18n::Get(StringId::FileMenu_Workfolder), L"Ctrl+B", true },
            { 1,  0xE8E5, I18n::Get(StringId::FileMenu_RunPs1), L"", false },
            { 2,  0xE943, I18n::Get(StringId::FileMenu_RunPy), L"", false },
            { 3,  0xEB9F, I18n::Get(StringId::FileMenu_OpenImage), L"", true },
            { 8,  0xE74E, I18n::Get(StringId::FileMenu_ExportSession), L"", false },
            { 10, 0xE713, I18n::Get(StringId::FileMenu_Settings), L"Ctrl+,", false },
            { 9,  0xE7E8, I18n::Get(StringId::FileMenu_CloseTab), L"Ctrl+W", false },
            { 11, 0xE8BB, L"Exit", L"Alt+F4", false }
        };

        int dividerCount = 2;
        float mh = items.size() * itemH + dividerCount * 9.0f + 12.0f;
        fileMenuRect = D2D1::RectF(mx, my, mx + mw, my + mh);

        // Subtle soft shadow behind menu
        pSolidBrush->SetColor(ColorRgba(0.0f, 0.0f, 0.0f, 0.45f));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(mx, my + 2.0f, mx + mw, my + mh + 2.0f), 8.0f, 8.0f), pSolidBrush);

        // Menu background (#141418) and subtle border (#282832)
        pSolidBrush->SetColor(ColorHex(0x141418));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(fileMenuRect, 8.0f, 8.0f), pSolidBrush);
        pSolidBrush->SetColor(ColorHex(0x282832));
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(fileMenuRect, 8.0f, 8.0f), pSolidBrush, 1.0f);

        fileMenuItems.clear();
        float curY = my + 6.0f;

        for (const auto& item : items) {
            D2D1_RECT_F itRect = D2D1::RectF(mx + 6.0f, curY, mx + mw - 6.0f, curY + itemH);
            fileMenuItems.push_back({ itRect, item.id });

            bool hov = PtInD2D1Rect(itRect, mousePos);
            if (hov) {
                pSolidBrush->SetColor(ColorHex(0x22222B));
                pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(itRect, 5.0f, 5.0f), pSolidBrush);
            }

            // Left icon column (20x20px at x + 10.0f)
            D2D1_RECT_F icRect = D2D1::RectF(mx + 10.0f, curY + 5.0f, mx + 30.0f, curY + 25.0f);
            DrawFluentIcon(item.glyph, icRect, hov ? theme.accent : ColorHex(0x9090A0), pIconFormatSmall);

            // Label
            DrawText(item.label, mx + 34.0f, curY + 6.0f, ColorHex(0xF0F0FF), pUIFormat);

            // Shortcut on right
            if (!item.shortcut.empty()) {
                float scW = MeasureStringWidth(item.shortcut, pUIFormatSmall);
                DrawText(item.shortcut, mx + mw - scW - 14.0f, curY + 7.0f, ColorHex(0x78788C), pUIFormatSmall);
            }

            curY += itemH;

            // Optional 1px section divider
            if (item.dividerAfter) {
                curY += 4.0f;
                pSolidBrush->SetColor(ColorHex(0x22222A));
                pRenderTarget->DrawLine(D2D1::Point2F(mx + 10.0f, curY), D2D1::Point2F(mx + mw - 10.0f, curY), pSolidBrush, 1.0f);
                curY += 5.0f;
            }
        }
    }

    void RenderProfileMenu(const Theme& theme, POINT mousePos, float menuX) {
        float mx = menuX;
        if (pRenderTarget) {
            D2D1_SIZE_F sz = pRenderTarget->GetSize();
            if (mx + 230.0f > sz.width - 10.0f) {
                mx = (std::max)(6.0f, sz.width - 230.0f - 10.0f);
            }
        }
        float my = TITLEBAR_HEIGHT + 4.0f;
        float mw = 230.0f;
        float itemH = 30.0f;

        struct ProfileItemDef {
            int id;
            wchar_t glyph;
            std::wstring label;
            std::wstring shortcut;
            bool dividerAfter;
        };

        std::vector<ProfileItemDef> items = {
            { 1, 0xE756, I18n::Get(StringId::ProfileMenu_PowerShell), L"Ctrl+T", false },
            { 2, 0xE756, I18n::Get(StringId::ProfileMenu_Cmd), L"Ctrl+Shift+T", false },
            { 3, 0xE7EF, I18n::Get(StringId::ProfileMenu_Admin), L"", true },
            { 4, 0xE745, I18n::Get(StringId::ProfileMenu_SplitRight), L"Alt+Shift+D", false },
            { 5, 0xE74B, I18n::Get(StringId::ProfileMenu_SplitDown), L"Alt+Shift+V", false }
        };

        int dividerCount = 1;
        float mh = items.size() * itemH + dividerCount * 9.0f + 12.0f;
        profileMenuRect = D2D1::RectF(mx, my, mx + mw, my + mh);

        pSolidBrush->SetColor(ColorRgba(0.0f, 0.0f, 0.0f, 0.45f));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(mx, my + 2.0f, mx + mw, my + mh + 2.0f), 8.0f, 8.0f), pSolidBrush);

        pSolidBrush->SetColor(ColorHex(0x141418));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(profileMenuRect, 8.0f, 8.0f), pSolidBrush);
        pSolidBrush->SetColor(ColorHex(0x282832));
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(profileMenuRect, 8.0f, 8.0f), pSolidBrush, 1.0f);

        profileMenuItems.clear();
        float curY = my + 6.0f;

        for (const auto& item : items) {
            D2D1_RECT_F itRect = D2D1::RectF(mx + 6.0f, curY, mx + mw - 6.0f, curY + itemH);
            profileMenuItems.push_back({ itRect, item.id });

            bool hov = PtInD2D1Rect(itRect, mousePos);
            if (hov) {
                pSolidBrush->SetColor(ColorHex(0x22222B));
                pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(itRect, 5.0f, 5.0f), pSolidBrush);
            }

            D2D1_RECT_F icRect = D2D1::RectF(mx + 10.0f, curY + 5.0f, mx + 30.0f, curY + 25.0f);
            D2D1_COLOR_F icCol = (item.id == 1) ? ColorHex(0x58B8FF) : (item.id == 2 ? ColorHex(0xFFB454) : (hov ? theme.accent : ColorHex(0x9090A0)));
            DrawFluentIcon(item.glyph, icRect, icCol, pIconFormatSmall);

            DrawText(item.label, mx + 34.0f, curY + 6.0f, ColorHex(0xF0F0FF), pUIFormat);

            if (!item.shortcut.empty()) {
                float scW = MeasureStringWidth(item.shortcut, pUIFormatSmall);
                DrawText(item.shortcut, mx + mw - scW - 14.0f, curY + 7.0f, ColorHex(0x78788C), pUIFormatSmall);
            }

            curY += itemH;
            if (item.dividerAfter) {
                curY += 4.0f;
                pSolidBrush->SetColor(ColorHex(0x22222A));
                pRenderTarget->DrawLine(D2D1::Point2F(mx + 10.0f, curY), D2D1::Point2F(mx + mw - 10.0f, curY), pSolidBrush, 1.0f);
                curY += 5.0f;
            }
        }
    }

    void RenderGitPopover(const Theme& theme, const GitStatusInfo& gitInfo, POINT mousePos) {
        float gx = (statusGitRect.left > 0.0f) ? statusGitRect.left : 12.0f;
        float gw = 280.0f;
        float itemH = 28.0f;

        struct GitAction {
            int id;
            std::wstring label;
            std::wstring hint;
        };

        std::vector<GitAction> actions = {
            { 1, L"Git Push (Auto-Upstream)", L"push -u origin <branch>" },
            { 2, L"Git Pull --rebase", L"sincroniza com remoto" },
            { 3, L"Git Status Detalhado", L"gstatus instantâneo" },
            { 4, L"Git Log (Últimos Commits)", L"glog árvore de commits" }
        };

        float gh = (actions.size() + 1 + gitInfo.localBranches.size()) * itemH + 24.0f;
        float gy = statusGitRect.bottom + 4.0f;
        if (pRenderTarget) {
            D2D1_SIZE_F sz = pRenderTarget->GetSize();
            if (gy + gh > sz.height - 10.0f) {
                gy = (std::max)(TITLEBAR_HEIGHT + 4.0f, statusGitRect.top - gh - 4.0f);
            }
            if (gx + gw > sz.width - 10.0f) {
                gx = (std::max)(10.0f, sz.width - gw - 10.0f);
            }
        }

        gitPopoverRect = D2D1::RectF(gx, gy, gx + gw, gy + gh);

        // Shadow & Body
        pSolidBrush->SetColor(ColorRgba(0.0f, 0.0f, 0.0f, 0.40f));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(gx, gy + 2.0f, gx + gw, gy + gh + 2.0f), 8.0f, 8.0f), pSolidBrush);

        pSolidBrush->SetColor(ColorHex(0x141418));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(gitPopoverRect, 8.0f, 8.0f), pSolidBrush);
        pSolidBrush->SetColor(ColorHex(0x282832));
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(gitPopoverRect, 8.0f, 8.0f), pSolidBrush, 1.0f);

        gitPopoverItems.clear();
        gitBranchSwitchItems.clear();

        float curY = gy + 8.0f;
        for (const auto& act : actions) {
            D2D1_RECT_F itRect = D2D1::RectF(gx + 6.0f, curY, gx + gw - 6.0f, curY + itemH);
            gitPopoverItems.push_back({ itRect, act.id });

            bool hov = PtInD2D1Rect(itRect, mousePos);
            if (hov) {
                pSolidBrush->SetColor(ColorHex(0x22222B));
                pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(itRect, 4.0f, 4.0f), pSolidBrush);
            }

            DrawText(act.label, gx + 12.0f, curY + 5.0f, hov ? theme.accent : ColorHex(0xF0F0FF), pUIFormatSmall);
            curY += itemH;
        }

        // Divider
        pSolidBrush->SetColor(ColorHex(0x22222A));
        pRenderTarget->DrawLine(D2D1::Point2F(gx + 10.0f, curY + 4.0f), D2D1::Point2F(gx + gw - 10.0f, curY + 4.0f), pSolidBrush, 1.0f);
        curY += 8.0f;

        DrawText(L"Branches Locais:", gx + 12.0f, curY, ColorHex(0x78788C), pUIFormatSmall);
        curY += 20.0f;

        for (const auto& br : gitInfo.localBranches) {
            D2D1_RECT_F brRect = D2D1::RectF(gx + 6.0f, curY, gx + gw - 6.0f, curY + itemH);
            gitBranchSwitchItems.push_back({ brRect, br });

            bool hov = PtInD2D1Rect(brRect, mousePos);
            if (hov) {
                pSolidBrush->SetColor(ColorHex(0x22222B));
                pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(brRect, 4.0f, 4.0f), pSolidBrush);
            }

            bool current = (br == gitInfo.branchName);
            std::wstring brLabel = (current ? L"* " : L"  ") + br;
            DrawText(brLabel, gx + 12.0f, curY + 4.0f, current ? theme.gitClean : ColorHex(0xF0F0FF), pUIFormatSmall);
            curY += itemH;
        }
    }

    void RenderSnippetsSidebar(const Theme& theme, float width, float height, const std::vector<SnippetItem>& snippets, POINT mousePos) {
        float sw = 280.0f;
        float sx = width - sw;
        float sy = TITLEBAR_HEIGHT;
        float sh = height - sy;

        snippetsPopoverRect = D2D1::RectF(sx, sy, width, height);

        // Sidebar background (#0E0E13)
        pSolidBrush->SetColor(ColorHex(0x0E0E13));
        pRenderTarget->FillRectangle(snippetsPopoverRect, pSolidBrush);

        // Left 1px subtle divider line (#22222B)
        pSolidBrush->SetColor(ColorHex(0x22222B));
        pRenderTarget->DrawLine(D2D1::Point2F(sx, sy), D2D1::Point2F(sx, height), pSolidBrush, 1.0f);

        // Top Header: Visual filter box ("Snippets salvos") + Add button [+]
        D2D1_RECT_F filterBox = D2D1::RectF(sx + 12.0f, sy + 12.0f, sx + sw - 44.0f, sy + 42.0f);
        pSolidBrush->SetColor(ColorHex(0x13131A));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(filterBox, 6.0f, 6.0f), pSolidBrush);
        pSolidBrush->SetColor(ColorHex(0x2A2A35));
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(filterBox, 6.0f, 6.0f), pSolidBrush, 1.0f);
        DrawText(L"Snippets salvos", sx + 22.0f, sy + 17.5f, ColorHex(0x78788C), pUIFormatSmall);

        // Add button [+] (26x26px, \uE710 Add)
        D2D1_RECT_F addR = D2D1::RectF(sx + sw - 38.0f, sy + 14.0f, sx + sw - 12.0f, sy + 40.0f);
        snippetsAddCurrentBtnRect = addR;
        bool addHov = PtInD2D1Rect(addR, mousePos);
        pSolidBrush->SetColor(addHov ? ColorHex(0x22222E) : ColorHex(0x16161E));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(addR, 4.0f, 4.0f), pSolidBrush);
        pSolidBrush->SetColor(ColorHex(0x2A2A35));
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(addR, 4.0f, 4.0f), pSolidBrush, 1.0f);
        DrawFluentIcon(0xE710, addR, addHov ? theme.accent : ColorHex(0xF0F0FF), pIconFormatSmall);

        // Snippets list starting at y = TITLEBAR_HEIGHT + 52.0f
        snippetsItemBtns.clear();
        float curY = sy + 52.0f;

        for (size_t i = 0; i < snippets.size(); ++i) {
            const auto& snip = snippets[i];
            D2D1_RECT_F itRect = D2D1::RectF(sx + 10.0f, curY, sx + sw - 10.0f, curY + 46.0f);
            snippetsItemBtns.push_back({ itRect, i });

            bool hov = PtInD2D1Rect(itRect, mousePos);
            if (hov) {
                pSolidBrush->SetColor(ColorHex(0x1A1A23));
                pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(itRect, 6.0f, 6.0f), pSolidBrush);
            }

            // Line 1: Snippet Name (#F0F0FF)
            DrawText(snip.name, sx + 18.0f, curY + 6.0f, hov ? theme.accent : ColorHex(0xF0F0FF), pUIFormatMedium);

            // Line 2: Snippet Command truncated with ellipsis (#78788C)
            std::wstring cmdPreview = snip.command;
            float maxCmdW = sw - 40.0f;
            while (MeasureStringWidth(cmdPreview, pUIFormatSmall) > maxCmdW && cmdPreview.size() > 4) {
                cmdPreview.pop_back();
            }
            if (cmdPreview != snip.command) cmdPreview += L"...";
            DrawText(cmdPreview, sx + 18.0f, curY + 25.0f, ColorHex(0x78788C), pUIFormatSmall);

            curY += 50.0f;
        }
    }

    void RenderSnippetsPopover(const Theme& theme, const std::vector<SnippetItem>& snippets, POINT mousePos) {
        if (!pRenderTarget) return;
        D2D1_SIZE_F sz = pRenderTarget->GetSize();
        RenderSnippetsSidebar(theme, sz.width, sz.height, snippets, mousePos);
    }

    void RenderWorkfolderPanel(
        const Theme& theme,
        float screenW,
        float screenH,
        const std::wstring& cwd,
        const std::vector<WorkfolderItem>& items,
        size_t selectedIndex,
        POINT mousePos,
        const std::wstring& filterQuery = L"",
        const std::string& language = "en-US"
    ) {
        // Dimmed backdrop
        pSolidBrush->SetColor(ColorRgba(0.0f, 0.0f, 0.0f, 0.70f));
        pRenderTarget->FillRectangle(D2D1::RectF(0, 0, screenW, screenH), pSolidBrush);

        float pw = (std::min)(760.0f, screenW - 40.0f);
        float ph = 460.0f;
        float px = (screenW - pw) * 0.5f;
        float py = (screenH - ph) * 0.45f;
        workfolderModalRect = D2D1::RectF(px, py, px + pw, py + ph);

        // Opaque clean box to completely prevent bleed-through from underneath
        FillAdaptiveBox(workfolderModalRect, theme, ColorHex(0x121218), ColorHex(0x323246), 6.0f, true);

        // Header (44px): Win98 folder_16.png + Fuzzy Search Input
        D2D1_RECT_F rdrR = D2D1::RectF(px + 12.0f, py + 12.0f, px + 30.0f, py + 30.0f);
        SpriteAtlas::Instance().DrawSpriteCentered(pRenderTarget, "folder_16.png", rdrR, 18.0f, 18.0f);

        std::vector<WorkfolderItem> filteredItems = WorkfolderScanner::FuzzyFilter(items, filterQuery);

        bool ptbr = (language == "pt-BR");
        std::wstring placeholder = ptbr ? L"Telescope: Digite para buscar arquivos..." : L"Telescope: Type to search files...";
        std::wstring searchPlaceholder = filterQuery.empty() ? placeholder : filterQuery;
        DrawText(searchPlaceholder, px + 40.0f, py + 12.0f, filterQuery.empty() ? ColorHex(0x606078) : ColorHex(0xF0F0FF), pUIFormatBold);

        // Item count badge
        wchar_t cntBuf[48];
        swprintf_s(cntBuf, ptbr ? L"[%zu itens]" : L"[%zu items]", filteredItems.size());
        float cntW = MeasureStringWidth(cntBuf, pUIFormatSmall);
        DrawText(cntBuf, px + pw - cntW - 48.0f, py + 13.0f, ColorHex(0x78788C), pUIFormatSmall);

        // Close button
        workfolderCloseBtnRect = D2D1::RectF(px + pw - 38.0f, py + 8.0f, px + pw - 10.0f, py + 34.0f);
        bool clHov = PtInD2D1Rect(workfolderCloseBtnRect, mousePos);
        pSolidBrush->SetColor(clHov ? ColorHex(0xE81123) : ColorHex(0x1C1C26));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(workfolderCloseBtnRect, 4.0f, 4.0f), pSolidBrush);
        DrawFluentIcon(0xE8BB, workfolderCloseBtnRect, ColorHex(0xFFFFFF), pIconFormatSmall);

        // Header divider
        pSolidBrush->SetColor(ColorHex(0x222230));
        pRenderTarget->DrawLine(D2D1::Point2F(px, py + 44.0f), D2D1::Point2F(px + pw, py + 44.0f), pSolidBrush, 1.0f);

        float leftW = 290.0f;
        float contentY = py + 45.0f;
        float contentH = ph - 45.0f - 40.0f; // footer 40px

        // Vertical divider
        pRenderTarget->DrawLine(D2D1::Point2F(px + leftW, contentY), D2D1::Point2F(px + leftW, contentY + contentH), pSolidBrush, 1.0f);

        // Left Column: Filtered Items
        workfolderItemRects.clear();
        float itemY = contentY + 4.0f;
        float itemH = 26.0f;

        for (size_t i = 0; i < filteredItems.size(); ++i) {
            if (itemY + itemH > contentY + contentH) break;

            const auto& it = filteredItems[i];
            D2D1_RECT_F r = D2D1::RectF(px + 6.0f, itemY, px + leftW - 6.0f, itemY + itemH);
            workfolderItemRects.push_back({ r, i });

            bool isSel = (i == selectedIndex);
            bool hov = PtInD2D1Rect(r, mousePos);

            if (isSel) {
                pSolidBrush->SetColor(ColorHex(0x222234));
                pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(r, 4.0f, 4.0f), pSolidBrush);
                pSolidBrush->SetColor(theme.accent);
                pRenderTarget->FillRectangle(D2D1::RectF(r.left, r.top, r.left + 2.5f, r.bottom), pSolidBrush);
            } else if (hov) {
                pSolidBrush->SetColor(ColorHex(0x181822));
                pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(r, 4.0f, 4.0f), pSolidBrush);
            }

            // Win98 File / Folder Icon
            std::string iconFile = "file_16.png";
            if (it.formattedSize == L"<DRIVE>") iconFile = "hard_disk_16.png";
            else if (it.name == L"..") iconFile = "folder_up_16.png";
            else if (it.isDirectory) iconFile = "folder_16.png";
            else if (it.extension == L".exe") iconFile = "exe_16.png";
            else if (it.isScript || it.extension == L".ps1" || it.extension == L".bat" || it.extension == L".cmd") iconFile = "file_script_16.png";
            else if (it.extension == L".txt" || it.extension == L".md" || it.extension == L".json" || it.extension == L".toml" || it.extension == L".cmake") iconFile = "file_text_16.png";
            else if (it.extension == L".png" || it.extension == L".jpg" || it.extension == L".ico" || it.extension == L".bmp") iconFile = "image_16.png";
            else if (it.extension == L".cpp" || it.extension == L".hpp" || it.extension == L".c" || it.extension == L".h") iconFile = "file_16.png";

            D2D1_RECT_F itIconR = D2D1::RectF(r.left + 6.0f, r.top + 3.0f, r.left + 24.0f, r.bottom - 3.0f);
            SpriteAtlas::Instance().DrawSpriteCentered(pRenderTarget, iconFile, itIconR, 16.0f, 16.0f);

            // Name
            float maxNameW = leftW - 90.0f;
            std::wstring nameDisp = TruncateTextToWidth(it.name, maxNameW, pUIFormatSmall);
            DrawText(nameDisp, r.left + 28.0f, r.top + 4.0f, isSel ? ColorHex(0xFFFFFF) : ColorHex(0xD4D4E0), pUIFormatSmall);

            // Formatted Size
            if (!it.formattedSize.empty()) {
                float szW = MeasureStringWidth(it.formattedSize, pUIFormatSmall);
                DrawText(it.formattedSize, r.right - szW - 6.0f, r.top + 4.0f, ColorHex(0x6A6A80), pUIFormatSmall);
            }

            itemY += itemH;
        }

        // Right Column: Preview & Inspector
        float rightX = px + leftW + 14.0f;
        float rightW = pw - leftW - 28.0f;

        if (selectedIndex < filteredItems.size()) {
            const auto& sel = filteredItems[selectedIndex];

            DrawText(sel.name, rightX, contentY + 8.0f, theme.accent, pUIFormatBold);
            std::wstring metaInfo = sel.isDirectory ? L"Pasta de Diretório" : (sel.extension + L"  |  " + sel.formattedSize);
            DrawText(metaInfo, rightX, contentY + 28.0f, ColorHex(0x78788C), pUIFormatSmall);

            pSolidBrush->SetColor(ColorHex(0x222230));
            pRenderTarget->DrawLine(D2D1::Point2F(rightX, contentY + 46.0f), D2D1::Point2F(px + pw - 12.0f, contentY + 46.0f), pSolidBrush, 1.0f);

            float prevY = contentY + 54.0f;

            if (sel.formattedSize == L"<DRIVE>") {
                // Drive Inspector Card
                D2D1_RECT_F hdR = D2D1::RectF(rightX, prevY, rightX + 64.0f, prevY + 64.0f);
                SpriteAtlas::Instance().DrawSpriteCentered(pRenderTarget, "hard_disk.png", hdR, 48.0f, 48.0f);

                DrawText(ptbr ? L"Unidade de Disco Local" : L"Local Disk Drive", rightX + 74.0f, prevY + 4.0f, ColorHex(0x00FFBF), pUIFormatBold);
                DrawText(ptbr ? L"Dispositivo de Armazenamento Windows" : L"Windows Storage Device", rightX + 74.0f, prevY + 24.0f, ColorHex(0xF0F0FF), pUIFormatSmall);
                DrawText(ptbr ? L"Pressione Enter para navegar nesta unidade" : L"Press Enter to open this drive", rightX + 74.0f, prevY + 42.0f, ColorHex(0x78788C), pUIFormatSmall);

                prevY += 76.0f;
                DrawText(ptbr ? L"Ponto de Montagem:" : L"Mount Point:", rightX, prevY, ColorHex(0x606078), pUIFormatSmall);
                DrawText(sel.fullPath, rightX, prevY + 18.0f, ColorHex(0xA0A0B8), pTermFormatSmall);
            } else if (sel.extension == L".exe" || sel.extension == L".dll") {
                // PE64 Inspector Card
                D2D1_RECT_F peChipR = D2D1::RectF(rightX, prevY, rightX + 64.0f, prevY + 64.0f);
                SpriteAtlas::Instance().DrawSpriteCentered(pRenderTarget, SpriteId::GreenChipClean, peChipR, 56.0f, 56.0f);

                DrawText(L"Formato: Executável Nativo PE32+ (PE64)", rightX + 74.0f, prevY + 4.0f, ColorHex(0x00FFBF), pUIFormatBold);
                DrawText(L"Arquitetura: x86_64 (64-bit Windows Binary)", rightX + 74.0f, prevY + 24.0f, ColorHex(0xF0F0FF), pUIFormatSmall);
                DrawText(L"Subsystem: Windows GUI / Console", rightX + 74.0f, prevY + 42.0f, ColorHex(0x78788C), pUIFormatSmall);

                prevY += 76.0f;
                DrawText(L"Caminho Completo:", rightX, prevY, ColorHex(0x606078), pUIFormatSmall);
                DrawText(sel.fullPath, rightX, prevY + 18.0f, ColorHex(0xA0A0B8), pTermFormatSmall);
            } else if (sel.extension == L".png" || sel.extension == L".jpg" || sel.extension == L".jpeg" || sel.extension == L".ico" || sel.extension == L".bmp") {
                // Image preview via WIC SpriteAtlas
                std::string utf8Path = WideToUtf8(sel.fullPath);
                ID2D1Bitmap* bmp = SpriteAtlas::Instance().GetBitmap(pRenderTarget, utf8Path);
                if (bmp) {
                    D2D1_SIZE_F bsz = bmp->GetSize();
                    wchar_t dimBuf[64];
                    swprintf_s(dimBuf, L"Dimensões: %.0f x %.0f px", bsz.width, bsz.height);
                    DrawText(dimBuf, rightX, prevY, ColorHex(0x00FFBF), pUIFormatBold);
                    DrawText(L"Formato: Imagem Bitmap (WIC Decoded)", rightX, prevY + 20.0f, ColorHex(0x78788C), pUIFormatSmall);

                    float maxThumbW = rightW - 20.0f;
                    float maxThumbH = contentH - 80.0f;
                    float aspect = (bsz.height > 0.0f) ? (bsz.width / bsz.height) : 1.0f;
                    float tw = maxThumbW;
                    float th = tw / aspect;
                    if (th > maxThumbH) { th = maxThumbH; tw = th * aspect; }
                    D2D1_RECT_F thumbR = D2D1::RectF(rightX, prevY + 44.0f, rightX + tw, prevY + 44.0f + th);
                    pRenderTarget->DrawBitmap(bmp, thumbR);
                    pSolidBrush->SetColor(ColorHex(0x34344A));
                    pRenderTarget->DrawRectangle(thumbR, pSolidBrush, 1.0f);
                } else {
                    DrawText(L"Visualização de imagem não disponível", rightX, prevY, ColorHex(0x78788C), pUIFormatSmall);
                }
            } else if (!sel.previewLines.empty()) {
                // Multi-token Syntax preview (up to 24 lines)
                for (size_t l = 0; l < sel.previewLines.size() && l < 24 && prevY + 16.0f <= contentY + contentH; ++l) {
                    wchar_t lNum[16];
                    swprintf_s(lNum, L"%2zu | ", l + 1);
                    DrawText(lNum, rightX, prevY, ColorHex(0x505064), pTermFormatSmall);

                    std::wstring lineTxt = sel.previewLines[l];
                    std::wstring trimmed = Trim(lineTxt);
                    if (StartsWith(trimmed, L"#") || StartsWith(trimmed, L"//")) {
                        DrawText(lineTxt, rightX + 40.0f, prevY, theme.syntaxComment, pTermFormatSmall);
                    } else {
                        float tokX = rightX + 40.0f;
                        size_t cIdx = 0;
                        while (cIdx < lineTxt.size()) {
                            if (iswspace(lineTxt[cIdx])) {
                                tokX += charWidth * 0.80f;
                                cIdx++;
                                continue;
                            }
                            size_t start = cIdx;
                            if (lineTxt[cIdx] == L'\"' || lineTxt[cIdx] == L'\'') {
                                wchar_t q = lineTxt[cIdx++];
                                while (cIdx < lineTxt.size() && lineTxt[cIdx] != q) {
                                    if (lineTxt[cIdx] == L'\\' && cIdx + 1 < lineTxt.size()) cIdx++;
                                    cIdx++;
                                }
                                if (cIdx < lineTxt.size()) cIdx++;
                                std::wstring strTok = lineTxt.substr(start, cIdx - start);
                                DrawText(strTok, tokX, prevY, theme.syntaxString, pTermFormatSmall);
                                tokX += MeasureStringWidth(strTok, pTermFormatSmall);
                            } else {
                                while (cIdx < lineTxt.size() && !iswspace(lineTxt[cIdx]) && lineTxt[cIdx] != L'\"' && lineTxt[cIdx] != L'\'') {
                                    cIdx++;
                                }
                                std::wstring word = lineTxt.substr(start, cIdx - start);
                                D2D1_COLOR_F wCol = theme.textPrimary;
                                if (StartsWith(word, L"$") || StartsWith(word, L"%")) wCol = theme.accent;
                                else if (StartsWith(word, L"-") || StartsWith(word, L"/")) wCol = theme.syntaxFlag;
                                else if (iswdigit(word[0])) wCol = theme.syntaxNumber;
                                else if (word == L"fn" || word == L"def" || word == L"class" || word == L"struct" || word == L"return" ||
                                         word == L"if" || word == L"else" || word == L"for" || word == L"while" || word == L"import" ||
                                         word == L"include" || word == L"const" || word == L"let" || word == L"var" || word == L"function") {
                                    wCol = theme.syntaxKeyword;
                                }
                                DrawText(word, tokX, prevY, wCol, pTermFormatSmall);
                                tokX += MeasureStringWidth(word, pTermFormatSmall);
                            }
                        }
                    }
                    prevY += 16.0f;
                }
            } else if (sel.isDirectory) {
                DrawText(ptbr ? L"Pressione Enter para navegar nesta pasta." : L"Press Enter to navigate this folder.", rightX, prevY + 10.0f, ColorHex(0x78788C), pUIFormatSmall);
            } else {
                DrawText(ptbr ? L"Arquivo binario ou vazio sem pre-visualizacao de texto." : L"Binary or empty file without text preview.", rightX, prevY + 10.0f, ColorHex(0x78788C), pUIFormatSmall);
            }
        }

        // Footer (40px)
        float footY = py + ph - 40.0f;
        pSolidBrush->SetColor(ColorHex(0x222230));
        pRenderTarget->DrawLine(D2D1::Point2F(px, footY), D2D1::Point2F(px + pw, footY), pSolidBrush, 1.0f);

        // Tactical Keybinding Pills across footer with crystal-clear vector keycaps
        float footCurX = px + 12.0f;
        auto DrawFooterPill = [&](D2D1_RECT_F& outRect, const std::vector<std::wstring>& keys, const std::wstring& label) {
            float txtW = MeasureStringWidth(label, pUIFormatSmall);
            float keysW = 0.0f;
            for (size_t i = 0; i < keys.size(); ++i) {
                float tw = MeasureStringWidth(keys[i], pUIFormatBold);
                keysW += (std::max)(18.0f, tw + 12.0f);
                if (i + 1 < keys.size()) keysW += 4.0f;
            }
            float pillW = 8.0f + keysW + 8.0f + txtW + 12.0f;
            outRect = D2D1::RectF(footCurX, footY + 6.0f, footCurX + pillW, footY + 34.0f);
            bool hov = PtInD2D1Rect(outRect, mousePos);
            pSolidBrush->SetColor(hov ? ColorHex(0x242434) : ColorHex(0x161620));
            pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(outRect, 4.0f, 4.0f), pSolidBrush);
            pSolidBrush->SetColor(hov ? theme.accent : ColorHex(0x2E2E3E));
            pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(outRect, 4.0f, 4.0f), pSolidBrush, 1.0f);

            float keyH = 18.0f;
            float keyY = footY + 6.0f + (28.0f - keyH) * 0.5f; // footY + 11.0f
            float used = DrawKeyComboVec(keys, footCurX + 8.0f, keyY, keyH);
            float labelY = footY + 6.0f + (28.0f - 14.0f) * 0.5f; // footY + 13.0f
            DrawText(label, footCurX + 8.0f + used + 8.0f, labelY, hov ? theme.accent : ColorHex(0xD4D4E0), pUIFormatSmall);
            footCurX += (pillW + 8.0f);
        };

        DrawFooterPill(workfolderRunBtnRect, { L"Enter" }, ptbr ? L"Executar / Abrir" : L"Execute / Open");
        DrawFooterPill(workfolderTabBtnRect, { L"Tab" }, ptbr ? L"Inserir no Prompt" : L"Insert to Prompt");
        DrawFooterPill(workfolderExplorerBtnRect, { L"Ctrl", L"E" }, L"Explorer");
        DrawFooterPill(workfolderFooterEscRect, { L"Esc" }, ptbr ? L"Fechar" : L"Close");
    }

    void RenderWorkspaceSidebar(
        const Theme& theme,
        float x, float y, float w, float h,
        const std::wstring& rootDir,
        const GitStatusInfo& gitInfo,
        POINT mousePos,
        const std::string& language = "en-US"
    ) {
        if (!pRenderTarget || w <= 0.0f) return;
        workspaceSidebarRect = D2D1::RectF(x, y, x + w, y + h);

        bool ptbr = (language == "pt-BR");

        // Sidebar Background (#121218)
        pSolidBrush->SetColor(ColorHex(0x121218));
        pRenderTarget->FillRectangle(workspaceSidebarRect, pSolidBrush);

        // Right 1px border (#232330)
        pSolidBrush->SetColor(ColorHex(0x232330));
        pRenderTarget->DrawLine(D2D1::Point2F(x + w - 0.5f, y), D2D1::Point2F(x + w - 0.5f, y + h), pSolidBrush, 1.0f);

        // Top Navigation Header (34px)
        float headerH = 34.0f;
        D2D1_RECT_F hdrRect = D2D1::RectF(x, y, x + w, y + headerH);
        pSolidBrush->SetColor(ColorHex(0x161620));
        pRenderTarget->FillRectangle(hdrRect, pSolidBrush);

        // Collapse button [◀] on top-right of sidebar header
        sidebarCollapseBtnRect = D2D1::RectF(x + w - 28.0f, y + 5.0f, x + w - 6.0f, y + headerH - 5.0f);
        bool colHov = PtInD2D1Rect(sidebarCollapseBtnRect, mousePos);
        pSolidBrush->SetColor(colHov ? ColorHex(0x282838) : ColorHex(0x181824));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(sidebarCollapseBtnRect, 4.0f, 4.0f), pSolidBrush);
        pSolidBrush->SetColor(colHov ? theme.accent : ColorHex(0x323246));
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(sidebarCollapseBtnRect, 4.0f, 4.0f), pSolidBrush, 1.0f);
        DrawChevronLeftIcon(sidebarCollapseBtnRect.left + 5.0f, sidebarCollapseBtnRect.top + 7.0f, 10.0f, colHov ? theme.accent : ColorHex(0x9090A4));

        float tabW = (w - 38.0f) * 0.5f;

        // Tab 1: [Arquivos / Files]
        workspaceFilesTabRect = D2D1::RectF(x + 6.0f, y + 4.0f, x + 6.0f + tabW, y + headerH - 4.0f);
        bool fHov = PtInD2D1Rect(workspaceFilesTabRect, mousePos);
        bool fActive = (workspaceSidebarActiveTab == 0);
        pSolidBrush->SetColor(fActive ? ColorHex(0x222230) : (fHov ? ColorHex(0x1B1B26) : ColorHex(0x161620)));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(workspaceFilesTabRect, 4.0f, 4.0f), pSolidBrush);
        D2D1_RECT_F fIconR = D2D1::RectF(workspaceFilesTabRect.left + 4.0f, workspaceFilesTabRect.top + 2.0f, workspaceFilesTabRect.left + 20.0f, workspaceFilesTabRect.bottom - 2.0f);
        SpriteAtlas::Instance().DrawSpriteCentered(pRenderTarget, "folder_16.png", fIconR, 14.0f, 14.0f);
        DrawText(ptbr ? L"Arquivos" : L"Files", workspaceFilesTabRect.left + 22.0f, workspaceFilesTabRect.top + 4.0f, fActive ? theme.accent : ColorHex(0xCCCCCC), pUIFormatSmall);

        // Tab 2: [Git]
        workspaceGitTabRect = D2D1::RectF(x + 8.0f + tabW, y + 4.0f, x + 8.0f + 2.0f * tabW, y + headerH - 4.0f);
        bool gHov = PtInD2D1Rect(workspaceGitTabRect, mousePos);
        bool gActive = (workspaceSidebarActiveTab == 1);
        pSolidBrush->SetColor(gActive ? ColorHex(0x222230) : (gHov ? ColorHex(0x1B1B26) : ColorHex(0x161620)));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(workspaceGitTabRect, 4.0f, 4.0f), pSolidBrush);
        D2D1_RECT_F gIconR = D2D1::RectF(workspaceGitTabRect.left + 4.0f, workspaceGitTabRect.top + 2.0f, workspaceGitTabRect.left + 20.0f, workspaceGitTabRect.bottom - 2.0f);
        SpriteAtlas::Instance().DrawSpriteCentered(pRenderTarget, "git.png", gIconR, 14.0f, 14.0f);
        DrawText(L"Git", workspaceGitTabRect.left + 22.0f, workspaceGitTabRect.top + 4.0f, gActive ? theme.accent : ColorHex(0xCCCCCC), pUIFormatSmall);

        // Divider below header
        pSolidBrush->SetColor(ColorHex(0x232330));
        pRenderTarget->DrawLine(D2D1::Point2F(x, y + headerH), D2D1::Point2F(x + w, y + headerH), pSolidBrush, 1.0f);

        float itemY = y + headerH + 6.0f;
        float itemH = 24.0f;

        workspaceFileItemRects.clear();
        workspaceFolderItemRects.clear();

        if (workspaceSidebarActiveTab == 0) {
            // [Arquivos] Tab
            std::wstring rootName = rootDir;
            size_t ls = rootName.find_last_of(L"\\/");
            if (ls != std::wstring::npos && ls + 1 < rootName.size()) rootName = rootName.substr(ls + 1);
            if (rootName.empty()) rootName = rootDir;
            DrawText(rootName, x + 10.0f, itemY, theme.accent, pUIFormatBold);
            itemY += 22.0f;

            // Parent folder return item (..) or System Drives when at drive root
            std::wstring rootClean = rootDir;
            while (!rootClean.empty() && (rootClean.back() == L'\\' || rootClean.back() == L'/')) {
                rootClean.pop_back();
            }
            size_t lsParent = rootClean.find_last_of(L"\\/");
            if (lsParent != std::wstring::npos && lsParent >= 2) {
                std::wstring parentDir = rootClean.substr(0, lsParent);
                if (parentDir.size() == 2 && parentDir[1] == L':') parentDir += L"\\";

                D2D1_RECT_F upR = D2D1::RectF(x + 4.0f, itemY, x + w - 4.0f, itemY + itemH);
                bool upHov = PtInD2D1Rect(upR, mousePos);
                if (upHov) {
                    pSolidBrush->SetColor(ColorHex(0x1C1C28));
                    pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(upR, 4.0f, 4.0f), pSolidBrush);
                }
                workspaceFolderItemRects.push_back({ upR, parentDir });

                D2D1_RECT_F upIcR = D2D1::RectF(x + 8.0f, itemY + 2.0f, x + 24.0f, itemY + itemH - 2.0f);
                SpriteAtlas::Instance().DrawSpriteCentered(pRenderTarget, "folder_up.png", upIcR, 14.0f, 14.0f);

                std::wstring upText = ptbr ? L".. (Subir nivel)" : L".. (Parent folder)";
                DrawText(upText, x + 28.0f, itemY + 3.0f, upHov ? ColorHex(0xFFFFFF) : ColorHex(0xA0A0B0), pUIFormatSmall);
                itemY += itemH;
            } else {
                // At drive root (e.g. C:\) -> show other available drives to switch to!
                wchar_t driveBuf[512];
                DWORD driveLen = GetLogicalDriveStringsW(512, driveBuf);
                if (driveLen > 0 && driveLen <= 512) {
                    wchar_t* pDrive = driveBuf;
                    while (*pDrive) {
                        std::wstring dStr = pDrive;
                        if (_wcsicmp(dStr.c_str(), (rootClean + L"\\").c_str()) != 0 && _wcsicmp(dStr.c_str(), rootDir.c_str()) != 0) {
                            D2D1_RECT_F drvR = D2D1::RectF(x + 4.0f, itemY, x + w - 4.0f, itemY + itemH);
                            bool drvHov = PtInD2D1Rect(drvR, mousePos);
                            if (drvHov) {
                                pSolidBrush->SetColor(ColorHex(0x1C1C28));
                                pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(drvR, 4.0f, 4.0f), pSolidBrush);
                            }
                            workspaceFolderItemRects.push_back({ drvR, dStr });

                            D2D1_RECT_F drvIcR = D2D1::RectF(x + 8.0f, itemY + 2.0f, x + 24.0f, itemY + itemH - 2.0f);
                            SpriteAtlas::Instance().DrawSpriteCentered(pRenderTarget, "hard_disk_16.png", drvIcR, 16.0f, 16.0f);

                            std::wstring drvText = L"Drive [" + dStr.substr(0, 2) + L"]";
                            DrawText(drvText, x + 28.0f, itemY + 3.0f, drvHov ? theme.accent : ColorHex(0x80AAFF), pUIFormatBold);
                            itemY += itemH;
                        }
                        pDrive += wcslen(pDrive) + 1;
                    }
                }
            }

            WIN32_FIND_DATAW ffd;
            std::wstring searchPattern = rootDir + L"\\*";
            HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &ffd);

            std::vector<std::pair<std::wstring, bool>> dirEntries;
            if (hFind != INVALID_HANDLE_VALUE) {
                do {
                    std::wstring name = ffd.cFileName;
                    if (name == L"." || name == L".." || name == L".git") continue;
                    bool isDir = (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
                    dirEntries.push_back({ name, isDir });
                } while (FindNextFileW(hFind, &ffd));
                FindClose(hFind);
            }

            std::sort(dirEntries.begin(), dirEntries.end(), [](const auto& a, const auto& b) {
                if (a.second != b.second) return a.second > b.second;
                return _wcsicmp(a.first.c_str(), b.first.c_str()) < 0;
            });

            for (const auto& entry : dirEntries) {
                if (itemY + itemH > y + h - 8.0f) break;

                D2D1_RECT_F itemR = D2D1::RectF(x + 4.0f, itemY, x + w - 4.0f, itemY + itemH);
                bool isHov = PtInD2D1Rect(itemR, mousePos);
                if (isHov) {
                    pSolidBrush->SetColor(ColorHex(0x1C1C28));
                    pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(itemR, 4.0f, 4.0f), pSolidBrush);
                }

                std::wstring fullPath = rootDir + L"\\" + entry.first;
                std::string iconName = "file.png";

                if (entry.second) {
                    iconName = "folder_16.png";
                    workspaceFolderItemRects.push_back({ itemR, fullPath });
                } else {
                    std::wstring ext = entry.first;
                    size_t dot = ext.find_last_of(L'.');
                    if (dot != std::wstring::npos) {
                        std::wstring e = ToLower(ext.substr(dot));
                        if (e == L".py" || e == L".ps1" || e == L".bat" || e == L".cmd") iconName = "file_script.png";
                        else if (e == L".txt" || e == L".md" || e == L".json" || e == L".ini") iconName = "file_text.png";
                        else if (e == L".png" || e == L".jpg" || e == L".jpeg" || e == L".gif" || e == L".bmp" || e == L".ico") iconName = "image.png";
                        else if (e == L".exe") iconName = "exe.png";
                    }
                    workspaceFileItemRects.push_back({ itemR, fullPath });
                }

                bool isDotFile = (!entry.first.empty() && entry.first[0] == L'.');
                float sprOpacity = isDotFile ? 0.65f : 1.0f;
                D2D1_RECT_F icR = D2D1::RectF(x + 8.0f, itemY + 2.0f, x + 24.0f, itemY + itemH - 2.0f);
                SpriteAtlas::Instance().DrawSpriteCentered(pRenderTarget, iconName, icR, 14.0f, 14.0f, sprOpacity);

                D2D1_COLOR_F txtCol = entry.second ? (isDotFile ? ColorHex(0x78788C) : ColorHex(0xE2E8F0)) : (isHov ? ColorHex(0xFFFFFF) : (isDotFile ? ColorHex(0x6A6A7E) : ColorHex(0xA0A0B0)));
                std::wstring dispName = entry.first;
                while (MeasureStringWidth(dispName, pUIFormatSmall) > (w - 38.0f) && dispName.size() > 4) {
                    dispName.pop_back();
                }
                if (dispName != entry.first) dispName += L"...";
                DrawText(dispName, x + 28.0f, itemY + 3.0f, txtCol, pUIFormatSmall);

                itemY += itemH;
            }
        } else {
            // [Git] Tab
            DrawText(ptbr ? L"BRANCH ATIVA" : L"ACTIVE BRANCH", x + 10.0f, itemY, ColorHex(0x78788C), pUIFormatSmall);
            itemY += 18.0f;

            D2D1_RECT_F bR = D2D1::RectF(x + 8.0f, itemY, x + w - 8.0f, itemY + 28.0f);
            pSolidBrush->SetColor(ColorHex(0x1A1A24));
            pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(bR, 4.0f, 4.0f), pSolidBrush);
            pSolidBrush->SetColor(ColorHex(0x2E2E3E));
            pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(bR, 4.0f, 4.0f), pSolidBrush, 1.0f);

            D2D1_RECT_F gic = D2D1::RectF(bR.left + 6.0f, bR.top + 4.0f, bR.left + 22.0f, bR.bottom - 4.0f);
            SpriteAtlas::Instance().DrawSpriteCentered(pRenderTarget, "git.png", gic, 14.0f, 14.0f);
            std::wstring branch = gitInfo.branchName.empty() ? L"detached" : gitInfo.branchName;
            DrawText(branch, bR.left + 26.0f, bR.top + 5.0f, theme.accent, pUIFormatBold);
            itemY += 36.0f;

            DrawText(ptbr ? L"ULTIMO COMMIT" : L"LAST COMMIT", x + 10.0f, itemY, ColorHex(0x78788C), pUIFormatSmall);
            itemY += 18.0f;
            std::wstring cHash = gitInfo.commitHashShort.empty() ? L"---" : gitInfo.commitHashShort;
            DrawText(cHash, x + 10.0f, itemY, ColorHex(0xCCCCCC), pUIFormat);
            itemY += 28.0f;

            DrawText(ptbr ? L"ESTADO" : L"STATUS", x + 10.0f, itemY, ColorHex(0x78788C), pUIFormatSmall);
            itemY += 18.0f;
            if (gitInfo.isDirty) {
                DrawText(ptbr ? L"Modificacoes pendentes" : L"Pending changes", x + 10.0f, itemY, ColorHex(0xF59E0B), pUIFormatBold);
            } else {
                DrawText(ptbr ? L"Working tree limpa" : L"Clean working tree", x + 10.0f, itemY, ColorHex(0x10B981), pUIFormatBold);
            }
        }
    }

    void RenderEditorBody(
        const Theme& theme,
        TerminalTab& tab,
        float px, float py, float pw, float ph,
        POINT mousePos,
        bool hasFocus,
        bool cursorBlink,
        const std::string& language = "en-US"
    ) {
        if (!pRenderTarget || pw <= 0.0f || ph <= 0.0f) return;
        D2D1_RECT_F bodyR = D2D1::RectF(px, py, px + pw, py + ph);

        bool ptbr = (language == "pt-BR");

        // Editor Body Background (#14141C)
        pSolidBrush->SetColor(ColorHex(0x14141C));
        pRenderTarget->FillRectangle(bodyR, pSolidBrush);

        // Top Editor Info Bar (28px)
        float barH = 28.0f;
        D2D1_RECT_F infoBar = D2D1::RectF(px, py, px + pw, py + barH);
        pSolidBrush->SetColor(ColorHex(0x181822));
        pRenderTarget->FillRectangle(infoBar, pSolidBrush);
        pSolidBrush->SetColor(ColorHex(0x262634));
        pRenderTarget->DrawLine(D2D1::Point2F(px, py + barH - 0.5f), D2D1::Point2F(px + pw, py + barH - 0.5f), pSolidBrush, 1.0f);

        // File path indicator + dirty badge
        std::wstring barTxt = tab.editorFilePath;
        if (tab.isDirty) barTxt += ptbr ? L" * [Modificado - Ctrl+S para salvar]" : L" * [Modified - Ctrl+S to save]";
        else barTxt += ptbr ? L" [Salvo]" : L" [Saved]";
        DrawText(barTxt, px + 12.0f, py + 5.0f, tab.isDirty ? ColorHex(0xF59E0B) : ColorHex(0x8E8E9E), pUIFormatSmall);

        // Save Button on the right
        editorSaveBtnRect = D2D1::RectF(px + pw - 80.0f, py + 3.0f, px + pw - 10.0f, py + barH - 3.0f);
        bool sHov = PtInD2D1Rect(editorSaveBtnRect, mousePos);
        pSolidBrush->SetColor(sHov ? ColorHex(0x282838) : ColorHex(0x1E1E2A));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(editorSaveBtnRect, 3.0f, 3.0f), pSolidBrush);
        pSolidBrush->SetColor(tab.isDirty ? theme.accent : ColorHex(0x38384A));
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(editorSaveBtnRect, 3.0f, 3.0f), pSolidBrush, 1.0f);
        std::wstring saveLabel = ptbr ? L"Salvar" : L"Save";
        DrawText(saveLabel, editorSaveBtnRect.left + 16.0f, py + 4.5f, tab.isDirty ? theme.accent : ColorHex(0xA0A0B0), pUIFormatSmall);

        float contentY = py + barH;
        float contentH = ph - barH;

        if (tab.isImagePreview && tab.editorImageBitmap) {
            // Image Preview mode
            float maxW = pw - 40.0f;
            float maxH = contentH - 40.0f;
            float iw = tab.editorImageSize.width;
            float ih = tab.editorImageSize.height;
            if (iw > 0.0f && ih > 0.0f) {
                float scale = (std::min)(1.0f, (std::min)(maxW / iw, maxH / ih));
                float dw = iw * scale;
                float dh = ih * scale;
                float dx = px + (pw - dw) * 0.5f;
                float dy = contentY + (contentH - dh) * 0.5f;
                D2D1_RECT_F imgR = D2D1::RectF(dx, dy, dx + dw, dy + dh);
                pRenderTarget->DrawBitmap(tab.editorImageBitmap, imgR);

                wchar_t szRes[64];
                swprintf_s(szRes, L"%.0f x %.0f px", iw, ih);
                DrawText(szRes, dx + 4.0f, dy + dh + 8.0f, ColorHex(0x78788C), pUIFormatSmall);
            }
        } else {
            // Text Code Editor mode
            float gutterW = 44.0f;
            D2D1_RECT_F gutterR = D2D1::RectF(px, contentY, px + gutterW, py + ph);
            pSolidBrush->SetColor(ColorHex(0x13131A));
            pRenderTarget->FillRectangle(gutterR, pSolidBrush);
            pSolidBrush->SetColor(ColorHex(0x232330));
            pRenderTarget->DrawLine(D2D1::Point2F(px + gutterW, contentY), D2D1::Point2F(px + gutterW, py + ph), pSolidBrush, 1.0f);

            std::vector<std::wstring> lines;
            std::wstringstream ss(tab.editorContent);
            std::wstring line;
            while (std::getline(ss, line)) {
                lines.push_back(line);
            }
            if (lines.empty()) lines.push_back(L"");

            float lineH = 20.0f;
            float textX = px + gutterW + 10.0f;
            float textY = contentY + 6.0f - tab.editorScrollY;

            for (size_t i = 0; i < lines.size(); ++i) {
                if (textY + lineH < contentY) {
                    textY += lineH;
                    continue;
                }
                if (textY > py + ph) break;

                std::wstring lineNum = std::to_wstring(i + 1);
                float nw = MeasureStringWidth(lineNum, pUIFormatSmall);
                DrawText(lineNum, px + gutterW - nw - 8.0f, textY + 2.0f, ColorHex(0x555566), pUIFormatSmall);

                if (i == tab.editorCursorLine) {
                    D2D1_RECT_F curLineR = D2D1::RectF(px + gutterW + 1.0f, textY, px + pw, textY + lineH);
                    pSolidBrush->SetColor(ColorHex(0x1A1A26));
                    pRenderTarget->FillRectangle(curLineR, pSolidBrush);
                }

                DrawText(lines[i], textX, textY + 1.0f, ColorHex(0xE2E8F0), pTermFormat);

                if (hasFocus && i == tab.editorCursorLine && cursorBlink) {
                    std::wstring beforeCursor = lines[i].substr(0, (std::min)(tab.editorCursorCol, lines[i].size()));
                    float cX = textX + MeasureStringWidth(beforeCursor, pTermFormat);
                    pSolidBrush->SetColor(theme.accent);
                    pRenderTarget->FillRectangle(D2D1::RectF(cX, textY + 2.0f, cX + 2.0f, textY + lineH - 2.0f), pSolidBrush);
                }

                textY += lineH;
            }
        }
    }

    struct SegmentOption {
        std::wstring id;
        std::wstring label;
        uint32_t dotColor = 0;
    };

    void DrawSegmentedControl(
        float x, float y, float totalW, float totalH,
        const std::vector<SegmentOption>& options,
        const std::wstring& activeId,
        std::vector<std::pair<D2D1_RECT_F, std::wstring>>& outHitBoxes,
        POINT mousePos
    ) {
        if (options.empty()) return;
        D2D1_RECT_F trackRect = D2D1::RectF(x, y, x + totalW, y + totalH);

        // 1. Outer container pill (#14141C, 6px border radius, 1px border #282836)
        pSolidBrush->SetColor(ColorHex(0x14141C));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(trackRect, 6.0f, 6.0f), pSolidBrush);
        pSolidBrush->SetColor(ColorHex(0x282836));
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(trackRect, 6.0f, 6.0f), pSolidBrush, 1.0f);

        float segW = totalW / static_cast<float>(options.size());

        for (size_t i = 0; i < options.size(); ++i) {
            const auto& opt = options[i];
            float sx1 = x + i * segW;
            float sx2 = (i == options.size() - 1) ? (x + totalW) : (sx1 + segW);
            D2D1_RECT_F segHit = D2D1::RectF(sx1, y, sx2, y + totalH);
            outHitBoxes.push_back({ segHit, opt.id });

            bool isSel = (_wcsicmp(activeId.c_str(), opt.id.c_str()) == 0);
            bool isHov = PtInD2D1Rect(segHit, mousePos);

            D2D1_RECT_F innerPill = D2D1::RectF(sx1 + 2.0f, y + 2.0f, sx2 - 2.0f, y + totalH - 2.0f);

            if (isSel) {
                // Active segment: elevated inner rounded rect (#2B2B3C with 1px subtle top highlight #44445A and bright white text #FFFFFF)
                pSolidBrush->SetColor(ColorHex(0x2B2B3C));
                pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(innerPill, 4.0f, 4.0f), pSolidBrush);
                pSolidBrush->SetColor(ColorHex(0x44445A));
                pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(innerPill, 4.0f, 4.0f), pSolidBrush, 1.0f);
            } else if (isHov) {
                pSolidBrush->SetColor(ColorHex(0x1C1C26));
                pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(innerPill, 4.0f, 4.0f), pSolidBrush);
            }

            // Text + optional color dot
            float textW = MeasureStringWidth(opt.label, pUIFormatSmall);
            float dotSpace = (opt.dotColor != 0) ? 12.0f : 0.0f;
            float contentW = textW + dotSpace;
            float startX = sx1 + (segW - contentW) / 2.0f;
            float textY = y + (totalH - 18.0f) / 2.0f;

            if (opt.dotColor != 0) {
                pSolidBrush->SetColor(ColorHex(opt.dotColor));
                pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(startX + 4.0f, y + totalH / 2.0f), 3.0f, 3.0f), pSolidBrush);
                startX += 12.0f;
            }

            D2D1_COLOR_F txtCol = isSel ? ColorHex(0xFFFFFF) : (isHov ? ColorHex(0xC0C0D4) : ColorHex(0x8A8A98));
            DrawText(opt.label, startX, textY, txtCol, pUIFormatSmall);
        }
    }

    void RenderSettingsPanel(
        const Theme& currentTheme,
        const AppSettings& settings,
        float screenW,
        float screenH,
        POINT mousePos
    ) {
        // Modal Backdrop overlay
        float overlayAlpha = 0.60f;
        pSolidBrush->SetColor(ColorRgba(0.0f, 0.0f, 0.0f, overlayAlpha));
        pRenderTarget->FillRectangle(D2D1::RectF(0, 0, screenW, screenH), pSolidBrush);

        float mw = 720.0f;
        float mh = (screenH < 840.0f) ? (screenH - 40.0f) : 780.0f;
        float mx = (screenW - mw) / 2.0f;
        float my = (screenH - mh) / 2.0f;
        settingsModalRect = D2D1::RectF(mx, my, mx + mw, my + mh);

        // Body (#121216)
        pSolidBrush->SetColor(ColorHex(0x121216));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(settingsModalRect, 10.0f, 10.0f), pSolidBrush);
        pSolidBrush->SetColor(ColorHex(0x282833));
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(settingsModalRect, 10.0f, 10.0f), pSolidBrush, 1.0f);

        // Header (48px high)
        DrawText(I18n::Get(StringId::Settings_Title), mx + 24.0f, my + 12.0f, ColorHex(0xF0F0FF), pUIFormatTitle);

        // Header bottom divider line
        pSolidBrush->SetColor(ColorHex(0x1F1F28));
        pRenderTarget->DrawLine(
            D2D1::Point2F(mx, my + 48.0f),
            D2D1::Point2F(mx + mw, my + 48.0f),
            pSolidBrush, 1.0f
        );

        // Close button (32x32px, radius 6px)
        settingsCloseBtnRect = D2D1::RectF(mx + mw - 44.0f, my + 8.0f, mx + mw - 12.0f, my + 40.0f);
        bool clHov = PtInD2D1Rect(settingsCloseBtnRect, mousePos);
        pSolidBrush->SetColor(clHov ? ColorHex(0xE81123) : ColorHex(0x1E1E24));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(settingsCloseBtnRect, 6.0f, 6.0f), pSolidBrush);
        DrawFluentIcon(0xE8BB, settingsCloseBtnRect, ColorHex(0xFFFFFF), pIconFormatSmall);

        // Push clipping rect for scrollable content below header
        D2D1_RECT_F clipRect = D2D1::RectF(mx, my + 49.0f, mx + mw, my + mh - 8.0f);
        pRenderTarget->PushAxisAlignedClip(clipRect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        float rowY = my + 54.0f - settingsScrollY;
        float rowH = 52.0f;
        float ctrlLeft = mx + 230.0f;
        float ctrlW = 460.0f;
        float ctrlH = 30.0f;

        auto DrawRowDivider = [&](float y) {
            pSolidBrush->SetColor(ColorHex(0x1A1A22));
            pRenderTarget->DrawLine(D2D1::Point2F(mx + 20.0f, y), D2D1::Point2F(mx + mw - 20.0f, y), pSolidBrush, 1.0f);
        };

        // Row 1: Tema de Cores (All 9 Themes in 2 rows of segmented controls)
        DrawText(I18n::Get(StringId::Settings_Theme), mx + 24.0f, rowY + 8.0f, ColorHex(0xF0F0FF), pUIFormatBold);
        DrawText(I18n::Get(StringId::Settings_ThemeDesc), mx + 24.0f, rowY + 28.0f, ColorHex(0x78788C), pUIFormatSmall);

        settingsThemeBtns.clear();
        std::vector<SegmentOption> themeOpts1 = {
            { L"Pitch Black", L"Pitch Black", 0x80AAFF },
            { L"VS Code Like", L"VS Code", 0x007ACC },
            { L"Galaxy", L"Galaxy", 0xBD93F9 },
            { L"Blue", L"Blue", 0x80AAFF },
            { L"Light", L"Light", 0x0969DA }
        };
        DrawSegmentedControl(ctrlLeft, rowY + 2.0f, ctrlW, 26.0f, themeOpts1, settings.themeName, settingsThemeBtns, mousePos);

        std::vector<SegmentOption> themeOpts2 = {
            { L"Pip-Boy Green", L"Pip-Boy Green", 0x14FE17 },
            { L"Pip-Boy Amber", L"Pip-Boy Amber", 0xFFB642 },
            { L"PowerShell ISE", L"PowerShell ISE", 0x38BDF8 },
            { L"Warm Sepia", L"Warm Sepia", 0xD49B54 }
        };
        DrawSegmentedControl(ctrlLeft, rowY + 32.0f, ctrlW, 26.0f, themeOpts2, settings.themeName, settingsThemeBtns, mousePos);

        rowY += 64.0f;
        DrawRowDivider(rowY);

        // Row 2: Posicao do Prompt (Bottom-Docked vs Top-Flow)
        DrawText(L"Posicao do Prompt", mx + 24.0f, rowY + 6.0f, ColorHex(0xF0F0FF), pUIFormatBold);
        DrawText(L"Ancorado na base ou no topo do historico", mx + 24.0f, rowY + 26.0f, ColorHex(0x78788C), pUIFormatSmall);

        settingsPromptPositionBtns.clear();
        std::vector<SegmentOption> promptPosOpts = {
            { L"bottom", L"Bottom-Docked" },
            { L"top", L"Top-Flow" }
        };
        std::vector<std::pair<D2D1_RECT_F, std::wstring>> rawPosHit;
        DrawSegmentedControl(ctrlLeft, rowY + 10.0f, ctrlW, ctrlH, promptPosOpts, Utf8ToWide(settings.promptPosition), rawPosHit, mousePos);
        for (const auto& rph : rawPosHit) {
            settingsPromptPositionBtns.push_back({ rph.first, WideToUtf8(rph.second) });
        }

        rowY += rowH;
        DrawRowDivider(rowY);

        // Row 2.5: Origem do Fluxo / Output Flow Start
        bool isPtBR = (settings.language == "pt-BR");
        bool isEsES = (settings.language == "es-ES");
        std::wstring startTitle = isPtBR ? L"Origem do Fluxo [Saida]" : (isEsES ? L"Origen del Flujo [Salida]" : L"Output Flow Start");
        std::wstring startDesc = isPtBR ? L"Comecar saidas no topo da tela ou ancoradas na base" : (isEsES ? L"Iniciar salidas arriba o ancladas en la base" : L"Start outputs at top of screen or docked at bottom");
        DrawText(startTitle, mx + 24.0f, rowY + 6.0f, ColorHex(0xF0F0FF), pUIFormatBold);
        DrawText(startDesc, mx + 24.0f, rowY + 26.0f, ColorHex(0x78788C), pUIFormatSmall);

        settingsStreamStartBtns.clear();
        std::vector<SegmentOption> streamStartOpts = {
            { L"top", isPtBR ? L"No Topo" : (isEsES ? L"Arriba" : L"Top") },
            { L"bottom", isPtBR ? L"Na Base" : (isEsES ? L"En Base" : L"Bottom") }
        };
        std::vector<std::pair<D2D1_RECT_F, std::wstring>> rawStartHit;
        DrawSegmentedControl(ctrlLeft, rowY + 10.0f, ctrlW, ctrlH, streamStartOpts, Utf8ToWide(settings.streamStart), rawStartHit, mousePos);
        for (const auto& rsh : rawStartHit) {
            settingsStreamStartBtns.push_back({ rsh.first, WideToUtf8(rsh.second) });
        }

        rowY += rowH;
        DrawRowDivider(rowY);

        // Row 3: Filtro Eye-Care (Warmth) Continuous Slider
        DrawText(L"Filtro Eye-Care (Warmth)", mx + 24.0f, rowY + 6.0f, ColorHex(0xF0F0FF), pUIFormatBold);
        DrawText(L"Slider continuo anti-luz azul calibrado", mx + 24.0f, rowY + 26.0f, ColorHex(0x78788C), pUIFormatSmall);

        settingsWarmthBtns.clear();
        settingsWarmthSliderTrackRect = D2D1::RectF(ctrlLeft, rowY + 18.0f, ctrlLeft + ctrlW - 80.0f, rowY + 26.0f);
        float sliderW = settingsWarmthSliderTrackRect.right - settingsWarmthSliderTrackRect.left;
        float thumbX = settingsWarmthSliderTrackRect.left + (std::clamp)(settings.warmthLevel, 0.0f, 1.0f) * sliderW;

        // Background track (#14141C)
        pSolidBrush->SetColor(ColorHex(0x14141C));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(settingsWarmthSliderTrackRect, 4.0f, 4.0f), pSolidBrush);
        pSolidBrush->SetColor(ColorHex(0x282836));
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(settingsWarmthSliderTrackRect, 4.0f, 4.0f), pSolidBrush, 1.0f);

        // Active track (Warmth amber fill up to thumb)
        if (thumbX > settingsWarmthSliderTrackRect.left + 2.0f) {
            D2D1_RECT_F activeTrack = D2D1::RectF(settingsWarmthSliderTrackRect.left, settingsWarmthSliderTrackRect.top, thumbX, settingsWarmthSliderTrackRect.bottom);
            pSolidBrush->SetColor(ColorHex(0xF59E0B));
            pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(activeTrack, 4.0f, 4.0f), pSolidBrush);
        }

        // Thumb (circle 18x18px)
        settingsWarmthThumbRect = D2D1::RectF(thumbX - 9.0f, rowY + 13.0f, thumbX + 9.0f, rowY + 31.0f);
        bool thumbHov = PtInD2D1Rect(settingsWarmthThumbRect, mousePos);
        pSolidBrush->SetColor(thumbHov ? ColorHex(0xFFFFFF) : ColorHex(0xF59E0B));
        pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(thumbX, rowY + 22.0f), 8.0f, 8.0f), pSolidBrush);
        pSolidBrush->SetColor(ColorHex(0x1A1A24));
        pRenderTarget->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(thumbX, rowY + 22.0f), 8.0f, 8.0f), pSolidBrush, 1.5f);

        // Percentage label next to slider
        wchar_t szWarmth[32];
        int wPct = static_cast<int>(std::round(settings.warmthLevel * 100.0f));
        if (wPct == 0) swprintf_s(szWarmth, L"Off");
        else swprintf_s(szWarmth, L"%d%%", wPct);
        D2D1_RECT_F valRect = D2D1::RectF(ctrlLeft + ctrlW - 70.0f, rowY + 10.0f, ctrlLeft + ctrlW, rowY + 32.0f);
        DrawText(szWarmth, valRect.left + 8.0f, rowY + 13.0f, (wPct > 0) ? ColorHex(0xF59E0B) : ColorHex(0x78788C), pUIFormatBold);

        rowY += rowH;
        DrawRowDivider(rowY);

        // Row 4: Fonte do Terminal (Unified Segmented Control, 460px / 4 = 115px per segment)
        DrawText(I18n::Get(StringId::Settings_FontFamily), mx + 24.0f, rowY + 6.0f, ColorHex(0xF0F0FF), pUIFormatBold);
        DrawText(I18n::Get(StringId::Settings_FontFamilyDesc), mx + 24.0f, rowY + 26.0f, ColorHex(0x78788C), pUIFormatSmall);

        settingsFontFamilyBtns.clear();
        std::vector<SegmentOption> fontOpts = {
            { L"Cascadia Mono", L"Cascadia Mono" },
            { L"Cascadia Code", L"Cascadia Code" },
            { L"Consolas", L"Consolas" },
            { L"Courier New", L"Courier New" }
        };
        DrawSegmentedControl(ctrlLeft, rowY + 10.0f, ctrlW, ctrlH, fontOpts, settings.fontFamily, settingsFontFamilyBtns, mousePos);

        rowY += rowH;
        DrawRowDivider(rowY);

        // Row 5: Tamanho da Fonte (Unified Segmented Container, 460px)
        DrawText(I18n::Get(StringId::Settings_FontSize), mx + 24.0f, rowY + 6.0f, ColorHex(0xF0F0FF), pUIFormatBold);
        DrawText(I18n::Get(StringId::Settings_FontSizeDesc), mx + 24.0f, rowY + 26.0f, ColorHex(0x78788C), pUIFormatSmall);

        D2D1_RECT_F fontTrack = D2D1::RectF(ctrlLeft, rowY + 10.0f, ctrlLeft + ctrlW, rowY + 10.0f + ctrlH);
        pSolidBrush->SetColor(ColorHex(0x14141C));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(fontTrack, 6.0f, 6.0f), pSolidBrush);
        pSolidBrush->SetColor(ColorHex(0x282836));
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(fontTrack, 6.0f, 6.0f), pSolidBrush, 1.0f);

        settingsFontMinusRect = D2D1::RectF(ctrlLeft + 2.0f, rowY + 12.0f, ctrlLeft + 70.0f, rowY + 8.0f + ctrlH);
        settingsFontPlusRect = D2D1::RectF(ctrlLeft + ctrlW - 70.0f, rowY + 12.0f, ctrlLeft + ctrlW - 2.0f, rowY + 8.0f + ctrlH);

        bool mHov = PtInD2D1Rect(settingsFontMinusRect, mousePos);
        pSolidBrush->SetColor(mHov ? ColorHex(0x242432) : ColorHex(0x181822));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(settingsFontMinusRect, 4.0f, 4.0f), pSolidBrush);
        pSolidBrush->SetColor(ColorHex(0x363648));
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(settingsFontMinusRect, 4.0f, 4.0f), pSolidBrush, 1.0f);
        float mwTxt = MeasureStringWidth(L"-", pUIFormatBold);
        DrawText(L"-", settingsFontMinusRect.left + (68.0f - mwTxt) / 2.0f, rowY + 16.0f, ColorHex(0xF0F0FF), pUIFormatBold);

        wchar_t szBuf[32];
        swprintf_s(szBuf, L"%.1f pt", settings.fontSize);
        float szW = MeasureStringWidth(szBuf, pUIFormatBold);
        DrawText(szBuf, ctrlLeft + (ctrlW - szW) / 2.0f, rowY + 17.0f, currentTheme.accent, pUIFormatBold);

        bool pHov = PtInD2D1Rect(settingsFontPlusRect, mousePos);
        pSolidBrush->SetColor(pHov ? ColorHex(0x242432) : ColorHex(0x181822));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(settingsFontPlusRect, 4.0f, 4.0f), pSolidBrush);
        pSolidBrush->SetColor(ColorHex(0x363648));
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(settingsFontPlusRect, 4.0f, 4.0f), pSolidBrush, 1.0f);
        float pwTxt = MeasureStringWidth(L"+", pUIFormatBold);
        DrawText(L"+", settingsFontPlusRect.left + (68.0f - pwTxt) / 2.0f, rowY + 16.0f, ColorHex(0xF0F0FF), pUIFormatBold);

        rowY += rowH;
        DrawRowDivider(rowY);

        // Row: Wallpaper (Grid of Thumbnails / Samples + ADD+)
        DrawText(I18n::Get(StringId::Settings_Wallpaper), mx + 24.0f, rowY + 6.0f, ColorHex(0xF0F0FF), pUIFormatBold);
        DrawText(I18n::Get(StringId::Settings_WallpaperDesc), mx + 24.0f, rowY + 26.0f, ColorHex(0x78788C), pUIFormatSmall);

        settingsWallpaperBtns.clear();
        settingsWallpaperAddBtnRect = D2D1::RectF(0, 0, 0, 0);

        std::wstring exeDir = AppSettings::GetExeDir();
        std::vector<WallpaperItem> wpList = DiscoverWallpapers(exeDir, settings.customWallpapers);

        float cardW = 146.0f;
        float cardH = 34.0f;
        float cardGap = 8.0f;
        int cols = 3;

        int totalWpCards = 1 + static_cast<int>(wpList.size()) + 1;
        float gridStartY = rowY + 6.0f;

        for (int i = 0; i < totalWpCards; ++i) {
            int c = i % cols;
            int r = i / cols;
            float cx = ctrlLeft + c * (cardW + cardGap);
            float cy = gridStartY + r * (cardH + cardGap);
            D2D1_RECT_F cardRect = D2D1::RectF(cx, cy, cx + cardW, cy + cardH);

            bool isHov = PtInD2D1Rect(cardRect, mousePos);
            bool isCur = false;
            std::wstring label;
            std::wstring itemPath;

            if (i == 0) {
                label = I18n::Get(StringId::Settings_WallpaperNone);
                itemPath = L"";
                isCur = (settings.wallpaperPath.empty() || _wcsicmp(settings.wallpaperPath.c_str(), L"none") == 0);
                settingsWallpaperBtns.push_back({ cardRect, itemPath });
            } else if (i <= static_cast<int>(wpList.size())) {
                const auto& item = wpList[i - 1];
                label = item.displayName;
                itemPath = item.path;
                isCur = (!settings.wallpaperPath.empty() && (_wcsicmp(settings.wallpaperPath.c_str(), itemPath.c_str()) == 0 || _wcsicmp(settings.wallpaperPath.c_str(), item.displayName.c_str()) == 0));
                settingsWallpaperBtns.push_back({ cardRect, itemPath });
            } else {
                label = I18n::Get(StringId::Settings_WallpaperAdd);
                settingsWallpaperAddBtnRect = cardRect;
            }

            pSolidBrush->SetColor(isCur ? ColorHex(0x222238) : (isHov ? ColorHex(0x22222C) : ColorHex(0x161620)));
            pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(cardRect, 4.0f, 4.0f), pSolidBrush);

            pSolidBrush->SetColor(isCur ? currentTheme.accent : (isHov ? ColorHex(0x404058) : ColorHex(0x2A2A38)));
            pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(cardRect, 4.0f, 4.0f), pSolidBrush, isCur ? 1.5f : 1.0f);

            std::wstring dispLabel = TruncateTextToWidth(label, cardW - 14.0f, pUIFormatSmall);
            float tw = MeasureStringWidth(dispLabel, pUIFormatSmall);
            float tx = cx + (cardW - tw) * 0.5f;
            DrawText(dispLabel, tx, cy + 8.0f, isCur ? currentTheme.accent : ColorHex(0xF0F0FF), pUIFormatSmall);
        }

        int wpRows = (totalWpCards + cols - 1) / cols;
        rowY += (std::max)(52.0f, wpRows * (cardH + cardGap) + 14.0f);
        DrawRowDivider(rowY);

        // Sub-Row: Escurecer (Dim Slider 0% to 90%)
        DrawText(I18n::Get(StringId::Settings_WallpaperDim), mx + 24.0f, rowY + 6.0f, ColorHex(0xF0F0FF), pUIFormatBold);
        DrawText(I18n::Get(StringId::Settings_WallpaperDimDesc), mx + 24.0f, rowY + 26.0f, ColorHex(0x78788C), pUIFormatSmall);

        float dimTrackW = 380.0f;
        float dimTrackH = 6.0f;
        settingsWallpaperDimTrackRect = D2D1::RectF(ctrlLeft, rowY + 18.0f, ctrlLeft + dimTrackW, rowY + 18.0f + dimTrackH);

        pSolidBrush->SetColor(ColorHex(0x222230));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(settingsWallpaperDimTrackRect, 3.0f, 3.0f), pSolidBrush);

        float dimFrac = (std::clamp)(static_cast<float>(settings.wallpaperDim) / 90.0f, 0.0f, 1.0f);
        D2D1_RECT_F activeTrack = settingsWallpaperDimTrackRect;
        activeTrack.right = activeTrack.left + dimTrackW * dimFrac;
        pSolidBrush->SetColor(currentTheme.accent);
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(activeTrack, 3.0f, 3.0f), pSolidBrush);

        float dimThumbX = activeTrack.right;
        settingsWallpaperDimThumbRect = D2D1::RectF(dimThumbX - 8.0f, rowY + 13.0f, dimThumbX + 8.0f, rowY + 29.0f);
        pSolidBrush->SetColor(ColorHex(0xF0F0FF));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(settingsWallpaperDimThumbRect, 4.0f, 4.0f), pSolidBrush);
        pSolidBrush->SetColor(currentTheme.accent);
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(settingsWallpaperDimThumbRect, 4.0f, 4.0f), pSolidBrush, 1.0f);

        wchar_t dimTxt[16];
        swprintf_s(dimTxt, L"%d%%", settings.wallpaperDim);
        DrawText(dimTxt, ctrlLeft + ctrlW - 46.0f, rowY + 14.0f, ColorHex(0xF0F0FF), pUIFormatBold);

        rowY += rowH;
        DrawRowDivider(rowY);

        // Sub-Row: Pausar quando nao estiver em foco
        isPtBR = (settings.language == "pt-BR");
        DrawText(I18n::Get(StringId::Settings_WallpaperPauseUnfocused), mx + 24.0f, rowY + 6.0f, ColorHex(0xF0F0FF), pUIFormatBold);
        settingsWallpaperPauseUnfocusedBtns.clear();
        std::vector<SegmentOption> pauseUnfOpts = {
            { L"1", isPtBR ? L"Ativado" : L"Enabled" },
            { L"0", isPtBR ? L"Desativado" : L"Disabled" }
        };
        std::vector<std::pair<D2D1_RECT_F, std::wstring>> rawUnfHit;
        DrawSegmentedControl(ctrlLeft, rowY + 10.0f, ctrlW, ctrlH, pauseUnfOpts, settings.wallpaperPauseUnfocused ? L"1" : L"0", rawUnfHit, mousePos);
        for (const auto& rh : rawUnfHit) {
            settingsWallpaperPauseUnfocusedBtns.push_back({ rh.first, (rh.second == L"1") });
        }

        rowY += rowH;
        DrawRowDivider(rowY);

        // Sub-Row: Pausar na bateria
        DrawText(I18n::Get(StringId::Settings_WallpaperPauseOnBattery), mx + 24.0f, rowY + 6.0f, ColorHex(0xF0F0FF), pUIFormatBold);
        settingsWallpaperPauseOnBatteryBtns.clear();
        std::vector<SegmentOption> pauseBatOpts = {
            { L"1", isPtBR ? L"Ativado" : L"Enabled" },
            { L"0", isPtBR ? L"Desativado" : L"Disabled" }
        };
        std::vector<std::pair<D2D1_RECT_F, std::wstring>> rawBatHit;
        DrawSegmentedControl(ctrlLeft, rowY + 10.0f, ctrlW, ctrlH, pauseBatOpts, settings.wallpaperPauseOnBattery ? L"1" : L"0", rawBatHit, mousePos);
        for (const auto& rh : rawBatHit) {
            settingsWallpaperPauseOnBatteryBtns.push_back({ rh.first, (rh.second == L"1") });
        }

        rowY += rowH;
        DrawRowDivider(rowY);

        // Row: Modo de Layout (Unified Segmented Control, 460px / 2 = 230px per segment)
        DrawText(I18n::Get(StringId::Settings_ViewMode), mx + 24.0f, rowY + 6.0f, ColorHex(0xF0F0FF), pUIFormatBold);
        DrawText(I18n::Get(StringId::Settings_ViewModeDesc), mx + 24.0f, rowY + 26.0f, ColorHex(0x78788C), pUIFormatSmall);

        std::vector<SegmentOption> modeOpts = {
            { L"classic", I18n::Get(StringId::Settings_ModeClassic) },
            { L"blocks", I18n::Get(StringId::Settings_ModeBlocks) }
        };
        std::vector<std::pair<D2D1_RECT_F, std::wstring>> rawModeHit;
        DrawSegmentedControl(ctrlLeft, rowY + 10.0f, ctrlW, ctrlH, modeOpts, settings.classicFlowMode ? L"classic" : L"blocks", rawModeHit, mousePos);
        if (rawModeHit.size() >= 2) {
            settingsClassicModeBtnRect = rawModeHit[0].first;
            settingsBlocksModeBtnRect = rawModeHit[1].first;
        }

        rowY += rowH;
        DrawRowDivider(rowY);

        // Row 8: Idioma / Language (Unified Segmented Control, 460px / 4 = 115px per segment)
        DrawText(I18n::Get(StringId::Settings_Language), mx + 24.0f, rowY + 6.0f, ColorHex(0xF0F0FF), pUIFormatBold);
        DrawText(I18n::Get(StringId::Settings_LanguageDesc), mx + 24.0f, rowY + 26.0f, ColorHex(0x78788C), pUIFormatSmall);

        settingsLanguageBtns.clear();
        std::vector<SegmentOption> langOpts = {
            { L"auto", (I18n::GetResolvedLanguage() == LanguageMode::EnUS) ? L"Auto (System)" : L"Auto (Sistema)" },
            { L"pt-BR", L"Português" },
            { L"en-US", L"English" },
            { L"es-ES", L"Español" }
        };
        std::vector<std::pair<D2D1_RECT_F, std::wstring>> rawLangHit;
        DrawSegmentedControl(ctrlLeft, rowY + 10.0f, ctrlW, ctrlH, langOpts, Utf8ToWide(settings.language), rawLangHit, mousePos);
        for (const auto& rlh : rawLangHit) {
            settingsLanguageBtns.push_back({ rlh.first, WideToUtf8(rlh.second) });
        }

        rowY += rowH;
        DrawRowDivider(rowY);

        // Row 9: Estilo do Cursor (Unified Segmented Control, 460px / 3 = 153.3px per segment)
        DrawText(I18n::Get(StringId::Settings_Cursor), mx + 24.0f, rowY + 6.0f, ColorHex(0xF0F0FF), pUIFormatBold);
        DrawText(I18n::Get(StringId::Settings_CursorDesc), mx + 24.0f, rowY + 26.0f, ColorHex(0x78788C), pUIFormatSmall);

        settingsCursorBtns.clear();
        std::vector<SegmentOption> curOpts = {
            { L"0", I18n::Get(StringId::Settings_CursorBar) },
            { L"1", I18n::Get(StringId::Settings_CursorBlock) },
            { L"2", I18n::Get(StringId::Settings_CursorUnderline) }
        };
        std::vector<std::pair<D2D1_RECT_F, std::wstring>> rawCurHit;
        DrawSegmentedControl(ctrlLeft, rowY + 10.0f, ctrlW, ctrlH, curOpts, std::to_wstring(settings.cursorStyle), rawCurHit, mousePos);
        for (const auto& rch : rawCurHit) {
            settingsCursorBtns.push_back({ rch.first, _wtoi(rch.second.c_str()) });
        }

        rowY += rowH;
        DrawRowDivider(rowY);

        // Row 10: System cursor vs custom Direct2D cursor
        bool ptbr = (settings.language == "pt-BR");
        DrawText(ptbr ? L"Cursor do mouse" : L"Mouse cursor", mx + 24.0f, rowY + 6.0f, ColorHex(0xF0F0FF), pUIFormatBold);
        DrawText(ptbr ? L"Cursor do sistema ou proprio (experimental)" : L"System cursor or custom (experimental)", mx + 24.0f, rowY + 26.0f, ColorHex(0x78788C), pUIFormatSmall);
        settingsSysCursorBtns.clear();
        std::vector<SegmentOption> sysOpts = {
            { L"1", ptbr ? L"Cursor do sistema" : L"System cursor" },
            { L"0", ptbr ? L"Cursor customizado" : L"Custom cursor" }
        };
        std::vector<std::pair<D2D1_RECT_F, std::wstring>> rawSysHit;
        DrawSegmentedControl(ctrlLeft, rowY + 10.0f, ctrlW, ctrlH, sysOpts, settings.systemCursor ? L"1" : L"0", rawSysHit, mousePos);
        for (const auto& rs : rawSysHit) {
            settingsSysCursorBtns.push_back({ rs.first, _wtoi(rs.second.c_str()) });
        }

        rowY += rowH;
        DrawRowDivider(rowY);

        // Row 11: Atalho 'term' no Win+R / Explorer
        DrawText(ptbr ? L"Atalho 'term' no Win+R" : L"'term' in Win+R / Explorer", mx + 24.0f, rowY + 6.0f, ColorHex(0xF0F0FF), pUIFormatBold);
        DrawText(ptbr ? L"Abrir digitando 'term' no Executar ou Explorer" : L"Launch by typing 'term' in Run or address bar", mx + 24.0f, rowY + 26.0f, ColorHex(0x78788C), pUIFormatSmall);
        settingsRegisterTermBtnRect = D2D1::RectF(ctrlLeft, rowY + 10.0f, ctrlLeft + ctrlW, rowY + 10.0f + ctrlH);
        bool termReg = AppSettings::IsAppPathTermRegistered();
        bool termHov = PtInD2D1Rect(settingsRegisterTermBtnRect, mousePos);
        pSolidBrush->SetColor(termReg ? (termHov ? ColorHex(0x1E462E) : ColorHex(0x163422)) : (termHov ? ColorHex(0x242432) : ColorHex(0x181822)));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(settingsRegisterTermBtnRect, 6.0f, 6.0f), pSolidBrush);
        pSolidBrush->SetColor(termReg ? ColorHex(0x4ADE80) : ColorHex(0x363648));
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(settingsRegisterTermBtnRect, 6.0f, 6.0f), pSolidBrush, 1.0f);
        std::wstring termBtnTxt;
        if (termReg) {
            termBtnTxt = ptbr ? L"[ Ativo ] Digite 'term' no Win+R ou Explorer" : L"[ Active ] Type 'term' in Win+R or Explorer";
        } else {
            termBtnTxt = ptbr ? L"Registrar 'term' no Win+R e Explorer" : L"Register 'term' in Win+R & Explorer";
        }
        float tbw = MeasureStringWidth(termBtnTxt, pUIFormatBold);
        DrawText(termBtnTxt, settingsRegisterTermBtnRect.left + (ctrlW - tbw) * 0.5f, rowY + 16.0f, termReg ? ColorHex(0x4ADE80) : ColorHex(0xF0F0FF), pUIFormatBold);

        rowY += rowH;
        DrawRowDivider(rowY);

        // Row 12: Aviso de Seguranca ao Colar Multiplas Linhas
        DrawText(ptbr ? L"Aviso de seguranca ao colar" : L"Paste security warning", mx + 24.0f, rowY + 6.0f, ColorHex(0xF0F0FF), pUIFormatBold);
        DrawText(ptbr ? L"Confirma antes de colar lote de comandos" : L"Confirms before pasting multi-line batch", mx + 24.0f, rowY + 26.0f, ColorHex(0x78788C), pUIFormatSmall);
        settingsWarnPasteBtns.clear();
        std::vector<SegmentOption> warnOpts = {
            { L"1", ptbr ? L"Ativado (Seguro)" : L"Enabled (Safe)" },
            { L"0", ptbr ? L"Desativado" : L"Disabled" }
        };
        std::vector<std::pair<D2D1_RECT_F, std::wstring>> rawWarnHit;
        DrawSegmentedControl(ctrlLeft, rowY + 10.0f, ctrlW, ctrlH, warnOpts, settings.warnMultiLinePaste ? L"1" : L"0", rawWarnHit, mousePos);
        for (const auto& rw : rawWarnHit) {
            settingsWarnPasteBtns.push_back({ rw.first, (rw.second == L"1") });
        }

        rowY += rowH;
        DrawRowDivider(rowY);

        // Row 13: Ver introducao novamente
        DrawText(I18n::Get(StringId::Settings_ViewIntro), mx + 24.0f, rowY + 6.0f, ColorHex(0xF0F0FF), pUIFormatBold);
        DrawText(ptbr ? L"Rever o passo a passo de apresentacao do Spet@term" : L"Review the Spet@term onboarding walkthrough", mx + 24.0f, rowY + 26.0f, ColorHex(0x78788C), pUIFormatSmall);
        settingsViewIntroBtnRect = D2D1::RectF(ctrlLeft, rowY + 10.0f, ctrlLeft + ctrlW, rowY + 10.0f + ctrlH);
        bool viHov = PtInD2D1Rect(settingsViewIntroBtnRect, mousePos);
        pSolidBrush->SetColor(viHov ? ColorHex(0x282838) : ColorHex(0x1C1C26));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(settingsViewIntroBtnRect, 6.0f, 6.0f), pSolidBrush);
        pSolidBrush->SetColor(ColorHex(0x3E3E52));
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(settingsViewIntroBtnRect, 6.0f, 6.0f), pSolidBrush, 1.0f);
        std::wstring viTxt = I18n::Get(StringId::Settings_ViewIntro);
        float viW = MeasureStringWidth(viTxt, pUIFormatBold);
        DrawText(viTxt, settingsViewIntroBtnRect.left + (ctrlW - viW) * 0.5f, rowY + 16.0f, ColorHex(0xF0F0FF), pUIFormatBold);

        rowY += rowH + 20.0f;
        pRenderTarget->PopAxisAlignedClip();

        float totalHeight = (rowY + settingsScrollY) - my;
        settingsMaxScrollY = (std::max)(0.0f, totalHeight - (mh - 56.0f));
        if (settingsMaxScrollY > 0.0f) {
            float trackH = mh - 60.0f;
            float thumbH = (std::max)(30.0f, trackH * (trackH / (trackH + settingsMaxScrollY)));
            float thumbY = my + 52.0f + (settingsScrollY / settingsMaxScrollY) * (trackH - thumbH);
            pSolidBrush->SetColor(ColorHex(0x363648));
            pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(mx + mw - 7.0f, thumbY, mx + mw - 3.0f, thumbY + thumbH), 2.0f, 2.0f), pSolidBrush);
        }
    }

    void RenderOnboarding(
        const Theme& currentTheme,
        AppSettings& settings,
        OnboardingState& state,
        float screenW,
        float screenH,
        POINT mousePos
    ) {
        if (!pRenderTarget) return;

        // Modal Backdrop overlay (70% darkness)
        pSolidBrush->SetColor(ColorRgba(0.0f, 0.0f, 0.0f, 0.70f));
        pRenderTarget->FillRectangle(D2D1::RectF(0, 0, screenW, screenH), pSolidBrush);

        float mw = 740.0f;
        float mh = (screenH < 760.0f) ? (screenH - 40.0f) : 660.0f;
        float mx = (screenW - mw) * 0.5f;
        float my = (screenH - mh) * 0.5f;
        state.modalRect = D2D1::RectF(mx, my, mx + mw, my + mh);

        // Body card (#121218)
        pSolidBrush->SetColor(ColorHex(0x121218));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(state.modalRect, 10.0f, 10.0f), pSolidBrush);
        pSolidBrush->SetColor(ColorHex(0x28283C));
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(state.modalRect, 10.0f, 10.0f), pSolidBrush, 1.0f);

        // Header (Height 62px)
        wchar_t stepBuf[64];
        swprintf_s(stepBuf, L"%s %d %s 7", I18n::Get(StringId::Onboarding_Step), state.step + 1, I18n::Get(StringId::Onboarding_Of));
        DrawText(stepBuf, mx + 28.0f, my + 14.0f, currentTheme.accent, pUIFormatBold);

        // 7 Progress indicator pills at top right
        float pillStartX = mx + mw - 28.0f - (7 * 22.0f + 6 * 4.0f);
        float pillY = my + 20.0f;
        for (int p = 0; p < 7; ++p) {
            float px = pillStartX + p * 26.0f;
            D2D1_RECT_F pR = D2D1::RectF(px, pillY, px + 22.0f, pillY + 6.0f);
            if (p == state.step) {
                pSolidBrush->SetColor(currentTheme.accent);
            } else if (p < state.step) {
                pSolidBrush->SetColor(ColorHex(0x4ADE80));
            } else {
                pSolidBrush->SetColor(ColorHex(0x2A2A3A));
            }
            pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(pR, 3.0f, 3.0f), pSolidBrush);
        }

        // Header divider
        pSolidBrush->SetColor(ColorHex(0x1F1F2C));
        pRenderTarget->DrawLine(D2D1::Point2F(mx, my + 54.0f), D2D1::Point2F(mx + mw, my + 54.0f), pSolidBrush, 1.0f);

        // Content Area (my + 64.0f to my + mh - 64.0f)
        float contentTop = my + 64.0f;
        float contentW = mw - 56.0f;
        float contentLeft = mx + 28.0f;

        // Clear step-specific hit rects
        state.languageBtns.clear();
        state.shellPwBtnRect = D2D1::RectF(0, 0, 0, 0);
        state.shellCmdBtnRect = D2D1::RectF(0, 0, 0, 0);
        state.themeBtns.clear();
        state.fontBtns.clear();
        state.wallpaperBtns.clear();
        state.wallpaperAddBtnRect = D2D1::RectF(0, 0, 0, 0);
        state.warnPasteBtns.clear();
        state.rightClickPasteBtns.clear();
        state.sysCursorBtns.clear();
        state.registerTermBtnRect = D2D1::RectF(0, 0, 0, 0);

        // Render Step Content
        switch (state.step) {
        case 0: {
            float iconX = contentLeft + (contentW - 48.0f) * 0.5f;
            float iconY = contentTop + 24.0f;
            D2D1_RECT_F icR = D2D1::RectF(iconX, iconY, iconX + 48.0f, iconY + 48.0f);
            SpriteAtlas::Instance().DrawSpriteCentered(pRenderTarget, "terminal_window.png", icR, 40.0f, 40.0f);

            std::wstring wTitle = I18n::Get(StringId::Onboarding_WelcomeTitle);
            float wtw = MeasureStringWidth(wTitle, pUIFormatTitle);
            DrawText(wTitle, contentLeft + (contentW - wtw) * 0.5f, iconY + 60.0f, ColorHex(0xF0F0FF), pUIFormatTitle);

            std::wstring wDesc = I18n::Get(StringId::Onboarding_WelcomeDesc);
            float wdw = MeasureStringWidth(wDesc, pUIFormatMedium);
            DrawText(wDesc, contentLeft + (contentW - wdw) * 0.5f, iconY + 98.0f, ColorHex(0x88889C), pUIFormatMedium);

            float langSectionY = iconY + 160.0f;
            std::wstring selLangTxt = I18n::Get(StringId::Onboarding_SelectLanguage);
            float slw = MeasureStringWidth(selLangTxt, pUIFormatBold);
            DrawText(selLangTxt, contentLeft + (contentW - slw) * 0.5f, langSectionY, currentTheme.accent, pUIFormatBold);

            std::vector<SegmentOption> langOpts = {
                { L"pt-BR", L"Português (BR)" },
                { L"en-US", L"English (US)" },
                { L"es-ES", L"Español (ES)" },
                { L"auto", (I18n::GetResolvedLanguage() == LanguageMode::EnUS) ? L"Auto (System)" : L"Auto (Sistema)" }
            };
            float ctrlW = 480.0f;
            float ctrlLeft = contentLeft + (contentW - ctrlW) * 0.5f;
            std::vector<std::pair<D2D1_RECT_F, std::wstring>> rawLang;
            DrawSegmentedControl(ctrlLeft, langSectionY + 30.0f, ctrlW, 36.0f, langOpts, Utf8ToWide(settings.language), rawLang, mousePos);
            for (const auto& rh : rawLang) {
                state.languageBtns.push_back({ rh.first, WideToUtf8(rh.second) });
            }
            break;
        }

        case 1: {
            DrawText(I18n::Get(StringId::Onboarding_ShellTitle), contentLeft, contentTop + 10.0f, ColorHex(0xF0F0FF), pUIFormatTitle);
            DrawText(I18n::Get(StringId::Onboarding_ShellDesc), contentLeft, contentTop + 42.0f, ColorHex(0x88889C), pUIFormatSmall);

            float cardW = 320.0f;
            float cardH = 180.0f;
            float gap = 24.0f;
            float totalW = cardW * 2.0f + gap;
            float startX = contentLeft + (contentW - totalW) * 0.5f;
            float cardY = contentTop + 90.0f;

            // PowerShell Card
            state.shellPwBtnRect = D2D1::RectF(startX, cardY, startX + cardW, cardY + cardH);
            bool isPw = settings.classicFlowMode;
            bool pwHov = PtInD2D1Rect(state.shellPwBtnRect, mousePos);
            pSolidBrush->SetColor(pwHov ? ColorHex(0x20202E) : ColorHex(0x161622));
            pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(state.shellPwBtnRect, 8.0f, 8.0f), pSolidBrush);
            pSolidBrush->SetColor(isPw ? currentTheme.accent : ColorHex(0x303044));
            pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(state.shellPwBtnRect, 8.0f, 8.0f), pSolidBrush, isPw ? 2.0f : 1.0f);

            D2D1_RECT_F pwIconR = D2D1::RectF(startX + 20.0f, cardY + 20.0f, startX + 52.0f, cardY + 52.0f);
            DrawFluentIcon(0xE756, pwIconR, currentTheme.accent, pIconFormatSmall);
            DrawText(L"Windows PowerShell", startX + 60.0f, cardY + 24.0f, ColorHex(0xF0F0FF), pUIFormatBold);
            DrawText(L"Prompt moderno com suporte a objetos,", startX + 20.0f, cardY + 70.0f, ColorHex(0x88889C), pUIFormatSmall);
            DrawText(L"scripts .ps1, módulos e automação.", startX + 20.0f, cardY + 92.0f, ColorHex(0x88889C), pUIFormatSmall);
            DrawText(L"powershell.exe / pwsh.exe", startX + 20.0f, cardY + 130.0f, currentTheme.syntaxKeyword, pUIFormatSmall);

            // CMD Card
            float cmdX = startX + cardW + gap;
            state.shellCmdBtnRect = D2D1::RectF(cmdX, cardY, cmdX + cardW, cardY + cardH);
            bool isCmd = !isPw;
            bool cmdHov = PtInD2D1Rect(state.shellCmdBtnRect, mousePos);
            pSolidBrush->SetColor(cmdHov ? ColorHex(0x20202E) : ColorHex(0x161622));
            pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(state.shellCmdBtnRect, 8.0f, 8.0f), pSolidBrush);
            pSolidBrush->SetColor(isCmd ? currentTheme.accent : ColorHex(0x303044));
            pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(state.shellCmdBtnRect, 8.0f, 8.0f), pSolidBrush, isCmd ? 2.0f : 1.0f);

            D2D1_RECT_F cmdIconR = D2D1::RectF(cmdX + 20.0f, cardY + 20.0f, cmdX + 52.0f, cardY + 52.0f);
            DrawFluentIcon(0xE756, cmdIconR, ColorHex(0xA0A0B0), pIconFormatSmall);
            DrawText(L"Command Prompt (CMD)", cmdX + 60.0f, cardY + 24.0f, ColorHex(0xF0F0FF), pUIFormatBold);
            DrawText(L"O clássico interpretador do Windows.", cmdX + 20.0f, cardY + 70.0f, ColorHex(0x88889C), pUIFormatSmall);
            DrawText(L"Leve, direto e ideal para batch files.", cmdX + 20.0f, cardY + 92.0f, ColorHex(0x88889C), pUIFormatSmall);
            DrawText(L"cmd.exe /q /d", cmdX + 20.0f, cardY + 130.0f, currentTheme.syntaxKeyword, pUIFormatSmall);
            break;
        }

        case 2: {
            DrawText(I18n::Get(StringId::Onboarding_AppearanceTitle), contentLeft, contentTop + 10.0f, ColorHex(0xF0F0FF), pUIFormatTitle);
            DrawText(I18n::Get(StringId::Onboarding_AppearanceDesc), contentLeft, contentTop + 42.0f, ColorHex(0x88889C), pUIFormatSmall);

            float thY = contentTop + 76.0f;
            std::vector<SegmentOption> themeOpts1 = {
                { L"Pitch Black", L"Pitch Black", 0x80AAFF },
                { L"VS Code Like", L"VS Code", 0x007ACC },
                { L"Galaxy", L"Galaxy", 0xBD93F9 },
                { L"Blue", L"Blue", 0x80AAFF },
                { L"Light", L"Light", 0x0969DA }
            };
            DrawSegmentedControl(contentLeft, thY, contentW, 28.0f, themeOpts1, settings.themeName, state.themeBtns, mousePos);

            std::vector<SegmentOption> themeOpts2 = {
                { L"Pip-Boy Green", L"Pip-Boy Green", 0x14FE17 },
                { L"Pip-Boy Amber", L"Pip-Boy Amber", 0xFFB642 },
                { L"PowerShell ISE", L"PowerShell ISE", 0x38BDF8 },
                { L"Warm Sepia", L"Warm Sepia", 0xD49B54 }
            };
            DrawSegmentedControl(contentLeft, thY + 34.0f, contentW, 28.0f, themeOpts2, settings.themeName, state.themeBtns, mousePos);

            float fontY = thY + 78.0f;
            DrawText(I18n::Get(StringId::Settings_FontFamily), contentLeft, fontY + 4.0f, ColorHex(0xF0F0FF), pUIFormatBold);
            std::vector<SegmentOption> fontOpts = {
                { L"Cascadia Mono", L"Cascadia Mono" },
                { L"Consolas", L"Consolas" },
                { L"Courier New", L"Courier New" }
            };
            DrawSegmentedControl(contentLeft + 160.0f, fontY, contentW - 160.0f, 28.0f, fontOpts, settings.fontFamily, state.fontBtns, mousePos);

            float sizeY = fontY + 40.0f;
            DrawText(I18n::Get(StringId::Settings_FontSize), contentLeft, sizeY + 4.0f, ColorHex(0xF0F0FF), pUIFormatBold);
            state.fontMinusBtnRect = D2D1::RectF(contentLeft + 160.0f, sizeY, contentLeft + 200.0f, sizeY + 28.0f);
            state.fontPlusBtnRect = D2D1::RectF(contentLeft + 270.0f, sizeY, contentLeft + 310.0f, sizeY + 28.0f);

            bool mHov = PtInD2D1Rect(state.fontMinusBtnRect, mousePos);
            pSolidBrush->SetColor(mHov ? ColorHex(0x282838) : ColorHex(0x1C1C26));
            pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(state.fontMinusBtnRect, 4.0f, 4.0f), pSolidBrush);
            pSolidBrush->SetColor(ColorHex(0x3E3E52));
            pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(state.fontMinusBtnRect, 4.0f, 4.0f), pSolidBrush, 1.0f);
            DrawText(L"-", state.fontMinusBtnRect.left + 15.0f, sizeY + 4.0f, ColorHex(0xF0F0FF), pUIFormatBold);

            wchar_t szTxt[32]; swprintf_s(szTxt, L"%.1f pt", settings.fontSize);
            DrawText(szTxt, contentLeft + 210.0f, sizeY + 5.0f, currentTheme.accent, pUIFormatBold);

            bool pHov = PtInD2D1Rect(state.fontPlusBtnRect, mousePos);
            pSolidBrush->SetColor(pHov ? ColorHex(0x282838) : ColorHex(0x1C1C26));
            pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(state.fontPlusBtnRect, 4.0f, 4.0f), pSolidBrush);
            pSolidBrush->SetColor(ColorHex(0x3E3E52));
            pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(state.fontPlusBtnRect, 4.0f, 4.0f), pSolidBrush, 1.0f);
            DrawText(L"+", state.fontPlusBtnRect.left + 14.0f, sizeY + 4.0f, ColorHex(0xF0F0FF), pUIFormatBold);

            // Preview Box dynamically sized and formatted with chosen terminal font
            float prevY = sizeY + 42.0f;
            float prevH = (std::max)(76.0f, settings.fontSize * 3.0f);
            D2D1_RECT_F prevRect = D2D1::RectF(contentLeft, prevY, contentLeft + contentW, prevY + prevH);
            pSolidBrush->SetColor(currentTheme.bg);
            pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(prevRect, 6.0f, 6.0f), pSolidBrush);
            pSolidBrush->SetColor(currentTheme.accent);
            pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(prevRect, 6.0f, 6.0f), pSolidBrush, 1.0f);

            DrawText(L"spet@term ~ $ git status", contentLeft + 16.0f, prevY + 12.0f, currentTheme.syntaxValidCommand, pTermFormatBold ? pTermFormatBold : pUIFormatBold);
            DrawText(L"On branch master: All 36 stability tests passing cleanly", contentLeft + 16.0f, prevY + 14.0f + settings.fontSize * 1.30f, currentTheme.textDim, pTermFormat ? pTermFormat : pUIFormatSmall);
            break;
        }

        case 3: {
            DrawText(I18n::Get(StringId::Onboarding_WallpaperTitle), contentLeft, contentTop + 10.0f, ColorHex(0xF0F0FF), pUIFormatTitle);
            DrawText(I18n::Get(StringId::Onboarding_WallpaperDesc), contentLeft, contentTop + 42.0f, ColorHex(0x88889C), pUIFormatSmall);

            std::wstring exeDir = AppSettings::GetExeDir();
            std::vector<WallpaperItem> wpList = DiscoverWallpapers(exeDir, settings.customWallpapers);

            float cardW = 124.0f;
            float cardH = 46.0f;
            float cardGap = 12.0f;
            int cols = 4;
            int totalWpCards = 1 + static_cast<int>(wpList.size()) + 1;
            float gridStartY = contentTop + 76.0f;

            for (int i = 0; i < totalWpCards; ++i) {
                int c = i % cols;
                int r = i / cols;
                float cx = contentLeft + c * (cardW + cardGap);
                float cy = gridStartY + r * (cardH + cardGap);
                D2D1_RECT_F cardRect = D2D1::RectF(cx, cy, cx + cardW, cy + cardH);

                bool isHov = PtInD2D1Rect(cardRect, mousePos);
                bool isCur = false;
                std::wstring label;
                std::wstring itemPath;

                if (i == 0) {
                    label = I18n::Get(StringId::Settings_WallpaperNone);
                    itemPath = L"";
                    isCur = (settings.wallpaperPath.empty() || _wcsicmp(settings.wallpaperPath.c_str(), L"none") == 0);
                    state.wallpaperBtns.push_back({ cardRect, itemPath });
                } else if (i <= static_cast<int>(wpList.size())) {
                    const auto& item = wpList[i - 1];
                    label = item.displayName;
                    itemPath = item.path;
                    isCur = (!settings.wallpaperPath.empty() && (_wcsicmp(settings.wallpaperPath.c_str(), itemPath.c_str()) == 0 || _wcsicmp(settings.wallpaperPath.c_str(), item.displayName.c_str()) == 0));
                    state.wallpaperBtns.push_back({ cardRect, itemPath });
                } else {
                    label = I18n::Get(StringId::Settings_WallpaperAdd);
                    state.wallpaperAddBtnRect = cardRect;
                }

                pSolidBrush->SetColor(isCur ? ColorHex(0x222238) : (isHov ? ColorHex(0x22222C) : ColorHex(0x161620)));
                pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(cardRect, 4.0f, 4.0f), pSolidBrush);
                pSolidBrush->SetColor(isCur ? currentTheme.accent : (isHov ? ColorHex(0x404058) : ColorHex(0x2A2A38)));
                pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(cardRect, 4.0f, 4.0f), pSolidBrush, isCur ? 1.5f : 1.0f);

                float tw = MeasureStringWidth(label, pUIFormatSmall);
                float tx = (tw < cardW - 8.0f) ? (cx + (cardW - tw) * 0.5f) : (cx + 4.0f);
                DrawText(label, tx, cy + 14.0f, isCur ? currentTheme.accent : ColorHex(0xF0F0FF), pUIFormatSmall);
            }

            int wpRows = (totalWpCards + cols - 1) / cols;
            float dimY = gridStartY + wpRows * (cardH + cardGap) + 20.0f;
            DrawText(I18n::Get(StringId::Settings_WallpaperDim), contentLeft, dimY + 4.0f, ColorHex(0xF0F0FF), pUIFormatBold);
            DrawText(I18n::Get(StringId::Settings_WallpaperDimDesc), contentLeft, dimY + 24.0f, ColorHex(0x88889C), pUIFormatSmall);

            float trackW = 340.0f;
            float trackH = 6.0f;
            float trackLeft = contentLeft + 200.0f;
            state.wallpaperDimTrackRect = D2D1::RectF(trackLeft, dimY + 14.0f, trackLeft + trackW, dimY + 14.0f + trackH);

            pSolidBrush->SetColor(ColorHex(0x222230));
            pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(state.wallpaperDimTrackRect, 3.0f, 3.0f), pSolidBrush);

            float dimFrac = (std::clamp)(static_cast<float>(settings.wallpaperDim) / 90.0f, 0.0f, 1.0f);
            D2D1_RECT_F activeTrack = state.wallpaperDimTrackRect;
            activeTrack.right = activeTrack.left + trackW * dimFrac;
            pSolidBrush->SetColor(currentTheme.accent);
            pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(activeTrack, 3.0f, 3.0f), pSolidBrush);

            float thumbX = activeTrack.right;
            state.wallpaperDimThumbRect = D2D1::RectF(thumbX - 8.0f, dimY + 9.0f, thumbX + 8.0f, dimY + 25.0f);
            pSolidBrush->SetColor(ColorHex(0xF0F0FF));
            pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(state.wallpaperDimThumbRect, 4.0f, 4.0f), pSolidBrush);
            pSolidBrush->SetColor(currentTheme.accent);
            pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(state.wallpaperDimThumbRect, 4.0f, 4.0f), pSolidBrush, 1.0f);

            wchar_t dimTxt[16]; swprintf_s(dimTxt, L"%d%%", settings.wallpaperDim);
            DrawText(dimTxt, trackLeft + trackW + 16.0f, dimY + 10.0f, ColorHex(0xF0F0FF), pUIFormatBold);
            break;
        }

        case 4: {
            DrawText(I18n::Get(StringId::Onboarding_FeaturesTitle), contentLeft, contentTop + 10.0f, ColorHex(0xF0F0FF), pUIFormatTitle);
            DrawText(I18n::Get(StringId::Onboarding_FeaturesDesc), contentLeft, contentTop + 42.0f, ColorHex(0x88889C), pUIFormatSmall);

            struct FeatItem {
                std::wstring name;
                std::wstring desc;
                std::vector<std::wstring> keys;
            };
            std::vector<FeatItem> feats = {
                { L"Command Blocks", L"Execucao isolada e estruturada", {} },
                { L"Workfolder Preview", L"Navegacao e execucao de projetos", { L"Ctrl", L"B" } },
                { L"Timeline History", L"Historico de comandos e tempos", { L"Ctrl", L"R" } },
                { L"Command Palette", L"Busca rapida e troca de temas", { L"Ctrl", L"Shift", L"P" } },
                { L"Abas e Splits", L"Multi-painel horizontal e vertical", { L"Ctrl", L"T" } },
                { L"Quake Mode", L"Terminal suspenso global", { L"F12" } }
            };

            float cardW = (contentW - 16.0f) * 0.5f;
            float cardH = 74.0f;
            float gap = 12.0f;
            float startY = contentTop + 80.0f;

            for (size_t i = 0; i < feats.size(); ++i) {
                int col = static_cast<int>(i % 2);
                int row = static_cast<int>(i / 2);
                float cx = contentLeft + col * (cardW + gap);
                float cy = startY + row * (cardH + gap);
                D2D1_RECT_F r = D2D1::RectF(cx, cy, cx + cardW, cy + cardH);

                pSolidBrush->SetColor(ColorHex(0x161622));
                pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(r, 6.0f, 6.0f), pSolidBrush);
                pSolidBrush->SetColor(ColorHex(0x28283C));
                pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(r, 6.0f, 6.0f), pSolidBrush, 1.0f);

                DrawText(feats[i].name, cx + 16.0f, cy + 14.0f, ColorHex(0xF0F0FF), pUIFormatBold);
                DrawText(feats[i].desc, cx + 16.0f, cy + 38.0f, ColorHex(0x88889C), pUIFormatSmall);

                if (!feats[i].keys.empty()) {
                    float keyH = 22.0f;
                    float totalKw = 0.0f;
                    for (size_t k = 0; k < feats[i].keys.size(); ++k) {
                        float tw = MeasureStringWidth(feats[i].keys[k], pUIFormatBold);
                        totalKw += (std::max)(22.0f, tw + 14.0f);
                        if (k + 1 < feats[i].keys.size()) totalKw += 4.0f;
                    }
                    DrawKeyComboVec(feats[i].keys, cx + cardW - totalKw - 14.0f, cy + 24.0f, keyH);
                }
            }
            break;
        }

        case 5: {
            DrawText(I18n::Get(StringId::Onboarding_QuickPrefsTitle), contentLeft, contentTop + 10.0f, ColorHex(0xF0F0FF), pUIFormatTitle);
            DrawText(I18n::Get(StringId::Onboarding_QuickPrefsDesc), contentLeft, contentTop + 42.0f, ColorHex(0x88889C), pUIFormatSmall);

            float rowH = 64.0f;
            float curY = contentTop + 84.0f;
            bool isPt = (settings.language == "pt-BR");

            // Row A: Multi-line Paste Warning
            DrawText(isPt ? L"Aviso ao colar multiplas linhas" : L"Paste security warning", contentLeft, curY + 6.0f, ColorHex(0xF0F0FF), pUIFormatBold);
            DrawText(isPt ? L"Alerta antes de executar lote de comandos colados" : L"Alert before executing pasted batch commands", contentLeft, curY + 28.0f, ColorHex(0x78788C), pUIFormatSmall);
            std::vector<SegmentOption> warnOpts = {
                { L"1", isPt ? L"Ativado (Seguro)" : L"Enabled (Safe)" },
                { L"0", isPt ? L"Desativado" : L"Disabled" }
            };
            std::vector<std::pair<D2D1_RECT_F, std::wstring>> rawW;
            DrawSegmentedControl(contentLeft + 360.0f, curY + 12.0f, contentW - 360.0f, 32.0f, warnOpts, settings.warnMultiLinePaste ? L"1" : L"0", rawW, mousePos);
            for (const auto& rw : rawW) state.warnPasteBtns.push_back({ rw.first, (rw.second == L"1") });

            curY += rowH;
            pSolidBrush->SetColor(ColorHex(0x1F1F2C));
            pRenderTarget->DrawLine(D2D1::Point2F(contentLeft, curY), D2D1::Point2F(contentLeft + contentW, curY), pSolidBrush, 1.0f);

            // Row B: Mouse Cursor
            curY += 8.0f;
            DrawText(isPt ? L"Cursor do mouse" : L"Mouse cursor style", contentLeft, curY + 6.0f, ColorHex(0xF0F0FF), pUIFormatBold);
            DrawText(isPt ? L"Cursor padrao do Windows ou cursor proprio" : L"Windows native cursor or custom Direct2D", contentLeft, curY + 28.0f, ColorHex(0x78788C), pUIFormatSmall);
            std::vector<SegmentOption> sysOpts = {
                { L"1", isPt ? L"Sistema" : L"System" },
                { L"0", isPt ? L"Proprio" : L"Custom" }
            };
            std::vector<std::pair<D2D1_RECT_F, std::wstring>> rawS;
            DrawSegmentedControl(contentLeft + 360.0f, curY + 12.0f, contentW - 360.0f, 32.0f, sysOpts, settings.systemCursor ? L"1" : L"0", rawS, mousePos);
            for (const auto& rs : rawS) state.sysCursorBtns.push_back({ rs.first, (rs.second == L"1") });

            curY += rowH;
            pSolidBrush->SetColor(ColorHex(0x1F1F2C));
            pRenderTarget->DrawLine(D2D1::Point2F(contentLeft, curY), D2D1::Point2F(contentLeft + contentW, curY), pSolidBrush, 1.0f);

            // Row C: Term in Win+R
            curY += 8.0f;
            DrawText(isPt ? L"Atalho 'term' no Win+R" : L"'term' in Win+R / Explorer", contentLeft, curY + 6.0f, ColorHex(0xF0F0FF), pUIFormatBold);
            DrawText(isPt ? L"Iniciar digitando 'term' no Executar ou Explorer" : L"Launch by typing 'term' in Run or address bar", contentLeft, curY + 28.0f, ColorHex(0x78788C), pUIFormatSmall);
            state.registerTermBtnRect = D2D1::RectF(contentLeft + 360.0f, curY + 12.0f, contentLeft + contentW, curY + 44.0f);
            bool tReg = AppSettings::IsAppPathTermRegistered();
            bool tHov = PtInD2D1Rect(state.registerTermBtnRect, mousePos);
            pSolidBrush->SetColor(tReg ? (tHov ? ColorHex(0x1E462E) : ColorHex(0x163422)) : (tHov ? ColorHex(0x282838) : ColorHex(0x1C1C26)));
            pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(state.registerTermBtnRect, 6.0f, 6.0f), pSolidBrush);
            pSolidBrush->SetColor(tReg ? ColorHex(0x4ADE80) : ColorHex(0x363648));
            pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(state.registerTermBtnRect, 6.0f, 6.0f), pSolidBrush, 1.0f);
            std::wstring regTxt = tReg ? (isPt ? L"[ Ativo ] 'term' registrado" : L"[ Active ] 'term' registered") : (isPt ? L"Registrar 'term'" : L"Register 'term'");
            float rtw = MeasureStringWidth(regTxt, pUIFormatBold);
            DrawText(regTxt, state.registerTermBtnRect.left + (state.registerTermBtnRect.right - state.registerTermBtnRect.left - rtw) * 0.5f, curY + 18.0f, tReg ? ColorHex(0x4ADE80) : ColorHex(0xF0F0FF), pUIFormatBold);
            break;
        }

        case 6: {
            float iconX = contentLeft + (contentW - 48.0f) * 0.5f;
            float iconY = contentTop + 16.0f;
            D2D1_RECT_F icR = D2D1::RectF(iconX, iconY, iconX + 48.0f, iconY + 48.0f);
            DrawFluentIcon(0xE73E, icR, ColorHex(0x4ADE80), pIconFormat);

            std::wstring rTitle = I18n::Get(StringId::Onboarding_ReadyTitle);
            float rtw = MeasureStringWidth(rTitle, pUIFormatTitle);
            DrawText(rTitle, contentLeft + (contentW - rtw) * 0.5f, iconY + 56.0f, ColorHex(0xF0F0FF), pUIFormatTitle);

            std::wstring rDesc = I18n::Get(StringId::Onboarding_ReadyDesc);
            float rdw = MeasureStringWidth(rDesc, pUIFormatMedium);
            DrawText(rDesc, contentLeft + (contentW - rdw) * 0.5f, iconY + 92.0f, ColorHex(0x88889C), pUIFormatMedium);

            // Summary Card
            float sumY = iconY + 130.0f;
            D2D1_RECT_F sumRect = D2D1::RectF(contentLeft + 60.0f, sumY, contentLeft + contentW - 60.0f, sumY + 110.0f);
            pSolidBrush->SetColor(ColorHex(0x161622));
            pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(sumRect, 8.0f, 8.0f), pSolidBrush);
            pSolidBrush->SetColor(ColorHex(0x28283C));
            pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(sumRect, 8.0f, 8.0f), pSolidBrush, 1.0f);

            bool isPt = (settings.language == "pt-BR");
            DrawText(isPt ? L"Tema escolhido:" : L"Selected theme:", sumRect.left + 24.0f, sumY + 18.0f, ColorHex(0x88889C), pUIFormatSmall);
            DrawText(settings.themeName, sumRect.left + 160.0f, sumY + 18.0f, currentTheme.accent, pUIFormatBold);

            DrawText(isPt ? L"Fonte:" : L"Font:", sumRect.left + 24.0f, sumY + 44.0f, ColorHex(0x88889C), pUIFormatSmall);
            wchar_t fBuf[64]; swprintf_s(fBuf, L"%s (%.1f pt)", settings.fontFamily.c_str(), settings.fontSize);
            DrawText(fBuf, sumRect.left + 160.0f, sumY + 44.0f, ColorHex(0xF0F0FF), pUIFormatSmall);

            DrawText(isPt ? L"Papel de parede:" : L"Wallpaper:", sumRect.left + 24.0f, sumY + 70.0f, ColorHex(0x88889C), pUIFormatSmall);
            std::wstring wpName = settings.wallpaperPath.empty() ? I18n::Get(StringId::Settings_WallpaperNone) : GetSampleFriendlyName(settings.wallpaperPath);
            std::wstring dispWp = TruncateTextToWidth(wpName, sumRect.right - (sumRect.left + 160.0f) - 16.0f, pUIFormatSmall);
            DrawText(dispWp, sumRect.left + 160.0f, sumY + 70.0f, ColorHex(0xF0F0FF), pUIFormatSmall);

            // Ready tip
            std::wstring tip = I18n::Get(StringId::Onboarding_ReadyTip);
            float tipW = MeasureStringWidth(tip, pUIFormatSmall);
            DrawText(tip, contentLeft + (contentW - tipW) * 0.5f, sumY + 130.0f, ColorHex(0x78788C), pUIFormatSmall);
            break;
        }
        }

        // Footer Divider (my + mh - 56.0f)
        float footerY = my + mh - 56.0f;
        pSolidBrush->SetColor(ColorHex(0x1F1F2C));
        pRenderTarget->DrawLine(D2D1::Point2F(mx, footerY), D2D1::Point2F(mx + mw, footerY), pSolidBrush, 1.0f);

        // Footer Buttons:
        // Left: Voltar (Back)
        if (state.step > 0) {
            state.backBtnRect = D2D1::RectF(mx + 28.0f, footerY + 12.0f, mx + 120.0f, footerY + 44.0f);
            bool bHov = PtInD2D1Rect(state.backBtnRect, mousePos);
            pSolidBrush->SetColor(bHov ? ColorHex(0x282838) : ColorHex(0x1C1C26));
            pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(state.backBtnRect, 6.0f, 6.0f), pSolidBrush);
            pSolidBrush->SetColor(ColorHex(0x38384C));
            pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(state.backBtnRect, 6.0f, 6.0f), pSolidBrush, 1.0f);
            std::wstring bTxt = I18n::Get(StringId::Onboarding_Back);
            float btw = MeasureStringWidth(bTxt, pUIFormatBold);
            DrawText(bTxt, state.backBtnRect.left + (92.0f - btw) * 0.5f, footerY + 18.0f, ColorHex(0xF0F0FF), pUIFormatBold);
        } else {
            state.backBtnRect = D2D1::RectF(0, 0, 0, 0);
        }

        // Center: Pular introdução (Skip intro)
        if (state.step < 6) {
            std::wstring sTxt = I18n::Get(StringId::Onboarding_Skip);
            float stw = MeasureStringWidth(sTxt, pUIFormatSmall);
            float sx = mx + (mw - stw) * 0.5f;
            state.skipBtnRect = D2D1::RectF(sx - 10.0f, footerY + 14.0f, sx + stw + 10.0f, footerY + 42.0f);
            bool sHov = PtInD2D1Rect(state.skipBtnRect, mousePos);
            DrawText(sTxt, sx, footerY + 18.0f, sHov ? ColorHex(0xF0F0FF) : ColorHex(0x68687C), pUIFormatSmall);
        } else {
            state.skipBtnRect = D2D1::RectF(0, 0, 0, 0);
        }

        // Right: Próximo / Começar (Next / Get Started)
        bool isLast = (state.step == 6);
        float nextW = isLast ? 130.0f : 110.0f;
        state.nextBtnRect = D2D1::RectF(mx + mw - 28.0f - nextW, footerY + 10.0f, mx + mw - 28.0f, footerY + 46.0f);
        bool nHov = PtInD2D1Rect(state.nextBtnRect, mousePos);
        pSolidBrush->SetColor(nHov ? ColorHex(0xFFFFFF) : currentTheme.accent);
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(state.nextBtnRect, 6.0f, 6.0f), pSolidBrush);

        std::wstring nTxt = isLast ? I18n::Get(StringId::Onboarding_Finish) : I18n::Get(StringId::Onboarding_Next);
        float ntw = MeasureStringWidth(nTxt, pUIFormatBold);
        DrawText(nTxt, state.nextBtnRect.left + (nextW - ntw) * 0.5f, footerY + 18.0f, ColorHex(0x000000), pUIFormatBold);
    }

    void RenderHelpModal(
        const Theme& theme,
        float screenW,
        float screenH,
        POINT mousePos,
        const std::string& language = "en-US"
    ) {
        if (!pRenderTarget) return;

        // Semi-transparent backdrop overlay
        pSolidBrush->SetColor(ColorRgba(0.0f, 0.0f, 0.0f, 0.65f));
        pRenderTarget->FillRectangle(D2D1::RectF(0, 0, screenW, screenH), pSolidBrush);

        float mw = (std::min)(780.0f, screenW - 40.0f);
        float mh = (std::min)(600.0f, screenH - 40.0f);
        float mx = (screenW - mw) * 0.5f;
        float my = (screenH - mh) * 0.5f;
        helpModalRect = D2D1::RectF(mx, my, mx + mw, my + mh);

        // Body
        pSolidBrush->SetColor(ColorHex(0x121218));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(helpModalRect, 10.0f, 10.0f), pSolidBrush);
        pSolidBrush->SetColor(ColorHex(0x28283C));
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(helpModalRect, 10.0f, 10.0f), pSolidBrush, 1.0f);

        // Header
        bool ptbr = (language == "pt-BR");
        std::wstring title = ptbr ? L"Spet@term: Guia de Atalhos e Comandos" : L"Spet@term: Keyboard Shortcuts & Guide";
        DrawText(title, mx + 24.0f, my + 14.0f, ColorHex(0xF0F0FF), pUIFormatTitle);

        pSolidBrush->SetColor(ColorHex(0x1F1F2C));
        pRenderTarget->DrawLine(D2D1::Point2F(mx, my + 48.0f), D2D1::Point2F(mx + mw, my + 48.0f), pSolidBrush, 1.0f);

        // Close button [X]
        helpCloseBtnRect = D2D1::RectF(mx + mw - 44.0f, my + 8.0f, mx + mw - 12.0f, my + 40.0f);
        bool clHov = PtInD2D1Rect(helpCloseBtnRect, mousePos);
        pSolidBrush->SetColor(clHov ? ColorHex(0xE81123) : ColorHex(0x1E1E28));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(helpCloseBtnRect, 6.0f, 6.0f), pSolidBrush);
        DrawFluentIcon(0xE8BB, helpCloseBtnRect, ColorHex(0xFFFFFF), pIconFormatSmall);

        // Categorized cheatsheet in 2 columns
        struct HelpEntry { std::wstring key; std::wstring desc; };
        struct HelpCategory { std::wstring name; std::vector<HelpEntry> items; };

        std::vector<HelpCategory> leftCol = {
            {
                ptbr ? L"NAVEGACAO E ABAS" : L"NAVIGATION & TABS",
                {
                    { L"Ctrl+T", ptbr ? L"Nova Aba PowerShell" : L"New PowerShell Tab" },
                    { L"Ctrl+Shift+T", ptbr ? L"Nova Aba CMD" : L"New CMD Tab" },
                    { L"Ctrl+W", ptbr ? L"Fechar Aba Atual" : L"Close Current Tab" },
                    { L"Ctrl+Tab", ptbr ? L"Alternar PowerShell / CMD" : L"Switch PowerShell / CMD" },
                    { L"Ctrl+1..9", ptbr ? L"Ir Direto para Aba 1..9" : L"Switch to Tab 1..9" },
                    { L"F12 / Win+`", ptbr ? L"Quake Mode (Terminal Suspenso)" : L"Quake Mode Dropdown" }
                }
            },
            {
                ptbr ? L"PAINEIS DIVIDIDOS" : L"SPLIT PANES",
                {
                    { L"Alt+Shift+D", ptbr ? L"Dividir Tela Horizontal" : L"Split Pane Horizontal" },
                    { L"Alt+Shift+V", ptbr ? L"Dividir Tela Vertical" : L"Split Pane Vertical" },
                    { L"Alt+Shift+W", ptbr ? L"Fechar Painel Ativo" : L"Close Active Pane" }
                }
            }
        };

        std::vector<HelpCategory> rightCol = {
            {
                ptbr ? L"TERMINAL E WORKSPACE" : L"TERMINAL & WORKSPACE",
                {
                    { L"Ctrl+B", ptbr ? L"Alternar Barra Workspace" : L"Toggle Workspace Sidebar" },
                    { L"Ctrl+Shift+P", ptbr ? L"Paleta de Comandos (Palette)" : L"Command Palette" },
                    { L"Ctrl+F", ptbr ? L"Buscar no Historico" : L"Search History" },
                    { L"Ctrl+R", ptbr ? L"Historico / Timeline" : L"Command Timeline History" },
                    { L"Ctrl+,", ptbr ? L"Abrir Configuracoes" : L"Open Settings Modal" },
                    { L"Alt+F", ptbr ? L"Smart Error Doctor (Auto-Fix)" : L"Smart Error Doctor Fix" },
                    { L"Right-Click", ptbr ? L"Colar / Copiar Selecao" : L"Paste / Copy Selection" },
                    { L"Left-Drag", ptbr ? L"Selecionar Texto do Terminal" : L"Select Terminal Text" }
                }
            },
            {
                ptbr ? L"UTILITARIOS BUILT-IN" : L"BUILT-IN UTILITIES",
                {
                    { L"gstatus / glog", ptbr ? L"Git Zero-Process (<1ms)" : L"Zero-Process Git (<1ms)" },
                    { L"img <arquivo>", ptbr ? L"Visualizar Imagem via GPU" : L"View Image via GPU" },
                    { L"cd <dir> / cd -", ptbr ? L"Navegacao Rapida com Memoria" : L"Directory Jump with Memory" }
                }
            }
        };

        float colW = (mw - 64.0f) * 0.5f;
        float curY = my + 60.0f;

        auto RenderCategoryList = [&](const std::vector<HelpCategory>& cats, float cx) {
            float y = curY;
            for (const auto& cat : cats) {
                DrawText(cat.name, cx, y, theme.accent, pUIFormatBold);
                y += 22.0f;
                for (const auto& item : cat.items) {
                    float kw = MeasureStringWidth(item.key, pUIFormatBold);
                    D2D1_RECT_F kr = D2D1::RectF(cx, y, cx + kw + 10.0f, y + 18.0f);
                    pSolidBrush->SetColor(ColorHex(0x1C1C28));
                    pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(kr, 3.0f, 3.0f), pSolidBrush);
                    pSolidBrush->SetColor(ColorHex(0x36364A));
                    pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(kr, 3.0f, 3.0f), pSolidBrush, 1.0f);
                    DrawText(item.key, cx + 5.0f, y + 1.0f, ColorHex(0xF0F0FF), pUIFormatBold);

                    DrawText(item.desc, cx + kw + 18.0f, y + 1.0f, ColorHex(0xC0C0D4), pUIFormatSmall);
                    y += 24.0f;
                }
                y += 12.0f;
            }
        };

        RenderCategoryList(leftCol, mx + 24.0f);
        RenderCategoryList(rightCol, mx + 36.0f + colW);

        // Footer hint
        std::wstring footer = ptbr ? L"Pressione Esc ou clique no [X] para fechar" : L"Press Esc or click [X] to close";
        float fw = MeasureStringWidth(footer, pUIFormatSmall);
        DrawText(footer, mx + (mw - fw) * 0.5f, my + mh - 26.0f, ColorHex(0x6A6A80), pUIFormatSmall);
    }

    void RenderMultiLinePasteModal(
        const Theme& theme,
        float screenW,
        float screenH,
        const std::wstring& pasteText,
        POINT mousePos,
        const std::string& language = "en-US"
    ) {
        if (!pRenderTarget) return;

        // Semi-transparent backdrop overlay
        pSolidBrush->SetColor(ColorRgba(0.0f, 0.0f, 0.0f, 0.65f));
        pRenderTarget->FillRectangle(D2D1::RectF(0, 0, screenW, screenH), pSolidBrush);

        float mw = (std::min)(560.0f, screenW - 40.0f);
        float mh = 310.0f;
        float mx = (screenW - mw) * 0.5f;
        float my = (screenH - mh) * 0.5f;
        multiLinePasteModalRect = D2D1::RectF(mx, my, mx + mw, my + mh);

        // Body
        pSolidBrush->SetColor(ColorHex(0x13131A));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(multiLinePasteModalRect, 10.0f, 10.0f), pSolidBrush);
        pSolidBrush->SetColor(ColorHex(0xF59E0B)); // Amber warning border
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(multiLinePasteModalRect, 10.0f, 10.0f), pSolidBrush, 1.5f);

        bool ptbr = (language == "pt-BR");
        // Title with warning icon
        D2D1_RECT_F icR = D2D1::RectF(mx + 20.0f, my + 16.0f, mx + 40.0f, my + 36.0f);
        SpriteAtlas::Instance().DrawSpriteCentered(pRenderTarget, "warning.png", icR, 20.0f, 20.0f);

        std::wstring title = ptbr ? L"Aviso de Seguranca: Colar Multiplas Linhas" : L"Security Warning: Multi-Line Paste";
        DrawText(title, mx + 48.0f, my + 16.0f, ColorHex(0xFFB642), pUIFormatTitle);

        // Description
        std::wstring desc = ptbr
            ? L"Voce esta prestes a colar um texto com varias linhas no terminal. Se continuar, o shell pode executar comandos imediatamente."
            : L"You are about to paste text containing multiple lines into the terminal. Pasting this may execute commands immediately.";
        DrawText(desc, mx + 24.0f, my + 54.0f, ColorHex(0xD0D0E2), pUIFormatMedium);

        // Snippet preview box
        D2D1_RECT_F prevBox = D2D1::RectF(mx + 24.0f, my + 104.0f, mx + mw - 24.0f, my + 210.0f);
        pSolidBrush->SetColor(ColorHex(0x0C0C10));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(prevBox, 6.0f, 6.0f), pSolidBrush);
        pSolidBrush->SetColor(ColorHex(0x282836));
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(prevBox, 6.0f, 6.0f), pSolidBrush, 1.0f);

        // Extract first 4 lines
        std::wistringstream ss(pasteText);
        std::wstring line;
        float py = prevBox.top + 10.0f;
        int lineCount = 0;
        while (std::getline(ss, line) && lineCount < 4) {
            if (line.size() > 65) line = line.substr(0, 62) + L"...";
            DrawText(line, prevBox.left + 12.0f, py, ColorHex(0x9E9EB8), pTermFormatSmall);
            py += 20.0f;
            lineCount++;
        }

        // Settings notice
        std::wstring notice = ptbr
            ? L"(Este aviso de seguranca pode ser desabilitado nas Configuracoes)"
            : L"(This security warning can be disabled in Settings)";
        DrawText(notice, mx + 24.0f, my + 222.0f, ColorHex(0x78788C), pUIFormatSmall);

        // Buttons
        float btnW = 160.0f, btnH = 34.0f;
        multiLinePasteConfirmBtnRect = D2D1::RectF(mx + mw - 24.0f - btnW * 2.0f - 12.0f, my + mh - 48.0f, mx + mw - 24.0f - btnW - 12.0f, my + mh - 14.0f);
        multiLinePasteCancelBtnRect = D2D1::RectF(mx + mw - 24.0f - btnW, my + mh - 48.0f, mx + mw - 24.0f, my + mh - 14.0f);

        bool confHov = PtInD2D1Rect(multiLinePasteConfirmBtnRect, mousePos);
        pSolidBrush->SetColor(confHov ? ColorHex(0xD97706) : ColorHex(0xB45309));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(multiLinePasteConfirmBtnRect, 6.0f, 6.0f), pSolidBrush);
        std::wstring confTxt = ptbr ? L"Colar Mesmo Assim" : L"Paste Anyway";
        float cw = MeasureStringWidth(confTxt, pUIFormatBold);
        DrawText(confTxt, multiLinePasteConfirmBtnRect.left + (btnW - cw) * 0.5f, multiLinePasteConfirmBtnRect.top + 8.0f, ColorHex(0xFFFFFF), pUIFormatBold);

        bool canHov = PtInD2D1Rect(multiLinePasteCancelBtnRect, mousePos);
        pSolidBrush->SetColor(canHov ? ColorHex(0x282838) : ColorHex(0x1C1C24));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(multiLinePasteCancelBtnRect, 6.0f, 6.0f), pSolidBrush);
        pSolidBrush->SetColor(ColorHex(0x38384C));
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(multiLinePasteCancelBtnRect, 6.0f, 6.0f), pSolidBrush, 1.0f);
        std::wstring canTxt = ptbr ? L"Cancelar" : L"Cancel";
        float cnw = MeasureStringWidth(canTxt, pUIFormatBold);
        DrawText(canTxt, multiLinePasteCancelBtnRect.left + (btnW - cnw) * 0.5f, multiLinePasteCancelBtnRect.top + 8.0f, ColorHex(0xD0D0E2), pUIFormatBold);
    }

    void RenderTooltip(const std::wstring& text, float x, float y, float screenW, float screenH) {
        if (text.empty() || !pRenderTarget) return;
        float tw = MeasureStringWidth(text, pUIFormatSmall) + 16.0f;
        float th = 24.0f;
        float tx = x;
        float ty = y + 18.0f;
        if (tx + tw > screenW - 8.0f) tx = (std::max)(8.0f, screenW - tw - 8.0f);
        if (ty + th > screenH - 8.0f) ty = y - th - 4.0f;

        D2D1_RECT_F r = D2D1::RectF(tx, ty, tx + tw, ty + th);
        pSolidBrush->SetColor(ColorHex(0x181822));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(r, 4.0f, 4.0f), pSolidBrush);
        pSolidBrush->SetColor(ColorHex(0x323246));
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(r, 4.0f, 4.0f), pSolidBrush, 1.0f);

        DrawText(text, tx + 8.0f, ty + 4.0f, ColorHex(0xF0F0FF), pUIFormatSmall);
    }

    void RenderToast(const std::wstring& text, float screenW, float screenH, float alpha) {
        if (text.empty() || alpha <= 0.0f || !pRenderTarget) return;
        float tw = MeasureStringWidth(text, pUIFormat) + 44.0f;
        float th = 34.0f;
        float tx = screenW - tw - 24.0f;
        float ty = screenH - 58.0f;

        D2D1_RECT_F r = D2D1::RectF(tx, ty, tx + tw, ty + th);
        D2D1_COLOR_F bgCol = ColorHex(0x20202C);
        bgCol.a = (std::min)(1.0f, (std::max)(0.0f, alpha));
        pSolidBrush->SetColor(bgCol);
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(r, 6.0f, 6.0f), pSolidBrush);

        D2D1_COLOR_F borderCol = ColorHex(0x3D3D52);
        borderCol.a = bgCol.a;
        pSolidBrush->SetColor(borderCol);
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(r, 6.0f, 6.0f), pSolidBrush, 1.0f);

        D2D1_RECT_F icRect = D2D1::RectF(tx + 10.0f, ty + 7.0f, tx + 26.0f, ty + 27.0f);
        DrawFluentIcon(0xE73E, icRect, ColorHex(0x66FFD9), pIconFormatSmall);

        DrawText(text, tx + 30.0f, ty + 7.0f, ColorHex(0xFFFFFF), pUIFormat);
    }

    void RenderSearchBar(
        const Theme& theme,
        float screenW,
        const std::wstring& query,
        int currentMatch,
        int totalMatches,
        POINT mousePos,
        bool snippetsOpen = false
    ) {
        float sw = 296.0f;
        float sh = 34.0f;
        float sx = screenW - sw - 14.0f - (snippetsOpen ? 280.0f : 0.0f);
        float sy = TITLEBAR_HEIGHT + 8.0f;
        searchBoxRect = D2D1::RectF(sx, sy, sx + sw, sy + sh);

        pSolidBrush->SetColor(ColorHex(0x15151A));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(searchBoxRect, 6.0f, 6.0f), pSolidBrush);
        pSolidBrush->SetColor(ColorHex(0x2C2C36));
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(searchBoxRect, 6.0f, 6.0f), pSolidBrush, 1.0f);

        DrawFluentIcon(L"\uE721", sx + 8.0f, sy + 9.0f, 16.0f, 16.0f, theme.accent, pIconFormatSmall);

        std::wstring dispQ = query.empty() ? L"Buscar histórico..." : query;
        DrawText(dispQ, sx + 30.0f, sy + 8.0f, query.empty() ? theme.textDim : theme.textPrimary, pUIFormatSmall);

        // Match count
        float rightControlsX = sx + sw - 82.0f;
        if (!query.empty()) {
            wchar_t cntBuf[32];
            swprintf_s(cntBuf, L"%d/%d", totalMatches > 0 ? currentMatch : 0, totalMatches);
            DrawText(cntBuf, rightControlsX - 44.0f, sy + 8.0f, theme.syntaxString, pUIFormatSmall);
        }

        // Navigation buttons: 22x22px each
        searchPrevBtnRect = D2D1::RectF(rightControlsX, sy + 6.0f, rightControlsX + 22.0f, sy + 28.0f);
        searchNextBtnRect = D2D1::RectF(rightControlsX + 26.0f, sy + 6.0f, rightControlsX + 48.0f, sy + 28.0f);
        searchCloseBtnRect = D2D1::RectF(rightControlsX + 52.0f, sy + 6.0f, rightControlsX + 74.0f, sy + 28.0f);

        auto DrawSmallIconBtn = [&](const D2D1_RECT_F& r, const wchar_t* icon) {
            bool hov = PtInD2D1Rect(r, mousePos);
            pSolidBrush->SetColor(hov ? ColorHex(0x27272A) : ColorHex(0x1A1A20));
            pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(r, 4.0f, 4.0f), pSolidBrush);
            pSolidBrush->SetColor(ColorHex(0x2A2A34));
            pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(r, 4.0f, 4.0f), pSolidBrush, 1.0f);
            DrawFluentIcon(icon, r, hov ? theme.accent : theme.textPrimary, pIconFormatSmall);
        };

        DrawSmallIconBtn(searchPrevBtnRect, L"\uE70E");  // ChevronUp
        DrawSmallIconBtn(searchNextBtnRect, L"\uE70D");  // ChevronDown
        DrawSmallIconBtn(searchCloseBtnRect, L"\uE711"); // Cancel
    }

    void RenderBackground(const Theme& theme, float width, float height, int dimPercent = 60) {
        if (!pRenderTarget) return;

        D2D1_COLOR_F solidBg = theme.bg;
        solidBg.a = 1.0f;
        pRenderTarget->Clear(solidBg);

        wallpaperEngine.Tick();
        ID2D1Bitmap* pWp = wallpaperEngine.GetCurrentBitmap();
        if (pWp) {
            float bmpW = pWp->GetSize().width;
            float bmpH = pWp->GetSize().height;
            D2D1_RECT_F coverRect = CalculateCoverRect(width, height, bmpW, bmpH);
            pRenderTarget->DrawBitmap(pWp, coverRect);

            D2D1_COLOR_F veil = theme.bg;
            veil.a = (std::clamp)(dimPercent / 100.0f, 0.0f, 0.90f);
            pSolidBrush->SetColor(veil);
            pRenderTarget->FillRectangle(D2D1::RectF(0.0f, 0.0f, width, height), pSolidBrush);
        }

        if (theme.isGalaxy) {
            galaxyAnimTime += 0.02f;
            for (auto& s : stars) {
                s.y += s.speed;
                if (s.y > height) {
                    s.y = 0;
                    s.x = static_cast<float>(rand() % (int)(width > 10 ? width : 100));
                }
                float pulse = 0.5f + 0.5f * sinf(galaxyAnimTime * 2.0f + s.phase);
                float a = s.alpha * pulse;

                pSolidBrush->SetColor(D2D1::ColorF(0.85f, 0.70f, 1.0f, a));
                D2D1_ELLIPSE ellipse = D2D1::Ellipse(D2D1::Point2F(s.x, s.y), s.radius, s.radius);
                pRenderTarget->FillEllipse(ellipse, pSolidBrush);
            }
        }
    }

    void RenderAutocompletePopup(
        const Theme& theme,
        float x,
        float y,
        const std::vector<AutocompleteItem>& items,
        int selectedIndex,
        ID2D1Bitmap* pPreviewThumb,
        const D2D1_SIZE_F& previewThumbSize,
        float maxAvailableWidth = 1200.0f
    ) {
        if (items.empty()) return;

        float itemH = 26.0f;
        float menuW = 320.0f;
        float menuH = items.size() * itemH + 12.0f;

        float popupY = y;
        if (popupY + menuH > 680.0f) popupY = (std::max)(42.0f, y - menuH - 35.0f);

        D2D1_RECT_F menuRect = D2D1::RectF(x, popupY, x + menuW, popupY + menuH);
        FillAdaptiveBox(theme, menuRect, ColorHex(0x131318), ColorHex(0x2A2A35));

        float itY = popupY + 6.0f;
        for (size_t i = 0; i < items.size(); ++i) {
            const auto& item = items[i];
            bool sel = (static_cast<int>(i) == selectedIndex);

            D2D1_RECT_F itemRect = D2D1::RectF(x + 4.0f, itY, x + menuW - 4.0f, itY + itemH);
            if (sel) {
                pSolidBrush->SetColor(ColorHex(0x20202A));
                pRenderTarget->FillRectangle(itemRect, pSolidBrush);

                pSolidBrush->SetColor(theme.accent);
                pRenderTarget->FillRectangle(D2D1::RectF(itemRect.left, itemRect.top, itemRect.left + 2.5f, itemRect.bottom), pSolidBrush);
            }

            std::string icon = "terminal_window.png";
            if (item.displayText == L"..") {
                icon = "saturn_folder_24.png";
            } else if (item.isDir) {
                icon = "blue_folder.png";
            } else if (item.displayText.find(L".ps1") != std::wstring::npos || item.displayText.find(L".py") != std::wstring::npos || item.displayText.find(L".bat") != std::wstring::npos || item.displayText.find(L".cmd") != std::wstring::npos || item.displayText.find(L".sh") != std::wstring::npos) {
                icon = "gba_cartridge.png";
            } else if (item.type.find(L"git") != std::wstring::npos || item.type.find(L"branch") != std::wstring::npos) {
                icon = "radio_tower.png";
            } else if (item.displayText.find(L".exe") != std::wstring::npos || item.displayText.find(L".cpp") != std::wstring::npos || item.displayText.find(L".hpp") != std::wstring::npos || item.displayText.find(L".c") != std::wstring::npos || item.displayText.find(L".h") != std::wstring::npos || item.displayText.find(L".dll") != std::wstring::npos || item.displayText.find(L"-") == 0) {
                icon = "green_chip.png";
            } else if (item.displayText.find(L".md") != std::wstring::npos || item.displayText.find(L".txt") != std::wstring::npos || item.displayText.find(L".json") != std::wstring::npos || item.displayText.find(L".cmake") != std::wstring::npos || item.displayText.find(L"CMakeLists") != std::wstring::npos) {
                icon = "wad_doc.png";
            }

            SpriteAtlas::Get().Render(pRenderTarget, icon, x + 8.0f, itY + 5.0f, 16.0f, 16.0f);
            DrawText(item.type, x + 28.0f, itY + 4.0f, theme.syntaxKeyword, pTextFormatSmall);
            DrawText(item.displayText, x + 80.0f, itY + 3.0f, sel ? theme.accent : theme.textPrimary, pTextFormat);
            if (!item.detail.empty()) {
                DrawText(item.detail, x + 215.0f, itY + 4.0f, theme.textDim, pTextFormatSmall);
            }

            itY += itemH;
        }

        // Side preview
        if (selectedIndex >= 0 && selectedIndex < static_cast<int>(items.size())) {
            const auto& selItem = items[selectedIndex];
            float cardX = x + menuW + 8.0f;
            float cardW = 320.0f;
            if (cardX + cardW > maxAvailableWidth - 10.0f) {
                if (x - cardW - 8.0f > 10.0f) {
                    cardX = x - cardW - 8.0f;
                } else {
                    cardW = (std::max)(180.0f, maxAvailableWidth - x - menuW - 16.0f);
                }
            }
            float cardH = (std::max)(menuH, 180.0f);
            D2D1_RECT_F cardRect = D2D1::RectF(cardX, popupY, cardX + cardW, popupY + cardH);

            FillAdaptiveBox(theme, cardRect, ColorHex(0x101015), ColorHex(0x24242E));

            DrawText(selItem.text, cardX + 12.0f, popupY + 8.0f, theme.accent, pUIFormatMedium);

            if (selItem.isImage && pPreviewThumb) {
                float thumbMaxW = cardW - 24.0f;
                float thumbMaxH = cardH - 50.0f;
                float aspect = (previewThumbSize.height > 0) ? (previewThumbSize.width / previewThumbSize.height) : 1.0f;
                float tw = thumbMaxW;
                float th = tw / aspect;
                if (th > thumbMaxH) { th = thumbMaxH; tw = th * aspect; }

                D2D1_RECT_F imgR = D2D1::RectF(cardX + 12.0f, popupY + 36.0f, cardX + 12.0f + tw, popupY + 36.0f + th);
                pRenderTarget->DrawBitmap(pPreviewThumb, imgR);
            } else if (selItem.isCode && !selItem.previewLines.empty()) {
                float py = popupY + 36.0f;
                for (size_t l = 0; l < selItem.previewLines.size() && l < 8; ++l) {
                    wchar_t lineNum[16];
                    swprintf_s(lineNum, L"%2zu | ", l + 1);
                    DrawText(lineNum, cardX + 10.0f, py, theme.textDim, pTextFormatSmall);
                    std::wstring pLine = selItem.previewLines[l];
                    if (pLine.size() > 34) pLine = pLine.substr(0, 31) + L"...";
                    DrawText(pLine, cardX + 45.0f, py, theme.textPrimary, pTextFormatSmall);
                    py += 16.0f;
                }
            } else {
                DrawText(L"Item do sistema de arquivos.", cardX + 12.0f, popupY + 40.0f, theme.textDim, pTextFormatSmall);
            }
        }
    }

    void RenderCommandPalette(
        const Theme& theme,
        float screenWidth,
        float screenHeight,
        const std::wstring& query,
        const std::vector<std::pair<std::wstring, std::wstring>>& filteredActions,
        int selectedIndex,
        int scrollOffset = 0,
        POINT mousePos = { 0, 0 },
        const std::string& language = "en-US"
    ) {
        if (!pRenderTarget) return;
        paletteItemRects.clear();

        pSolidBrush->SetColor(ColorRgba(0.0f, 0.0f, 0.0f, 0.65f));
        pRenderTarget->FillRectangle(D2D1::RectF(0, 0, screenWidth, screenHeight), pSolidBrush);

        float palW = 540.0f;
        float itemH = 34.0f;
        const size_t maxVisible = 7;
        size_t totalCount = filteredActions.size();
        size_t visibleCount = (std::min)(totalCount, maxVisible);
        float palH = 64.0f + (visibleCount > 0 ? (visibleCount * itemH + 8.0f) : 48.0f);
        float palX = (screenWidth - palW) * 0.5f;
        float palY = screenHeight * 0.15f;

        paletteModalRect = D2D1::RectF(palX, palY, palX + palW, palY + palH);

        pSolidBrush->SetColor(ColorHex(0x141417));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(paletteModalRect, 8.0f, 8.0f), pSolidBrush);
        pSolidBrush->SetColor(ColorHex(0x2E2E36));
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(paletteModalRect, 8.0f, 8.0f), pSolidBrush, 1.0f);

        // Close button [X] at top-right
        paletteCloseBtnRect = D2D1::RectF(palX + palW - 36.0f, palY + 12.0f, palX + palW - 12.0f, palY + 36.0f);
        bool clHov = PtInD2D1Rect(paletteCloseBtnRect, mousePos);
        pSolidBrush->SetColor(clHov ? ColorHex(0xE81123) : ColorHex(0x1C1C22));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(paletteCloseBtnRect, 4.0f, 4.0f), pSolidBrush);
        DrawFluentIcon(0xE8BB, paletteCloseBtnRect, ColorHex(0xFFFFFF), pIconFormatSmall);

        // Input Box
        D2D1_RECT_F inputRect = D2D1::RectF(palX + 12.0f, palY + 12.0f, palX + palW - 42.0f, palY + 46.0f);
        pSolidBrush->SetColor(ColorHex(0x1C1C22));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(inputRect, 4.0f, 4.0f), pSolidBrush);
        pSolidBrush->SetColor(ColorHex(0x2E2E36));
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(inputRect, 4.0f, 4.0f), pSolidBrush, 1.0f);

        bool ptbr = (language == "pt-BR");
        std::wstring placeholder = ptbr ? L"Digite um comando, tema ou acao..." : L"Type a command, theme or action...";
        std::wstring promptText = L"> " + (query.empty() ? placeholder : query);
        DrawText(promptText, palX + 22.0f, palY + 18.0f, query.empty() ? theme.textDim : theme.textPrimary, pUIFormat);

        // Item Counter Badge in Search Input
        if (totalCount > 0) {
            wchar_t cntBuf[48];
            swprintf_s(cntBuf, ptbr ? L"[%d de %zu]" : L"[%d of %zu]", (selectedIndex >= 0 ? selectedIndex + 1 : 1), totalCount);
            float cntW = MeasureStringWidth(cntBuf, pUIFormatSmall);
            DrawText(cntBuf, inputRect.right - cntW - 8.0f, palY + 18.0f, ColorHex(0x78788C), pUIFormatSmall);
        }

        // Scroll window clamped to total count
        size_t startIndex = static_cast<size_t>((std::max)(0, scrollOffset));
        if (totalCount > maxVisible && startIndex + maxVisible > totalCount) {
            startIndex = totalCount - maxVisible;
        }

        float itY = palY + 54.0f;
        for (size_t i = startIndex; i < totalCount && (i - startIndex) < maxVisible; ++i) {
            bool sel = (static_cast<int>(i) == selectedIndex);
            D2D1_RECT_F actRect = D2D1::RectF(palX + 12.0f, itY, palX + palW - (totalCount > maxVisible ? 16.0f : 12.0f), itY + itemH);
            paletteItemRects.push_back({ actRect, static_cast<int>(i) });

            bool isHov = PtInD2D1Rect(actRect, mousePos);
            if (sel) {
                pSolidBrush->SetColor(ColorHex(0x27272A));
                pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(actRect, 4.0f, 4.0f), pSolidBrush);
                pSolidBrush->SetColor(theme.accent);
                pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(actRect, 4.0f, 4.0f), pSolidBrush, 1.0f);
            } else if (isHov) {
                pSolidBrush->SetColor(ColorHex(0x1F1F24));
                pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(actRect, 4.0f, 4.0f), pSolidBrush);
            }

            DrawText(filteredActions[i].first, palX + 22.0f, itY + 4.0f, sel ? theme.accent : theme.textPrimary, pUIFormatMedium);
            DrawText(filteredActions[i].second, palX + 22.0f, itY + 18.0f, theme.textDim, pUIFormatSmall);
            itY += itemH;
        }

        // Scrollbar track and thumb on right
        if (totalCount > maxVisible) {
            float trackX = palX + palW - 8.0f;
            float trackY = palY + 54.0f;
            float trackH = visibleCount * itemH;
            pSolidBrush->SetColor(ColorHex(0x1E1E28));
            pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(trackX, trackY, trackX + 3.0f, trackY + trackH), 1.5f, 1.5f), pSolidBrush);

            float thumbH = (static_cast<float>(maxVisible) / static_cast<float>(totalCount)) * trackH;
            if (thumbH < 14.0f) thumbH = 14.0f;
            float scrollProgress = static_cast<float>(startIndex) / static_cast<float>(totalCount - maxVisible);
            float thumbY = trackY + scrollProgress * (trackH - thumbH);

            pSolidBrush->SetColor(ColorHex(0x525268));
            pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(trackX, thumbY, trackX + 3.0f, thumbY + thumbH), 1.5f, 1.5f), pSolidBrush);
        }

        if (totalCount == 0) {
            std::wstring noCmd = ptbr ? L"Nenhum comando correspondente encontrado." : L"No matching command found.";
            DrawText(noCmd, palX + 22.0f, palY + 60.0f, theme.textDim, pUIFormatSmall);
        }
    }

    void RenderSidebarContextMenu(const Theme& theme, POINT mousePos) {
        if (!sidebarContextMenuOpen || sidebarContextMenuItems.empty() || !pRenderTarget) return;

        pSolidBrush->SetColor(ColorHex(0x161622));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(sidebarContextMenuRect, 6.0f, 6.0f), pSolidBrush);
        pSolidBrush->SetColor(ColorHex(0x2E2E40));
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(sidebarContextMenuRect, 6.0f, 6.0f), pSolidBrush, 1.0f);

        for (const auto& item : sidebarContextMenuItems) {
            if (item.id == -1) {
                pSolidBrush->SetColor(ColorHex(0x282836));
                float sepY = (item.rect.top + item.rect.bottom) * 0.5f;
                pRenderTarget->DrawLine(D2D1::Point2F(item.rect.left + 6.0f, sepY), D2D1::Point2F(item.rect.right - 6.0f, sepY), pSolidBrush, 1.0f);
                continue;
            }

            bool hov = PtInD2D1Rect(item.rect, mousePos);
            if (hov) {
                pSolidBrush->SetColor(ColorHex(0x262638));
                pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(item.rect, 4.0f, 4.0f), pSolidBrush);
            }

            if (!item.icon.empty()) {
                D2D1_RECT_F icR = D2D1::RectF(item.rect.left + 6.0f, item.rect.top + 5.0f, item.rect.left + 22.0f, item.rect.bottom - 5.0f);
                SpriteAtlas::Instance().DrawSpriteCentered(pRenderTarget, item.icon, icR, 14.0f, 14.0f);
            }

            DrawText(item.label, item.rect.left + 26.0f, item.rect.top + 5.0f, hov ? theme.accent : ColorHex(0xE0E0EC), pUIFormatSmall);
        }
    }

    void DrawSplitDivider(const Theme& theme, const D2D1_RECT_F& paneRect, bool isVertical, bool isActive) {
        pSolidBrush->SetColor(ColorHex(0x24242E));
        if (isVertical) {
            pRenderTarget->DrawLine(
                D2D1::Point2F(paneRect.right, paneRect.top),
                D2D1::Point2F(paneRect.right, paneRect.bottom),
                pSolidBrush, 1.0f
            );
        } else {
            pRenderTarget->DrawLine(
                D2D1::Point2F(paneRect.left, paneRect.bottom),
                D2D1::Point2F(paneRect.right, paneRect.bottom),
                pSolidBrush, 1.0f
            );
        }

        if (isActive) {
            pSolidBrush->SetColor(theme.accent);
            pRenderTarget->DrawLine(
                D2D1::Point2F(paneRect.left + 20.0f, paneRect.top + 1.0f),
                D2D1::Point2F(paneRect.right - 20.0f, paneRect.top + 1.0f),
                pSolidBrush, 1.0f
            );
        }
    }

    float MeasureStringWidth(const std::wstring& str, IDWriteTextFormat* format) {
        if (str.empty() || !pDWriteFactory || !format) return 0.0f;

        IDWriteTextLayout* pLayout = nullptr;
        HRESULT hr = pDWriteFactory->CreateTextLayout(
            str.c_str(), static_cast<UINT32>(str.size()), format, 10000.0f, 1000.0f, &pLayout
        );

        if (SUCCEEDED(hr) && pLayout) {
            DWRITE_TEXT_METRICS tm;
            pLayout->GetMetrics(&tm);
            SafeRelease(pLayout);
            return tm.widthIncludingTrailingWhitespace;
        }
        return str.size() * charWidth;
    }

    std::wstring TruncateTextToWidth(const std::wstring& text, float maxWidth, IDWriteTextFormat* fmt) {
        if (text.empty() || maxWidth <= 0.0f) return L"";
        std::wstring s = text;
        size_t nl = s.find_first_of(L"\r\n");
        if (nl != std::wstring::npos) {
            s = s.substr(0, nl);
        }
        s = Trim(s);
        if (s.empty()) return L"";

        float totalW = MeasureStringWidth(s, fmt);
        if (totalW <= maxWidth) return s;

        float dotsW = MeasureStringWidth(L"...", fmt);
        float availW = maxWidth - dotsW;
        if (availW <= 0.0f) {
            return (maxWidth >= dotsW) ? L"..." : L"";
        }

        float avgCharW = totalW / static_cast<float>(s.size());
        if (avgCharW <= 1.0f) avgCharW = (charWidth > 0.0f ? charWidth : 8.0f);

        int estChars = static_cast<int>(availW / avgCharW);
        if (estChars < 1) estChars = 1;
        if (static_cast<size_t>(estChars) >= s.size()) estChars = static_cast<int>(s.size()) - 1;

        s = s.substr(0, estChars);
        while (s.size() > 1 && MeasureStringWidth(s + L"...", fmt) > maxWidth) {
            s.pop_back();
        }
        return s + L"...";
    }

    void DrawText(const std::wstring& str, float x, float y, D2D1_COLOR_F color, IDWriteTextFormat* format, bool bloom = false) {
        if (str.empty() || !pRenderTarget || !format) return;

        std::wstring_view sv(str);
        size_t firstBreak = sv.find_first_of(L"\r\n");
        if (firstBreak != std::wstring_view::npos) {
            sv = sv.substr(0, firstBreak);
            if (sv.empty()) return;
        }

        float maxH = (lineHeight > 0.0f) ? (lineHeight * 1.5f) : 30.0f;
        if (bloom) {
            D2D1_COLOR_F glow = color;
            glow.a = 0.40f;
            pSolidBrush->SetColor(glow);
            for (float dx : { -1.0f, 1.0f }) {
                for (float dy : { -1.0f, 1.0f }) {
                    D2D1_RECT_F glowRect = D2D1::RectF(x + dx, y + dy, x + dx + 10000.0f, y + dy + maxH);
                    pRenderTarget->DrawTextW(sv.data(), static_cast<UINT32>(sv.size()), format, glowRect, pSolidBrush);
                }
            }
        }
        color.a = 1.0f; // All text MUST always be drawn with alpha = 1.0f (100% opaque)
        pSolidBrush->SetColor(color);
        D2D1_RECT_F layoutRect = D2D1::RectF(x, y, x + 10000.0f, y + maxH);
        pRenderTarget->DrawTextW(sv.data(), static_cast<UINT32>(sv.size()), format, layoutRect, pSolidBrush);
    }

    static bool PtInD2D1Rect(const D2D1_RECT_F& r, POINT pt) {
        return (pt.x >= r.left && pt.x <= r.right && pt.y >= r.top && pt.y <= r.bottom);
    }

private:
    void MeasureFontMetrics() {
        if (!pDWriteFactory || !pTextFormat) return;

        IDWriteTextLayout* pLayout = nullptr;
        const wchar_t* test = L"MMMMMMMMMM";
        HRESULT hr = pDWriteFactory->CreateTextLayout(test, 10, pTextFormat, 1000.0f, 100.0f, &pLayout);
        if (SUCCEEDED(hr) && pLayout) {
            DWRITE_TEXT_METRICS tm;
            pLayout->GetMetrics(&tm);
            charWidth = tm.width / 10.0f;
            lineHeight = tm.height + 5.0f;
            SafeRelease(pLayout);
        }
    }

    void InitGalaxyStars() {
        stars.clear();
        for (int i = 0; i < 60; ++i) {
            Star s;
            s.x = static_cast<float>(rand() % 1920);
            s.y = static_cast<float>(rand() % 1080);
            s.radius = 0.6f + (rand() % 15) / 10.0f;
            s.alpha = 0.3f + (rand() % 70) / 100.0f;
            s.speed = 0.04f + (rand() % 15) / 100.0f;
            s.phase = static_cast<float>(rand() % 314) / 100.0f;
            stars.push_back(s);
        }
    }
};

} // namespace speta

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
#include <string>
#include <string_view>
#include <cctype>

namespace speta {

enum class LanguageMode {
    Auto,
    PtBR,
    EnUS,
    EsES
};

enum class StringId {
    // Top Bar & Menus
    FileMenu_Workfolder,
    FileMenu_RunPs1,
    FileMenu_RunPy,
    FileMenu_OpenImage,
    FileMenu_ExportSession,
    FileMenu_Settings,
    FileMenu_CloseTab,

    ProfileMenu_PowerShell,
    ProfileMenu_Cmd,
    ProfileMenu_Admin,
    ProfileMenu_SplitRight,
    ProfileMenu_SplitDown,

    // Settings Modal
    Settings_Title,
    Settings_Theme,
    Settings_ThemeDesc,
    Settings_FontFamily,
    Settings_FontFamilyDesc,
    Settings_FontSize,
    Settings_FontSizeDesc,
    Settings_Wallpaper,
    Settings_WallpaperDesc,
    Settings_WallpaperNone,
    Settings_WallpaperAdd,
    Settings_WallpaperDim,
    Settings_WallpaperDimDesc,
    Settings_WallpaperPauseUnfocused,
    Settings_WallpaperPauseOnBattery,
    Settings_WallpaperEmptyPrompt,
    Settings_ViewMode,
    Settings_ViewModeDesc,
    Settings_Language,
    Settings_LanguageDesc,
    Settings_Cursor,
    Settings_CursorDesc,

    Settings_ModeClassic,
    Settings_ModeBlocks,

    Settings_CursorBar,
    Settings_CursorBlock,
    Settings_CursorUnderline,
    Settings_ViewIntro,

    // Onboarding Strings
    Onboarding_Step,
    Onboarding_Of,
    Onboarding_Next,
    Onboarding_Back,
    Onboarding_Skip,
    Onboarding_Finish,
    Onboarding_WelcomeTitle,
    Onboarding_WelcomeDesc,
    Onboarding_SelectLanguage,
    Onboarding_ShellTitle,
    Onboarding_ShellDesc,
    Onboarding_AppearanceTitle,
    Onboarding_AppearanceDesc,
    Onboarding_WallpaperTitle,
    Onboarding_WallpaperDesc,
    Onboarding_FeaturesTitle,
    Onboarding_FeaturesDesc,
    Onboarding_QuickPrefsTitle,
    Onboarding_QuickPrefsDesc,
    Onboarding_ReadyTitle,
    Onboarding_ReadyDesc,
    Onboarding_ReadyTip,

    // Workfolder Explorer
    Workfolder_Title,
    Workfolder_RunInTerminal,
    Workfolder_InsertPath,
    Workfolder_OpenExplorer,
    Workfolder_ItemsCount,
    Workfolder_Empty,

    // Search
    Search_Placeholder,

    // Tooltips
    Tooltip_Menu,
    Tooltip_NewTab,
    Tooltip_Profiles,
    Tooltip_Workfolder,
    Tooltip_Snippets,
    Tooltip_Search,
    Tooltip_Settings,

    // Toasts
    Toast_Copied,
    Toast_Exported,
    Toast_ThemeChanged,
    Toast_LanguageChanged,

    // Prompts & Badges
    Prompt_Running,
    Prompt_QuickFix,

    // 5-Mode Launcher
    Launcher_Title,
    Launcher_Subtitle,
    Launcher_FileExplorer,
    Launcher_FileExplorerDesc,
    Launcher_PowerShell,
    Launcher_PowerShellDesc,
    Launcher_PowerShellAdmin,
    Launcher_PowerShellAdminDesc,
    Launcher_Cmd,
    Launcher_CmdDesc,
    Launcher_CmdAdmin,
    Launcher_CmdAdminDesc,

    // File Explorer UI
    Explorer_SearchPlaceholder,
    Explorer_FilterPlaceholder,
    Explorer_EmptyFolder,
    Explorer_ExtractHere,
    Explorer_ExtractToFolder,
    Explorer_CompressZip,
    Explorer_CalculateHash,
    Explorer_HashCopiedToast,
    Explorer_UnlockFile,
    Explorer_LinkCwd,
    Explorer_CommanderCopyF5,
    Explorer_CommanderMoveF6,
    Explorer_SaveAndRepack,
    Explorer_RepackSuccessToast,
    Explorer_ExportPng,
    Explorer_AudioPlay,
    Explorer_AudioPause,
    Explorer_3DWireframe,
    Explorer_3DAutoRotate,
    Explorer_3DResetCam,
    Explorer_ShowFolderSizesSetting,
    Explorer_ShowFolderSizesDesc,
    Explorer_StartupModeSetting,
    Explorer_StartupModeDesc
};

class I18n {
private:
    static LanguageMode& GetModeRef() {
        static LanguageMode currentMode = LanguageMode::Auto;
        return currentMode;
    }

    static std::string& GetModeStringRef() {
        static std::string modeStr = "auto";
        return modeStr;
    }

public:
    static void SetLanguage(std::string_view mode) {
        std::string lower(mode);
        for (auto& c : lower) c = static_cast<char>(tolower(c));

        GetModeStringRef() = lower;
        if (lower == "pt-br" || lower == "pt" || lower == "portuguese") {
            GetModeRef() = LanguageMode::PtBR;
        } else if (lower == "es-es" || lower == "es" || lower == "spanish") {
            GetModeRef() = LanguageMode::EsES;
        } else if (lower == "en-us" || lower == "en" || lower == "english") {
            GetModeRef() = LanguageMode::EnUS;
        } else {
            GetModeRef() = LanguageMode::Auto;
        }
    }

    static LanguageMode GetLanguageMode() {
        return GetModeRef();
    }

    static const std::string& GetLanguageModeString() {
        return GetModeStringRef();
    }

    static LanguageMode GetResolvedLanguage() {
        LanguageMode m = GetModeRef();
        if (m != LanguageMode::Auto) return m;

        LANGID uiLang = GetUserDefaultUILanguage();
        WORD primary = PRIMARYLANGID(uiLang);
        if (primary == LANG_PORTUGUESE) return LanguageMode::PtBR;
        if (primary == LANG_SPANISH) return LanguageMode::EsES;
        return LanguageMode::EnUS;
    }

    static const wchar_t* Tr(const wchar_t* pt, const wchar_t* en, const wchar_t* es = nullptr) {
        LanguageMode m = GetResolvedLanguage();
        if (m == LanguageMode::PtBR) return pt;
        if (m == LanguageMode::EsES) return (es && es[0]) ? es : en;
        return en;
    }

    static const wchar_t* Get(StringId id) {
        LanguageMode lang = GetResolvedLanguage();

        switch (id) {
        // File Menu
        case StringId::FileMenu_Workfolder:
            if (lang == LanguageMode::PtBR) return L"Explorar Pasta de Trabalho";
            if (lang == LanguageMode::EsES) return L"Explorar Carpeta de Trabajo";
            return L"Workfolder Preview";

        case StringId::FileMenu_RunPs1:
            if (lang == LanguageMode::PtBR) return L"Executar Script PowerShell (.ps1)...";
            if (lang == LanguageMode::EsES) return L"Ejecutar Script PowerShell (.ps1)...";
            return L"Load PowerShell Script (.ps1)...";

        case StringId::FileMenu_RunPy:
            if (lang == LanguageMode::PtBR) return L"Executar Script Python (.py)...";
            if (lang == LanguageMode::EsES) return L"Ejecutar Script Python (.py)...";
            return L"Load Python Script (.py)...";

        case StringId::FileMenu_OpenImage:
            if (lang == LanguageMode::PtBR) return L"Visualizar Imagem no Terminal...";
            if (lang == LanguageMode::EsES) return L"Abrir Imagen en Terminal...";
            return L"Open Inline Image...";

        case StringId::FileMenu_ExportSession:
            if (lang == LanguageMode::PtBR) return L"Exportar Transcrição da Sessão...";
            if (lang == LanguageMode::EsES) return L"Exportar Transcripción de Sesión...";
            return L"Export Session Output...";

        case StringId::FileMenu_Settings:
            if (lang == LanguageMode::PtBR) return L"Configurações...";
            if (lang == LanguageMode::EsES) return L"Configuración...";
            return L"Settings...";

        case StringId::FileMenu_CloseTab:
            if (lang == LanguageMode::PtBR) return L"Fechar Aba";
            if (lang == LanguageMode::EsES) return L"Cerrar Pestaña";
            return L"Close Tab";

        // Profile Menu
        case StringId::ProfileMenu_PowerShell:
            return L"PowerShell";

        case StringId::ProfileMenu_Cmd:
            return L"Command Prompt";

        case StringId::ProfileMenu_Admin:
            if (lang == LanguageMode::PtBR) return L"Sessão como Administrador";
            if (lang == LanguageMode::EsES) return L"Sesión como Administrador";
            return L"Admin Session (Run as Admin)";

        case StringId::ProfileMenu_SplitRight:
            if (lang == LanguageMode::PtBR) return L"Dividir Painel à Direita";
            if (lang == LanguageMode::EsES) return L"Dividir Panel a la Derecha";
            return L"Split Pane Right";

        case StringId::ProfileMenu_SplitDown:
            if (lang == LanguageMode::PtBR) return L"Dividir Painel Abaixo";
            if (lang == LanguageMode::EsES) return L"Dividir Panel Abajo";
            return L"Split Pane Down";

        // Settings Modal
        case StringId::Settings_Title:
            if (lang == LanguageMode::PtBR) return L"Configurações";
            if (lang == LanguageMode::EsES) return L"Configuración";
            return L"Settings";

        case StringId::Settings_Theme:
            if (lang == LanguageMode::PtBR) return L"Tema de Cores";
            if (lang == LanguageMode::EsES) return L"Tema de Color";
            return L"Color Theme";

        case StringId::Settings_ThemeDesc:
            if (lang == LanguageMode::PtBR) return L"Paleta do terminal e sintaxe";
            if (lang == LanguageMode::EsES) return L"Paleta del terminal y sintaxis";
            return L"Terminal palette and syntax";

        case StringId::Settings_FontFamily:
            if (lang == LanguageMode::PtBR) return L"Fonte do Terminal";
            if (lang == LanguageMode::EsES) return L"Fuente del Terminal";
            return L"Terminal Font";

        case StringId::Settings_FontFamilyDesc:
            if (lang == LanguageMode::PtBR) return L"Tipografia monoespaçada";
            if (lang == LanguageMode::EsES) return L"Tipografía monoespaciada";
            return L"Monospace typography";

        case StringId::Settings_FontSize:
            if (lang == LanguageMode::PtBR) return L"Tamanho da Fonte";
            if (lang == LanguageMode::EsES) return L"Tamaño de Fuente";
            return L"Font Size";

        case StringId::Settings_FontSizeDesc:
            if (lang == LanguageMode::PtBR) return L"Zoom também via Ctrl + Scroll";
            if (lang == LanguageMode::EsES) return L"Zoom también vía Ctrl + Scroll";
            return L"Zoom also via Ctrl + Scroll";

        case StringId::Settings_Wallpaper:
            if (lang == LanguageMode::PtBR) return L"Papel de Parede";
            if (lang == LanguageMode::EsES) return L"Fondo de Pantalla";
            return L"Wallpaper";

        case StringId::Settings_WallpaperDesc:
            if (lang == LanguageMode::PtBR) return L"Personalize o fundo com imagem, GIF ou vídeo";
            if (lang == LanguageMode::EsES) return L"Personaliza el fondo con imagen, GIF o vídeo";
            return L"Customize background with image, GIF or video";

        case StringId::Settings_WallpaperNone:
            if (lang == LanguageMode::PtBR) return L"Nenhum";
            if (lang == LanguageMode::EsES) return L"Ninguno";
            return L"None";

        case StringId::Settings_WallpaperAdd:
            return L"ADD+";

        case StringId::Settings_WallpaperDim:
            if (lang == LanguageMode::PtBR) return L"Escurecer";
            if (lang == LanguageMode::EsES) return L"Oscurecer";
            return L"Dim";

        case StringId::Settings_WallpaperDimDesc:
            if (lang == LanguageMode::PtBR) return L"Ajusta a intensidade do veu sobre o papel de parede";
            if (lang == LanguageMode::EsES) return L"Ajusta la intensidad del velo sobre el fondo de pantalla";
            return L"Adjusts background veil opacity over wallpaper";

        case StringId::Settings_WallpaperPauseUnfocused:
            if (lang == LanguageMode::PtBR) return L"Pausar quando nao estiver em foco";
            if (lang == LanguageMode::EsES) return L"Pausar cuando no este enfocado";
            return L"Pause when unfocused";

        case StringId::Settings_WallpaperPauseOnBattery:
            if (lang == LanguageMode::PtBR) return L"Pausar na bateria";
            if (lang == LanguageMode::EsES) return L"Pausar en bateria";
            return L"Pause on battery";

        case StringId::Settings_WallpaperEmptyPrompt:
            if (lang == LanguageMode::PtBR) return L"Coloque imagens, GIFs ou videos em assets/wallpapers ou clique em ADD+";
            if (lang == LanguageMode::EsES) return L"Coloque imagenes, GIFs o videos en assets/wallpapers o haga clic en ADD+";
            return L"Place images, GIFs or videos in assets/wallpapers or click ADD+";

        case StringId::Settings_ViewMode:
            if (lang == LanguageMode::PtBR) return L"Modo de Visualização";
            if (lang == LanguageMode::EsES) return L"Modo de Vista";
            return L"View Mode";

        case StringId::Settings_ViewModeDesc:
            if (lang == LanguageMode::PtBR) return L"Fluxo contínuo ou em blocos";
            if (lang == LanguageMode::EsES) return L"Flujo continuo o en bloques";
            return L"Fluid stream or command blocks";

        case StringId::Settings_Language:
            if (lang == LanguageMode::PtBR) return L"Idioma";
            if (lang == LanguageMode::EsES) return L"Idioma";
            return L"Language";

        case StringId::Settings_LanguageDesc:
            if (lang == LanguageMode::PtBR) return L"Idioma da interface do usuário";
            if (lang == LanguageMode::EsES) return L"Idioma de la interfaz de usuario";
            return L"User interface language";

        case StringId::Settings_Cursor:
            if (lang == LanguageMode::PtBR) return L"Estilo do Cursor";
            if (lang == LanguageMode::EsES) return L"Estilo del Cursor";
            return L"Cursor Style";

        case StringId::Settings_CursorDesc:
            if (lang == LanguageMode::PtBR) return L"Formato do indicador de digitação";
            if (lang == LanguageMode::EsES) return L"Formato del cursor";
            return L"Input cursor appearance";

        case StringId::Settings_ModeClassic:
            if (lang == LanguageMode::PtBR) return L"Fluxo Clássico";
            if (lang == LanguageMode::EsES) return L"Flujo Clásico";
            return L"Classic Stream";

        case StringId::Settings_ModeBlocks:
            if (lang == LanguageMode::PtBR) return L"Blocos";
            if (lang == LanguageMode::EsES) return L"Bloques";
            return L"Blocks";

        case StringId::Settings_CursorBar:
            if (lang == LanguageMode::PtBR) return L"Barra";
            if (lang == LanguageMode::EsES) return L"Barra";
            return L"Bar";

        case StringId::Settings_CursorBlock:
            if (lang == LanguageMode::PtBR) return L"Bloco";
            if (lang == LanguageMode::EsES) return L"Bloque";
            return L"Block";

        case StringId::Settings_CursorUnderline:
            if (lang == LanguageMode::PtBR) return L"Sublinhado";
            if (lang == LanguageMode::EsES) return L"Subrayado";
            return L"Underline";

        case StringId::Settings_ViewIntro:
            if (lang == LanguageMode::PtBR) return L"Ver introdução novamente";
            if (lang == LanguageMode::EsES) return L"Ver introducción de nuevo";
            return L"View intro again";

        case StringId::Onboarding_Step:
            if (lang == LanguageMode::PtBR) return L"Passo";
            if (lang == LanguageMode::EsES) return L"Paso";
            return L"Step";

        case StringId::Onboarding_Of:
            if (lang == LanguageMode::PtBR) return L"de";
            if (lang == LanguageMode::EsES) return L"de";
            return L"of";

        case StringId::Onboarding_Next:
            if (lang == LanguageMode::PtBR) return L"Próximo";
            if (lang == LanguageMode::EsES) return L"Siguiente";
            return L"Next";

        case StringId::Onboarding_Back:
            if (lang == LanguageMode::PtBR) return L"Voltar";
            if (lang == LanguageMode::EsES) return L"Atrás";
            return L"Back";

        case StringId::Onboarding_Skip:
            if (lang == LanguageMode::PtBR) return L"Pular introdução";
            if (lang == LanguageMode::EsES) return L"Saltar introducción";
            return L"Skip intro";

        case StringId::Onboarding_Finish:
            if (lang == LanguageMode::PtBR) return L"Começar";
            if (lang == LanguageMode::EsES) return L"Empezar";
            return L"Get Started";

        case StringId::Onboarding_WelcomeTitle:
            if (lang == LanguageMode::PtBR) return L"Bem-vindo ao Spet@term";
            if (lang == LanguageMode::EsES) return L"Bienvenido a Spet@term";
            return L"Welcome to Spet@term";

        case StringId::Onboarding_WelcomeDesc:
            if (lang == LanguageMode::PtBR) return L"Terminal nativo de alto desempenho com Direct2D e Mini IDE";
            if (lang == LanguageMode::EsES) return L"Terminal nativo de alto rendimiento con Direct2D y Mini IDE";
            return L"High-performance native terminal with Direct2D and Mini IDE";

        case StringId::Onboarding_SelectLanguage:
            if (lang == LanguageMode::PtBR) return L"Escolha seu idioma";
            if (lang == LanguageMode::EsES) return L"Elige tu idioma";
            return L"Choose your language";

        case StringId::Onboarding_ShellTitle:
            if (lang == LanguageMode::PtBR) return L"Shell Padrão";
            if (lang == LanguageMode::EsES) return L"Shell Predeterminado";
            return L"Default Shell";

        case StringId::Onboarding_ShellDesc:
            if (lang == LanguageMode::PtBR) return L"Escolha qual interpretador iniciar nas novas abas";
            if (lang == LanguageMode::EsES) return L"Elige qué intérprete de shell iniciar en nuevas pestañas";
            return L"Choose which shell interpreter to launch in new tabs";

        case StringId::Onboarding_AppearanceTitle:
            if (lang == LanguageMode::PtBR) return L"Aparência";
            if (lang == LanguageMode::EsES) return L"Apariencia";
            return L"Appearance";

        case StringId::Onboarding_AppearanceDesc:
            if (lang == LanguageMode::PtBR) return L"Escolha seu tema visual, família de fonte e tamanho";
            if (lang == LanguageMode::EsES) return L"Elige tu tema visual, familia de fuente y tamaño";
            return L"Choose your visual theme, font family and size";

        case StringId::Onboarding_WallpaperTitle:
            if (lang == LanguageMode::PtBR) return L"Papel de Parede Animado";
            if (lang == LanguageMode::EsES) return L"Fondo de Pantalla Animado";
            return L"Animated Wallpaper";

        case StringId::Onboarding_WallpaperDesc:
            if (lang == LanguageMode::PtBR) return L"Escolha um vídeo, GIF ou imagem com ajuste de escurecimento";
            if (lang == LanguageMode::EsES) return L"Elige un vídeo, GIF o imagen con ajuste de oscurecimiento";
            return L"Choose a video, GIF or image with dimming adjustment";

        case StringId::Onboarding_FeaturesTitle:
            if (lang == LanguageMode::PtBR) return L"Recursos Principais";
            if (lang == LanguageMode::EsES) return L"Características Principales";
            return L"Core Features";

        case StringId::Onboarding_FeaturesDesc:
            if (lang == LanguageMode::PtBR) return L"Atalhos essenciais para produtividade máxima";
            if (lang == LanguageMode::EsES) return L"Accesos directos esenciales para máxima productividad";
            return L"Essential shortcuts for maximum productivity";

        case StringId::Onboarding_QuickPrefsTitle:
            if (lang == LanguageMode::PtBR) return L"Preferências Rápidas";
            if (lang == LanguageMode::EsES) return L"Preferencias Rápidas";
            return L"Quick Preferences";

        case StringId::Onboarding_QuickPrefsDesc:
            if (lang == LanguageMode::PtBR) return L"Ajuste segurança, integração ao sistema e cursor";
            if (lang == LanguageMode::EsES) return L"Ajusta seguridad, integración al sistema y cursor";
            return L"Tune security, system integration and cursor";

        case StringId::Onboarding_ReadyTitle:
            if (lang == LanguageMode::PtBR) return L"Tudo Pronto!";
            if (lang == LanguageMode::EsES) return L"¡Todo Listo!";
            return L"All Set!";

        case StringId::Onboarding_ReadyDesc:
            if (lang == LanguageMode::PtBR) return L"Seu Spet@term está configurado e pronto para uso";
            if (lang == LanguageMode::EsES) return L"Tu Spet@term está configurado y listo para usar";
            return L"Your Spet@term is configured and ready to use";

        case StringId::Onboarding_ReadyTip:
            if (lang == LanguageMode::PtBR) return L"Abra as Configurações a qualquer momento com Ctrl+,";
            if (lang == LanguageMode::EsES) return L"Abre la Configuración en cualquier momento con Ctrl+,";
            return L"Open Settings anytime with Ctrl+,";

        // Workfolder Explorer
        case StringId::Workfolder_Title:
            if (lang == LanguageMode::PtBR) return L"Explorador da Pasta";
            if (lang == LanguageMode::EsES) return L"Explorador de Carpeta";
            return L"Workfolder Preview";

        case StringId::Workfolder_RunInTerminal:
            if (lang == LanguageMode::PtBR) return L"Executar no Terminal";
            if (lang == LanguageMode::EsES) return L"Ejecutar en Terminal";
            return L"Run in Terminal";

        case StringId::Workfolder_InsertPath:
            if (lang == LanguageMode::PtBR) return L"Inserir Caminho";
            if (lang == LanguageMode::EsES) return L"Insertar Ruta";
            return L"Insert Path";

        case StringId::Workfolder_OpenExplorer:
            if (lang == LanguageMode::PtBR) return L"Abrir no Explorer";
            if (lang == LanguageMode::EsES) return L"Abrir en Explorer";
            return L"Open in Explorer";

        case StringId::Workfolder_ItemsCount:
            if (lang == LanguageMode::PtBR) return L"itens";
            if (lang == LanguageMode::EsES) return L"elementos";
            return L"items";

        case StringId::Workfolder_Empty:
            if (lang == LanguageMode::PtBR) return L"Pasta vazia";
            if (lang == LanguageMode::EsES) return L"Carpeta vacía";
            return L"Folder is empty";

        // Search
        case StringId::Search_Placeholder:
            if (lang == LanguageMode::PtBR) return L"Localizar no terminal (Enter / Shift+Enter)...";
            if (lang == LanguageMode::EsES) return L"Buscar en terminal (Enter / Shift+Enter)...";
            return L"Find in terminal (Enter / Shift+Enter)...";

        // Tooltips
        case StringId::Tooltip_Menu:
            if (lang == LanguageMode::PtBR) return L"Menu de Arquivo (Alt+F)";
            if (lang == LanguageMode::EsES) return L"Menú de Archivo (Alt+F)";
            return L"File Menu (Alt+F)";

        case StringId::Tooltip_NewTab:
            if (lang == LanguageMode::PtBR) return L"Nova Aba (Ctrl+T)";
            if (lang == LanguageMode::EsES) return L"Nueva Pestaña (Ctrl+T)";
            return L"New Tab (Ctrl+T)";

        case StringId::Tooltip_Profiles:
            if (lang == LanguageMode::PtBR) return L"Perfis e Sessões";
            if (lang == LanguageMode::EsES) return L"Perfiles y Sesiones";
            return L"Profiles and Sessions";

        case StringId::Tooltip_Workfolder:
            if (lang == LanguageMode::PtBR) return L"Workfolder Preview (Ctrl+B)";
            if (lang == LanguageMode::EsES) return L"Explorador de Carpeta (Ctrl+B)";
            return L"Workfolder Preview (Ctrl+B)";

        case StringId::Tooltip_Snippets:
            if (lang == LanguageMode::PtBR) return L"Snippets Rápidos (Alt+S)";
            if (lang == LanguageMode::EsES) return L"Snippets Rápidos (Alt+S)";
            return L"Quick Snippets (Alt+S)";

        case StringId::Tooltip_Search:
            if (lang == LanguageMode::PtBR) return L"Buscar no Histórico (Ctrl+F)";
            if (lang == LanguageMode::EsES) return L"Buscar en Historial (Ctrl+F)";
            return L"Search History (Ctrl+F)";

        case StringId::Tooltip_Settings:
            if (lang == LanguageMode::PtBR) return L"Configurações (Ctrl+,)";
            if (lang == LanguageMode::EsES) return L"Configuración (Ctrl+,)";
            return L"Settings (Ctrl+,)";

        // Toasts
        case StringId::Toast_Copied:
            if (lang == LanguageMode::PtBR) return L"Copiado para a área de transferência";
            if (lang == LanguageMode::EsES) return L"Copiado al portapapeles";
            return L"Copied to clipboard";

        case StringId::Toast_Exported:
            if (lang == LanguageMode::PtBR) return L"Transcrição exportada com sucesso";
            if (lang == LanguageMode::EsES) return L"Transcripción exportada con éxito";
            return L"Transcript exported successfully";

        case StringId::Toast_ThemeChanged:
            if (lang == LanguageMode::PtBR) return L"Tema aplicado";
            if (lang == LanguageMode::EsES) return L"Tema aplicado";
            return L"Theme applied";

        case StringId::Toast_LanguageChanged:
            if (lang == LanguageMode::PtBR) return L"Idioma alterado para Português";
            if (lang == LanguageMode::EsES) return L"Idioma cambiado a Español";
            return L"Language changed to English";

        case StringId::Prompt_Running:
            if (lang == LanguageMode::PtBR) return L"executando...";
            if (lang == LanguageMode::EsES) return L"ejecutando...";
            return L"running...";

        case StringId::Prompt_QuickFix:
            if (lang == LanguageMode::PtBR) return L"Aplicar: ";
            if (lang == LanguageMode::EsES) return L"Aplicar: ";
            return L"Apply: ";

        // 5-Mode Launcher
        case StringId::Launcher_Title:
            if (lang == LanguageMode::PtBR) return L"Selecione o Modo da Nova Aba";
            if (lang == LanguageMode::EsES) return L"Seleccione el Modo de la Nueva Pestaña";
            return L"Select New Tab Mode";

        case StringId::Launcher_Subtitle:
            if (lang == LanguageMode::PtBR) return L"Use as teclas 1-5, setas ou clique para iniciar";
            if (lang == LanguageMode::EsES) return L"Use las teclas 1-5, flechas o clic para iniciar";
            return L"Press 1-5, use arrow keys, or click to launch";

        case StringId::Launcher_FileExplorer:
            if (lang == LanguageMode::PtBR) return L"Explorador de Arquivos";
            if (lang == LanguageMode::EsES) return L"Explorador de Archivos";
            return L"File Explorer";

        case StringId::Launcher_FileExplorerDesc:
            if (lang == LanguageMode::PtBR) return L"Gerenciador nativo com arquivos compactados, pacotes de jogos e preview 3D/midia";
            if (lang == LanguageMode::EsES) return L"Gestor nativo con archivos comprimidos, paquetes de juegos y vista previa 3D/multimedia";
            return L"Native file manager with archives, game assets, and 3D/media preview";

        case StringId::Launcher_PowerShell:
            if (lang == LanguageMode::PtBR) return L"PowerShell";
            if (lang == LanguageMode::EsES) return L"PowerShell";
            return L"PowerShell";

        case StringId::Launcher_PowerShellDesc:
            if (lang == LanguageMode::PtBR) return L"Sessão padrão do PowerShell com autocomplete, blocos e diagnóstico inteligente";
            if (lang == LanguageMode::EsES) return L"Sesión estándar de PowerShell con autocompletado, bloques y diagnóstico inteligente";
            return L"Standard PowerShell session with autocomplete, blocks, and smart diagnostics";

        case StringId::Launcher_PowerShellAdmin:
            if (lang == LanguageMode::PtBR) return L"PowerShell (Admin)";
            if (lang == LanguageMode::EsES) return L"PowerShell (Admin)";
            return L"PowerShell (Admin)";

        case StringId::Launcher_PowerShellAdminDesc:
            if (lang == LanguageMode::PtBR) return L"Sessão elevada do PowerShell com privilégios de Administrador";
            if (lang == LanguageMode::EsES) return L"Sesión elevada de PowerShell con privilegios de Administrador";
            return L"Elevated PowerShell session with Administrator privileges";

        case StringId::Launcher_Cmd:
            if (lang == LanguageMode::PtBR) return L"Prompt de Comando (CMD)";
            if (lang == LanguageMode::EsES) return L"Símbolo del Sistema (CMD)";
            return L"Command Prompt (CMD)";

        case StringId::Launcher_CmdDesc:
            if (lang == LanguageMode::PtBR) return L"Interpretador clássico cmd.exe ultrarrápido com comandos nativos";
            if (lang == LanguageMode::EsES) return L"Intérprete clásico cmd.exe ultrarrápido con comandos nativos";
            return L"Fast classic cmd.exe interpreter with native built-in commands";

        case StringId::Launcher_CmdAdmin:
            if (lang == LanguageMode::PtBR) return L"Prompt de Comando (Admin)";
            if (lang == LanguageMode::EsES) return L"Símbolo del Sistema (Admin)";
            return L"Command Prompt (Admin)";

        case StringId::Launcher_CmdAdminDesc:
            if (lang == LanguageMode::PtBR) return L"Sessão elevada do CMD para manutenção do sistema, diskpart, sfc e dism";
            if (lang == LanguageMode::EsES) return L"Sesión elevada de CMD para mantenimiento del sistema, diskpart, sfc y dism";
            return L"Elevated CMD session for system maintenance, diskpart, sfc, and dism";

        // File Explorer UI
        case StringId::Explorer_SearchPlaceholder:
            if (lang == LanguageMode::PtBR) return L"Buscar arquivos e pastas (Ctrl+F)...";
            if (lang == LanguageMode::EsES) return L"Buscar archivos y carpetas (Ctrl+F)...";
            return L"Search files and folders (Ctrl+F)...";

        case StringId::Explorer_FilterPlaceholder:
            if (lang == LanguageMode::PtBR) return L"Filtrar lista atual...";
            if (lang == LanguageMode::EsES) return L"Filtrar lista actual...";
            return L"Filter current list...";

        case StringId::Explorer_EmptyFolder:
            if (lang == LanguageMode::PtBR) return L"Esta pasta está vazia";
            if (lang == LanguageMode::EsES) return L"Esta carpeta está vacía";
            return L"This folder is empty";

        case StringId::Explorer_ExtractHere:
            if (lang == LanguageMode::PtBR) return L"Extrair Aqui";
            if (lang == LanguageMode::EsES) return L"Extraer Aquí";
            return L"Extract Here";

        case StringId::Explorer_ExtractToFolder:
            if (lang == LanguageMode::PtBR) return L"Extrair para Pasta";
            if (lang == LanguageMode::EsES) return L"Extraer en Carpeta";
            return L"Extract to Folder";

        case StringId::Explorer_CompressZip:
            if (lang == LanguageMode::PtBR) return L"Compactar em .ZIP";
            if (lang == LanguageMode::EsES) return L"Comprimir en .ZIP";
            return L"Compress to .ZIP";

        case StringId::Explorer_CalculateHash:
            if (lang == LanguageMode::PtBR) return L"Calcular Hash (CRC32 / MD5 / SHA-256)";
            if (lang == LanguageMode::EsES) return L"Calcular Hash (CRC32 / MD5 / SHA-256)";
            return L"Calculate Hash (CRC32 / MD5 / SHA-256)";

        case StringId::Explorer_HashCopiedToast:
            if (lang == LanguageMode::PtBR) return L"Hash SHA-256 copiado para a área de transferência";
            if (lang == LanguageMode::EsES) return L"Hash SHA-256 copiado al portapapeles";
            return L"SHA-256 hash copied to clipboard";

        case StringId::Explorer_UnlockFile:
            if (lang == LanguageMode::PtBR) return L"Quem está travando este arquivo?";
            if (lang == LanguageMode::EsES) return L"¿Quién bloquea este archivo?";
            return L"Who is locking this file?";

        case StringId::Explorer_LinkCwd:
            if (lang == LanguageMode::PtBR) return L"Sincronizar pasta com Terminal";
            if (lang == LanguageMode::EsES) return L"Sincronizar carpeta con Terminal";
            return L"Sync CWD with Terminal";

        case StringId::Explorer_CommanderCopyF5:
            if (lang == LanguageMode::PtBR) return L"F5 Copiar para Painel Oposto";
            if (lang == LanguageMode::EsES) return L"F5 Copiar al Panel Opuesto";
            return L"F5 Copy to Opposite Pane";

        case StringId::Explorer_CommanderMoveF6:
            if (lang == LanguageMode::PtBR) return L"F6 Mover para Painel Oposto";
            if (lang == LanguageMode::EsES) return L"F6 Mover al Panel Opuesto";
            return L"F6 Move to Opposite Pane";

        case StringId::Explorer_SaveAndRepack:
            if (lang == LanguageMode::PtBR) return L"Salvar e Reempacotar Arquivo (Ctrl+S)";
            if (lang == LanguageMode::EsES) return L"Guardar y Reempaquetar Archivo (Ctrl+S)";
            return L"Save & Repack Archive (Ctrl+S)";

        case StringId::Explorer_RepackSuccessToast:
            if (lang == LanguageMode::PtBR) return L"Arquivo salvo e reempacotado com sucesso";
            if (lang == LanguageMode::EsES) return L"Archivo guardado y reempaquetado con éxito";
            return L"Archive saved and repacked successfully";

        case StringId::Explorer_ExportPng:
            if (lang == LanguageMode::PtBR) return L"Exportar Textura como .PNG";
            if (lang == LanguageMode::EsES) return L"Exportar Textura como .PNG";
            return L"Export Texture as .PNG";

        case StringId::Explorer_AudioPlay:
            if (lang == LanguageMode::PtBR) return L"Reproduzir";
            if (lang == LanguageMode::EsES) return L"Reproducir";
            return L"Play";

        case StringId::Explorer_AudioPause:
            if (lang == LanguageMode::PtBR) return L"Pausar";
            if (lang == LanguageMode::EsES) return L"Pausar";
            return L"Pause";

        case StringId::Explorer_3DWireframe:
            if (lang == LanguageMode::PtBR) return L"Wireframe";
            if (lang == LanguageMode::EsES) return L"Malla (Wireframe)";
            return L"Wireframe";

        case StringId::Explorer_3DAutoRotate:
            if (lang == LanguageMode::PtBR) return L"Rotação Automática";
            if (lang == LanguageMode::EsES) return L"Rotación Automática";
            return L"Auto-Rotate";

        case StringId::Explorer_3DResetCam:
            if (lang == LanguageMode::PtBR) return L"Resetar Câmera";
            if (lang == LanguageMode::EsES) return L"Restablecer Cámara";
            return L"Reset Camera";

        case StringId::Explorer_ShowFolderSizesSetting:
            if (lang == LanguageMode::PtBR) return L"Calcular Tamanho de Pastas no Explorer";
            if (lang == LanguageMode::EsES) return L"Calcular Tamaño de Carpetas en Explorer";
            return L"Calculate Folder Sizes in Explorer";

        case StringId::Explorer_ShowFolderSizesDesc:
            if (lang == LanguageMode::PtBR) return L"Calcula tamanhos em segundo plano (desative em HDDs lentos)";
            if (lang == LanguageMode::EsES) return L"Calcula tamaños en segundo plano (desactivar en HDDs lentos)";
            return L"Compute folder sizes in background (disable on slow HDDs)";

        case StringId::Explorer_StartupModeSetting:
            if (lang == LanguageMode::PtBR) return L"Modo Inicial ao Abrir / Nova Aba";
            if (lang == LanguageMode::EsES) return L"Modo Inicial al Abrir / Nueva Pestaña";
            return L"Default Startup / New Tab Mode";

        case StringId::Explorer_StartupModeDesc:
            if (lang == LanguageMode::PtBR) return L"Seletor de 5 modos, Explorador de Arquivos, PowerShell ou CMD";
            if (lang == LanguageMode::EsES) return L"Selector de 5 modos, Explorador de Archivos, PowerShell o CMD";
            return L"5-mode Launcher, File Explorer, PowerShell, or CMD";
        }

        return L"";
    }

    static const wchar_t* Get(std::string_view key) {
        if (key == "file_workfolder") return Get(StringId::FileMenu_Workfolder);
        if (key == "settings_title") return Get(StringId::Settings_Title);
        if (key == "toast_copied") return Get(StringId::Toast_Copied);
        return L"";
    }
};

} // namespace speta

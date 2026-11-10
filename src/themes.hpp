#pragma once

#include "common.hpp"
#include "i18n.hpp"

namespace speta {

inline D2D1_COLOR_F ColorHex(uint32_t rgb, float a = 1.0f) {
    float r = ((rgb >> 16) & 0xFF) / 255.0f;
    float g = ((rgb >> 8) & 0xFF) / 255.0f;
    float b = (rgb & 0xFF) / 255.0f;
    return D2D1::ColorF(r, g, b, a);
}

inline D2D1_COLOR_F ColorRgba(float r, float g, float b, float a = 1.0f) {
    return D2D1::ColorF(r, g, b, a);
}

struct Star {
    float x = 0.0f;
    float y = 0.0f;
    float radius = 1.0f;
    float alpha = 1.0f;
    float speed = 0.05f;
    float phase = 0.0f;
};

enum class GeometryStyle {
    BrutalistChamfer,
    TacticalGlass
};

struct Theme {
    std::wstring name;
    std::wstring description;

    D2D1_COLOR_F bg;
    D2D1_COLOR_F bgSecondary;
    D2D1_COLOR_F blockBg;
    D2D1_COLOR_F blockBorder;
    D2D1_COLOR_F blockHeaderBg;
    D2D1_COLOR_F textPrimary;
    D2D1_COLOR_F textDim;
    D2D1_COLOR_F accent;
    D2D1_COLOR_F promptCmd;
    D2D1_COLOR_F promptPw;
    D2D1_COLOR_F syntaxValidCommand;
    D2D1_COLOR_F syntaxInvalidCommand;
    D2D1_COLOR_F syntaxString;
    D2D1_COLOR_F syntaxFlag;
    D2D1_COLOR_F syntaxNumber;
    D2D1_COLOR_F syntaxComment;
    D2D1_COLOR_F syntaxKeyword;
    D2D1_COLOR_F gitClean;
    D2D1_COLOR_F gitDirty;
    D2D1_COLOR_F selectionBg;
    D2D1_COLOR_F hudBg;
    D2D1_COLOR_F buttonHoverBg;

    bool isGalaxy = false;

    // Theme Engine v4 Extensions
    GeometryStyle geometry = GeometryStyle::TacticalGlass;
    float chamferSize = 6.0f;
    bool crtScanlines = false;
    bool phosphorBloom = false;
    bool crtVignette = false;
    bool uppercaseHud = false;
    std::string heroSprite = "saturn_folder_clean.png";
};

class ThemeManager {
public:
    static Theme CreatePitchBlack() {
        Theme t;
        t.name = L"Pitch Black";
        t.description = I18n::Tr(L"OLED puro com paleta refinada (#000000)", L"Pure OLED with refined palette (#000000)", L"OLED puro con paleta refinada (#000000)");
        t.bg = ColorHex(0x000000);
        t.bgSecondary = ColorHex(0x0D0D11);
        t.blockBg = ColorHex(0x0A0A0E);
        t.blockBorder = ColorHex(0x1F1F28);
        t.blockHeaderBg = ColorHex(0x141418);
        t.textPrimary = ColorHex(0xF0F0FF);
        t.textDim = ColorHex(0x78788C);
        t.accent = ColorHex(0x80AAFF);
        t.promptCmd = ColorHex(0xF0F0FF);
        t.promptPw = ColorHex(0x80D4FF);
        t.syntaxValidCommand = ColorHex(0x00FFBF);
        t.syntaxInvalidCommand = ColorHex(0xFF80AA);
        t.syntaxString = ColorHex(0xFFD480);
        t.syntaxFlag = ColorHex(0xE2A1F7);
        t.syntaxNumber = ColorHex(0x99DDFF);
        t.syntaxComment = ColorHex(0x64748B);
        t.syntaxKeyword = ColorHex(0x80AAFF);
        t.gitClean = ColorHex(0x66FFD9);
        t.gitDirty = ColorHex(0xFFD480);
        t.selectionBg = ColorHex(0x505064);
        t.hudBg = ColorHex(0x0D0D11);
        t.buttonHoverBg = ColorHex(0x1F1F28);
        t.isGalaxy = false;
        t.geometry = GeometryStyle::BrutalistChamfer;
        t.chamferSize = 6.0f;
        t.heroSprite = "saturn_folder_clean.png";
        return t;
    }

    static Theme CreateBlue() {
        Theme t;
        t.name = L"Blue";
        t.description = I18n::Tr(L"Deep Navy maritimo com azul suave (#06162E)", L"Deep Navy maritime with soft blue (#06162E)", L"Azul marino profundo con azul suave (#06162E)");
        t.bg = ColorHex(0x06162E);
        t.bgSecondary = ColorHex(0x092040);
        t.blockBg = ColorHex(0x0B2244);
        t.blockBorder = ColorHex(0x163A6E);
        t.blockHeaderBg = ColorHex(0x0E2C58);
        t.textPrimary = ColorHex(0xE2F1FF);
        t.textDim = ColorHex(0x7BA3CD);
        t.accent = ColorHex(0x80AAFF);
        t.promptCmd = ColorHex(0xFACC15);
        t.promptPw = ColorHex(0x80D4FF);
        t.syntaxValidCommand = ColorHex(0x00FFBF);
        t.syntaxInvalidCommand = ColorHex(0xFF80AA);
        t.syntaxString = ColorHex(0xFFD480);
        t.syntaxFlag = ColorHex(0xE2A1F7);
        t.syntaxNumber = ColorHex(0x99DDFF);
        t.syntaxComment = ColorHex(0x64748B);
        t.syntaxKeyword = ColorHex(0x80AAFF);
        t.gitClean = ColorHex(0x66FFD9);
        t.gitDirty = ColorHex(0xFFD480);
        t.selectionBg = ColorRgba(0.20f, 0.40f, 0.70f, 0.35f);
        t.hudBg = ColorHex(0x040F20);
        t.buttonHoverBg = ColorHex(0x153A68);
        t.isGalaxy = false;
        t.geometry = GeometryStyle::TacticalGlass;
        t.heroSprite = "terminal_window.png";
        return t;
    }

    static Theme CreateLight() {
        Theme t;
        t.name = L"Light";
        t.description = I18n::Tr(L"Claro e limpo com alta legibilidade (#F8FAFC)", L"Clean light mode with high readability (#F8FAFC)", L"Claro y limpio con alta legibilidad (#F8FAFC)");
        t.bg = ColorHex(0xF8FAFC);
        t.bgSecondary = ColorHex(0xF1F5F9);
        t.blockBg = ColorHex(0xFFFFFF);
        t.blockBorder = ColorHex(0xCBD5E1);
        t.blockHeaderBg = ColorHex(0xE2E8F0);
        t.textPrimary = ColorHex(0x0F172A);
        t.textDim = ColorHex(0x64748B);
        t.accent = ColorHex(0x2563EB);
        t.promptCmd = ColorHex(0xD97706);
        t.promptPw = ColorHex(0x2563EB);
        t.syntaxValidCommand = ColorHex(0x15803D);
        t.syntaxInvalidCommand = ColorHex(0xDC2626);
        t.syntaxString = ColorHex(0xB45309);
        t.syntaxFlag = ColorHex(0x7C3AED);
        t.syntaxNumber = ColorHex(0x0369A1);
        t.syntaxComment = ColorHex(0x94A3B8);
        t.syntaxKeyword = ColorHex(0x9333EA);
        t.gitClean = ColorHex(0x16A34A);
        t.gitDirty = ColorHex(0xD97706);
        t.selectionBg = ColorRgba(0.15f, 0.39f, 0.92f, 0.20f);
        t.hudBg = ColorHex(0xE2E8F0);
        t.buttonHoverBg = ColorHex(0xCBD5E1);
        t.isGalaxy = false;
        t.geometry = GeometryStyle::TacticalGlass;
        t.heroSprite = "saturn_folder_clean.png";
        return t;
    }

    static Theme CreateVSCode() {
        Theme t;
        t.name = L"VS Code Like";
        t.description = I18n::Tr(L"Estilo Dark+ do VS Code com status azul (#1E1E1E)", L"VS Code Dark+ style with blue status bar (#1E1E1E)", L"Estilo Dark+ de VS Code con barra azul (#1E1E1E)");
        t.bg = ColorHex(0x1E1E1E);
        t.bgSecondary = ColorHex(0x252526);
        t.blockBg = ColorHex(0x252526);
        t.blockBorder = ColorHex(0x333333);
        t.blockHeaderBg = ColorHex(0x2D2D2D);
        t.textPrimary = ColorHex(0xD4D4D4);
        t.textDim = ColorHex(0x808080);
        t.accent = ColorHex(0x007ACC);
        t.promptCmd = ColorHex(0xDCDCAA);
        t.promptPw = ColorHex(0x569CD6);
        t.syntaxValidCommand = ColorHex(0x4EC9B0);
        t.syntaxInvalidCommand = ColorHex(0xF44747);
        t.syntaxString = ColorHex(0xCE9178);
        t.syntaxFlag = ColorHex(0x9CDCFE);
        t.syntaxNumber = ColorHex(0xB5CEA8);
        t.syntaxComment = ColorHex(0x6A9955);
        t.syntaxKeyword = ColorHex(0xC586C0);
        t.gitClean = ColorHex(0x4EC9B0);
        t.gitDirty = ColorHex(0xDCDCAA);
        t.selectionBg = ColorRgba(0.16f, 0.38f, 0.65f, 0.40f);
        t.hudBg = ColorHex(0x007ACC);
        t.buttonHoverBg = ColorHex(0x3C3C3C);
        t.isGalaxy = false;
        t.geometry = GeometryStyle::TacticalGlass;
        t.heroSprite = "terminal_window.png";
        return t;
    }

    static Theme CreateGalaxy() {
        Theme t;
        t.name = L"Galaxy";
        t.description = I18n::Tr(L"Cosmico translucido com nebulosa e estrelas procedurais", L"Translucent cosmic theme with nebula and procedural stars", L"Cósmico translúcido con nebulosa y estrellas procedurales");
        t.bg = ColorHex(0x160A2C);
        t.bgSecondary = ColorRgba(0.10f, 0.05f, 0.18f, 0.85f);
        t.blockBg = ColorRgba(0.10f, 0.06f, 0.18f, 0.78f);
        t.blockBorder = ColorRgba(0.70f, 0.40f, 1.0f, 0.35f);
        t.blockHeaderBg = ColorRgba(0.15f, 0.08f, 0.28f, 0.85f);
        t.textPrimary = ColorHex(0xF5E8FF);
        t.textDim = ColorHex(0xA88FB8);
        t.accent = ColorHex(0xE879F9);
        t.promptCmd = ColorHex(0x38BDF8);
        t.promptPw = ColorHex(0xE879F9);
        t.syntaxValidCommand = ColorHex(0x38BDF8);
        t.syntaxInvalidCommand = ColorHex(0xFF4976);
        t.syntaxString = ColorHex(0xFDE047);
        t.syntaxFlag = ColorHex(0xE879F9);
        t.syntaxNumber = ColorHex(0x67E8F9);
        t.syntaxComment = ColorHex(0x818CF8);
        t.syntaxKeyword = ColorHex(0xF472B6);
        t.gitClean = ColorHex(0x4ADE80);
        t.gitDirty = ColorHex(0xFDE047);
        t.selectionBg = ColorRgba(0.91f, 0.47f, 0.98f, 0.30f);
        t.hudBg = ColorRgba(0.06f, 0.02f, 0.12f, 0.90f);
        t.buttonHoverBg = ColorRgba(0.40f, 0.20f, 0.60f, 0.40f);
        t.isGalaxy = true;
        t.geometry = GeometryStyle::TacticalGlass;
        t.heroSprite = "saturn_folder_clean.png";
        return t;
    }

    static Theme CreatePipBoyGreen() {
        Theme t;
        t.name = L"Pip-Boy Green";
        t.description = L"RobCo Termlink Green CRT (#041005)";
        t.bg = ColorHex(0x041005);
        t.bgSecondary = ColorHex(0x071C09);
        t.blockBg = ColorHex(0x061608);
        t.blockBorder = ColorHex(0x14FE17);
        t.blockHeaderBg = ColorHex(0x0A260E);
        t.textPrimary = ColorHex(0x14FE17);
        t.textDim = ColorHex(0x0E9811);
        t.accent = ColorHex(0x5FFF61);
        t.promptCmd = ColorHex(0x5FFF61);
        t.promptPw = ColorHex(0x14FE17);
        t.syntaxValidCommand = ColorHex(0x5FFF61);
        t.syntaxInvalidCommand = ColorHex(0xFF4976);
        t.syntaxString = ColorHex(0xA3FF9E);
        t.syntaxFlag = ColorHex(0x14FE17);
        t.syntaxNumber = ColorHex(0x85FF7A);
        t.syntaxComment = ColorHex(0x0E9811);
        t.syntaxKeyword = ColorHex(0x5FFF61);
        t.gitClean = ColorHex(0x14FE17);
        t.gitDirty = ColorHex(0xFFD480);
        t.selectionBg = ColorRgba(0.08f, 0.99f, 0.09f, 0.25f);
        t.hudBg = ColorHex(0x071C09);
        t.buttonHoverBg = ColorHex(0x0D3311);
        t.isGalaxy = false;
        t.geometry = GeometryStyle::BrutalistChamfer;
        t.chamferSize = 6.0f;
        t.crtScanlines = true;
        t.phosphorBloom = true;
        t.crtVignette = true;
        t.uppercaseHud = true;
        t.heroSprite = "terminal_window.png";
        return t;
    }

    static Theme CreatePipBoyAmber() {
        Theme t;
        t.name = L"Pip-Boy Amber";
        t.description = L"New Vegas Amber CRT (#120B03)";
        t.bg = ColorHex(0x120B03);
        t.bgSecondary = ColorHex(0x1E1305);
        t.blockBg = ColorHex(0x180F04);
        t.blockBorder = ColorHex(0xFFB642);
        t.blockHeaderBg = ColorHex(0x2B1A08);
        t.textPrimary = ColorHex(0xFFB642);
        t.textDim = ColorHex(0xB87D1E);
        t.accent = ColorHex(0xFFD27D);
        t.promptCmd = ColorHex(0xFFD27D);
        t.promptPw = ColorHex(0xFFB642);
        t.syntaxValidCommand = ColorHex(0xFFD27D);
        t.syntaxInvalidCommand = ColorHex(0xFF4976);
        t.syntaxString = ColorHex(0xFFE4A0);
        t.syntaxFlag = ColorHex(0xFFB642);
        t.syntaxNumber = ColorHex(0xFFC766);
        t.syntaxComment = ColorHex(0xB87D1E);
        t.syntaxKeyword = ColorHex(0xFFD27D);
        t.gitClean = ColorHex(0xFFB642);
        t.gitDirty = ColorHex(0xFFE4A0);
        t.selectionBg = ColorRgba(1.0f, 0.71f, 0.26f, 0.25f);
        t.hudBg = ColorHex(0x1E1305);
        t.buttonHoverBg = ColorHex(0x36210A);
        t.isGalaxy = false;
        t.geometry = GeometryStyle::BrutalistChamfer;
        t.chamferSize = 6.0f;
        t.crtScanlines = true;
        t.phosphorBloom = true;
        t.crtVignette = true;
        t.uppercaseHud = true;
        t.heroSprite = "terminal_window.png";
        return t;
    }

    static Theme CreatePowerShellISE() {
        Theme t;
        t.name = L"PowerShell ISE";
        t.description = L"PowerShell ISE Authentic Cobalt (#011C40)";
        t.bg = ColorHex(0x011C40);
        t.bgSecondary = ColorHex(0x012456);
        t.blockBg = ColorHex(0x022B66);
        t.blockBorder = ColorHex(0x1E56A0);
        t.blockHeaderBg = ColorHex(0x003478);
        t.textPrimary = ColorHex(0xFFFFFF);
        t.textDim = ColorHex(0x93C5FD);
        t.accent = ColorHex(0xFACC15);
        t.promptCmd = ColorHex(0xFACC15);
        t.promptPw = ColorHex(0x38BDF8);
        t.syntaxValidCommand = ColorHex(0xFACC15);
        t.syntaxInvalidCommand = ColorHex(0xEF4444);
        t.syntaxString = ColorHex(0xFDBA74);
        t.syntaxFlag = ColorHex(0x38BDF8);
        t.syntaxNumber = ColorHex(0x93C5FD);
        t.syntaxComment = ColorHex(0x60A5FA);
        t.syntaxKeyword = ColorHex(0xFDE047);
        t.gitClean = ColorHex(0x4ADE80);
        t.gitDirty = ColorHex(0xFACC15);
        t.selectionBg = ColorRgba(0.12f, 0.34f, 0.63f, 0.40f);
        t.hudBg = ColorHex(0x003478);
        t.buttonHoverBg = ColorHex(0x033C8C);
        t.isGalaxy = false;
        t.geometry = GeometryStyle::TacticalGlass;
        t.heroSprite = "terminal_window.png";
        return t;
    }

    static Theme CreateWarmSepia() {
        Theme t;
        t.name = L"Warm Sepia";
        t.description = I18n::Tr(L"Read Mode E-Ink Escuro Sepia (#17130F)", L"Dark Sepia E-Ink Read Mode (#17130F)", L"Modo Lectura E-Ink Sepia Oscuro (#17130F)");
        t.bg = ColorHex(0x17130F);
        t.bgSecondary = ColorHex(0x211B15);
        t.blockBg = ColorHex(0x1C1712);
        t.blockBorder = ColorHex(0x3A3025);
        t.blockHeaderBg = ColorHex(0x261F18);
        t.textPrimary = ColorHex(0xE8DEC8);
        t.textDim = ColorHex(0xA3927E);
        t.accent = ColorHex(0xD49B54);
        t.promptCmd = ColorHex(0xD49B54);
        t.promptPw = ColorHex(0xE8DEC8);
        t.syntaxValidCommand = ColorHex(0xD49B54);
        t.syntaxInvalidCommand = ColorHex(0xC25E5E);
        t.syntaxString = ColorHex(0xE0B47A);
        t.syntaxFlag = ColorHex(0xB89D78);
        t.syntaxNumber = ColorHex(0xD9C6A7);
        t.syntaxComment = ColorHex(0x7D6D5C);
        t.syntaxKeyword = ColorHex(0xD49B54);
        t.gitClean = ColorHex(0x8DAE74);
        t.gitDirty = ColorHex(0xD49B54);
        t.selectionBg = ColorRgba(0.83f, 0.61f, 0.33f, 0.25f);
        t.hudBg = ColorHex(0x211B15);
        t.buttonHoverBg = ColorHex(0x30271E);
        t.isGalaxy = false;
        t.geometry = GeometryStyle::TacticalGlass;
        t.heroSprite = "saturn_folder_clean.png";
        return t;
    }

    static std::vector<Theme> GetAllThemes() {
        return {
            CreatePitchBlack(),
            CreateGalaxy(),
            CreateVSCode(),
            CreateBlue(),
            CreateLight(),
            CreatePipBoyGreen(),
            CreatePipBoyAmber(),
            CreatePowerShellISE(),
            CreateWarmSepia()
        };
    }

    static Theme GetThemeByName(const std::wstring& name) {
        std::wstring lower = ToLower(name);
        if (lower.find(L"galaxy") != std::wstring::npos) return CreateGalaxy();
        if (lower.find(L"blue") != std::wstring::npos) return CreateBlue();
        if (lower.find(L"light") != std::wstring::npos) return CreateLight();
        if (lower.find(L"vscode") != std::wstring::npos || lower.find(L"vs code") != std::wstring::npos) return CreateVSCode();
        if (lower.find(L"pip-boy green") != std::wstring::npos || lower.find(L"pipboy green") != std::wstring::npos || lower.find(L"termlink") != std::wstring::npos) return CreatePipBoyGreen();
        if (lower.find(L"pip-boy amber") != std::wstring::npos || lower.find(L"pipboy amber") != std::wstring::npos || lower.find(L"vegas") != std::wstring::npos) return CreatePipBoyAmber();
        if (lower.find(L"powershell") != std::wstring::npos || lower.find(L"ise") != std::wstring::npos || lower.find(L"cobalt") != std::wstring::npos) return CreatePowerShellISE();
        if (lower.find(L"warm sepia") != std::wstring::npos || lower.find(L"sepia") != std::wstring::npos || lower.find(L"read mode") != std::wstring::npos) return CreateWarmSepia();
        return CreatePitchBlack();
    }
};

} // namespace speta

#pragma once

#include "common.hpp"
#include "i18n.hpp"

namespace speta {

class ErrorDoctor {
public:
    static ErrorDiagnosis DiagnoseOutput(const std::wstring& command, int exitCode, const std::vector<StyledLine>& lines) {
        ErrorDiagnosis diag;
        if (exitCode == 0 && lines.empty()) return diag;

        // Collect all text from lines for multi-line and single-line regex/string matching
        std::vector<std::wstring> rawLines;
        for (const auto& l : lines) {
            rawLines.push_back(l.rawText);
        }

        // 1. Check for Python Tracebacks
        if (CheckPythonTraceback(rawLines, diag)) {
            diag.hasError = true;
            diag.errorType = ErrorType::PythonTraceback;
            return diag;
        }

        // 2. Check for C/C++ Compiler Errors (Clang, GCC, MSVC)
        if (CheckCompilerErrors(rawLines, diag)) {
            diag.hasError = true;
            diag.errorType = ErrorType::CompilerError;
            return diag;
        }

        // 3. Check for Git Errors
        if (CheckGitErrors(rawLines, command, diag)) {
            diag.hasError = true;
            diag.errorType = ErrorType::GitError;
            return diag;
        }

        // 4. Check for Command not found / unrecognized
        if (exitCode != 0) {
            if (CheckCommandNotFound(rawLines, command, diag)) {
                diag.hasError = true;
                diag.errorType = ErrorType::CommandNotFound;
                return diag;
            }
        }

        return diag;
    }

private:
    static bool CheckPythonTraceback(const std::vector<std::wstring>& lines, ErrorDiagnosis& diag) {
        LanguageMode lang = I18n::GetResolvedLanguage();
        bool inTraceback = false;
        std::wstring lastFile;
        int lastLine = 0;
        std::wstring lastException;

        for (const auto& line : lines) {
            if (line.find(L"Traceback (most recent call last):") != std::wstring::npos) {
                inTraceback = true;
                continue;
            }

            if (inTraceback) {
                // Match File "...", line \d+
                size_t filePos = line.find(L"File \"");
                if (filePos != std::wstring::npos) {
                    size_t endQuote = line.find(L"\"", filePos + 6);
                    if (endQuote != std::wstring::npos) {
                        lastFile = line.substr(filePos + 6, endQuote - (filePos + 6));
                        size_t linePos = line.find(L", line ", endQuote);
                        if (linePos != std::wstring::npos) {
                            size_t numStart = linePos + 7;
                            size_t numEnd = numStart;
                            while (numEnd < line.size() && iswdigit(line[numEnd])) {
                                numEnd++;
                            }
                            if (numEnd > numStart) {
                                lastLine = std::stoi(line.substr(numStart, numEnd - numStart));
                            }
                        }
                    }
                }

                // If non-indented line after traceback start, it's the exception
                if (!line.empty() && line[0] != L' ' && line[0] != L'\t' && line.find(L"File \"") == std::wstring::npos) {
                    lastException = line;
                }
            }
        }

        if (inTraceback && !lastException.empty()) {
            diag.errorTitle = (lang == LanguageMode::PtBR) ? L"Python: Exceção Detectada" :
                              ((lang == LanguageMode::EsES) ? L"Python: Excepción Detectada" : L"Python Exception Detected");
            diag.errorMessage = lastException;
            diag.sourceFile = lastFile;
            diag.sourceLine = lastLine;

            // Check ModuleNotFoundError
            size_t modPos = lastException.find(L"ModuleNotFoundError: No module named '");
            if (modPos != std::wstring::npos) {
                size_t start = modPos + 38;
                size_t end = lastException.find(L"'", start);
                if (end != std::wstring::npos) {
                    std::wstring modName = lastException.substr(start, end - start);
                    diag.suggestedFix = L"pip install " + modName;
                }
            } else if (lastException.find(L"SyntaxError") != std::wstring::npos) {
                diag.suggestedFix = (lang == LanguageMode::PtBR) ? (L"Revisar sintaxe no arquivo " + lastFile + L" linha " + std::to_wstring(lastLine)) :
                                    ((lang == LanguageMode::EsES) ? (L"Revisar sintaxis en el archivo " + lastFile + L" línea " + std::to_wstring(lastLine)) :
                                    (L"Review syntax in file " + lastFile + L" line " + std::to_wstring(lastLine)));
            }
            return true;
        }

        return false;
    }

    static bool CheckCompilerErrors(const std::vector<std::wstring>& lines, ErrorDiagnosis& diag) {
        LanguageMode lang = I18n::GetResolvedLanguage();
        for (const auto& line : lines) {
            // Clang / GCC format: file:line:col: error: message
            size_t errPos = line.find(L": error:");
            if (errPos == std::wstring::npos) {
                errPos = line.find(L": fatal error:");
            }
            if (errPos != std::wstring::npos) {
                std::wstring prefix = line.substr(0, errPos);
                // prefix should be file:line:col or file:line
                std::vector<std::wstring> parts = Split(prefix, L':');
                if (parts.size() >= 2) {
                    // Drive letter handling on Windows e.g. C:\path\file.cpp:15:3
                    if (parts.size() >= 3 && parts[0].size() == 1) {
                        diag.sourceFile = parts[0] + L":" + parts[1];
                        try {
                            diag.sourceLine = std::stoi(parts[2]);
                            if (parts.size() >= 4) {
                                diag.sourceCol = std::stoi(parts[3]);
                            }
                        } catch (...) {}
                    } else {
                        diag.sourceFile = parts[0];
                        try {
                            diag.sourceLine = std::stoi(parts[1]);
                            if (parts.size() >= 3) {
                                diag.sourceCol = std::stoi(parts[2]);
                            }
                        } catch (...) {}
                    }
                    diag.errorTitle = (lang == LanguageMode::PtBR) ? L"Erro de Compilação C/C++" :
                                      ((lang == LanguageMode::EsES) ? L"Error de Compilación C/C++" : L"C/C++ Compilation Error");
                    diag.errorMessage = Trim(line.substr(errPos + 1));
                    diag.suggestedFix = (lang == LanguageMode::PtBR) ? (L"Abrir " + diag.sourceFile + L" na linha " + std::to_wstring(diag.sourceLine)) :
                                        ((lang == LanguageMode::EsES) ? (L"Abrir " + diag.sourceFile + L" en la línea " + std::to_wstring(diag.sourceLine)) :
                                        (L"Open " + diag.sourceFile + L" at line " + std::to_wstring(diag.sourceLine)));
                    return true;
                }
            }

            // MSVC format: file(line): error Cxxxx: message
            size_t msvcErr = line.find(L"): error C");
            if (msvcErr != std::wstring::npos) {
                size_t openParen = line.rfind(L'(', msvcErr);
                if (openParen != std::wstring::npos) {
                    diag.sourceFile = line.substr(0, openParen);
                    std::wstring lineNumStr = line.substr(openParen + 1, msvcErr - openParen - 1);
                    try {
                        diag.sourceLine = std::stoi(lineNumStr);
                    } catch (...) {}
                    diag.errorTitle = (lang == LanguageMode::PtBR) ? L"Erro de Compilação MSVC" :
                                      ((lang == LanguageMode::EsES) ? L"Error de Compilación MSVC" : L"MSVC Compilation Error");
                    diag.errorMessage = Trim(line.substr(msvcErr + 2));
                    diag.suggestedFix = (lang == LanguageMode::PtBR) ? (L"Abrir " + diag.sourceFile + L" na linha " + std::to_wstring(diag.sourceLine)) :
                                        ((lang == LanguageMode::EsES) ? (L"Abrir " + diag.sourceFile + L" en la línea " + std::to_wstring(diag.sourceLine)) :
                                        (L"Open " + diag.sourceFile + L" at line " + std::to_wstring(diag.sourceLine)));
                    return true;
                }
            }
        }
        return false;
    }

    static bool CheckGitErrors(const std::vector<std::wstring>& lines, const std::wstring& cmd, ErrorDiagnosis& diag) {
        LanguageMode lang = I18n::GetResolvedLanguage();
        bool hasNoUpstream = false;
        std::wstring pushFix;

        for (const auto& line : lines) {
            if (line.find(L"has no upstream branch") != std::wstring::npos) {
                hasNoUpstream = true;
            }
            size_t gitPushPos = line.find(L"git push --set-upstream");
            if (gitPushPos != std::wstring::npos) {
                pushFix = Trim(line.substr(gitPushPos));
            }

            // Rejected push
            if (line.find(L"failed to push some refs") != std::wstring::npos ||
                line.find(L"Updates were rejected because the remote contains work") != std::wstring::npos) {
                diag.errorTitle = (lang == LanguageMode::PtBR) ? L"Git: Push Rejeitado" :
                                  ((lang == LanguageMode::EsES) ? L"Git: Push Rechazado" : L"Git: Push Rejected");
                diag.errorMessage = (lang == LanguageMode::PtBR) ? L"O repositório remoto contém commits mais novos." :
                                    ((lang == LanguageMode::EsES) ? L"El repositorio remoto contiene commits más recientes." : L"The remote repository contains newer commits.");
                diag.suggestedFix = L"git pull --rebase";
                return true;
            }

            // Not a git repository
            if (line.find(L"not a git repository") != std::wstring::npos) {
                diag.errorTitle = (lang == LanguageMode::PtBR) ? L"Git: Não é Repositório" :
                                  ((lang == LanguageMode::EsES) ? L"Git: No es un Repositorio" : L"Git: Not a Repository");
                diag.errorMessage = (lang == LanguageMode::PtBR) ? L"A pasta atual não é um repositório git." :
                                    ((lang == LanguageMode::EsES) ? L"La carpeta actual no es un repositorio git." : L"The current folder is not a git repository.");
                diag.suggestedFix = L"git init";
                return true;
            }
        }

        if (hasNoUpstream) {
            diag.errorTitle = (lang == LanguageMode::PtBR) ? L"Git: Branch sem Upstream" :
                              ((lang == LanguageMode::EsES) ? L"Git: Rama sin Upstream" : L"Git: Upstream Branch Not Set");
            diag.errorMessage = (lang == LanguageMode::PtBR) ? L"A branch local ainda não está conectada ao repositório remoto." :
                                ((lang == LanguageMode::EsES) ? L"La rama local aún no está conectada al repositorio remoto." : L"The local branch is not tracking a remote branch yet.");
            diag.suggestedFix = !pushFix.empty() ? pushFix : L"git push -u origin HEAD";
            return true;
        }

        return false;
    }

    static bool CheckCommandNotFound(const std::vector<std::wstring>& lines, const std::wstring& cmd, ErrorDiagnosis& diag) {
        LanguageMode lang = I18n::GetResolvedLanguage();
        for (const auto& line : lines) {
            if (line.find(L"is not recognized as an internal or external command") != std::wstring::npos ||
                line.find(L"não é reconhecido como um comando interno") != std::wstring::npos ||
                line.find(L"The term '") != std::wstring::npos) {
                diag.errorTitle = (lang == LanguageMode::PtBR) ? L"Comando Não Encontrado" :
                                  ((lang == LanguageMode::EsES) ? L"Comando No Encontrado" : L"Command Not Found");
                diag.errorMessage = line;
                return true;
            }
        }
        return false;
    }
};

} // namespace speta

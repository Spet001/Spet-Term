#pragma once

#include "common.hpp"

namespace speta {

struct WorkfolderItem {
    std::wstring name;
    std::wstring fullPath;
    bool isDirectory = false;
    bool isScript = false;
    uint64_t sizeBytes = 0;
    std::wstring formattedSize;
    std::wstring extension;
    std::vector<std::wstring> previewLines;
};

class WorkfolderScanner {
public:
    static bool IsScriptExtension(const std::wstring& ext) {
        return (ext == L".ps1" || ext == L".py" || ext == L".bat" || ext == L".cmd" || ext == L".sh");
    }

    static bool IsTextExtension(const std::wstring& ext) {
        static const wchar_t* textExts[] = {
            L".txt", L".md", L".cpp", L".hpp", L".h", L".c", L".cc", L".cxx",
            L".py", L".ps1", L".bat", L".cmd", L".json", L".toml", L".yaml",
            L".yml", L".xml", L".html", L".css", L".js", L".ts", L".sh", L".ini"
        };
        for (const auto* e : textExts) {
            if (_wcsicmp(ext.c_str(), e) == 0) return true;
        }
        return false;
    }

    static std::vector<std::wstring> ReadFilePreview(const std::wstring& path, size_t maxLines = 20) {
        std::vector<std::wstring> lines;
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) return lines;

        std::string line;
        while (lines.size() < maxLines && std::getline(file, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            lines.push_back(Utf8ToWide(line));
        }
        return lines;
    }

    static std::vector<WorkfolderItem> ScanWorkfolder(const std::wstring& cwd) {
        std::vector<WorkfolderItem> dirs;
        std::vector<WorkfolderItem> files;

        if (cwd.empty()) return {};

        // Parent directory [..] entry if not at drive root
        std::wstring cleanCwd = cwd;
        while (cleanCwd.size() > 3 && (cleanCwd.back() == L'\\' || cleanCwd.back() == L'/')) {
            cleanCwd.pop_back();
        }

        if (cleanCwd.size() > 3 || (cleanCwd.find(L':') == std::wstring::npos && !cleanCwd.empty())) {
            size_t slashPos = cleanCwd.find_last_of(L"\\/");
            std::wstring parentPath;
            if (slashPos != std::wstring::npos) {
                parentPath = cleanCwd.substr(0, slashPos);
                if (parentPath.size() == 2 && parentPath[1] == L':') {
                    parentPath += L"\\";
                }
            }
            WorkfolderItem parentItem;
            parentItem.name = L"..";
            parentItem.fullPath = parentPath;
            parentItem.isDirectory = true;
            parentItem.formattedSize = L"<DIR>";
            dirs.push_back(std::move(parentItem));
        } else {
            // At drive root (e.g. C:\) - list all other available system drives
            wchar_t driveBuf[512];
            DWORD driveLen = GetLogicalDriveStringsW(512, driveBuf);
            if (driveLen > 0 && driveLen <= 512) {
                wchar_t* pDrive = driveBuf;
                while (*pDrive) {
                    std::wstring dStr = pDrive;
                    if (_wcsicmp(dStr.c_str(), cleanCwd.c_str()) != 0 && _wcsicmp(dStr.c_str(), (cleanCwd + L"\\").c_str()) != 0) {
                        WorkfolderItem driveItem;
                        driveItem.name = L"[" + dStr.substr(0, 2) + L"] " + dStr;
                        driveItem.fullPath = dStr;
                        driveItem.isDirectory = true;
                        driveItem.formattedSize = L"<DRIVE>";
                        driveItem.previewLines = { L"Unidade de disco / Drive " + dStr, L"Pressione Enter para alternar para esta unidade." };
                        dirs.push_back(std::move(driveItem));
                    }
                    pDrive += wcslen(pDrive) + 1;
                }
            }
        }

        std::wstring searchPattern = cleanCwd;
        if (searchPattern.back() != L'\\' && searchPattern.back() != L'/') {
            searchPattern += L"\\";
        }
        searchPattern += L"*";

        WIN32_FIND_DATAW ffd;
        HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &ffd);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0) {
                    continue;
                }

                WorkfolderItem item;
                item.name = ffd.cFileName;
                item.fullPath = cleanCwd + L"\\" + ffd.cFileName;
                item.isDirectory = (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

                if (item.isDirectory) {
                    item.formattedSize = L"<DIR>";
                    item.extension = L"";
                    dirs.push_back(std::move(item));
                } else {
                    item.sizeBytes = (static_cast<uint64_t>(ffd.nFileSizeHigh) << 32) | ffd.nFileSizeLow;
                    item.formattedSize = FormatBytes(item.sizeBytes);

                    const wchar_t* pExt = PathFindExtensionW(ffd.cFileName);
                    item.extension = pExt ? ToLower(pExt) : L"";
                    item.isScript = IsScriptExtension(item.extension);

                    if (IsTextExtension(item.extension)) {
                        item.previewLines = ReadFilePreview(item.fullPath, 20);
                    }
                    files.push_back(std::move(item));
                }
            } while (FindNextFileW(hFind, &ffd));
            FindClose(hFind);
        }

        // Sort directories alphabetically (case-insensitive), preserving ".." at index 0
        size_t sortStart = (!dirs.empty() && dirs[0].name == L"..") ? 1 : 0;
        if (dirs.size() > sortStart) {
            std::sort(dirs.begin() + sortStart, dirs.end(), [](const WorkfolderItem& a, const WorkfolderItem& b) {
                return _wcsicmp(a.name.c_str(), b.name.c_str()) < 0;
            });
        }

        // Sort files alphabetically (case-insensitive)
        std::sort(files.begin(), files.end(), [](const WorkfolderItem& a, const WorkfolderItem& b) {
            return _wcsicmp(a.name.c_str(), b.name.c_str()) < 0;
        });

        // Combine directories followed by files
        std::vector<WorkfolderItem> results;
        results.reserve(dirs.size() + files.size());
        for (auto& d : dirs) results.push_back(std::move(d));
        for (auto& f : files) results.push_back(std::move(f));
        return results;
    }

    static bool FuzzyMatch(const std::wstring& text, const std::wstring& pattern) {
        if (pattern.empty()) return true;
        std::wstring lowerText = ToLower(text);
        std::wstring lowerPat = ToLower(pattern);

        if (lowerText.find(lowerPat) != std::wstring::npos) return true;

        size_t pIdx = 0;
        for (size_t tIdx = 0; tIdx < lowerText.size() && pIdx < lowerPat.size(); ++tIdx) {
            if (lowerText[tIdx] == lowerPat[pIdx]) {
                pIdx++;
            }
        }
        return (pIdx == lowerPat.size());
    }

    static std::vector<WorkfolderItem> FuzzyFilter(const std::vector<WorkfolderItem>& items, const std::wstring& query) {
        if (query.empty()) return items;
        std::vector<WorkfolderItem> filtered;
        for (const auto& it : items) {
            if (it.name == L".." || FuzzyMatch(it.name, query)) {
                filtered.push_back(it);
            }
        }
        return filtered;
    }
};

inline std::vector<WorkfolderItem> ScanWorkfolder(const std::wstring& cwd) {
    return WorkfolderScanner::ScanWorkfolder(cwd);
}

struct ProjectTask {
    std::wstring title;
    std::wstring command;
    std::wstring description;
    std::string icon = "green_chip.png";
};

inline std::vector<ProjectTask> DetectProjectTasks(const std::wstring& cwd) {
    std::vector<ProjectTask> tasks;
    if (cwd.empty()) return tasks;

    std::wstring root = cwd;
    if (root.back() != L'\\' && root.back() != L'/') root += L'\\';

    // 1. build.ps1
    if (PathFileExistsW((root + L"build.ps1").c_str())) {
        tasks.push_back({
            L"Run build.ps1",
            L"powershell -ExecutionPolicy Bypass -File .\\build.ps1",
            L"Compila projeto com Clang++ / Direct2D",
            "green_chip_24.png"
        });
    }

    // 2. tests/test_suite.exe
    if (PathFileExistsW((root + L"tests\\test_suite.exe").c_str())) {
        tasks.push_back({
            L"Run test_suite.exe",
            L".\\tests\\test_suite.exe",
            L"Executa suite de testes automatizados",
            "green_chip.png"
        });
    }

    // 3. CMakeLists.txt
    if (PathFileExistsW((root + L"CMakeLists.txt").c_str())) {
        tasks.push_back({
            L"CMake Build",
            L"cmake --build build",
            L"Compila via CMake",
            "green_chip.png"
        });
    }

    // 4. Makefile
    if (PathFileExistsW((root + L"Makefile").c_str())) {
        tasks.push_back({
            L"Make",
            L"make",
            L"Compila via Makefile",
            "green_chip.png"
        });
    }

    // 5. package.json
    if (PathFileExistsW((root + L"package.json").c_str())) {
        tasks.push_back({
            L"npm test",
            L"npm test",
            L"Roda testes de Node/JS",
            "gba_cartridge.png"
        });
    }

    return tasks;
}

struct BreadcrumbSegment {
    std::wstring name;
    std::wstring fullPath;
};

inline std::vector<BreadcrumbSegment> SplitPathBreadcrumbs(const std::wstring& path) {
    std::vector<BreadcrumbSegment> segments;
    if (path.empty()) return segments;
    std::wstring norm = path;
    for (auto& c : norm) if (c == L'/') c = L'\\';

    size_t start = 0;
    while (start < norm.size()) {
        size_t nextSlash = norm.find(L'\\', start);
        if (nextSlash == std::wstring::npos) {
            std::wstring part = norm.substr(start);
            if (!part.empty()) {
                segments.push_back({ part, norm });
            }
            break;
        } else {
            if (nextSlash == 2 && norm[1] == L':') {
                std::wstring root = norm.substr(0, 3);
                segments.push_back({ root, root });
                start = 3;
            } else {
                std::wstring part = norm.substr(start, nextSlash - start);
                if (!part.empty()) {
                    segments.push_back({ part, norm.substr(0, nextSlash) });
                }
                start = nextSlash + 1;
            }
        }
    }
    return segments;
}

struct TimelineEntry {
    std::wstring command;
    std::wstring cwd;
    double durationMs = 0.0;
    int exitCode = 0;
    std::wstring timestamp;
};

inline std::vector<TimelineEntry> FilterTimelineHistory(const std::vector<TimelineEntry>& history, bool successOnly) {
    if (!successOnly) return history;
    std::vector<TimelineEntry> res;
    for (const auto& item : history) {
        if (item.exitCode == 0) res.push_back(item);
    }
    return res;
}

} // namespace speta

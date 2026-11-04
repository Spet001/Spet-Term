#pragma once

#include "common.hpp"

namespace speta {

struct GitStatusInfo {
    bool isGitRepo = false;
    std::wstring gitDir;
    std::wstring repoRoot;
    std::wstring branchName;
    std::wstring commitHashShort;
    std::wstring lastCommitMsg;
    std::wstring lastCommitAuthor;
    std::wstring lastCommitRelativeTime;
    bool isDirty = false;
    std::vector<std::wstring> localBranches;

    std::wstring GetPromptBadge() const {
        if (!isGitRepo) return L"";
        std::wstring badge = L"[git: ";
        badge += branchName;
        if (isDirty) {
            badge += L" *";
        }
        if (!lastCommitMsg.empty()) {
            badge += L" | \"";
            if (lastCommitMsg.size() > 28) {
                badge += lastCommitMsg.substr(0, 25) + L"...";
            } else {
                badge += lastCommitMsg;
            }
            badge += L"\"";
            if (!lastCommitRelativeTime.empty()) {
                badge += L" (" + lastCommitRelativeTime + L")";
            }
        }
        badge += L"]";
        return badge;
    }
};

struct GitIgnoreRule {
    std::wstring pattern;
    bool isDirectoryOnly = false;
    bool isNegated = false;
    bool hasSlash = false;
};

class GitIgnoreMatcher {
public:
    std::vector<GitIgnoreRule> rules;

    void Load(const std::wstring& repoRoot, const std::wstring& gitDir) {
        LoadFile(repoRoot + L"\\.gitignore");
        LoadFile(gitDir + L"\\info\\exclude");
    }

    void LoadFile(const std::wstring& path, const std::wstring& prefix = L"") {
        std::ifstream file(path);
        if (!file.is_open()) return;

        std::string line;
        while (std::getline(file, line)) {
            size_t start = line.find_first_not_of(" \t\r\n");
            if (start == std::string::npos) continue;
            size_t end = line.find_last_not_of(" \t\r\n");
            line = line.substr(start, end - start + 1);

            if (line.empty() || line[0] == '#') continue;

            GitIgnoreRule rule;
            if (line[0] == '!') {
                rule.isNegated = true;
                line = line.substr(1);
            }

            if (!line.empty() && line.back() == '/') {
                rule.isDirectoryOnly = true;
                line.pop_back();
            }

            bool leadingSlash = false;
            if (!line.empty() && line.front() == '/') {
                leadingSlash = true;
                line = line.substr(1);
            }

            if (leadingSlash || line.find('/') != std::string::npos) {
                rule.hasSlash = true;
            }

            std::wstring wpat;
            for (char c : line) {
                wpat += (c == '/') ? L'\\' : static_cast<wchar_t>(c);
            }
            if (!prefix.empty()) {
                rule.pattern = prefix + L"\\" + wpat;
                rule.hasSlash = true;
            } else {
                rule.pattern = wpat;
            }
            if (!rule.pattern.empty()) {
                rules.push_back(rule);
            }
        }
    }

    bool IsIgnored(const std::wstring& relPath, const std::wstring& fileName, bool isDir) const {
        static const wchar_t* commonIgnoreDirs[] = {
            L".git", L".vs", L".idea", L".vscode", L"node_modules",
            L".next", L".nuxt", L".turbo", L".cache", L"dist",
            L"build", L"out", L"target", L"bin", L"obj", L"coverage", L"vendor"
        };
        for (const auto* ign : commonIgnoreDirs) {
            if (_wcsicmp(fileName.c_str(), ign) == 0) return true;
        }

        bool ignored = false;
        for (const auto& r : rules) {
            if (r.isDirectoryOnly && !isDir) continue;

            bool match = false;
            if (r.hasSlash) {
                match = (PathMatchSpecW(relPath.c_str(), r.pattern.c_str()) != FALSE);
                if (!match) {
                    std::wstring dirWildcard = r.pattern + L"\\*";
                    match = (PathMatchSpecW(relPath.c_str(), dirWildcard.c_str()) != FALSE);
                }
            } else {
                match = (PathMatchSpecW(fileName.c_str(), r.pattern.c_str()) != FALSE) ||
                        (PathMatchSpecW(relPath.c_str(), r.pattern.c_str()) != FALSE);
            }

            if (match) {
                ignored = !r.isNegated;
            }
        }
        return ignored;
    }
};

class GitEngine {
public:
    static std::wstring FindGitDirectory(const std::wstring& startPath) {
        wchar_t current[MAX_PATH];
        wcscpy_s(current, startPath.c_str());
        // Normalize forward slashes to backslashes
        for (wchar_t* p = current; *p; ++p) {
            if (*p == L'/') *p = L'\\';
        }

        while (true) {
            std::wstring testPath = std::wstring(current) + L"\\.git";
            DWORD attrs = GetFileAttributesW(testPath.c_str());
            if (attrs != INVALID_FILE_ATTRIBUTES) {
                if (attrs & FILE_ATTRIBUTE_DIRECTORY) {
                    return testPath;
                } else {
                    // Could be a worktree/submodule with gitdir: ...
                    std::ifstream f(testPath);
                    if (f.is_open()) {
                        std::string line;
                        if (std::getline(f, line)) {
                            std::string prefix = "gitdir: ";
                            if (line.rfind(prefix, 0) == 0) {
                                std::string target = line.substr(prefix.size());
                                while (!target.empty() && (target.back() == '\r' || target.back() == '\n' || target.back() == ' ')) {
                                    target.pop_back();
                                }
                                std::wstring wTarget = Utf8ToWide(target);
                                if (PathIsRelativeW(wTarget.c_str())) {
                                    wchar_t resolved[MAX_PATH];
                                    PathCombineW(resolved, current, wTarget.c_str());
                                    return resolved;
                                }
                                return wTarget;
                            }
                        }
                    }
                }
            }

            if (!PathRemoveFileSpecW(current)) {
                break;
            }
            if (wcslen(current) <= 3 && current[1] == L':') {
                // Check root drive C:\.git
                std::wstring rootTest = std::wstring(current) + (current[wcslen(current) - 1] == L'\\' ? L".git" : L"\\.git");
                DWORD rootAttrs = GetFileAttributesW(rootTest.c_str());
                if (rootAttrs != INVALID_FILE_ATTRIBUTES && (rootAttrs & FILE_ATTRIBUTE_DIRECTORY)) {
                    return rootTest;
                }
                break;
            }
        }
        return L"";
    }

    static GitStatusInfo InspectRepository(const std::wstring& cwd) {
        GitStatusInfo info;
        std::wstring gitDir = FindGitDirectory(cwd);
        if (gitDir.empty()) {
            return info;
        }

        info.isGitRepo = true;
        info.gitDir = gitDir;

        wchar_t rootBuf[MAX_PATH];
        wcscpy_s(rootBuf, gitDir.c_str());
        PathRemoveFileSpecW(rootBuf);
        info.repoRoot = rootBuf;

        std::wstring headPath = gitDir + L"\\HEAD";
        std::ifstream headFile(headPath);
        std::string headLine;
        if (headFile.is_open() && std::getline(headFile, headLine)) {
            while (!headLine.empty() && (headLine.back() == '\r' || headLine.back() == '\n' || headLine.back() == ' ')) {
                headLine.pop_back();
            }
            std::string refPrefix = "ref: refs/heads/";
            if (headLine.rfind(refPrefix, 0) == 0) {
                info.branchName = Utf8ToWide(headLine.substr(refPrefix.size()));
            } else {
                if (headLine.size() >= 7) {
                    info.branchName = Utf8ToWide(headLine.substr(0, 7));
                    info.commitHashShort = info.branchName;
                } else {
                    info.branchName = Utf8ToWide(headLine);
                }
            }
        } else {
            info.branchName = L"unknown";
        }

        if (!info.branchName.empty() && info.commitHashShort.empty()) {
            std::wstring branchRefPath = gitDir + L"\\refs\\heads\\" + info.branchName;
            std::ifstream branchFile(branchRefPath);
            std::string hashLine;
            if (branchFile.is_open() && std::getline(branchFile, hashLine)) {
                if (hashLine.size() >= 7) {
                    info.commitHashShort = Utf8ToWide(hashLine.substr(0, 7));
                }
            } else {
                std::wstring packedRefsPath = gitDir + L"\\packed-refs";
                std::ifstream packedFile(packedRefsPath);
                if (packedFile.is_open()) {
                    std::string pLine;
                    std::string targetRef = "refs/heads/" + WideToUtf8(info.branchName);
                    while (std::getline(packedFile, pLine)) {
                        if (pLine.empty() || pLine[0] == '#' || pLine[0] == '^') continue;
                        size_t spacePos = pLine.find(' ');
                        if (spacePos != std::string::npos) {
                            std::string ref = pLine.substr(spacePos + 1);
                            while (!ref.empty() && (ref.back() == '\r' || ref.back() == '\n')) ref.pop_back();
                            if (ref == targetRef) {
                                std::string hash = pLine.substr(0, spacePos);
                                if (hash.size() >= 7) {
                                    info.commitHashShort = Utf8ToWide(hash.substr(0, 7));
                                }
                                break;
                            }
                        }
                    }
                }
            }
        }

        std::wstring logsHeadPath = gitDir + L"\\logs\\HEAD";
        std::ifstream logsFile(logsHeadPath);
        if (logsFile.is_open()) {
            std::string line;
            std::string lastLine;
            while (std::getline(logsFile, line)) {
                if (!line.empty() && line != "\r") {
                    lastLine = line;
                }
            }
            if (!lastLine.empty()) {
                ParseLogLine(lastLine, info);
            }
        }

        ScanBranches(gitDir, info.localBranches);
        info.isDirty = CheckIsDirty(info.repoRoot, gitDir);

        return info;
    }

    static std::vector<std::wstring> GetRecentCommitLogs(const std::wstring& cwd, int maxCount = 10) {
        std::vector<std::wstring> logs;
        std::wstring gitDir = FindGitDirectory(cwd);
        if (gitDir.empty()) return logs;

        std::wstring logsHeadPath = gitDir + L"\\logs\\HEAD";
        std::ifstream logsFile(logsHeadPath);
        if (!logsFile.is_open()) return logs;

        std::vector<std::string> lines;
        std::string line;
        while (std::getline(logsFile, line)) {
            if (!line.empty() && line != "\r") {
                lines.push_back(line);
            }
        }

        int count = 0;
        for (int i = static_cast<int>(lines.size()) - 1; i >= 0 && count < maxCount; --i, ++count) {
            GitStatusInfo temp;
            ParseLogLine(lines[i], temp);

            std::wstring entry = L"* ";
            if (!temp.commitHashShort.empty()) {
                entry += L"[" + temp.commitHashShort + L"] ";
            }
            entry += temp.lastCommitMsg;
            if (!temp.lastCommitAuthor.empty()) {
                entry += L" - " + temp.lastCommitAuthor;
            }
            if (!temp.lastCommitRelativeTime.empty()) {
                entry += L" (" + temp.lastCommitRelativeTime + L")";
            }
            logs.push_back(entry);
        }

        return logs;
    }

private:
    static void ParseLogLine(const std::string& line, GitStatusInfo& info) {
        // Format: <from-sha> <to-sha> <author-name> <<email>> <epoch-sec> <tz>\t<action>: <message>
        size_t tabPos = line.find('\t');
        if (tabPos != std::string::npos) {
            std::string header = line.substr(0, tabPos);
            std::string msg = line.substr(tabPos + 1);

            // Strip action like "commit: " or "commit (initial): " or "checkout: "
            size_t colonPos = msg.find(':');
            if (colonPos != std::string::npos && colonPos < 25) {
                msg = msg.substr(colonPos + 1);
            }
            msg = TrimAscii(msg);
            info.lastCommitMsg = Utf8ToWide(msg);

            std::istringstream iss(header);
            std::string fromSha, toSha;
            if (iss >> fromSha >> toSha) {
                if (toSha.size() >= 7) {
                    info.commitHashShort = Utf8ToWide(toSha.substr(0, 7));
                }

                std::string token;
                std::string author;
                while (iss >> token) {
                    if (!token.empty() && token.front() == '<') {
                        break;
                    }
                    if (!author.empty()) author += " ";
                    author += token;
                }
                info.lastCommitAuthor = Utf8ToWide(author);

                int64_t epochSec = 0;
                if (iss >> epochSec) {
                    auto now = std::chrono::system_clock::now();
                    int64_t currentSec = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
                    int64_t diff = currentSec - epochSec;
                    info.lastCommitRelativeTime = FormatRelativeTime(diff);
                }
            }
        }
    }

    static std::string TrimAscii(const std::string& s) {
        size_t start = 0;
        while (start < s.size() && (s[start] == ' ' || s[start] == '\r' || s[start] == '\n' || s[start] == '\t')) start++;
        size_t end = s.size();
        while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\r' || s[end - 1] == '\n' || s[end - 1] == '\t')) end--;
        return s.substr(start, end - start);
    }

    static void ScanBranches(const std::wstring& gitDir, std::vector<std::wstring>& branches) {
        std::wstring headsPath = gitDir + L"\\refs\\heads\\*";
        WIN32_FIND_DATAW fd;
        HANDLE hFind = FindFirstFileW(headsPath.c_str(), &fd);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    branches.push_back(fd.cFileName);
                }
            } while (FindNextFileW(hFind, &fd));
            FindClose(hFind);
        }

        std::wstring packedRefs = gitDir + L"\\packed-refs";
        std::ifstream pf(packedRefs);
        if (pf.is_open()) {
            std::string line;
            while (std::getline(pf, line)) {
                if (line.empty() || line[0] == '#' || line[0] == '^') continue;
                size_t space = line.find(' ');
                if (space != std::string::npos) {
                    std::string ref = line.substr(space + 1);
                    while (!ref.empty() && (ref.back() == '\r' || ref.back() == '\n')) ref.pop_back();
                    std::string prefix = "refs/heads/";
                    if (ref.rfind(prefix, 0) == 0) {
                        std::wstring b = Utf8ToWide(ref.substr(prefix.size()));
                        if (std::find(branches.begin(), branches.end(), b) == branches.end()) {
                            branches.push_back(b);
                        }
                    }
                }
            }
        }
    }

    static bool CheckIsDirty(const std::wstring& repoRoot, const std::wstring& gitDir) {
        GitIgnoreMatcher ignoreMatcher;
        ignoreMatcher.Load(repoRoot, gitDir);

        std::wstring indexPath = gitDir + L"\\index";
        WIN32_FILE_ATTRIBUTE_DATA indexData;
        if (!GetFileAttributesExW(indexPath.c_str(), GetFileExInfoStandard, &indexData)) {
            // No index file exists (untracked files before initial git add)
            return HasAnyWorktreeFiles(repoRoot, ignoreMatcher);
        }

        // Fast zero-heap-allocation timestamp check
        ULARGE_INTEGER indexTime;
        indexTime.LowPart = indexData.ftLastWriteTime.dwLowDateTime;
        indexTime.HighPart = indexData.ftLastWriteTime.dwHighDateTime;
        int visitedCount = 0;
        return ScanDirForNewer(repoRoot, L"", indexTime, ignoreMatcher, 0, visitedCount);
    }

    static bool HasAnyWorktreeFiles(const std::wstring& dir, const GitIgnoreMatcher& ignoreMatcher) {
        std::wstring pattern = dir + L"\\*";
        WIN32_FIND_DATAW fd;
        HANDLE hFind = FindFirstFileExW(pattern.c_str(), FindExInfoBasic, &fd, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);
        if (hFind == INVALID_HANDLE_VALUE) return false;
        bool found = false;
        do {
            if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
            bool isDir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            if (ignoreMatcher.IsIgnored(fd.cFileName, fd.cFileName, isDir)) continue;
            found = true;
            break;
        } while (FindNextFileW(hFind, &fd));
        FindClose(hFind);
        return found;
    }

    static bool ParseIndexFile(const std::wstring& indexPath, std::unordered_map<std::wstring, uint32_t>& indexFiles) {
        HANDLE hFile = CreateFileW(indexPath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE) return false;

        DWORD fsize = GetFileSize(hFile, nullptr);
        if (fsize < 12 || fsize > 20000000) { CloseHandle(hFile); return false; }

        std::vector<uint8_t> data(fsize);
        DWORD bytesRead = 0;
        if (!ReadFile(hFile, data.data(), fsize, &bytesRead, nullptr) || bytesRead != fsize) {
            CloseHandle(hFile);
            return false;
        }
        CloseHandle(hFile);

        if (data[0] != 'D' || data[1] != 'I' || data[2] != 'R' || data[3] != 'C') return false;

        auto ReadBe32 = [](const uint8_t* p) -> uint32_t {
            return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | uint32_t(p[3]);
        };

        uint32_t version = ReadBe32(&data[4]);
        uint32_t count = ReadBe32(&data[8]);
        if (version < 2 || version > 4) return false;

        size_t offset = 12;
        for (uint32_t i = 0; i < count && offset + 62 <= fsize; ++i) {
            uint32_t fileSizeEntry = ReadBe32(&data[offset + 36]);
            offset += 62;
            if (offset >= fsize) break;

            const char* namePtr = reinterpret_cast<const char*>(&data[offset]);
            size_t actualLen = 0;
            while (offset + actualLen < fsize && data[offset + actualLen] != 0) {
                actualLen++;
            }
            std::string sname(namePtr, actualLen);
            offset += actualLen + 1;

            if (version <= 3) {
                while ((offset % 8) != 4 && offset < fsize) {
                    if (data[offset] == 0) offset++;
                    else break;
                }
            }

            std::wstring wname;
            for (char c : sname) {
                wname += (c == '/') ? L'\\' : static_cast<wchar_t>(c);
            }
            indexFiles[ToLower(wname)] = fileSizeEntry;
        }
        return true;
    }

    static bool HasUntrackedFiles(
        const std::wstring& repoRoot,
        const std::wstring& relDir,
        const std::unordered_map<std::wstring, uint32_t>& indexFiles,
        const GitIgnoreMatcher& ignoreMatcher,
        int depth,
        int& visitedCount)
    {
        if (depth > 4) return false;
        if (++visitedCount > 150) return false;
        std::wstring currentDir = relDir.empty() ? repoRoot : (repoRoot + L"\\" + relDir);
        std::wstring pattern = currentDir + L"\\*";
        WIN32_FIND_DATAW fd;
        HANDLE hFind = FindFirstFileExW(pattern.c_str(), FindExInfoBasic, &fd, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);
        if (hFind == INVALID_HANDLE_VALUE) return false;

        bool untracked = false;
        do {
            if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;

            bool isDir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            std::wstring relPath = relDir.empty() ? fd.cFileName : (relDir + L"\\" + fd.cFileName);

            if (ignoreMatcher.IsIgnored(relPath, fd.cFileName, isDir)) {
                continue;
            }

            if (!isDir) {
                if (indexFiles.find(ToLower(relPath)) == indexFiles.end()) {
                    untracked = true;
                    break;
                }
            } else {
                if (HasUntrackedFiles(repoRoot, relPath, indexFiles, ignoreMatcher, depth + 1, visitedCount)) {
                    untracked = true;
                    break;
                }
            }
        } while (FindNextFileW(hFind, &fd));

        FindClose(hFind);
        return untracked;
    }

    static bool ScanDirForNewer(
        const std::wstring& repoRoot,
        const std::wstring& relDir,
        const ULARGE_INTEGER& indexTime,
        const GitIgnoreMatcher& ignoreMatcher,
        int depth,
        int& visitedCount)
    {
        if (depth > 4) return false;
        if (++visitedCount > 150) return false;

        std::wstring currentDir = relDir.empty() ? repoRoot : (repoRoot + L"\\" + relDir);
        std::wstring pattern = currentDir + L"\\*";
        WIN32_FIND_DATAW fd;
        HANDLE hFind = FindFirstFileExW(pattern.c_str(), FindExInfoBasic, &fd, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);
        if (hFind == INVALID_HANDLE_VALUE) return false;

        bool dirty = false;
        do {
            if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;

            bool isDir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            std::wstring relPath = relDir.empty() ? fd.cFileName : (relDir + L"\\" + fd.cFileName);

            if (ignoreMatcher.IsIgnored(relPath, fd.cFileName, isDir)) {
                continue;
            }

            ULARGE_INTEGER fileTime;
            fileTime.LowPart = fd.ftLastWriteTime.dwLowDateTime;
            fileTime.HighPart = fd.ftLastWriteTime.dwHighDateTime;

            if (!isDir) {
                if (fileTime.QuadPart > indexTime.QuadPart) {
                    dirty = true;
                    break;
                }
            } else {
                if (ScanDirForNewer(repoRoot, relPath, indexTime, ignoreMatcher, depth + 1, visitedCount)) {
                    dirty = true;
                    break;
                }
            }
        } while (FindNextFileW(hFind, &fd));

        FindClose(hFind);
        return dirty;
    }
};

}

#pragma once

#include "common.hpp"
#include "i18n.hpp"
#include "compression_engine.hpp"
#include "game_file_engine.hpp"

#include <winioctl.h>
#include <shlobj.h>
#include <knownfolders.h>
#include <condition_variable>
#include <deque>
#include <future>
#include <type_traits>

namespace speta {

// Forward declaration so FileExplorerPaneState works seamlessly whether
// game_file_engine.hpp is included before, after, or not at all.
struct ParsedGameArchive;

struct FallbackVirtualArchiveEntry {
    std::wstring name;
    std::wstring path;
    std::wstring folder;
    uint64_t offset = 0;
    uint64_t compressedSize = 0;
    uint64_t uncompressedSize = 0;
    bool isDirectory = false;
    uint32_t crc32 = 0;
};

// ============================================================================
// Reflections / Adapters for ParsedGameArchive (C++20 concepts & constexpr)
// ============================================================================
namespace detail {

template <typename StringLike>
inline std::wstring ConvertArchiveStringToWide(const StringLike& s) {
    if constexpr (std::is_convertible_v<StringLike, std::wstring_view>) {
        return std::wstring(std::wstring_view(s));
    } else if constexpr (std::is_convertible_v<StringLike, std::string_view>) {
        return Utf8ToWide(std::string_view(s));
    } else {
        return L"";
    }
}

template <typename EntryT>
inline std::wstring ExtractArchiveEntryPath(const EntryT& e) {
    if constexpr (requires { e.virtualPath; }) {
        std::wstring s = ConvertArchiveStringToWide(e.virtualPath);
        if (!s.empty()) return s;
    }
    if constexpr (requires { e.relativePath; }) {
        std::wstring s = ConvertArchiveStringToWide(e.relativePath);
        if (!s.empty()) return s;
    }
    if constexpr (requires { e.fullPath; }) {
        std::wstring s = ConvertArchiveStringToWide(e.fullPath);
        if (!s.empty()) return s;
    }
    if constexpr (requires { e.path; }) {
        std::wstring s = ConvertArchiveStringToWide(e.path);
        if (!s.empty()) return s;
    }
    if constexpr (requires { e.entryPath; }) {
        std::wstring s = ConvertArchiveStringToWide(e.entryPath);
        if (!s.empty()) return s;
    }
    if constexpr (requires { e.name; }) {
        std::wstring s = ConvertArchiveStringToWide(e.name);
        if (!s.empty()) return s;
    }
    if constexpr (requires { e.fileName; }) {
        std::wstring s = ConvertArchiveStringToWide(e.fileName);
        if (!s.empty()) return s;
    }
    return L"";
}

template <typename EntryT>
inline uint64_t ExtractArchiveEntrySize(const EntryT& e) {
    if constexpr (requires { static_cast<uint64_t>(e.uncompressedSize); }) {
        if (e.uncompressedSize > 0) return static_cast<uint64_t>(e.uncompressedSize);
    }
    if constexpr (requires { static_cast<uint64_t>(e.sizeBytes); }) {
        if (e.sizeBytes > 0) return static_cast<uint64_t>(e.sizeBytes);
    }
    if constexpr (requires { static_cast<uint64_t>(e.size); }) {
        if (e.size > 0) return static_cast<uint64_t>(e.size);
    }
    if constexpr (requires { static_cast<uint64_t>(e.fileSize); }) {
        if (e.fileSize > 0) return static_cast<uint64_t>(e.fileSize);
    }
    if constexpr (requires { static_cast<uint64_t>(e.compressedSize); }) {
        return static_cast<uint64_t>(e.compressedSize);
    }
    return 0;
}

template <typename EntryT>
inline bool ExtractArchiveEntryIsDirectory(const EntryT& e, std::wstring_view rawPath) {
    if constexpr (requires { static_cast<bool>(e.isDirectory); }) {
        if (e.isDirectory) return true;
    }
    if constexpr (requires { static_cast<bool>(e.isDir); }) {
        if (e.isDir) return true;
    }
    if constexpr (requires { static_cast<bool>(e.isFolder); }) {
        if (e.isFolder) return true;
    }
    return !rawPath.empty() && (rawPath.back() == L'/' || rawPath.back() == L'\\');
}

template <typename EntryT>
inline void PopulateFallbackArchiveEntry(EntryT& entry, const std::wstring& path, uint64_t offset, uint64_t compSize, uint64_t uncompSize, bool isDir) {
    if constexpr (requires { entry.path = path; }) {
        entry.path = path;
    } else if constexpr (requires { entry.path = WideToUtf8(path); }) {
        entry.path = WideToUtf8(path);
    }
    if constexpr (requires { entry.virtualPath = path; }) {
        entry.virtualPath = path;
    }
    if constexpr (requires { entry.name = path; }) {
        size_t slash = path.find_last_of(L"/\\");
        entry.name = (slash == std::wstring::npos) ? path : path.substr(slash + 1);
    } else if constexpr (requires { entry.name = WideToUtf8(path); }) {
        size_t slash = path.find_last_of(L"/\\");
        std::wstring base = (slash == std::wstring::npos) ? path : path.substr(slash + 1);
        entry.name = WideToUtf8(base);
    }
    if constexpr (requires { entry.offset = offset; }) {
        entry.offset = offset;
    }
    if constexpr (requires { entry.compressedSize = compSize; }) {
        entry.compressedSize = compSize;
    }
    if constexpr (requires { entry.uncompressedSize = uncompSize; }) {
        entry.uncompressedSize = uncompSize;
    }
    if constexpr (requires { entry.size = uncompSize; }) {
        entry.size = uncompSize;
    }
    if constexpr (requires { entry.sizeBytes = uncompSize; }) {
        entry.sizeBytes = uncompSize;
    }
    if constexpr (requires { entry.isDirectory = isDir; }) {
        entry.isDirectory = isDir;
    }
}

// Fast built-in ZIP / PAK / WAD central directory reader when external parser is not invoked or returns empty
template <typename ArchiveT>
inline bool TryQuickParseArchiveFromDisk(const std::wstring& archivePath, ArchiveT& outArchive) {
    HANDLE hFile = CreateFileW(archivePath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                               nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    LARGE_INTEGER liSize{};
    if (!GetFileSizeEx(hFile, &liSize) || liSize.QuadPart < 12) {
        CloseHandle(hFile);
        return false;
    }
    uint64_t fileSize = static_cast<uint64_t>(liSize.QuadPart);

    uint8_t header[16]{};
    DWORD bytesRead = 0;
    ReadFile(hFile, header, (std::min)(static_cast<DWORD>(sizeof(header)), static_cast<DWORD>(fileSize)), &bytesRead, nullptr);

    using EntryT = typename decltype(outArchive.entries)::value_type;

    // 1. Check Quake PAK ("PACK")
    if (bytesRead >= 12 && memcmp(header, "PACK", 4) == 0) {
        uint32_t dirOffset = *reinterpret_cast<const uint32_t*>(header + 4);
        uint32_t dirLength = *reinterpret_cast<const uint32_t*>(header + 8);
        if (static_cast<uint64_t>(dirOffset) + dirLength <= fileSize && (dirLength % 64) == 0) {
            uint32_t count = dirLength / 64;
            LARGE_INTEGER seekPos{};
            seekPos.QuadPart = dirOffset;
            SetFilePointerEx(hFile, seekPos, nullptr, FILE_BEGIN);
            std::vector<uint8_t> table(dirLength);
            if (ReadFile(hFile, table.data(), dirLength, &bytesRead, nullptr) && bytesRead == dirLength) {
                outArchive.entries.reserve(count);
                for (uint32_t i = 0; i < count; ++i) {
                    const uint8_t* rec = table.data() + static_cast<size_t>(i) * 64;
                    char rawName[57]{};
                    memcpy(rawName, rec, 56);
                    uint32_t eOffset = *reinterpret_cast<const uint32_t*>(rec + 56);
                    uint32_t eSize = *reinterpret_cast<const uint32_t*>(rec + 60);
                    EntryT entry{};
                    PopulateFallbackArchiveEntry(entry, Utf8ToWide(rawName), eOffset, eSize, eSize, false);
                    outArchive.entries.push_back(std::move(entry));
                }
                if constexpr (requires { outArchive.isValid = true; }) outArchive.isValid = true;
                CloseHandle(hFile);
                return true;
            }
        }
    }

    // 2. Check DOOM WAD ("IWAD" or "PWAD")
    if (bytesRead >= 12 && (memcmp(header, "IWAD", 4) == 0 || memcmp(header, "PWAD", 4) == 0)) {
        uint32_t numLumps = *reinterpret_cast<const uint32_t*>(header + 4);
        uint32_t infoTableOfs = *reinterpret_cast<const uint32_t*>(header + 8);
        uint64_t tableBytes = static_cast<uint64_t>(numLumps) * 16ULL;
        if (numLumps <= 500000 && static_cast<uint64_t>(infoTableOfs) + tableBytes <= fileSize) {
            LARGE_INTEGER seekPos{};
            seekPos.QuadPart = infoTableOfs;
            SetFilePointerEx(hFile, seekPos, nullptr, FILE_BEGIN);
            std::vector<uint8_t> table(static_cast<size_t>(tableBytes));
            if (ReadFile(hFile, table.data(), static_cast<DWORD>(tableBytes), &bytesRead, nullptr) && bytesRead == tableBytes) {
                outArchive.entries.reserve(numLumps);
                std::wstring currentMapPrefix;
                for (uint32_t i = 0; i < numLumps; ++i) {
                    const uint8_t* rec = table.data() + static_cast<size_t>(i) * 16;
                    uint32_t filePos = *reinterpret_cast<const uint32_t*>(rec + 0);
                    uint32_t size = *reinterpret_cast<const uint32_t*>(rec + 4);
                    char lumpName[9]{};
                    memcpy(lumpName, rec + 8, 8);
                    std::wstring wName = Utf8ToWide(lumpName);
                    if (size == 0 && wName.size() >= 4 &&
                        ((wName[0] == L'E' && iswdigit(wName[1]) && wName[2] == L'M' && iswdigit(wName[3])) ||
                         StartsWith(wName, L"MAP"))) {
                        currentMapPrefix = wName;
                        EntryT dirEntry{};
                        PopulateFallbackArchiveEntry(dirEntry, currentMapPrefix + L"/", filePos, 0, 0, true);
                        outArchive.entries.push_back(std::move(dirEntry));
                        continue;
                    }
                    std::wstring fullVirtPath = currentMapPrefix.empty() ? wName : (currentMapPrefix + L"/" + wName);
                    EntryT entry{};
                    PopulateFallbackArchiveEntry(entry, fullVirtPath, filePos, size, size, false);
                    outArchive.entries.push_back(std::move(entry));
                }
                if constexpr (requires { outArchive.isValid = true; }) outArchive.isValid = true;
                CloseHandle(hFile);
                return true;
            }
        }
    }

    // 3. Check ZIP / PK3 / PK4 / JAR / APK / OBB / IPA / BA2 / VPK End of Central Directory (EOCD)
    uint64_t tailSize = (std::min)(fileSize, static_cast<uint64_t>(65557));
    LARGE_INTEGER seekPos{};
    seekPos.QuadPart = static_cast<LONGLONG>(fileSize - tailSize);
    if (SetFilePointerEx(hFile, seekPos, nullptr, FILE_BEGIN)) {
        std::vector<uint8_t> tail(static_cast<size_t>(tailSize));
        if (ReadFile(hFile, tail.data(), static_cast<DWORD>(tailSize), &bytesRead, nullptr) && bytesRead >= 22) {
            int64_t eocdPos = -1;
            for (int64_t i = static_cast<int64_t>(bytesRead) - 22; i >= 0; --i) {
                if (tail[static_cast<size_t>(i)] == 0x50 &&
                    tail[static_cast<size_t>(i) + 1] == 0x4B &&
                    tail[static_cast<size_t>(i) + 2] == 0x05 &&
                    tail[static_cast<size_t>(i) + 3] == 0x06) {
                    eocdPos = i;
                    break;
                }
            }
            if (eocdPos >= 0) {
                const uint8_t* eocd = tail.data() + eocdPos;
                uint16_t totalEntries = *reinterpret_cast<const uint16_t*>(eocd + 10);
                uint32_t cdSize = *reinterpret_cast<const uint32_t*>(eocd + 12);
                uint32_t cdOffset = *reinterpret_cast<const uint32_t*>(eocd + 16);
                if (static_cast<uint64_t>(cdOffset) + cdSize <= fileSize && cdSize <= 64ULL * 1024 * 1024) {
                    seekPos.QuadPart = cdOffset;
                    SetFilePointerEx(hFile, seekPos, nullptr, FILE_BEGIN);
                    std::vector<uint8_t> cdBuf(cdSize);
                    DWORD cdRead = 0;
                    if (ReadFile(hFile, cdBuf.data(), cdSize, &cdRead, nullptr) && cdRead == cdSize) {
                        outArchive.entries.reserve(totalEntries);
                        size_t pos = 0;
                        while (pos + 46 <= cdSize) {
                            const uint8_t* p = cdBuf.data() + pos;
                            if (p[0] != 0x50 || p[1] != 0x4B || p[2] != 0x01 || p[3] != 0x02) break;
                            uint32_t compSize = *reinterpret_cast<const uint32_t*>(p + 20);
                            uint32_t uncompSize = *reinterpret_cast<const uint32_t*>(p + 24);
                            uint16_t nameLen = *reinterpret_cast<const uint16_t*>(p + 28);
                            uint16_t extraLen = *reinterpret_cast<const uint16_t*>(p + 30);
                            uint16_t commentLen = *reinterpret_cast<const uint16_t*>(p + 32);
                            uint32_t localHeaderOffset = *reinterpret_cast<const uint32_t*>(p + 42);
                            if (pos + 46 + nameLen > cdSize) break;
                            std::string rawName(reinterpret_cast<const char*>(p + 46), nameLen);
                            std::wstring wPath = Utf8ToWide(rawName);
                            bool isDir = !wPath.empty() && (wPath.back() == L'/' || wPath.back() == L'\\');
                            EntryT entry{};
                            PopulateFallbackArchiveEntry(entry, wPath, localHeaderOffset, compSize, uncompSize, isDir);
                            outArchive.entries.push_back(std::move(entry));
                            pos += 46 + nameLen + extraLen + commentLen;
                        }
                        if constexpr (requires { outArchive.isValid = true; }) outArchive.isValid = true;
                        CloseHandle(hFile);
                        return true;
                    }
                }
            }
        }
    }

    CloseHandle(hFile);
    return false;
}

template <typename ArchiveT>
inline std::shared_ptr<ArchiveT> OpenOrParseArchive(const std::wstring& archivePath) {
    if constexpr (std::is_same_v<ArchiveT, ParsedGameArchive>) {
        auto parsed = std::make_shared<ParsedGameArchive>(ParseArchiveFile(archivePath));
        if (parsed->entries.empty()) {
            TryQuickParseArchiveFromDisk(archivePath, *parsed);
        }
        return parsed;
    } else if constexpr (requires { sizeof(ArchiveT); }) {
        auto parsed = std::make_shared<ArchiveT>();
        if constexpr (requires { parsed->filePath = archivePath; }) {
            parsed->filePath = archivePath;
        }
        TryQuickParseArchiveFromDisk(archivePath, *parsed);
        return parsed;
    } else {
        return nullptr;
    }
}

} // namespace detail

// ============================================================================
// Formatting, Natural Sort & Archive Container Detection Helpers
// ============================================================================

inline std::wstring FormatFileTimeShort(const FILETIME& ft) {
    if (ft.dwLowDateTime == 0 && ft.dwHighDateTime == 0) {
        return L"-";
    }
    FILETIME localFt{};
    SYSTEMTIME st{};
    if (!FileTimeToLocalFileTime(&ft, &localFt) || !FileTimeToSystemTime(&localFt, &st)) {
        return L"-";
    }
    wchar_t buf[64]{};
    swprintf_s(buf, L"%04u-%02u-%02u %02u:%02u",
               st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);
    return buf;
}

inline std::wstring ExtractFileExtensionLower(std::wstring_view fileName) {
    size_t dotPos = fileName.find_last_of(L'.');
    if (dotPos == std::wstring_view::npos || dotPos == 0 || dotPos + 1 >= fileName.size()) {
        return L"";
    }
    return ToLower(fileName.substr(dotPos));
}

inline bool IsArchiveOrGameContainerExtension(std::wstring_view extWithOrWithoutDot) {
    if (extWithOrWithoutDot.empty()) return false;
    std::wstring lower = ToLower(extWithOrWithoutDot);
    if (lower.front() != L'.') {
        lower.insert(lower.begin(), L'.');
    }

    static const std::unordered_set<std::wstring_view> kContainerExtensions = {
        // Standard Archives & Disk Images
        L".zip", L".rar", L".7z", L".tar", L".gz", L".tgz", L".bz2", L".xz", L".zst", L".cab", L".iso",
        // Call of Duty / id Tech / Quake / Doom
        L".ff", L".pak", L".iwd", L".xpak", L".ipak", L".wad", L".pk3", L".pk4",
        // Valve Source Engine
        L".vpk", L".gcf", L".bsp",
        // Ren'Py Visual Novel Engine
        L".rpa",
        // Bethesda Creation Engine / Gamebryo
        L".bsa", L".ba2",
        // Rockstar RAGE Engine & RenderWare
        L".rpf", L".img",
        // EA / Blizzard / RTS
        L".big", L".mpq", L".casc",
        // Unreal Engine
        L".upk", L".uasset", L".utx", L".uax", L".umx",
        // Godot & RPG Maker & Wolf RPG
        L".pck", L".rgssad", L".rgss2a", L".rgss3a", L".wolf",
        // Console ROMs & Containers
        L".nds", L".3ds", L".cia", L".rom", L".nes", L".sfc", L".smc", L".gba", L".gbc", L".gb",
        L".n64", L".z64", L".v64", L".chd", L".rvz", L".wbfs", L".gcm", L".cso", L".xex", L".xcp",
        L".sarc", L".szs", L".wad",
        // Unity / Mobile / CRI Middleware / Sony / Other Game Containers
        L".unity3d", L".assets", L".bundle", L".obb", L".apk", L".jar",
        L".cpk", L".afs", L".awb", L".acb", L".psarc", L".hog", L".vp", L".grp",
        L".rez", L".scs", L".erf", L".rim", L".bif", L".xp3", L".arc", L".nsa", L".sar", L".kpf", L".forge", L".ark"
    };

    return kContainerExtensions.find(lower) != kContainerExtensions.end();
}

// Natural alphanumeric comparator (e.g., "file2.txt" < "file10.txt", case-insensitive)
inline int CompareNaturalAlphanumeric(std::wstring_view a, std::wstring_view b) {
    size_t ia = 0;
    size_t ib = 0;
    const size_t na = a.size();
    const size_t nb = b.size();

    while (ia < na && ib < nb) {
        wchar_t ca = a[ia];
        wchar_t cb = b[ib];

        if (iswdigit(ca) && iswdigit(cb)) {
            // Skip leading zeros while tracking count
            size_t za = ia;
            while (ia < na && a[ia] == L'0') ++ia;
            size_t zb = ib;
            while (ib < nb && b[ib] == L'0') ++ib;

            size_t startDigitsA = ia;
            while (ia < na && iswdigit(a[ia])) ++ia;
            size_t startDigitsB = ib;
            while (ib < nb && iswdigit(b[ib])) ++ib;

            size_t lenA = ia - startDigitsA;
            size_t lenB = ib - startDigitsB;
            if (lenA != lenB) {
                return (lenA < lenB) ? -1 : 1;
            }
            for (size_t k = 0; k < lenA; ++k) {
                if (a[startDigitsA + k] != b[startDigitsB + k]) {
                    return (a[startDigitsA + k] < b[startDigitsB + k]) ? -1 : 1;
                }
            }
            // Same numeric value; fewer leading zeros comes first
            size_t zerosA = startDigitsA - za;
            size_t zerosB = startDigitsB - zb;
            if (zerosA != zerosB) {
                return (zerosA < zerosB) ? -1 : 1;
            }
            continue;
        }

        wchar_t la = static_cast<wchar_t>(towlower(ca));
        wchar_t lb = static_cast<wchar_t>(towlower(cb));
        if (la != lb) {
            return (la < lb) ? -1 : 1;
        }
        ++ia;
        ++ib;
    }

    if (na != nb) {
        return (na < nb) ? -1 : 1;
    }
    return 0;
}

// ============================================================================
// Storage Hardware Detection (SSD vs Mechanical HDD Seek Penalty)
// ============================================================================

struct VolumeHardwareProfile {
    wchar_t driveLetter = L'C';
    bool isMechanicalHDD = false;
    bool incursSeekPenalty = false;
    bool isNTFS = false;
    std::wstring fileSystemName;
    std::wstring volumeLabel;
};

inline VolumeHardwareProfile QueryVolumeHardwareProfile(std::wstring_view pathOnVolume) {
    VolumeHardwareProfile profile{};
    if (pathOnVolume.size() >= 2 && iswalpha(pathOnVolume[0]) && pathOnVolume[1] == L':') {
        profile.driveLetter = static_cast<wchar_t>(towupper(pathOnVolume[0]));
    } else {
        wchar_t curDir[MAX_PATH]{};
        if (GetCurrentDirectoryW(MAX_PATH, curDir) >= 2 && iswalpha(curDir[0])) {
            profile.driveLetter = static_cast<wchar_t>(towupper(curDir[0]));
        }
    }

    wchar_t rootPath[8]{};
    swprintf_s(rootPath, L"%c:\\", profile.driveLetter);

    wchar_t volName[MAX_PATH]{};
    wchar_t fsName[MAX_PATH]{};
    DWORD serialNumber = 0;
    DWORD maxCompLen = 0;
    DWORD fsFlags = 0;
    if (GetVolumeInformationW(rootPath, volName, MAX_PATH, &serialNumber, &maxCompLen, &fsFlags, fsName, MAX_PATH)) {
        profile.volumeLabel = volName;
        profile.fileSystemName = fsName;
        profile.isNTFS = (_wcsicmp(fsName, L"NTFS") == 0);
    }

    wchar_t devicePath[16]{};
    swprintf_s(devicePath, L"\\\\.\\%c:", profile.driveLetter);
    // Open volume with 0 access rights so it works without Administrator elevation
    HANDLE hDevice = CreateFileW(devicePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                 nullptr, OPEN_EXISTING, 0, nullptr);
    if (hDevice != INVALID_HANDLE_VALUE) {
        STORAGE_PROPERTY_QUERY query{};
        query.PropertyId = StorageDeviceSeekPenaltyProperty;
        query.QueryType = PropertyStandardQuery;

        DEVICE_SEEK_PENALTY_DESCRIPTOR desc{};
        DWORD bytesReturned = 0;
        if (DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY,
                            &query, sizeof(query),
                            &desc, sizeof(desc),
                            &bytesReturned, nullptr) &&
            bytesReturned >= sizeof(DEVICE_SEEK_PENALTY_DESCRIPTOR)) {
            profile.incursSeekPenalty = (desc.IncursSeekPenalty != FALSE);
            profile.isMechanicalHDD = profile.incursSeekPenalty;
        }
        CloseHandle(hDevice);
    }

    return profile;
}

// ============================================================================
// 1. ExplorerEntry, DriveInfo, QuickAccessItem, BreadcrumbSegment
// ============================================================================

struct ExplorerEntry {
    std::wstring name;
    std::wstring fullPath;
    std::wstring extension;
    uint64_t sizeBytes = 0;
    std::wstring sizeFormatted;
    FILETIME modifiedTime{ 0, 0 };
    std::wstring modifiedFormatted;

    bool isDirectory = false;
    bool isHidden = false;
    bool isSystem = false;
    bool isArchiveOrGameContainer = false;
    bool isVirtualEntry = false;
    bool isParentDirectoryEntry = false;

    size_t virtualArchiveEntryIndex = SIZE_MAX;

    uint64_t calculatedFolderSize = UINT64_MAX;
    float sizeRatioOfParent = 0.0f;

    bool isSelected = false;
    D2D1_RECT_F rowRect{ 0, 0, 0, 0 };
    D2D1_RECT_F hitRect{ 0, 0, 0, 0 };

    uint64_t GetEffectiveSize() const noexcept {
        if (isDirectory) {
            return (calculatedFolderSize != UINT64_MAX) ? calculatedFolderSize : 0ULL;
        }
        return sizeBytes;
    }
};

enum class ExplorerSortColumn {
    Name,
    Size,
    ModifiedDate,
    TypeExtension
};

struct DriveInfo {
    std::wstring rootPath;       // e.g. "C:\"
    std::wstring displayName;    // e.g. "C: (NVMe)"
    std::wstring volumeLabel;
    std::wstring fileSystem;     // e.g. "NTFS"
    uint64_t totalBytes = 0;
    uint64_t freeBytes = 0;
    uint64_t usedBytes = 0;
    float usageRatio = 0.0f;
    bool isSSD = true;
    bool incursSeekPenalty = false;
    UINT driveType = DRIVE_FIXED;
    D2D1_RECT_F pillRect{ 0, 0, 0, 0 };
};

struct QuickAccessItem {
    std::wstring label;
    std::wstring fullPath;
    D2D1_RECT_F pillRect{ 0, 0, 0, 0 };
};

struct ExplorerBreadcrumbSegment {
    std::wstring label;
    std::wstring diskPath;
    std::wstring virtualSubDir;
    bool isArchiveRoot = false;
    bool isVirtualSegment = false;
    D2D1_RECT_F pillRect{ 0, 0, 0, 0 };
};

// ============================================================================
// 4. Optional Async Folder Size Analyzer (FolderSizeWorker)
// ============================================================================

struct FolderSizeResult {
    std::wstring parentDirectory;
    std::wstring folderFullPath;
    std::wstring folderName;
    uint64_t totalBytes = 0;
};

class FolderSizeWorker {
private:
    std::atomic<bool> cancelFlag_{ false };
    std::atomic<bool> isRunning_{ false };
    std::thread workerThread_;
    mutable std::mutex resultsMutex_;
    std::vector<FolderSizeResult> completedResults_;

    static uint64_t ComputeDirectorySizeRecursive(const std::wstring& dirPath, const std::atomic<bool>& cancelFlag, int depth = 0) {
        if (cancelFlag.load(std::memory_order_relaxed) || depth > 64) return 0;

        std::wstring searchPattern = dirPath;
        if (!searchPattern.empty() && searchPattern.back() != L'\\' && searchPattern.back() != L'/') {
            searchPattern.push_back(L'\\');
        }
        std::wstring baseWithSlash = searchPattern;
        searchPattern.push_back(L'*');

        WIN32_FIND_DATAW ffd{};
        HANDLE hFind = FindFirstFileExW(
            searchPattern.c_str(),
            FindExInfoBasic,
            &ffd,
            FindExSearchNameMatch,
            nullptr,
            FIND_FIRST_EX_LARGE_FETCH);

        if (hFind == INVALID_HANDLE_VALUE) return 0;

        uint64_t total = 0;
        std::vector<std::wstring> subDirs;

        do {
            if (cancelFlag.load(std::memory_order_relaxed)) break;

            const wchar_t* fn = ffd.cFileName;
            if (fn[0] == L'.' && (fn[1] == L'\0' || (fn[1] == L'.' && fn[2] == L'\0'))) {
                continue;
            }

            if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
                // Skip reparse points / junctions to prevent infinite loops
                if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0) {
                    subDirs.push_back(baseWithSlash + fn);
                }
            } else {
                uint64_t sz = (static_cast<uint64_t>(ffd.nFileSizeHigh) << 32) | ffd.nFileSizeLow;
                total += sz;
            }
        } while (FindNextFileW(hFind, &ffd));

        FindClose(hFind);

        for (const auto& sub : subDirs) {
            if (cancelFlag.load(std::memory_order_relaxed)) break;
            total += ComputeDirectorySizeRecursive(sub, cancelFlag, depth + 1);
        }
        return total;
    }

public:
    FolderSizeWorker() = default;
    ~FolderSizeWorker() {
        Cancel();
    }

    FolderSizeWorker(const FolderSizeWorker&) = delete;
    FolderSizeWorker& operator=(const FolderSizeWorker&) = delete;

    void Cancel() {
        cancelFlag_.store(true, std::memory_order_release);
        if (workerThread_.joinable()) {
            workerThread_.join();
        }
        isRunning_.store(false, std::memory_order_release);
    }

    bool IsRunning() const noexcept {
        return isRunning_.load(std::memory_order_acquire);
    }

    void StartAnalysis(const std::wstring& parentDir, const std::vector<ExplorerEntry>& entriesSnapshot) {
        Cancel();

        std::vector<std::pair<std::wstring, std::wstring>> foldersToScan;
        for (const auto& e : entriesSnapshot) {
            if (e.isDirectory && !e.isParentDirectoryEntry && !e.isVirtualEntry) {
                foldersToScan.emplace_back(e.name, e.fullPath);
            }
        }
        if (foldersToScan.empty()) return;

        {
            std::lock_guard<std::mutex> lock(resultsMutex_);
            completedResults_.clear();
        }

        cancelFlag_.store(false, std::memory_order_release);
        isRunning_.store(true, std::memory_order_release);

        workerThread_ = std::thread([this, parentDir, folders = std::move(foldersToScan)]() {
            for (const auto& [folderName, folderPath] : folders) {
                if (cancelFlag_.load(std::memory_order_relaxed)) break;

                uint64_t sz = ComputeDirectorySizeRecursive(folderPath, cancelFlag_, 0);
                if (cancelFlag_.load(std::memory_order_relaxed)) break;

                FolderSizeResult res;
                res.parentDirectory = parentDir;
                res.folderName = folderName;
                res.folderFullPath = folderPath;
                res.totalBytes = sz;

                {
                    std::lock_guard<std::mutex> lock(resultsMutex_);
                    completedResults_.push_back(std::move(res));
                }
            }
            isRunning_.store(false, std::memory_order_release);
        });
    }

    // Applies newly computed folder sizes to entries and updates sizeRatioOfParent (0.0f .. 1.0f)
    bool ApplyPendingResults(const std::wstring& currentDirectory, std::vector<ExplorerEntry>& entries) {
        std::vector<FolderSizeResult> pending;
        {
            std::lock_guard<std::mutex> lock(resultsMutex_);
            if (completedResults_.empty()) return false;
            pending.swap(completedResults_);
        }

        bool anyUpdated = false;
        for (const auto& res : pending) {
            if (_wcsicmp(res.parentDirectory.c_str(), currentDirectory.c_str()) != 0) {
                continue;
            }
            for (auto& e : entries) {
                if (e.isDirectory && _wcsicmp(e.fullPath.c_str(), res.folderFullPath.c_str()) == 0) {
                    e.calculatedFolderSize = res.totalBytes;
                    e.sizeFormatted = FormatBytes(res.totalBytes);
                    anyUpdated = true;
                    break;
                }
            }
        }

        if (anyUpdated) {
            RecomputeParentSizeRatios(entries);
        }
        return anyUpdated;
    }

    static void RecomputeParentSizeRatios(std::vector<ExplorerEntry>& entries) {
        uint64_t totalParentBytes = 0;
        uint64_t maxSingleEntryBytes = 0;
        for (const auto& e : entries) {
            if (e.isParentDirectoryEntry) continue;
            uint64_t sz = e.GetEffectiveSize();
            totalParentBytes += sz;
            if (sz > maxSingleEntryBytes) maxSingleEntryBytes = sz;
        }

        for (auto& e : entries) {
            if (e.isParentDirectoryEntry || totalParentBytes == 0) {
                e.sizeRatioOfParent = 0.0f;
            } else {
                uint64_t sz = e.GetEffectiveSize();
                float ratio = static_cast<float>( static_cast<double>(sz) / static_cast<double>(totalParentBytes) );
                e.sizeRatioOfParent = (std::clamp)(ratio, 0.0f, 1.0f);
            }
        }
    }
};

// ============================================================================
// 3. HDD & SSD Optimized Recursive Search Engine (FileSearchEngine)
// ============================================================================

struct FileSearchMatch {
    ExplorerEntry entry;
    int relevanceScore = 0; // Higher is better (exact match > prefix match > substring match)
};

enum class SearchExecutionStrategy {
    Idle,
    NtfsUsnMftFastScan,
    HddSequentialBreadthFirst,
    SsdParallelWorkQueue
};

class FileSearchEngine {
public:
    static constexpr size_t kMaxSearchResults = 5000;

private:
    std::atomic<bool> cancelFlag_{ false };
    std::atomic<bool> isSearching_{ false };
    std::atomic<uint64_t> scannedFilesCount_{ 0 };
    std::atomic<uint64_t> scannedDirsCount_{ 0 };
    std::atomic<SearchExecutionStrategy> activeStrategy_{ SearchExecutionStrategy::Idle };

    std::thread coordinatorThread_;
    mutable std::mutex resultsMutex_;
    std::vector<FileSearchMatch> matches_;

    static int ComputeMatchRelevance(std::wstring_view fileNameLower, std::wstring_view queryLower) {
        if (queryLower.empty()) return 1;
        if (fileNameLower == queryLower) return 1000;
        if (StartsWith(fileNameLower, queryLower)) return 500 - static_cast<int>((std::min)(fileNameLower.size(), size_t(100)));
        size_t pos = fileNameLower.find(queryLower);
        if (pos != std::wstring_view::npos) {
            return 200 - static_cast<int>((std::min)(pos, size_t(100)));
        }
        return 0;
    }

    void AddMatchThreadSafe(ExplorerEntry&& entry, int score) {
        std::lock_guard<std::mutex> lock(resultsMutex_);
        if (matches_.size() < kMaxSearchResults) {
            matches_.push_back({ std::move(entry), score });
        } else {
            // Replace lowest-scoring item if the new match is strictly more relevant
            auto minIt = std::min_element(matches_.begin(), matches_.end(),
                [](const FileSearchMatch& a, const FileSearchMatch& b) {
                    return a.relevanceScore < b.relevanceScore;
                });
            if (minIt != matches_.end() && score > minIt->relevanceScore) {
                *minIt = { std::move(entry), score };
            }
        }
    }

    // Elevated NTFS USN Journal / MFT Fast Scanner (FSCTL_ENUM_USN_DATA on \\.\X:)
    bool TryNtfsUsnMftFastScan(wchar_t driveLetter, const std::wstring& rootScopePrefix, const std::wstring& queryLower) {
        if (!IsRunningAsAdmin()) return false;

        wchar_t volumeDevice[16]{};
        swprintf_s(volumeDevice, L"\\\\.\\%c:", driveLetter);

        HANDLE hVol = CreateFileW(volumeDevice, GENERIC_READ,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                  nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        if (hVol == INVALID_HANDLE_VALUE) return false;

        USN_JOURNAL_DATA_V0 journalData{};
        DWORD bytesRet = 0;
        if (!DeviceIoControl(hVol, FSCTL_QUERY_USN_JOURNAL, nullptr, 0,
                             &journalData, sizeof(journalData), &bytesRet, nullptr)) {
            CloseHandle(hVol);
            return false;
        }

        activeStrategy_.store(SearchExecutionStrategy::NtfsUsnMftFastScan, std::memory_order_release);

        struct FrnDirRecord {
            DWORDLONG parentFrn = 0;
            std::wstring name;
        };
        std::unordered_map<DWORDLONG, FrnDirRecord> dirMap;
        dirMap.reserve(65536);

        struct RawMatchedNode {
            DWORDLONG frn = 0;
            DWORDLONG parentFrn = 0;
            std::wstring name;
            DWORD attributes = 0;
            int relevance = 0;
        };
        std::vector<RawMatchedNode> rawMatches;
        rawMatches.reserve(4096);

        MFT_ENUM_DATA_V0 med{};
        med.StartFileReferenceNumber = 0;
        med.LowUsn = 0;
        med.HighUsn = journalData.NextUsn;

        constexpr DWORD kBufSize = 256 * 1024;
        std::vector<uint8_t> buffer(kBufSize);

        while (!cancelFlag_.load(std::memory_order_relaxed)) {
            bytesRet = 0;
            if (!DeviceIoControl(hVol, FSCTL_ENUM_USN_DATA,
                                 &med, sizeof(med),
                                 buffer.data(), kBufSize,
                                 &bytesRet, nullptr) || bytesRet <= sizeof(USN)) {
                break;
            }

            DWORD offset = sizeof(USN);
            while (offset + sizeof(USN_RECORD_V2) <= bytesRet) {
                if (cancelFlag_.load(std::memory_order_relaxed)) break;

                const auto* rec = reinterpret_cast<const USN_RECORD_V2*>(buffer.data() + offset);
                if (rec->RecordLength == 0) break;

                const wchar_t* fileNamePtr = reinterpret_cast<const wchar_t*>(
                    reinterpret_cast<const uint8_t*>(rec) + rec->FileNameOffset);
                size_t fileNameChars = rec->FileNameLength / sizeof(wchar_t);

                if (fileNameChars > 0) {
                    std::wstring name(fileNamePtr, fileNameChars);
                    bool isDir = (rec->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
                    if (isDir) {
                        dirMap[rec->FileReferenceNumber] = { rec->ParentFileReferenceNumber, name };
                        scannedDirsCount_.fetch_add(1, std::memory_order_relaxed);
                    } else {
                        scannedFilesCount_.fetch_add(1, std::memory_order_relaxed);
                    }

                    std::wstring nameLower = ToLower(name);
                    int score = ComputeMatchRelevance(nameLower, queryLower);
                    if (score > 0 && rawMatches.size() < kMaxSearchResults * 4) {
                        rawMatches.push_back({
                            rec->FileReferenceNumber,
                            rec->ParentFileReferenceNumber,
                            std::move(name),
                            rec->FileAttributes,
                            score
                        });
                    }
                }

                offset += rec->RecordLength;
            }

            med.StartFileReferenceNumber = *reinterpret_cast<const DWORDLONG*>(buffer.data());
        }

        CloseHandle(hVol);
        if (cancelFlag_.load(std::memory_order_relaxed)) return true;

        std::wstring driveRootPrefix;
        driveRootPrefix.push_back(driveLetter);
        driveRootPrefix.append(L":\\");

        std::wstring normalizedScope = ToLower(rootScopePrefix);
        if (!normalizedScope.empty() && normalizedScope.back() != L'\\') {
            normalizedScope.push_back(L'\\');
        }

        // Reconstruct full paths for matched MFT entries
        for (const auto& rm : rawMatches) {
            if (cancelFlag_.load(std::memory_order_relaxed)) break;

            std::vector<std::wstring> chain;
            DWORDLONG curParent = rm.parentFrn;
            int hops = 0;
            while (hops < 64) {
                auto it = dirMap.find(curParent);
                if (it == dirMap.end()) break;
                chain.push_back(it->second.name);
                if (it->second.parentFrn == curParent) break;
                curParent = it->second.parentFrn;
                ++hops;
            }

            std::wstring fullPath = driveRootPrefix;
            for (auto rIt = chain.rbegin(); rIt != chain.rend(); ++rIt) {
                fullPath += *rIt;
                fullPath.push_back(L'\\');
            }
            fullPath += rm.name;

            if (!normalizedScope.empty()) {
                std::wstring fullLower = ToLower(fullPath);
                if (!StartsWith(fullLower, normalizedScope)) {
                    continue;
                }
            }

            ExplorerEntry e{};
            e.name = rm.name;
            e.fullPath = fullPath;
            e.isDirectory = (rm.attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            e.isHidden = (rm.attributes & FILE_ATTRIBUTE_HIDDEN) != 0;
            e.isSystem = (rm.attributes & FILE_ATTRIBUTE_SYSTEM) != 0;
            e.extension = e.isDirectory ? L"" : ExtractFileExtensionLower(e.name);
            e.isArchiveOrGameContainer = !e.isDirectory && IsArchiveOrGameContainerExtension(e.extension);

            WIN32_FILE_ATTRIBUTE_DATA fad{};
            if (GetFileAttributesExW(fullPath.c_str(), GetFileExInfoStandard, &fad)) {
                e.modifiedTime = fad.ftLastWriteTime;
                e.modifiedFormatted = FormatFileTimeShort(e.modifiedTime);
                if (!e.isDirectory) {
                    e.sizeBytes = (static_cast<uint64_t>(fad.nFileSizeHigh) << 32) | fad.nFileSizeLow;
                    e.sizeFormatted = FormatBytes(e.sizeBytes);
                }
            }

            AddMatchThreadSafe(std::move(e), rm.relevance);
        }

        return true;
    }

    // Mechanical HDD Strategy: Single-threaded Breadth-First directory queue using FindExInfoBasic + FIND_FIRST_EX_LARGE_FETCH
    void RunHddSequentialBreadthFirstScan(const std::wstring& rootDir, const std::wstring& queryLower) {
        activeStrategy_.store(SearchExecutionStrategy::HddSequentialBreadthFirst, std::memory_order_release);

        std::deque<std::wstring> dirQueue;
        dirQueue.push_back(rootDir);

        while (!dirQueue.empty() && !cancelFlag_.load(std::memory_order_relaxed)) {
            std::wstring currentDir = std::move(dirQueue.front());
            dirQueue.pop_front();
            scannedDirsCount_.fetch_add(1, std::memory_order_relaxed);

            std::wstring baseWithSlash = currentDir;
            if (!baseWithSlash.empty() && baseWithSlash.back() != L'\\' && baseWithSlash.back() != L'/') {
                baseWithSlash.push_back(L'\\');
            }
            std::wstring searchPattern = baseWithSlash + L"*";

            WIN32_FIND_DATAW ffd{};
            HANDLE hFind = FindFirstFileExW(
                searchPattern.c_str(),
                FindExInfoBasic,
                &ffd,
                FindExSearchNameMatch,
                nullptr,
                FIND_FIRST_EX_LARGE_FETCH);

            if (hFind == INVALID_HANDLE_VALUE) continue;

            do {
                if (cancelFlag_.load(std::memory_order_relaxed)) break;

                const wchar_t* fn = ffd.cFileName;
                if (fn[0] == L'.' && (fn[1] == L'\0' || (fn[1] == L'.' && fn[2] == L'\0'))) {
                    continue;
                }

                std::wstring name(fn);
                std::wstring fullPath = baseWithSlash + name;
                bool isDir = (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

                if (isDir) {
                    if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0) {
                        dirQueue.push_back(fullPath);
                    }
                } else {
                    scannedFilesCount_.fetch_add(1, std::memory_order_relaxed);
                }

                std::wstring nameLower = ToLower(name);
                int score = ComputeMatchRelevance(nameLower, queryLower);
                if (score > 0) {
                    ExplorerEntry e{};
                    e.name = std::move(name);
                    e.fullPath = std::move(fullPath);
                    e.isDirectory = isDir;
                    e.isHidden = (ffd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0;
                    e.isSystem = (ffd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) != 0;
                    e.extension = isDir ? L"" : ExtractFileExtensionLower(e.name);
                    e.isArchiveOrGameContainer = !isDir && IsArchiveOrGameContainerExtension(e.extension);
                    e.modifiedTime = ffd.ftLastWriteTime;
                    e.modifiedFormatted = FormatFileTimeShort(e.modifiedTime);
                    if (!isDir) {
                        e.sizeBytes = (static_cast<uint64_t>(ffd.nFileSizeHigh) << 32) | ffd.nFileSizeLow;
                        e.sizeFormatted = FormatBytes(e.sizeBytes);
                    }
                    AddMatchThreadSafe(std::move(e), score);
                }
            } while (FindNextFileW(hFind, &ffd));

            FindClose(hFind);
        }
    }

    // SSD Strategy: Multi-threaded work queue traversal
    void RunSsdParallelWorkQueueScan(const std::wstring& rootDir, const std::wstring& queryLower) {
        activeStrategy_.store(SearchExecutionStrategy::SsdParallelWorkQueue, std::memory_order_release);

        std::mutex queueMutex;
        std::condition_variable queueCv;
        std::deque<std::wstring> workQueue;
        workQueue.push_back(rootDir);
        size_t activeWorkers = 0;

        unsigned int hwThreads = std::thread::hardware_concurrency();
        size_t numThreads = (std::clamp)(static_cast<size_t>(hwThreads == 0 ? 4 : hwThreads), size_t(2), size_t(8));

        auto workerFn = [&]() {
            while (!cancelFlag_.load(std::memory_order_relaxed)) {
                std::wstring currentDir;
                {
                    std::unique_lock<std::mutex> lock(queueMutex);
                    queueCv.wait(lock, [&]() {
                        return cancelFlag_.load(std::memory_order_relaxed) ||
                               !workQueue.empty() ||
                               activeWorkers == 0;
                    });

                    if (cancelFlag_.load(std::memory_order_relaxed) ||
                        (workQueue.empty() && activeWorkers == 0)) {
                        return;
                    }

                    currentDir = std::move(workQueue.front());
                    workQueue.pop_front();
                    ++activeWorkers;
                }

                scannedDirsCount_.fetch_add(1, std::memory_order_relaxed);

                std::wstring baseWithSlash = currentDir;
                if (!baseWithSlash.empty() && baseWithSlash.back() != L'\\' && baseWithSlash.back() != L'/') {
                    baseWithSlash.push_back(L'\\');
                }
                std::wstring searchPattern = baseWithSlash + L"*";

                std::vector<std::wstring> discoveredSubDirs;
                WIN32_FIND_DATAW ffd{};
                HANDLE hFind = FindFirstFileExW(
                    searchPattern.c_str(),
                    FindExInfoBasic,
                    &ffd,
                    FindExSearchNameMatch,
                    nullptr,
                    FIND_FIRST_EX_LARGE_FETCH);

                if (hFind != INVALID_HANDLE_VALUE) {
                    do {
                        if (cancelFlag_.load(std::memory_order_relaxed)) break;

                        const wchar_t* fn = ffd.cFileName;
                        if (fn[0] == L'.' && (fn[1] == L'\0' || (fn[1] == L'.' && fn[2] == L'\0'))) {
                            continue;
                        }

                        std::wstring name(fn);
                        std::wstring fullPath = baseWithSlash + name;
                        bool isDir = (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

                        if (isDir) {
                            if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0) {
                                discoveredSubDirs.push_back(fullPath);
                            }
                        } else {
                            scannedFilesCount_.fetch_add(1, std::memory_order_relaxed);
                        }

                        std::wstring nameLower = ToLower(name);
                        int score = ComputeMatchRelevance(nameLower, queryLower);
                        if (score > 0) {
                            ExplorerEntry e{};
                            e.name = std::move(name);
                            e.fullPath = std::move(fullPath);
                            e.isDirectory = isDir;
                            e.isHidden = (ffd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0;
                            e.isSystem = (ffd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) != 0;
                            e.extension = isDir ? L"" : ExtractFileExtensionLower(e.name);
                            e.isArchiveOrGameContainer = !isDir && IsArchiveOrGameContainerExtension(e.extension);
                            e.modifiedTime = ffd.ftLastWriteTime;
                            e.modifiedFormatted = FormatFileTimeShort(e.modifiedTime);
                            if (!isDir) {
                                e.sizeBytes = (static_cast<uint64_t>(ffd.nFileSizeHigh) << 32) | ffd.nFileSizeLow;
                                e.sizeFormatted = FormatBytes(e.sizeBytes);
                            }
                            AddMatchThreadSafe(std::move(e), score);
                        }
                    } while (FindNextFileW(hFind, &ffd));

                    FindClose(hFind);
                }

                {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    for (auto& sub : discoveredSubDirs) {
                        workQueue.push_back(std::move(sub));
                    }
                    --activeWorkers;
                }
                queueCv.notify_all();
            }
        };

        std::vector<std::thread> pool;
        pool.reserve(numThreads);
        for (size_t i = 0; i < numThreads; ++i) {
            pool.emplace_back(workerFn);
        }
        for (auto& t : pool) {
            if (t.joinable()) t.join();
        }
    }

public:
    FileSearchEngine() = default;
    ~FileSearchEngine() {
        Cancel();
    }

    FileSearchEngine(const FileSearchEngine&) = delete;
    FileSearchEngine& operator=(const FileSearchEngine&) = delete;

    void Cancel() {
        cancelFlag_.store(true, std::memory_order_release);
        if (coordinatorThread_.joinable()) {
            coordinatorThread_.join();
        }
        isSearching_.store(false, std::memory_order_release);
    }

    bool IsSearching() const noexcept {
        return isSearching_.load(std::memory_order_acquire);
    }

    uint64_t GetScannedFilesCount() const noexcept {
        return scannedFilesCount_.load(std::memory_order_relaxed);
    }

    uint64_t GetScannedDirsCount() const noexcept {
        return scannedDirsCount_.load(std::memory_order_relaxed);
    }

    SearchExecutionStrategy GetActiveStrategy() const noexcept {
        return activeStrategy_.load(std::memory_order_acquire);
    }

    void StartSearch(const std::wstring& rootDir, const std::wstring& rawQuery, bool allowNtfsMftFastScan = true) {
        Cancel();

        std::wstring trimmedQuery = Trim(rawQuery);
        {
            std::lock_guard<std::mutex> lock(resultsMutex_);
            matches_.clear();
        }
        scannedFilesCount_.store(0, std::memory_order_relaxed);
        scannedDirsCount_.store(0, std::memory_order_relaxed);

        if (trimmedQuery.empty() || rootDir.empty()) {
            activeStrategy_.store(SearchExecutionStrategy::Idle, std::memory_order_release);
            return;
        }

        cancelFlag_.store(false, std::memory_order_release);
        isSearching_.store(true, std::memory_order_release);
        std::wstring queryLower = ToLower(trimmedQuery);

        coordinatorThread_ = std::thread([this, rootDir, queryLower, allowNtfsMftFastScan]() {
            VolumeHardwareProfile hw = QueryVolumeHardwareProfile(rootDir);

            bool completedViaMft = false;
            // Use NTFS USN MFT Fast Scanner when elevated on NTFS volume root or deep tree
            if (allowNtfsMftFastScan && hw.isNTFS && IsRunningAsAdmin()) {
                completedViaMft = TryNtfsUsnMftFastScan(hw.driveLetter, rootDir, queryLower);
            }

            if (!completedViaMft && !cancelFlag_.load(std::memory_order_relaxed)) {
                if (hw.incursSeekPenalty) {
                    RunHddSequentialBreadthFirstScan(rootDir, queryLower);
                } else {
                    RunSsdParallelWorkQueueScan(rootDir, queryLower);
                }
            }

            // Final sort by relevance score descending, then natural name ascending
            {
                std::lock_guard<std::mutex> lock(resultsMutex_);
                std::sort(matches_.begin(), matches_.end(), [](const FileSearchMatch& a, const FileSearchMatch& b) {
                    if (a.relevanceScore != b.relevanceScore) {
                        return a.relevanceScore > b.relevanceScore;
                    }
                    if (a.entry.isDirectory != b.entry.isDirectory) {
                        return a.entry.isDirectory > b.entry.isDirectory;
                    }
                    return CompareNaturalAlphanumeric(a.entry.name, b.entry.name) < 0;
                });
            }

            isSearching_.store(false, std::memory_order_release);
        });
    }

    // Progressive UI snapshot of current top matches
    std::vector<ExplorerEntry> GetTopMatchesSnapshot(size_t maxCount = kMaxSearchResults) const {
        std::vector<FileSearchMatch> copy;
        {
            std::lock_guard<std::mutex> lock(resultsMutex_);
            copy = matches_;
        }

        std::sort(copy.begin(), copy.end(), [](const FileSearchMatch& a, const FileSearchMatch& b) {
            if (a.relevanceScore != b.relevanceScore) {
                return a.relevanceScore > b.relevanceScore;
            }
            if (a.entry.isDirectory != b.entry.isDirectory) {
                return a.entry.isDirectory > b.entry.isDirectory;
            }
            return CompareNaturalAlphanumeric(a.entry.name, b.entry.name) < 0;
        });

        size_t limit = (std::min)(maxCount, copy.size());
        std::vector<ExplorerEntry> out;
        out.reserve(limit);
        for (size_t i = 0; i < limit; ++i) {
            out.push_back(std::move(copy[i].entry));
        }
        return out;
    }
};

// ============================================================================
// 1 & 2. FileExplorerPaneState (Fast Enumeration + VFS Navigation + Filtering)
// ============================================================================

struct FallbackVirtualArchiveContainer {
    std::wstring filePath;
    bool isValid = false;
    std::vector<FallbackVirtualArchiveEntry> entries;
};

template <typename ArchiveT = ParsedGameArchive>
struct FileExplorerPaneStateT {
    // Current navigation state
    std::wstring currentDirectory;
    std::vector<ExplorerEntry> entries;
    std::vector<size_t> visibleEntryIndices;

    // Navigation history (Back / Forward)
    std::vector<std::wstring> historyStack;
    int historyIndex = -1;

    // Sorting state
    ExplorerSortColumn sortColumn = ExplorerSortColumn::Name;
    bool sortAscending = true;
    bool showHiddenFiles = false;
    bool showFolderSizes = false; // False by default so HDDs are never stressed unless enabled
    bool linkCwdWithTerminal = true;

    // Inline filter & recursive search
    std::wstring filterQuery;
    bool filterBarFocused = false;
    bool recursiveSearchMode = false;
    std::unique_ptr<FileSearchEngine> searchEngine = std::make_unique<FileSearchEngine>();
    std::unique_ptr<FolderSizeWorker> folderSizeWorker = std::make_unique<FolderSizeWorker>();

    // Drive & Quick Access Bars
    std::vector<DriveInfo> drives;
    std::vector<QuickAccessItem> quickAccess;
    std::vector<ExplorerBreadcrumbSegment> breadcrumbs;

    // Virtual File System (VFS) state for Archives & 27+ Game Engine Containers
    bool inVirtualArchive = false;
    std::wstring archiveHostPath;
    std::wstring virtualSubDir; // Normalized with '/' and no leading/trailing slash, e.g. "MAP01" or "textures/world"
    std::shared_ptr<ArchiveT> activeArchive;
    std::shared_ptr<FallbackVirtualArchiveContainer> fallbackArchive;

    // UI Scroll & Selection state
    int selectedVisibleIndex = 0;
    int hoveredVisibleIndex = -1;
    float scrollOffsetY = 0.0f;
    D2D1_RECT_F paneRect{ 0, 0, 0, 0 };

    FileExplorerPaneStateT() {
        RefreshDrivesAndQuickAccess();
    }

    // Refresh logical drives (including SSD vs HDD seek penalty) and Quick Access folders
    void RefreshDrivesAndQuickAccess() {
        drives.clear();
        wchar_t driveStrings[512]{};
        DWORD len = GetLogicalDriveStringsW(511, driveStrings);
        const wchar_t* p = driveStrings;
        while (len > 0 && *p != L'\0') {
            std::wstring root(p);
            size_t step = root.size() + 1;
            p += step;

            DriveInfo info{};
            info.rootPath = root;
            info.driveType = GetDriveTypeW(root.c_str());

            ULARGE_INTEGER freeBytesAvailable{}, totalNumberOfBytes{}, totalNumberOfFreeBytes{};
            if (GetDiskFreeSpaceExW(root.c_str(), &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes)) {
                info.totalBytes = totalNumberOfBytes.QuadPart;
                info.freeBytes = totalNumberOfFreeBytes.QuadPart;
                info.usedBytes = (info.totalBytes >= info.freeBytes) ? (info.totalBytes - info.freeBytes) : 0;
                if (info.totalBytes > 0) {
                    info.usageRatio = static_cast<float>(static_cast<double>(info.usedBytes) / static_cast<double>(info.totalBytes));
                }
            }

            VolumeHardwareProfile hw = QueryVolumeHardwareProfile(root);
            info.volumeLabel = hw.volumeLabel;
            info.fileSystem = hw.fileSystemName;
            info.incursSeekPenalty = hw.incursSeekPenalty;
            info.isSSD = !hw.incursSeekPenalty && (info.driveType == DRIVE_FIXED);

            std::wstring shortDrive = root.substr(0, 2);
            if (!info.volumeLabel.empty()) {
                info.displayName = shortDrive + L" " + info.volumeLabel + (info.isSSD ? L" [SSD]" : L" [HDD]");
            } else {
                info.displayName = shortDrive + (info.isSSD ? L" [SSD]" : L" [HDD]");
            }

            drives.push_back(std::move(info));
        }

        quickAccess.clear();
        auto addKnownFolder = [&](const KNOWNFOLDERID& rfid, const wchar_t* fallbackEnvSub, const wchar_t* label) {
            PWSTR pszPath = nullptr;
            std::wstring resolved;
            if (SUCCEEDED(SHGetKnownFolderPath(rfid, 0, nullptr, &pszPath)) && pszPath) {
                resolved = pszPath;
                CoTaskMemFree(pszPath);
            } else {
                wchar_t userProfile[MAX_PATH]{};
                if (GetEnvironmentVariableW(L"USERPROFILE", userProfile, MAX_PATH) > 0) {
                    resolved = userProfile;
                    if (fallbackEnvSub && *fallbackEnvSub) {
                        resolved += L"\\";
                        resolved += fallbackEnvSub;
                    }
                }
            }
            if (!resolved.empty()) {
                quickAccess.push_back({ label, std::move(resolved), { 0, 0, 0, 0 } });
            }
        };

        addKnownFolder(FOLDERID_Desktop, L"Desktop", L"Desktop");
        addKnownFolder(FOLDERID_Downloads, L"Downloads", L"Downloads");
        addKnownFolder(FOLDERID_Documents, L"Documents", L"Documents");
        addKnownFolder(FOLDERID_Profile, L"", L"User Profile");
    }

    static std::wstring NormalizeDiskDirectoryPath(std::wstring_view rawPath) {
        std::wstring p = TrimQuotes(rawPath);
        for (auto& c : p) {
            if (c == L'/') c = L'\\';
        }
        while (p.size() > 3 && p.back() == L'\\') {
            p.pop_back();
        }
        if (p.size() == 2 && iswalpha(p[0]) && p[1] == L':') {
            p.push_back(L'\\');
        }
        return p;
    }

    static std::wstring NormalizeVirtualSubDir(std::wstring_view subDir) {
        std::wstring s(subDir);
        for (auto& c : s) {
            if (c == L'\\') c = L'/';
        }
        while (!s.empty() && s.front() == L'/') s.erase(s.begin());
        while (!s.empty() && s.back() == L'/') s.pop_back();
        return s;
    }

    // Rebuilds breadcrumb segments for both real disk paths and virtual archive paths
    // Example: C:\Games\DOOM\DOOM.WAD > MAP01
    void RebuildBreadcrumbs() {
        breadcrumbs.clear();
        std::wstring baseDiskPath = inVirtualArchive ? archiveHostPath : currentDirectory;
        if (baseDiskPath.empty()) return;

        std::vector<std::wstring> parts = Split(baseDiskPath, L'\\');
        std::wstring accumulated;
        for (size_t i = 0; i < parts.size(); ++i) {
            if (parts[i].empty()) continue;
            if (accumulated.empty()) {
                accumulated = parts[i];
                if (accumulated.size() == 2 && accumulated[1] == L':') {
                    accumulated.push_back(L'\\');
                }
            } else {
                if (accumulated.back() != L'\\') accumulated.push_back(L'\\');
                accumulated += parts[i];
            }

            ExplorerBreadcrumbSegment seg{};
            seg.label = parts[i];
            seg.diskPath = accumulated;
            seg.isArchiveRoot = (inVirtualArchive && i + 1 == parts.size());
            seg.isVirtualSegment = false;
            breadcrumbs.push_back(std::move(seg));
        }

        if (inVirtualArchive && !virtualSubDir.empty()) {
            std::vector<std::wstring> vParts = Split(virtualSubDir, L'/');
            std::wstring vAccum;
            for (const auto& vp : vParts) {
                if (vp.empty()) continue;
                if (!vAccum.empty()) vAccum.push_back(L'/');
                vAccum += vp;

                ExplorerBreadcrumbSegment vSeg{};
                vSeg.label = vp;
                vSeg.diskPath = archiveHostPath;
                vSeg.virtualSubDir = vAccum;
                vSeg.isArchiveRoot = false;
                vSeg.isVirtualSegment = true;
                breadcrumbs.push_back(std::move(vSeg));
            }
        }
    }

    // Ultra-fast NTFS/FAT32 directory enumeration via FindFirstFileExW(FindExInfoBasic, FIND_FIRST_EX_LARGE_FETCH)
    bool NavigateToDirectory(std::wstring_view targetDir, bool recordHistory = true) {
        std::wstring normDir = NormalizeDiskDirectoryPath(targetDir);
        if (normDir.empty()) return false;

        DWORD attrs = GetFileAttributesW(normDir.c_str());
        if (attrs == INVALID_FILE_ATTRIBUTES) return false;

        // If targetDir is actually an archive/container file on disk, enter VFS mode directly!
        if ((attrs & FILE_ATTRIBUTE_DIRECTORY) == 0) {
            std::wstring ext = ExtractFileExtensionLower(normDir);
            if (IsArchiveOrGameContainerExtension(ext)) {
                return EnterVirtualArchive(normDir, L"");
            }
            return false;
        }

        if (folderSizeWorker) folderSizeWorker->Cancel();
        if (searchEngine && recursiveSearchMode) searchEngine->Cancel();

        inVirtualArchive = false;
        archiveHostPath.clear();
        virtualSubDir.clear();
        activeArchive.reset();
        currentDirectory = normDir;

        if (recordHistory) {
            if (historyIndex >= 0 && historyIndex + 1 < static_cast<int>(historyStack.size())) {
                historyStack.erase(historyStack.begin() + historyIndex + 1, historyStack.end());
            }
            if (historyStack.empty() || _wcsicmp(historyStack.back().c_str(), currentDirectory.c_str()) != 0) {
                historyStack.push_back(currentDirectory);
                historyIndex = static_cast<int>(historyStack.size()) - 1;
            }
        }

        entries.clear();
        entries.reserve(256);

        // Add ".." parent entry if not at drive root
        if (currentDirectory.size() > 3) {
            size_t lastSlash = currentDirectory.find_last_of(L'\\');
            std::wstring parentPath = (lastSlash != std::wstring::npos && lastSlash > 2)
                ? currentDirectory.substr(0, lastSlash)
                : currentDirectory.substr(0, 3);

            ExplorerEntry parentEntry{};
            parentEntry.name = L"..";
            parentEntry.fullPath = parentPath;
            parentEntry.isDirectory = true;
            parentEntry.isParentDirectoryEntry = true;
            parentEntry.sizeFormatted = L"<DIR>";
            entries.push_back(std::move(parentEntry));
        }

        std::wstring searchPattern = currentDirectory;
        if (searchPattern.back() != L'\\') searchPattern.push_back(L'\\');
        std::wstring baseWithSlash = searchPattern;
        searchPattern.push_back(L'*');

        WIN32_FIND_DATAW ffd{};
        HANDLE hFind = FindFirstFileExW(
            searchPattern.c_str(),
            FindExInfoBasic,
            &ffd,
            FindExSearchNameMatch,
            nullptr,
            FIND_FIRST_EX_LARGE_FETCH);

        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                const wchar_t* fn = ffd.cFileName;
                if (fn[0] == L'.' && (fn[1] == L'\0' || (fn[1] == L'.' && fn[2] == L'\0'))) {
                    continue;
                }

                bool isHidden = (ffd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0;
                bool isSystem = (ffd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) != 0;
                if (!showHiddenFiles && isHidden) {
                    continue;
                }

                ExplorerEntry e{};
                e.name = fn;
                e.fullPath = baseWithSlash + e.name;
                e.isDirectory = (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
                e.isHidden = isHidden;
                e.isSystem = isSystem;
                e.modifiedTime = ffd.ftLastWriteTime;
                e.modifiedFormatted = FormatFileTimeShort(e.modifiedTime);

                if (e.isDirectory) {
                    e.sizeBytes = 0;
                    e.sizeFormatted = L"<DIR>";
                } else {
                    e.extension = ExtractFileExtensionLower(e.name);
                    e.isArchiveOrGameContainer = IsArchiveOrGameContainerExtension(e.extension);
                    e.sizeBytes = (static_cast<uint64_t>(ffd.nFileSizeHigh) << 32) | ffd.nFileSizeLow;
                    e.sizeFormatted = FormatBytes(e.sizeBytes);
                }

                entries.push_back(std::move(e));
            } while (FindNextFileW(hFind, &ffd));

            FindClose(hFind);
        }

        FolderSizeWorker::RecomputeParentSizeRatios(entries);
        SortAndFilterEntries();
        RebuildBreadcrumbs();

        selectedVisibleIndex = 0;
        scrollOffsetY = 0.0f;

        if (showFolderSizes && folderSizeWorker) {
            folderSizeWorker->StartAnalysis(currentDirectory, entries);
        }
        return true;
    }

    // 2. Seamless VFS Navigation for Archives & 27+ Game Engine Containers (< 1 ms synthesis)
    bool EnterVirtualArchive(const std::wstring& archiveFileOnDisk,
                             std::wstring_view targetVirtualSubDir = L"",
                             std::shared_ptr<ArchiveT> preParsedArchive = nullptr) {
        if (folderSizeWorker) folderSizeWorker->Cancel();

        std::wstring normHost = NormalizeDiskDirectoryPath(archiveFileOnDisk);
        if constexpr (requires { sizeof(ArchiveT); }) {
            if (!preParsedArchive) {
                if (inVirtualArchive && activeArchive && _wcsicmp(archiveHostPath.c_str(), normHost.c_str()) == 0) {
                    preParsedArchive = activeArchive;
                } else {
                    preParsedArchive = detail::OpenOrParseArchive<ArchiveT>(normHost);
                }
            }
            if (!preParsedArchive) return false;
            activeArchive = std::move(preParsedArchive);
        } else {
            if (!inVirtualArchive || !fallbackArchive || _wcsicmp(archiveHostPath.c_str(), normHost.c_str()) != 0) {
                fallbackArchive = detail::OpenOrParseArchive<FallbackVirtualArchiveContainer>(normHost);
            }
            if (!fallbackArchive) return false;
        }

        inVirtualArchive = true;
        archiveHostPath = normHost;
        virtualSubDir = NormalizeVirtualSubDir(targetVirtualSubDir);

        size_t lastSlash = archiveHostPath.find_last_of(L'\\');
        currentDirectory = (lastSlash != std::wstring::npos)
            ? archiveHostPath.substr(0, lastSlash)
            : archiveHostPath;

        SynthesizeVirtualDirectoryEntries();
        SortAndFilterEntries();
        RebuildBreadcrumbs();

        selectedVisibleIndex = 0;
        scrollOffsetY = 0.0f;
        return true;
    }

    template <typename ContainerT>
    void SynthesizeFromContainer(const ContainerT& container) {
        // Always provide ".." at index 0 to navigate up or exit VFS back to host filesystem
        ExplorerEntry parentEntry{};
        parentEntry.name = L"..";
        parentEntry.isDirectory = true;
        parentEntry.isVirtualEntry = true;
        parentEntry.isParentDirectoryEntry = true;
        parentEntry.sizeFormatted = L"<VFS-UP>";
        if (virtualSubDir.empty()) {
            parentEntry.fullPath = currentDirectory;
        } else {
            size_t slashPos = virtualSubDir.find_last_of(L'/');
            parentEntry.fullPath = (slashPos == std::wstring::npos) ? L"" : virtualSubDir.substr(0, slashPos);
        }
        entries.push_back(std::move(parentEntry));

        std::wstring prefix = virtualSubDir;
        if (!prefix.empty()) prefix.push_back(L'/');

        struct SynthDirStats {
            uint64_t aggregatedBytes = 0;
            size_t firstEntryIdx = SIZE_MAX;
        };
        std::unordered_map<std::wstring, SynthDirStats> seenSubDirs;
        seenSubDirs.reserve(64);

        const size_t totalArchiveEntries = container.entries.size();
        for (size_t idx = 0; idx < totalArchiveEntries; ++idx) {
            const auto& rawEntry = container.entries[idx];
            std::wstring rawPath = detail::ExtractArchiveEntryPath(rawEntry);
            if (rawPath.empty()) continue;

            for (auto& c : rawPath) {
                if (c == L'\\') c = L'/';
            }
            while (!rawPath.empty() && rawPath.front() == L'/') {
                rawPath.erase(rawPath.begin());
            }

            if (!prefix.empty()) {
                if (rawPath.size() <= prefix.size() ||
                    _wcsnicmp(rawPath.data(), prefix.data(), prefix.size()) != 0) {
                    continue;
                }
            }

            std::wstring remainder = rawPath.substr(prefix.size());
            while (!remainder.empty() && remainder.back() == L'/') {
                remainder.pop_back();
            }
            if (remainder.empty()) continue;

            uint64_t entryBytes = detail::ExtractArchiveEntrySize(rawEntry);
            size_t nextSlash = remainder.find(L'/');

            if (nextSlash != std::wstring::npos) {
                // Direct child virtual folder
                std::wstring childDirName = remainder.substr(0, nextSlash);
                std::wstring childKey = ToLower(childDirName);
                auto& dirStat = seenSubDirs[childKey];
                dirStat.aggregatedBytes += entryBytes;
                if (dirStat.firstEntryIdx == SIZE_MAX) {
                    dirStat.firstEntryIdx = idx;

                    ExplorerEntry dirE{};
                    dirE.name = childDirName;
                    dirE.fullPath = prefix + childDirName;
                    dirE.isDirectory = true;
                    dirE.isVirtualEntry = true;
                    dirE.virtualArchiveEntryIndex = idx;
                    dirE.sizeFormatted = L"<VFS-DIR>";
                    entries.push_back(std::move(dirE));
                }
            } else {
                bool isDirExplicit = detail::ExtractArchiveEntryIsDirectory(rawEntry, detail::ExtractArchiveEntryPath(rawEntry));
                if (isDirExplicit) {
                    std::wstring childKey = ToLower(remainder);
                    auto& dirStat = seenSubDirs[childKey];
                    if (dirStat.firstEntryIdx == SIZE_MAX) {
                        dirStat.firstEntryIdx = idx;
                        ExplorerEntry dirE{};
                        dirE.name = remainder;
                        dirE.fullPath = prefix + remainder;
                        dirE.isDirectory = true;
                        dirE.isVirtualEntry = true;
                        dirE.virtualArchiveEntryIndex = idx;
                        dirE.sizeFormatted = L"<VFS-DIR>";
                        entries.push_back(std::move(dirE));
                    }
                } else {
                    ExplorerEntry fileE{};
                    fileE.name = remainder;
                    fileE.fullPath = archiveHostPath + L"::" + prefix + remainder;
                    fileE.extension = ExtractFileExtensionLower(fileE.name);
                    fileE.sizeBytes = entryBytes;
                    fileE.sizeFormatted = FormatBytes(entryBytes);
                    fileE.modifiedFormatted = L"VFS";
                    fileE.isDirectory = false;
                    fileE.isVirtualEntry = true;
                    fileE.isArchiveOrGameContainer = IsArchiveOrGameContainerExtension(fileE.extension);
                    fileE.virtualArchiveEntryIndex = idx;
                    entries.push_back(std::move(fileE));
                }
            }
        }

        // Populate aggregated subfolder sizes inside VFS
        for (auto& e : entries) {
            if (e.isDirectory && !e.isParentDirectoryEntry) {
                auto it = seenSubDirs.find(ToLower(e.name));
                if (it != seenSubDirs.end() && it->second.aggregatedBytes > 0) {
                    e.calculatedFolderSize = it->second.aggregatedBytes;
                    e.sizeFormatted = FormatBytes(it->second.aggregatedBytes);
                }
            }
        }

        FolderSizeWorker::RecomputeParentSizeRatios(entries);
    }

    void SynthesizeVirtualDirectoryEntries() {
        entries.clear();
        if constexpr (requires { sizeof(ArchiveT); }) {
            if (!activeArchive) return;
            SynthesizeFromContainer(*activeArchive);
        } else {
            if (!fallbackArchive) return;
            SynthesizeFromContainer(*fallbackArchive);
        }
    }

    // Navigate up (".."): if at root of virtual archive, exits VFS mode back to host filesystem directory
    bool NavigateUp() {
        if (inVirtualArchive) {
            if (virtualSubDir.empty()) {
                std::wstring hostDir = currentDirectory;
                return NavigateToDirectory(hostDir, true);
            } else {
                size_t slashPos = virtualSubDir.find_last_of(L'/');
                std::wstring parentVirt = (slashPos == std::wstring::npos) ? L"" : virtualSubDir.substr(0, slashPos);
                return EnterVirtualArchive(archiveHostPath, parentVirt, activeArchive);
            }
        }

        if (currentDirectory.size() <= 3) return false;
        size_t lastSlash = currentDirectory.find_last_of(L'\\');
        std::wstring parentDir = (lastSlash != std::wstring::npos && lastSlash > 2)
            ? currentDirectory.substr(0, lastSlash)
            : currentDirectory.substr(0, 3);
        return NavigateToDirectory(parentDir, true);
    }

    // Activate / Open an entry (Enter or Double-Click)
    bool ActivateEntry(size_t entryIndex) {
        if (entryIndex >= entries.size()) return false;
        const ExplorerEntry target = entries[entryIndex];

        if (target.isParentDirectoryEntry) {
            return NavigateUp();
        }

        if (inVirtualArchive) {
            if (target.isDirectory) {
                std::wstring nextVirt = virtualSubDir.empty()
                    ? target.name
                    : (virtualSubDir + L"/" + target.name);
                return EnterVirtualArchive(archiveHostPath, nextVirt, activeArchive);
            }
            return false;
        }

        if (target.isDirectory) {
            return NavigateToDirectory(target.fullPath, true);
        }

        if (target.isArchiveOrGameContainer) {
            return EnterVirtualArchive(target.fullPath, L"");
        }

        return false;
    }

    // Sort by Name (natural alphanumeric), Size, Modified Date, or Type/Extension (directories always first)
    void SortAndFilterEntries() {
        if (entries.size() > 1) {
            auto sortBegin = entries.begin();
            if (entries.front().isParentDirectoryEntry) {
                ++sortBegin;
            }

            std::sort(sortBegin, entries.end(), [this](const ExplorerEntry& a, const ExplorerEntry& b) {
                // Directories always first regardless of ascending/descending
                if (a.isDirectory != b.isDirectory) {
                    return a.isDirectory > b.isDirectory;
                }

                int cmp = 0;
                switch (sortColumn) {
                case ExplorerSortColumn::Name:
                    cmp = CompareNaturalAlphanumeric(a.name, b.name);
                    break;

                case ExplorerSortColumn::Size: {
                    uint64_t sa = a.GetEffectiveSize();
                    uint64_t sb = b.GetEffectiveSize();
                    if (sa != sb) {
                        cmp = (sa < sb) ? -1 : 1;
                    } else {
                        cmp = CompareNaturalAlphanumeric(a.name, b.name);
                    }
                    break;
                }

                case ExplorerSortColumn::ModifiedDate: {
                    LONG ftCmp = CompareFileTime(&a.modifiedTime, &b.modifiedTime);
                    if (ftCmp != 0) {
                        cmp = (ftCmp < 0) ? -1 : 1;
                    } else {
                        cmp = CompareNaturalAlphanumeric(a.name, b.name);
                    }
                    break;
                }

                case ExplorerSortColumn::TypeExtension: {
                    cmp = CompareNaturalAlphanumeric(a.extension, b.extension);
                    if (cmp == 0) {
                        cmp = CompareNaturalAlphanumeric(a.name, b.name);
                    }
                    break;
                }
                }

                return sortAscending ? (cmp < 0) : (cmp > 0);
            });
        }

        RebuildVisibleFilterIndices();
    }

    void RebuildVisibleFilterIndices() {
        visibleEntryIndices.clear();
        visibleEntryIndices.reserve(entries.size());

        std::wstring q = ToLower(Trim(filterQuery));
        for (size_t i = 0; i < entries.size(); ++i) {
            const auto& e = entries[i];
            if (e.isParentDirectoryEntry) {
                visibleEntryIndices.push_back(i);
                continue;
            }
            if (q.empty() || ToLower(e.name).find(q) != std::wstring::npos) {
                visibleEntryIndices.push_back(i);
            }
        }

        if (selectedVisibleIndex >= static_cast<int>(visibleEntryIndices.size())) {
            selectedVisibleIndex = visibleEntryIndices.empty()
                ? -1
                : static_cast<int>(visibleEntryIndices.size()) - 1;
        }
        if (selectedVisibleIndex < 0 && !visibleEntryIndices.empty()) {
            selectedVisibleIndex = 0;
        }
    }

    void SetSortColumn(ExplorerSortColumn col) {
        if (sortColumn == col) {
            sortAscending = !sortAscending;
        } else {
            sortColumn = col;
            sortAscending = true;
        }
        SortAndFilterEntries();
    }

    void SetFilterQuery(std::wstring_view query) {
        filterQuery = std::wstring(query);
        RebuildVisibleFilterIndices();
        if (recursiveSearchMode && searchEngine && !inVirtualArchive) {
            searchEngine->StartSearch(currentDirectory, filterQuery);
        }
    }

    // Poll background workers (folder size analyzer & recursive search) from UI timer
    bool PollAsyncUpdates() {
        bool changed = false;
        if (showFolderSizes && folderSizeWorker && !inVirtualArchive) {
            if (folderSizeWorker->ApplyPendingResults(currentDirectory, entries)) {
                if (sortColumn == ExplorerSortColumn::Size) {
                    SortAndFilterEntries();
                }
                changed = true;
            }
        }
        return changed;
    }

    std::vector<ExplorerEntry> GetSelectedOrFocusedEntries() const {
        std::vector<ExplorerEntry> result;
        for (const auto& e : entries) {
            if (e.isSelected && !e.isParentDirectoryEntry) {
                result.push_back(e);
            }
        }
        if (result.empty() &&
            selectedVisibleIndex >= 0 &&
            selectedVisibleIndex < static_cast<int>(visibleEntryIndices.size())) {
            size_t idx = visibleEntryIndices[static_cast<size_t>(selectedVisibleIndex)];
            if (idx < entries.size() && !entries[idx].isParentDirectoryEntry) {
                result.push_back(entries[idx]);
            }
        }
        return result;
    }

    const ExplorerEntry* GetSelectedEntry() const {
        if (selectedVisibleIndex >= 0 && selectedVisibleIndex < static_cast<int>(visibleEntryIndices.size())) {
            size_t idx = visibleEntryIndices[static_cast<size_t>(selectedVisibleIndex)];
            if (idx < entries.size()) return &entries[idx];
        }
        return nullptr;
    }

    std::vector<uint8_t> ExtractEntryToMemory(const ExplorerEntry& entry) const {
        std::vector<uint8_t> data;
        if (inVirtualArchive && activeArchive && entry.virtualArchiveEntryIndex < activeArchive->entries.size()) {
            ExtractArchiveEntryBytes(*activeArchive, activeArchive->entries[entry.virtualArchiveEntryIndex], data);
        }
        return data;
    }

    void Refresh() {
        if (inVirtualArchive) {
            SynthesizeVirtualDirectoryEntries();
        } else {
            NavigateToDirectory(currentDirectory, false);
        }
    }

    bool NavigateBack() {
        if (historyIndex > 0 && historyIndex < static_cast<int>(historyStack.size())) {
            historyIndex--;
            return NavigateToDirectory(historyStack[historyIndex], false);
        }
        return false;
    }

    bool ExitVirtualArchive() {
        if (!inVirtualArchive) return false;
        std::wstring host = archiveHostPath;
        inVirtualArchive = false;
        archiveHostPath.clear();
        virtualSubDir.clear();
        activeArchive.reset();
        fallbackArchive.reset();
        size_t lastSlash = host.find_last_of(L'\\');
        std::wstring parentDir = (lastSlash != std::wstring::npos) ? host.substr(0, lastSlash) : host;
        return NavigateToDirectory(parentDir.empty() ? L"C:\\" : parentDir, true);
    }

    std::wstring searchFilter;
    bool searchFastHddMode = true;

    void ApplyFilter() {
        SetFilterQuery(searchFilter);
    }
};

using FileExplorerPaneState = FileExplorerPaneStateT<ParsedGameArchive>;

// ============================================================================
// 5. Robust Non-Blocking File Transfer Queue (FileTransferQueue) & Dual-Pane Commander
// ============================================================================

enum class FileTransferOperationType {
    Copy,
    Move,
    DeleteToRecycleBin,
    DeletePermanent,
    ArchiveExtract
};

enum class FileTransferJobStatus {
    Queued,
    Running,
    Paused,
    Completed,
    Cancelled,
    Failed
};

struct FileTransferJob {
    uint64_t jobId = 0;
    FileTransferOperationType operation = FileTransferOperationType::Copy;
    std::vector<std::wstring> sourcePaths;
    std::wstring destinationDirectory;

    // For ArchiveExtract jobs
    std::wstring archiveFilePath;
    std::vector<size_t> archiveEntryIndices;

    wchar_t primaryDriveLetter = L'C';
    FileTransferJobStatus status = FileTransferJobStatus::Queued;
    std::wstring currentFileName;
    std::wstring errorMessage;

    uint64_t completedFiles = 0;
    uint64_t totalFiles = 0;
    uint64_t transferredBytes = 0;
    uint64_t totalBytes = 0;
    double transferSpeedMBps = 0.0;

    bool paused = false;
    BOOL cancelRequested = FALSE;
};

struct FileTransferProgressSnapshot {
    uint64_t jobId = 0;
    FileTransferOperationType operation = FileTransferOperationType::Copy;
    FileTransferJobStatus status = FileTransferJobStatus::Queued;
    std::wstring currentFileName;
    std::wstring errorMessage;
    uint64_t completedFiles = 0;
    uint64_t totalFiles = 0;
    uint64_t transferredBytes = 0;
    uint64_t totalBytes = 0;
    double transferSpeedMBps = 0.0;
    bool paused = false;
};

class FileTransferQueue {
private:
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> shutdownRequested_{ false };
    uint64_t nextJobId_ = 1;

    std::vector<std::shared_ptr<FileTransferJob>> jobs_;
    std::unordered_set<wchar_t> busyDriveLetters_;
    std::thread schedulerThread_;

    struct CopyProgressContext {
        FileTransferQueue* queue = nullptr;
        std::shared_ptr<FileTransferJob> job;
        uint64_t baseBytesBeforeCurrentFile = 0;
        std::chrono::steady_clock::time_point startTime;
    };

    static DWORD CALLBACK CopyFileProgressCallback(
        LARGE_INTEGER totalFileSize,
        LARGE_INTEGER totalBytesTransferred,
        LARGE_INTEGER /*streamSize*/,
        LARGE_INTEGER /*streamBytesTransferred*/,
        DWORD /*dwStreamNumber*/,
        DWORD /*dwCallbackReason*/,
        HANDLE /*hSourceFile*/,
        HANDLE /*hDestinationFile*/,
        LPVOID lpData) {
        auto* ctx = static_cast<CopyProgressContext*>(lpData);
        if (!ctx || !ctx->job) return PROGRESS_CONTINUE;

        // Wait while paused without burning CPU or disk I/O
        while (ctx->job->paused && !ctx->job->cancelRequested &&
               !ctx->queue->shutdownRequested_.load(std::memory_order_relaxed)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        if (ctx->job->cancelRequested || ctx->queue->shutdownRequested_.load(std::memory_order_relaxed)) {
            return PROGRESS_CANCEL;
        }

        uint64_t currentFileTransferred = static_cast<uint64_t>(totalBytesTransferred.QuadPart);
        uint64_t overallTransferred = ctx->baseBytesBeforeCurrentFile + currentFileTransferred;

        auto now = std::chrono::steady_clock::now();
        double elapsedSec = std::chrono::duration<double>(now - ctx->startTime).count();
        double mbps = (elapsedSec > 0.001)
            ? (static_cast<double>(overallTransferred) / (1024.0 * 1024.0)) / elapsedSec
            : 0.0;

        {
            std::lock_guard<std::mutex> lock(ctx->queue->mutex_);
            ctx->job->transferredBytes = overallTransferred;
            if (ctx->job->totalBytes < overallTransferred) {
                ctx->job->totalBytes = (std::max)(ctx->job->totalBytes,
                    ctx->baseBytesBeforeCurrentFile + static_cast<uint64_t>(totalFileSize.QuadPart));
            }
            ctx->job->transferSpeedMBps = mbps;
        }

        return PROGRESS_CONTINUE;
    }

    static wchar_t DetermineTargetDriveLetter(const FileTransferJob& job) {
        if (!job.destinationDirectory.empty() && job.destinationDirectory.size() >= 2 &&
            iswalpha(job.destinationDirectory[0]) && job.destinationDirectory[1] == L':') {
            return static_cast<wchar_t>(towupper(job.destinationDirectory[0]));
        }
        if (!job.sourcePaths.empty() && job.sourcePaths[0].size() >= 2 &&
            iswalpha(job.sourcePaths[0][0]) && job.sourcePaths[0][1] == L':') {
            return static_cast<wchar_t>(towupper(job.sourcePaths[0][0]));
        }
        return L'C';
    }

    static void CollectFilePairsRecursive(
        const std::wstring& srcPath,
        const std::wstring& dstPath,
        std::vector<std::pair<std::wstring, std::wstring>>& outFilePairs,
        std::vector<std::wstring>& outDirsToCreate,
        uint64_t& outTotalBytes,
        const BOOL& cancelFlag) {
        if (cancelFlag) return;

        WIN32_FILE_ATTRIBUTE_DATA fad{};
        if (!GetFileAttributesExW(srcPath.c_str(), GetFileExInfoStandard, &fad)) {
            return;
        }

        if ((fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
            outDirsToCreate.push_back(dstPath);

            std::wstring searchPattern = srcPath;
            if (!searchPattern.empty() && searchPattern.back() != L'\\') searchPattern.push_back(L'\\');
            std::wstring srcBase = searchPattern;
            searchPattern.push_back(L'*');

            std::wstring dstBase = dstPath;
            if (!dstBase.empty() && dstBase.back() != L'\\') dstBase.push_back(L'\\');

            WIN32_FIND_DATAW ffd{};
            HANDLE hFind = FindFirstFileExW(
                searchPattern.c_str(),
                FindExInfoBasic,
                &ffd,
                FindExSearchNameMatch,
                nullptr,
                FIND_FIRST_EX_LARGE_FETCH);
            if (hFind == INVALID_HANDLE_VALUE) return;

            do {
                if (cancelFlag) break;
                const wchar_t* fn = ffd.cFileName;
                if (fn[0] == L'.' && (fn[1] == L'\0' || (fn[1] == L'.' && fn[2] == L'\0'))) {
                    continue;
                }
                CollectFilePairsRecursive(srcBase + fn, dstBase + fn, outFilePairs, outDirsToCreate, outTotalBytes, cancelFlag);
            } while (FindNextFileW(hFind, &ffd));

            FindClose(hFind);
        } else {
            uint64_t sz = (static_cast<uint64_t>(fad.nFileSizeHigh) << 32) | fad.nFileSizeLow;
            outTotalBytes += sz;
            outFilePairs.emplace_back(srcPath, dstPath);
        }
    }

    void ExecuteJob(std::shared_ptr<FileTransferJob> job) {
        auto startTime = std::chrono::steady_clock::now();

        if (job->operation == FileTransferOperationType::DeleteToRecycleBin ||
            job->operation == FileTransferOperationType::DeletePermanent) {
            // Double-null-terminated string required by SHFileOperationW
            std::wstring fromBuffer;
            for (const auto& sp : job->sourcePaths) {
                fromBuffer += sp;
                fromBuffer.push_back(L'\0');
            }
            fromBuffer.push_back(L'\0');

            {
                std::lock_guard<std::mutex> lock(mutex_);
                job->totalFiles = job->sourcePaths.size();
                if (!job->sourcePaths.empty()) {
                    size_t slash = job->sourcePaths[0].find_last_of(L"\\/");
                    job->currentFileName = (slash == std::wstring::npos) ? job->sourcePaths[0] : job->sourcePaths[0].substr(slash + 1);
                }
            }

            SHFILEOPSTRUCTW fileOp{};
            fileOp.wFunc = FO_DELETE;
            fileOp.pFrom = fromBuffer.c_str();
            fileOp.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;
            if (job->operation == FileTransferOperationType::DeleteToRecycleBin) {
                fileOp.fFlags |= FOF_ALLOWUNDO;
            }

            int res = SHFileOperationW(&fileOp);
            std::lock_guard<std::mutex> lock(mutex_);
            if (fileOp.fAnyOperationsAborted || job->cancelRequested) {
                job->status = FileTransferJobStatus::Cancelled;
            } else if (res == 0) {
                job->completedFiles = job->totalFiles;
                job->status = FileTransferJobStatus::Completed;
            } else {
                job->errorMessage = L"SHFileOperationW failed with code " + std::to_wstring(res);
                job->status = FileTransferJobStatus::Failed;
            }
            return;
        }

        if (job->operation == FileTransferOperationType::Copy ||
            job->operation == FileTransferOperationType::Move) {
            std::vector<std::pair<std::wstring, std::wstring>> filePairs;
            std::vector<std::wstring> dirsToCreate;
            uint64_t totalBytesCalc = 0;

            std::wstring dstDir = job->destinationDirectory;
            if (!dstDir.empty() && dstDir.back() != L'\\') dstDir.push_back(L'\\');

            for (const auto& src : job->sourcePaths) {
                size_t slash = src.find_last_of(L"\\/");
                std::wstring leafName = (slash == std::wstring::npos) ? src : src.substr(slash + 1);
                CollectFilePairsRecursive(src, dstDir + leafName, filePairs, dirsToCreate, totalBytesCalc, job->cancelRequested);
            }

            {
                std::lock_guard<std::mutex> lock(mutex_);
                job->totalFiles = filePairs.size();
                job->totalBytes = totalBytesCalc;
            }

            for (const auto& dirPath : dirsToCreate) {
                if (job->cancelRequested) break;
                CreateDirectoryW(dirPath.c_str(), nullptr);
            }

            uint64_t accumulatedBytes = 0;
            bool hadError = false;

            for (size_t i = 0; i < filePairs.size(); ++i) {
                while (job->paused && !job->cancelRequested && !shutdownRequested_.load(std::memory_order_relaxed)) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
                if (job->cancelRequested || shutdownRequested_.load(std::memory_order_relaxed)) {
                    break;
                }

                const auto& [srcFile, dstFile] = filePairs[i];
                size_t slash = srcFile.find_last_of(L"\\/");
                std::wstring leaf = (slash == std::wstring::npos) ? srcFile : srcFile.substr(slash + 1);

                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    job->currentFileName = leaf;
                }

                // Fast same-volume move optimization via MoveFileExW before falling back to CopyFileExW
                bool movedDirectly = false;
                if (job->operation == FileTransferOperationType::Move) {
                    if (MoveFileExW(srcFile.c_str(), dstFile.c_str(), MOVEFILE_REPLACE_EXISTING)) {
                        movedDirectly = true;
                        WIN32_FILE_ATTRIBUTE_DATA fad{};
                        if (GetFileAttributesExW(dstFile.c_str(), GetFileExInfoStandard, &fad)) {
                            uint64_t fsz = (static_cast<uint64_t>(fad.nFileSizeHigh) << 32) | fad.nFileSizeLow;
                            accumulatedBytes += fsz;
                        }
                    }
                }

                if (!movedDirectly) {
                    CopyProgressContext ctx{};
                    ctx.queue = this;
                    ctx.job = job;
                    ctx.baseBytesBeforeCurrentFile = accumulatedBytes;
                    ctx.startTime = startTime;

                    BOOL ok = CopyFileExW(
                        srcFile.c_str(),
                        dstFile.c_str(),
                        &FileTransferQueue::CopyFileProgressCallback,
                        &ctx,
                        &job->cancelRequested,
                        0);

                    if (!ok) {
                        if (job->cancelRequested) break;
                        hadError = true;
                        std::lock_guard<std::mutex> lock(mutex_);
                        job->errorMessage = L"Failed transferring: " + leaf + L" (Win32 Error " + std::to_wstring(GetLastError()) + L")";
                        break;
                    }

                    WIN32_FILE_ATTRIBUTE_DATA fad{};
                    if (GetFileAttributesExW(dstFile.c_str(), GetFileExInfoStandard, &fad)) {
                        uint64_t fsz = (static_cast<uint64_t>(fad.nFileSizeHigh) << 32) | fad.nFileSizeLow;
                        accumulatedBytes += fsz;
                    }

                    if (job->operation == FileTransferOperationType::Move) {
                        DeleteFileW(srcFile.c_str());
                    }
                }

                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    job->completedFiles = i + 1;
                    job->transferredBytes = accumulatedBytes;
                }
            }

            // Clean up empty source directories on Move
            if (!hadError && !job->cancelRequested && job->operation == FileTransferOperationType::Move) {
                for (auto rIt = job->sourcePaths.rbegin(); rIt != job->sourcePaths.rend(); ++rIt) {
                    RemoveDirectoryW(rIt->c_str());
                }
            }

            std::lock_guard<std::mutex> lock(mutex_);
            if (job->cancelRequested) {
                job->status = FileTransferJobStatus::Cancelled;
            } else if (hadError) {
                job->status = FileTransferJobStatus::Failed;
            } else {
                job->status = FileTransferJobStatus::Completed;
            }
            return;
        }

        // ArchiveExtract job
        if (job->operation == FileTransferOperationType::ArchiveExtract) {
            CreateDirectoryW(job->destinationDirectory.c_str(), nullptr);
            ParsedGameArchive arc = ParseArchiveFile(job->archiveFilePath);
            std::vector<size_t> indices = job->archiveEntryIndices;
            if (indices.empty()) {
                for (size_t i = 0; i < arc.entries.size(); ++i) {
                    if (!arc.entries[i].isDirectory) indices.push_back(i);
                }
            }
            {
                std::lock_guard<std::mutex> lock(mutex_);
                job->totalFiles = indices.empty() ? 1 : indices.size();
                uint64_t totB = 0;
                for (size_t idx : indices) {
                    if (idx < arc.entries.size()) totB += arc.entries[idx].uncompressedSize;
                }
                job->totalBytes = totB;
            }
            uint64_t accumBytes = 0;
            std::wstring baseDst = job->destinationDirectory;
            if (!baseDst.empty() && baseDst.back() != L'\\') baseDst.push_back(L'\\');

            for (size_t k = 0; k < indices.size(); ++k) {
                while (job->paused && !job->cancelRequested && !shutdownRequested_.load(std::memory_order_relaxed)) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
                if (job->cancelRequested || shutdownRequested_.load(std::memory_order_relaxed)) break;
                size_t idx = indices[k];
                if (idx >= arc.entries.size()) continue;
                const auto& entry = arc.entries[idx];
                if (entry.isDirectory) continue;

                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    job->currentFileName = entry.name;
                }

                std::vector<uint8_t> data;
                if (ExtractArchiveEntryBytes(arc, entry, data)) {
                    std::wstring relPath = entry.path;
                    for (auto& ch : relPath) {
                        if (ch == L'/') ch = L'\\';
                        if (ch == L':' || ch == L'*' || ch == L'?' || ch == L'"' || ch == L'<' || ch == L'>' || ch == L'|') ch = L'_';
                    }
                    while (!relPath.empty() && relPath.front() == L'\\') relPath.erase(relPath.begin());
                    std::wstring fullOut = baseDst + (relPath.empty() ? entry.name : relPath);
                    std::error_code ec;
                    std::filesystem::path outFs(fullOut);
                    if (outFs.has_parent_path()) {
                        std::filesystem::create_directories(outFs.parent_path(), ec);
                    }
                    gfe_repack_detail::WriteAllBytesToDisk(fullOut, data);
                    accumBytes += data.size();
                }

                auto now = std::chrono::steady_clock::now();
                double elapsedSec = std::chrono::duration<double>(now - startTime).count();
                double mbps = (elapsedSec > 0.001) ? ((static_cast<double>(accumBytes) / (1024.0 * 1024.0)) / elapsedSec) : 0.0;
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    job->completedFiles = k + 1;
                    job->transferredBytes = accumBytes;
                    job->transferSpeedMBps = mbps;
                }
            }

            std::lock_guard<std::mutex> lock(mutex_);
            job->status = job->cancelRequested ? FileTransferJobStatus::Cancelled : FileTransferJobStatus::Completed;
        }
    }

    void SchedulerLoop() {
        while (!shutdownRequested_.load(std::memory_order_acquire)) {
            std::shared_ptr<FileTransferJob> runnableJob;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.wait_for(lock, std::chrono::milliseconds(100), [this]() {
                    if (shutdownRequested_.load(std::memory_order_relaxed)) return true;
                    for (const auto& j : jobs_) {
                        if (j->status == FileTransferJobStatus::Queued &&
                            busyDriveLetters_.find(j->primaryDriveLetter) == busyDriveLetters_.end()) {
                            return true;
                        }
                    }
                    return false;
                });

                if (shutdownRequested_.load(std::memory_order_relaxed)) return;

                // Pick next queued job whose target drive letter is not currently busy
                // This serializes transfers on the same physical drive letter so HDDs never thrash
                for (auto& j : jobs_) {
                    if (j->status == FileTransferJobStatus::Queued &&
                        busyDriveLetters_.find(j->primaryDriveLetter) == busyDriveLetters_.end()) {
                        j->status = FileTransferJobStatus::Running;
                        busyDriveLetters_.insert(j->primaryDriveLetter);
                        runnableJob = j;
                        break;
                    }
                }
            }

            if (runnableJob) {
                ExecuteJob(runnableJob);
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    busyDriveLetters_.erase(runnableJob->primaryDriveLetter);
                }
                cv_.notify_all();
            }
        }
    }

public:
    FileTransferQueue() {
        schedulerThread_ = std::thread(&FileTransferQueue::SchedulerLoop, this);
    }

    void Shutdown() {
        shutdownRequested_.store(true, std::memory_order_release);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (auto& j : jobs_) {
                j->cancelRequested = TRUE;
                j->paused = false;
            }
        }
        cv_.notify_all();
        if (schedulerThread_.joinable()) {
            schedulerThread_.join();
        }
    }

    ~FileTransferQueue() {
        Shutdown();
    }

    FileTransferQueue(const FileTransferQueue&) = delete;
    FileTransferQueue& operator=(const FileTransferQueue&) = delete;

    uint64_t EnqueueJob(FileTransferOperationType op,
                        std::vector<std::wstring> sourcePaths,
                        std::wstring destinationDir = L"") {
        auto job = std::make_shared<FileTransferJob>();
        job->operation = op;
        job->sourcePaths = std::move(sourcePaths);
        job->destinationDirectory = std::move(destinationDir);
        job->primaryDriveLetter = DetermineTargetDriveLetter(*job);

        uint64_t assignedId = 0;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            job->jobId = nextJobId_++;
            assignedId = job->jobId;
            jobs_.push_back(std::move(job));
        }
        cv_.notify_all();
        return assignedId;
    }

    uint64_t EnqueueArchiveExtraction(const std::wstring& archivePath,
                                      std::vector<size_t> entryIndices,
                                      const std::wstring& destinationDir) {
        auto job = std::make_shared<FileTransferJob>();
        job->operation = FileTransferOperationType::ArchiveExtract;
        job->archiveFilePath = archivePath;
        job->archiveEntryIndices = std::move(entryIndices);
        job->destinationDirectory = destinationDir;
        job->totalFiles = job->archiveEntryIndices.empty() ? 1 : job->archiveEntryIndices.size();
        job->primaryDriveLetter = DetermineTargetDriveLetter(*job);

        uint64_t assignedId = 0;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            job->jobId = nextJobId_++;
            assignedId = job->jobId;
            jobs_.push_back(std::move(job));
        }
        cv_.notify_all();
        return assignedId;
    }

    // Dual-Pane Commander F5: Copy selected items from srcPane to dstPane
    uint64_t EnqueueCopyBetweenPanes(const FileExplorerPaneState& srcPane, const FileExplorerPaneState& dstPane) {
        if (dstPane.inVirtualArchive || dstPane.currentDirectory.empty()) return 0;

        auto selected = srcPane.GetSelectedOrFocusedEntries();
        if (selected.empty()) return 0;

        if (srcPane.inVirtualArchive) {
            std::vector<size_t> indices;
            for (const auto& e : selected) {
                if (e.virtualArchiveEntryIndex != SIZE_MAX) {
                    indices.push_back(e.virtualArchiveEntryIndex);
                }
            }
            return EnqueueArchiveExtraction(srcPane.archiveHostPath, std::move(indices), dstPane.currentDirectory);
        }

        std::vector<std::wstring> sources;
        sources.reserve(selected.size());
        for (const auto& e : selected) {
            sources.push_back(e.fullPath);
        }
        return EnqueueJob(FileTransferOperationType::Copy, std::move(sources), dstPane.currentDirectory);
    }

    // Dual-Pane Commander F6: Move selected items from srcPane to dstPane
    uint64_t EnqueueMoveBetweenPanes(const FileExplorerPaneState& srcPane, const FileExplorerPaneState& dstPane) {
        if (srcPane.inVirtualArchive || dstPane.inVirtualArchive || dstPane.currentDirectory.empty()) return 0;

        auto selected = srcPane.GetSelectedOrFocusedEntries();
        if (selected.empty()) return 0;

        std::vector<std::wstring> sources;
        sources.reserve(selected.size());
        for (const auto& e : selected) {
            sources.push_back(e.fullPath);
        }
        return EnqueueJob(FileTransferOperationType::Move, std::move(sources), dstPane.currentDirectory);
    }

    uint64_t QueueCopy(std::vector<std::wstring> sources, const std::wstring& destDir) {
        return EnqueueJob(FileTransferOperationType::Copy, std::move(sources), destDir);
    }

    uint64_t QueueMove(std::vector<std::wstring> sources, const std::wstring& destDir) {
        return EnqueueJob(FileTransferOperationType::Move, std::move(sources), destDir);
    }

    uint64_t QueueExtractArchive(std::shared_ptr<ParsedGameArchive> archive, const std::wstring& destDir) {
        if (!archive) return 0;
        std::vector<size_t> indices(archive->entries.size());
        for (size_t i = 0; i < indices.size(); ++i) indices[i] = i;
        return EnqueueArchiveExtraction(archive->filePath, std::move(indices), destDir);
    }

    void PauseJob(uint64_t jobId) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& j : jobs_) {
            if (j->jobId == jobId && j->status == FileTransferJobStatus::Running) {
                j->paused = true;
                j->status = FileTransferJobStatus::Paused;
                break;
            }
        }
    }

    void ResumeJob(uint64_t jobId) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& j : jobs_) {
            if (j->jobId == jobId && j->status == FileTransferJobStatus::Paused) {
                j->paused = false;
                j->status = FileTransferJobStatus::Running;
                break;
            }
        }
    }

    void CancelJob(uint64_t jobId) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& j : jobs_) {
            if (j->jobId == jobId) {
                j->cancelRequested = TRUE;
                j->paused = false;
                if (j->status == FileTransferJobStatus::Queued) {
                    j->status = FileTransferJobStatus::Cancelled;
                }
                break;
            }
        }
    }

    std::vector<FileTransferProgressSnapshot> GetSnapshots() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<FileTransferProgressSnapshot> out;
        out.reserve(jobs_.size());
        for (const auto& j : jobs_) {
            FileTransferProgressSnapshot snap{};
            snap.jobId = j->jobId;
            snap.operation = j->operation;
            snap.status = j->status;
            snap.currentFileName = j->currentFileName;
            snap.errorMessage = j->errorMessage;
            snap.completedFiles = j->completedFiles;
            snap.totalFiles = j->totalFiles;
            snap.transferredBytes = j->transferredBytes;
            snap.totalBytes = j->totalBytes;
            snap.transferSpeedMBps = j->transferSpeedMBps;
            snap.paused = j->paused;
            out.push_back(std::move(snap));
        }
        return out;
    }
};

} // namespace speta

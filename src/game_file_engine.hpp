#pragma once

#ifndef _ALLOW_COMPILER_AND_STL_VERSION_MISMATCH
#define _ALLOW_COMPILER_AND_STL_VERSION_MISMATCH
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>
#include <span>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cwctype>
#include <cmath>
#include <memory>
#include <filesystem>

#include "compression_engine.hpp"

namespace speta {

enum class ArchiveFormatId {
    Unknown,
    ZIP,
    RAR,
    ISO,
    FF,
    WAD,
    PAK,
    PK3,
    VPK,
    UNREAL,
    UNITY,
    BSA,
    BA2,
    RPF,
    FORGE,
    FROMSOFT,
    IDTECH,
    FF13,
    SQUARE,
    CRIWARE,
    PAR,
    BIG,
    BIK,
    RPA,
    JAR,
    PS1,
    PKG,
    GOD,
    XEX,
    APPX,
    NDS,
    ROM,
    ZLIB_CARVED
};

enum class AssetCategory {
    Code,
    Texture,
    Audio,
    Video,
    Model3D,
    Map2D,
    Palette,
    Config,
    Binary,
    Folder
};

struct ArchiveHeaderField {
    std::wstring label;
    std::wstring offsetStr;
    uint32_t length = 0;
    std::wstring value;
    std::wstring description;
};

// Extraction mode flags stored in ArchiveEntry::customFlags (low 8 bits)
namespace ExtractMode {
    constexpr uint32_t Stored           = 0;
    constexpr uint32_t ZlibStream       = 1;
    constexpr uint32_t RawDeflate       = 2;
    constexpr uint32_t Lz4Block         = 3;
    constexpr uint32_t Ps1Raw2352Mode2  = 4;
    constexpr uint32_t Ps1Raw2352Mode1  = 5;
    constexpr uint32_t BsaZlib          = 6;
    constexpr uint32_t Ba2Dx10Tex       = 7;
    constexpr uint32_t InlineOnly       = 8;
}

struct ArchiveEntry {
    std::wstring name;
    std::wstring path;
    std::wstring folder;
    uint64_t offset = 0;
    uint64_t compressedSize = 0;
    uint64_t uncompressedSize = 0;
    std::wstring compressionMethod;
    uint32_t crc32 = 0;
    AssetCategory category = AssetCategory::Binary;
    bool isDirectory = false;
    uint32_t customFlags = 0;
    std::vector<uint8_t> inlineData;
};

struct FastFileItemRecord {
    std::string id;
    std::string kind; // "rawfile", "script", "menu", "stringtable", "localize"
    std::string name;
    uint32_t hdr = 0;
    uint32_t data = 0;
    uint32_t size = 0;
    bool compressed = false;
    int32_t cols = 0;
    int32_t rows = 0;
    uint32_t end = 0;
    std::vector<std::string> cells;
};

struct FastFileZoneState {
    bool valid = false;
    std::string magic;
    uint32_t version = 0;
    bool isBo1 = false;
    std::vector<uint32_t> xfile;
    std::vector<uint32_t> origXfile;
    std::vector<uint8_t> xfileTail;
    std::vector<uint8_t> zone;
    size_t origZoneLen = 0;
    uint32_t gaps = 0;
    uint32_t nblocks = 0;
    std::vector<std::string> scriptStrings;
    std::vector<std::string> depends;
    std::vector<uint32_t> assetTypes;
    uint32_t dataStart = 0;
    std::vector<FastFileItemRecord> items;
};

struct ParsedGameArchive {
    bool valid = false;
    ArchiveFormatId formatId = ArchiveFormatId::Unknown;
    std::wstring formatName;
    std::wstring engineName;
    std::wstring filePath;
    uint64_t fileSize = 0;
    bool isLittleEndian = true;
    bool supportsRepack = false;
    std::vector<ArchiveHeaderField> headerFields;
    std::vector<ArchiveEntry> entries;
    std::vector<uint8_t> doomPlaypal;
    FastFileZoneState ffZoneState;
};

class MappedFile {
public:
    MappedFile() = default;

    explicit MappedFile(const std::wstring& path) {
        Open(path);
    }

    ~MappedFile() {
        Close();
    }

    MappedFile(const MappedFile&) = delete;
    MappedFile& operator=(const MappedFile&) = delete;

    MappedFile(MappedFile&& other) noexcept
        : hFile_(other.hFile_),
          hMapping_(other.hMapping_),
          view_(other.view_),
          size_(other.size_) {
        other.hFile_ = INVALID_HANDLE_VALUE;
        other.hMapping_ = nullptr;
        other.view_ = nullptr;
        other.size_ = 0;
    }

    MappedFile& operator=(MappedFile&& other) noexcept {
        if (this != &other) {
            Close();
            hFile_ = other.hFile_;
            hMapping_ = other.hMapping_;
            view_ = other.view_;
            size_ = other.size_;
            other.hFile_ = INVALID_HANDLE_VALUE;
            other.hMapping_ = nullptr;
            other.view_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    bool Open(const std::wstring& path) {
        Close();
        hFile_ = CreateFileW(
            path.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );
        if (hFile_ == INVALID_HANDLE_VALUE) {
            return false;
        }

        LARGE_INTEGER li{};
        if (!GetFileSizeEx(hFile_, &li) || li.QuadPart < 0) {
            Close();
            return false;
        }
        size_ = static_cast<size_t>(li.QuadPart);
        if (size_ == 0) {
            return true;
        }

        hMapping_ = CreateFileMappingW(hFile_, nullptr, PAGE_READONLY, 0, 0, nullptr);
        if (!hMapping_) {
            Close();
            return false;
        }

        view_ = static_cast<const uint8_t*>(MapViewOfFile(hMapping_, FILE_MAP_READ, 0, 0, 0));
        if (!view_) {
            Close();
            return false;
        }
        return true;
    }

    void Close() {
        if (view_) {
            UnmapViewOfFile(view_);
            view_ = nullptr;
        }
        if (hMapping_) {
            CloseHandle(hMapping_);
            hMapping_ = nullptr;
        }
        if (hFile_ != INVALID_HANDLE_VALUE) {
            CloseHandle(hFile_);
            hFile_ = INVALID_HANDLE_VALUE;
        }
        size_ = 0;
    }

    bool valid() const {
        return hFile_ != INVALID_HANDLE_VALUE && (size_ == 0 || view_ != nullptr);
    }

    size_t size() const {
        return size_;
    }

    const uint8_t* data() const {
        return view_;
    }

    std::span<const uint8_t> span() const {
        if (!view_ || size_ == 0) return {};
        return std::span<const uint8_t>(view_, size_);
    }

private:
    HANDLE hFile_ = INVALID_HANDLE_VALUE;
    HANDLE hMapping_ = nullptr;
    const uint8_t* view_ = nullptr;
    size_t size_ = 0;
};

class BinaryReader {
public:
    explicit constexpr BinaryReader(std::span<const uint8_t> buf) : buf_(buf) {}

    constexpr size_t size() const { return buf_.size(); }
    constexpr bool empty() const { return buf_.empty(); }
    constexpr const uint8_t* data() const { return buf_.data(); }
    constexpr std::span<const uint8_t> span() const { return buf_; }

    constexpr uint8_t operator[](size_t idx) const {
        return idx < buf_.size() ? buf_[idx] : 0;
    }

    template <typename T>
    T read_le(size_t offset, T defaultVal = T{}) const {
        if (offset + sizeof(T) > buf_.size()) return defaultVal;
        uint64_t acc = 0;
        for (size_t i = 0; i < sizeof(T); ++i) {
            acc |= static_cast<uint64_t>(buf_[offset + i]) << (i * 8);
        }
        return static_cast<T>(acc);
    }

    template <typename T>
    T read_be(size_t offset, T defaultVal = T{}) const {
        if (offset + sizeof(T) > buf_.size()) return defaultVal;
        uint64_t acc = 0;
        for (size_t i = 0; i < sizeof(T); ++i) {
            acc = (acc << 8) | static_cast<uint64_t>(buf_[offset + i]);
        }
        return static_cast<T>(acc);
    }

    std::string read_ascii(size_t offset, size_t maxLen, bool stopAtNull = true) const {
        if (offset >= buf_.size() || maxLen == 0) return {};
        size_t end = std::min(buf_.size(), offset + maxLen);
        std::string out;
        out.reserve(end - offset);
        for (size_t i = offset; i < end; ++i) {
            uint8_t b = buf_[i];
            if (b == 0 && stopAtNull) break;
            if (b >= 32 && b <= 126) {
                out.push_back(static_cast<char>(b));
            } else if (!stopAtNull) {
                out.push_back(static_cast<char>(b));
            } else {
                out.push_back('.');
            }
        }
        return out;
    }

    std::string read_cstr(size_t offset, size_t maxLen = 512) const {
        if (offset >= buf_.size()) return {};
        size_t end = std::min(buf_.size(), offset + maxLen);
        std::string out;
        for (size_t i = offset; i < end; ++i) {
            if (buf_[i] == 0) break;
            out.push_back(static_cast<char>(buf_[i]));
        }
        return out;
    }

    std::wstring read_utf16le(size_t offset, size_t maxChars) const {
        if (offset + 2 > buf_.size() || maxChars == 0) return {};
        size_t end = std::min(buf_.size(), offset + maxChars * 2);
        std::wstring out;
        out.reserve((end - offset) / 2);
        for (size_t i = offset; i + 1 < end; i += 2) {
            uint16_t ch = static_cast<uint16_t>(buf_[i]) | (static_cast<uint16_t>(buf_[i + 1]) << 8);
            if (ch == 0) break;
            out.push_back(static_cast<wchar_t>(ch));
        }
        return out;
    }

    std::wstring read_utf16be(size_t offset, size_t maxBytes) const {
        if (offset + 2 > buf_.size() || maxBytes < 2) return {};
        size_t end = std::min(buf_.size(), offset + maxBytes);
        std::wstring out;
        out.reserve((end - offset) / 2);
        for (size_t i = offset; i + 1 < end; i += 2) {
            uint16_t ch = (static_cast<uint16_t>(buf_[i]) << 8) | static_cast<uint16_t>(buf_[i + 1]);
            if (ch == 0) break;
            out.push_back(static_cast<wchar_t>(ch));
        }
        return out;
    }

    std::span<const uint8_t> slice(size_t offset, size_t len) const {
        if (offset >= buf_.size() || len == 0) return {};
        size_t safeLen = std::min(len, buf_.size() - offset);
        return buf_.subspan(offset, safeLen);
    }

    std::vector<uint8_t> copy_slice(size_t offset, size_t len) const {
        auto sp = slice(offset, len);
        return std::vector<uint8_t>(sp.begin(), sp.end());
    }

private:
    std::span<const uint8_t> buf_;
};

namespace gfe_detail {

inline std::wstring AsciiToWide(std::string_view s) {
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    if (len > 0) {
        std::wstring out(static_cast<size_t>(len), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), out.data(), len);
        return out;
    }
    std::wstring fallback;
    fallback.reserve(s.size());
    for (unsigned char c : s) {
        fallback.push_back(static_cast<wchar_t>(c));
    }
    return fallback;
}

inline std::string WideToUtf8Str(std::wstring_view w) {
    if (w.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
    if (len > 0) {
        std::string out(static_cast<size_t>(len), '\0');
        WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), out.data(), len, nullptr, nullptr);
        return out;
    }
    std::string fallback;
    fallback.reserve(w.size());
    for (wchar_t c : w) {
        fallback.push_back(static_cast<char>(c & 0xFF));
    }
    return fallback;
}

inline std::wstring ToLowerW(std::wstring_view s) {
    std::wstring r(s);
    for (wchar_t& c : r) c = static_cast<wchar_t>(std::towlower(c));
    return r;
}

inline std::string ToLowerA(std::string_view s) {
    std::string r(s);
    for (char& c : r) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    return r;
}

inline std::string TrimA(std::string_view s) {
    size_t a = 0;
    while (a < s.size() && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r' || s[a] == '\n' || s[a] == '\0')) ++a;
    size_t b = s.size();
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r' || s[b - 1] == '\n' || s[b - 1] == '\0')) --b;
    return std::string(s.substr(a, b - a));
}

inline std::wstring TrimW(std::wstring_view s) {
    size_t a = 0;
    while (a < s.size() && (s[a] == L' ' || s[a] == L'\t' || s[a] == L'\r' || s[a] == L'\n' || s[a] == L'\0')) ++a;
    size_t b = s.size();
    while (b > a && (s[b - 1] == L' ' || s[b - 1] == L'\t' || s[b - 1] == L'\r' || s[b - 1] == L'\n' || s[b - 1] == L'\0')) --b;
    return std::wstring(s.substr(a, b - a));
}

inline bool EndsWithW(std::wstring_view s, std::wstring_view suffix) {
    return s.size() >= suffix.size() && s.substr(s.size() - suffix.size()) == suffix;
}

inline bool EndsWithA(std::string_view s, std::string_view suffix) {
    return s.size() >= suffix.size() && s.substr(s.size() - suffix.size()) == suffix;
}

inline std::wstring ToHexW(uint64_t val, int pad = 8) {
    wchar_t buf[64]{};
    swprintf_s(buf, L"0x%0*llX", pad, static_cast<unsigned long long>(val));
    return buf;
}

inline std::string ToHexA(uint64_t val, int pad = 8) {
    char buf[64]{};
    snprintf(buf, sizeof(buf), "0x%0*llX", pad, static_cast<unsigned long long>(val));
    return buf;
}

inline std::wstring BytesHexPreviewW(std::span<const uint8_t> bytes, size_t maxLen = 8) {
    size_t n = std::min(bytes.size(), maxLen);
    std::wstring out;
    for (size_t i = 0; i < n; ++i) {
        if (i > 0) out.push_back(L' ');
        wchar_t buf[8]{};
        swprintf_s(buf, L"%02X", bytes[i]);
        out += buf;
    }
    return out;
}

inline std::wstring FormatBytesW(uint64_t b) {
    wchar_t buf[64]{};
    if (b < 1024) {
        swprintf_s(buf, L"%llu B", static_cast<unsigned long long>(b));
    } else if (b < 1024ULL * 1024) {
        swprintf_s(buf, L"%.2f KB", static_cast<double>(b) / 1024.0);
    } else if (b < 1024ULL * 1024 * 1024) {
        swprintf_s(buf, L"%.2f MB", static_cast<double>(b) / (1024.0 * 1024.0));
    } else {
        swprintf_s(buf, L"%.2f GB", static_cast<double>(b) / (1024.0 * 1024.0 * 1024.0));
    }
    return buf;
}

inline std::wstring ExtractFileNameW(std::wstring_view fullPath) {
    size_t pos = fullPath.find_last_of(L"/\\");
    return (pos == std::wstring_view::npos) ? std::wstring(fullPath) : std::wstring(fullPath.substr(pos + 1));
}

struct SplitPathResult {
    std::wstring folder;
    std::wstring name;
    std::wstring cleanPath;
};

inline SplitPathResult SplitVirtualPath(std::wstring_view rawPath) {
    std::wstring norm;
    norm.reserve(rawPath.size());
    for (wchar_t c : rawPath) {
        if (c == L'\\') c = L'/';
        if (c == L'\0') break;
        norm.push_back(c);
    }
    // Strip drive prefix like C:/
    if (norm.size() >= 3 && std::iswalpha(norm[0]) && norm[1] == L':' && norm[2] == L'/') {
        norm.erase(0, 3);
    }
    while (!norm.empty() && norm.front() == L'/') {
        norm.erase(norm.begin());
    }
    while (!norm.empty() && norm.back() == L'/') {
        norm.pop_back();
    }
    if (norm.empty()) {
        return { L"(root)", L"unnamed.bin", L"unnamed.bin" };
    }
    size_t lastSlash = norm.rfind(L'/');
    if (lastSlash == std::wstring::npos) {
        return { L"(root)", norm, norm };
    }
    return {
        norm.substr(0, lastSlash),
        norm.substr(lastSlash + 1),
        norm
    };
}

inline AssetCategory ClassifyAsset(std::wstring_view vpath, std::span<const uint8_t> sampleData = {}) {
    std::wstring lower = ToLowerW(vpath);
    size_t slash = lower.rfind(L'/');
    std::wstring base = (slash == std::wstring::npos) ? lower : lower.substr(slash + 1);

    if (base == L"playpal" || base == L"colormap" || EndsWithW(lower, L".pal") || EndsWithW(lower, L".nclr")) {
        return AssetCategory::Palette;
    }
    if (base == L"vertexes" || base == L"linedefs" || base == L"sidedefs" ||
        base == L"sectors" || base == L"things" || base == L"segs" ||
        base == L"ssectors" || base == L"nodes" || base == L"reject" || base == L"blockmap" ||
        EndsWithW(lower, L".bsp") || EndsWithW(lower, L".umap") || EndsWithW(lower, L".ent") ||
        (base.size() == 4 && base[0] == L'e' && std::iswdigit(base[1]) && base[2] == L'm' && std::iswdigit(base[3])) ||
        (base.size() == 5 && base.substr(0, 3) == L"map" && std::iswdigit(base[3]) && std::iswdigit(base[4]))) {
        return AssetCategory::Map2D;
    }
    if (EndsWithW(lower, L".gsc") || EndsWithW(lower, L".csc") || EndsWithW(lower, L".shader") ||
        EndsWithW(lower, L".menu") || EndsWithW(lower, L".vision") || EndsWithW(lower, L".lua") ||
        EndsWithW(lower, L".rpy") || EndsWithW(lower, L".rpym") || EndsWithW(lower, L".py") ||
        EndsWithW(lower, L".c") || EndsWithW(lower, L".cpp") || EndsWithW(lower, L".h") ||
        EndsWithW(lower, L".hlsl") || EndsWithW(lower, L".glsl") || EndsWithW(lower, L".cs") ||
        EndsWithW(lower, L".js") || EndsWithW(lower, L".ts") || EndsWithW(lower, L".class")) {
        return AssetCategory::Code;
    }
    if (EndsWithW(lower, L".cfg") || EndsWithW(lower, L".ini") || EndsWithW(lower, L".json") ||
        EndsWithW(lower, L".xml") || EndsWithW(lower, L".txt") || EndsWithW(lower, L".csv") ||
        EndsWithW(lower, L".cnf") || EndsWithW(lower, L".skin") || EndsWithW(lower, L".vmt") ||
        EndsWithW(lower, L".toml") || EndsWithW(lower, L".config") || EndsWithW(lower, L".sfo") ||
        EndsWithW(lower, L".utf")) {
        return AssetCategory::Config;
    }
    if (EndsWithW(lower, L".bik") || EndsWithW(lower, L".bk2") || EndsWithW(lower, L".str") ||
        EndsWithW(lower, L".mp4") || EndsWithW(lower, L".roq") || EndsWithW(lower, L".usm") ||
        EndsWithW(lower, L".m2v")) {
        return AssetCategory::Video;
    }
    if (EndsWithW(lower, L".png") || EndsWithW(lower, L".jpg") || EndsWithW(lower, L".jpeg") ||
        EndsWithW(lower, L".webp") || EndsWithW(lower, L".dds") || EndsWithW(lower, L".tga") ||
        EndsWithW(lower, L".bmp") || EndsWithW(lower, L".vtf") || EndsWithW(lower, L".flat") ||
        EndsWithW(lower, L".tim") || EndsWithW(lower, L".imgb") || EndsWithW(lower, L".image") ||
        EndsWithW(lower, L".rgba") || EndsWithW(lower, L".ncgr") || EndsWithW(lower, L".nsbtx") ||
        (sampleData.size() == 4096 && base.find(L'.') == std::wstring::npos)) {
        return AssetCategory::Texture;
    }
    if (EndsWithW(lower, L".wav") || EndsWithW(lower, L".ogg") || EndsWithW(lower, L".mp3") ||
        EndsWithW(lower, L".xa") || EndsWithW(lower, L".scd") || EndsWithW(lower, L".adx") ||
        EndsWithW(lower, L".hca") || EndsWithW(lower, L".awb") || EndsWithW(lower, L".acb") ||
        EndsWithW(lower, L".sdat") || EndsWithW(lower, L".swav") || EndsWithW(lower, L".swar") ||
        EndsWithW(lower, L".sseq") || EndsWithW(lower, L".sbnk") ||
        (sampleData.size() >= 12 && std::memcmp(sampleData.data(), "RIFF", 4) == 0) ||
        (sampleData.size() >= 4 && std::memcmp(sampleData.data(), "OggS", 4) == 0)) {
        return AssetCategory::Audio;
    }
    if (EndsWithW(lower, L".mdl") || EndsWithW(lower, L".md3") || EndsWithW(lower, L".obj") ||
        EndsWithW(lower, L".xmodel") || EndsWithW(lower, L".bmodel") || EndsWithW(lower, L".mesh") ||
        EndsWithW(lower, L".nsbmd") || EndsWithW(lower, L".bmd") || EndsWithW(lower, L".fbx") ||
        EndsWithW(lower, L".gltf")) {
        return AssetCategory::Model3D;
    }

    if (!sampleData.empty()) {
        size_t checkLen = std::min<size_t>(sampleData.size(), 128);
        size_t printable = 0;
        for (size_t i = 0; i < checkLen; ++i) {
            uint8_t c = sampleData[i];
            if ((c >= 32 && c <= 126) || c == 9 || c == 10 || c == 13) ++printable;
        }
        if (checkLen >= 8 && printable * 10 >= checkLen * 9) {
            return AssetCategory::Config;
        }
    }

    return AssetCategory::Binary;
}

inline AssetCategory ClassifyFile(std::wstring_view vpath, std::span<const uint8_t> sampleData = {}) {
    return ClassifyAsset(vpath, sampleData);
}

inline bool IsPrintableAscii(std::string_view s) {
    if (s.empty()) return false;
    for (unsigned char c : s) {
        if (c < 0x20 || c > 0x7E) return false;
    }
    return true;
}

inline void AddHeaderField(
    ParsedGameArchive& arc,
    std::wstring label,
    std::wstring offsetStr,
    uint32_t length,
    std::wstring value,
    std::wstring description
) {
    arc.headerFields.push_back(ArchiveHeaderField{
        std::move(label),
        std::move(offsetStr),
        length,
        std::move(value),
        std::move(description)
    });
}

inline std::vector<uint8_t> StringToBytes(std::string_view s) {
    return std::vector<uint8_t>(
        reinterpret_cast<const uint8_t*>(s.data()),
        reinterpret_cast<const uint8_t*>(s.data()) + s.size()
    );
}

// ============================================================================
// 1. DOOM / Quake WAD (.wad: IWAD, PWAD, WAD2, WAD3)
// ============================================================================
inline void ParseWadArchive(const std::wstring& path, BinaryReader& r, ParsedGameArchive& arc) {
    std::string magic = r.read_ascii(0, 4, false);
    int32_t numLumps = r.read_le<int32_t>(4);
    int32_t infoTableOfs = r.read_le<int32_t>(8);
    bool isQuakeWad = (magic == "WAD2" || magic == "WAD3");
    size_t entrySize = isQuakeWad ? 32 : 16;

    arc.valid = true;
    arc.formatId = ArchiveFormatId::WAD;
    arc.formatName = AsciiToWide(magic) + (isQuakeWad ? L" (Quake/GoldSrc Texture WAD)" : L" (DOOM Engine Lump Archive)");
    arc.engineName = isQuakeWad ? L"id Tech 2 / GoldSrc" : L"id Tech 1 (DOOM / Heretic / Hexen)";
    arc.isLittleEndian = true;
    arc.supportsRepack = !isQuakeWad;

    AddHeaderField(arc, L"Identification Magic", L"0x00000000", 4,
        AsciiToWide(magic) + L" (" + BytesHexPreviewW(r.slice(0, 4), 4) + L")",
        magic == "IWAD" ? L"Internal WAD (Standalone Commercial Game Archive)"
                        : (magic == "PWAD" ? L"Patch WAD (Mod / Add-on Override Archive)" : L"Quake / GoldSrc Texture WAD"));
    AddHeaderField(arc, L"Lump Count (numlumps)", L"0x00000004", 4,
        std::to_wstring(std::max(0, numLumps)) + L" lumps", L"Total number of directory entries in lump table");
    AddHeaderField(arc, L"Directory Offset (infotableofs)", L"0x00000008", 4,
        ToHexW(static_cast<uint32_t>(infoTableOfs)) + L" (" + std::to_wstring(infoTableOfs) + L" bytes)",
        L"Byte offset to start of lump directory array");

    std::wstring currentMapGroup = L"(root)";
    int32_t maxSafe = std::min<int32_t>(std::max<int32_t>(0, numLumps), 32768);

    for (int32_t i = 0; i < maxSafe; ++i) {
        size_t dirPos = static_cast<size_t>(infoTableOfs) + static_cast<size_t>(i) * entrySize;
        if (dirPos + entrySize > r.size()) break;

        uint32_t filePos = r.read_le<uint32_t>(dirPos);
        uint32_t size = r.read_le<uint32_t>(dirPos + 4);

        if (isQuakeWad) {
            uint32_t uSize = r.read_le<uint32_t>(dirPos + 8);
            uint8_t typeByte = r[dirPos + 12];
            std::string lumpName = TrimA(r.read_ascii(dirPos + 16, 16));
            if (lumpName.empty()) lumpName = "LUMP_" + std::to_string(i);
            std::wstring wName = AsciiToWide(lumpName);
            auto lumpSlice = r.slice(filePos, size);

            ArchiveEntry e;
            e.name = wName;
            e.folder = L"textures";
            e.path = L"textures/" + wName;
            e.offset = filePos;
            e.compressedSize = size;
            e.uncompressedSize = uSize ? uSize : size;
            e.compressionMethod = L"WAD2 Type " + ToHexW(typeByte, 2);
            e.crc32 = ComputeCrc32(lumpSlice);
            e.category = ClassifyAsset(wName, lumpSlice);
            e.customFlags = ExtractMode::Stored;
            arc.entries.push_back(std::move(e));
        } else {
            std::string lumpName = TrimA(r.read_ascii(dirPos + 8, 8));
            if (lumpName.empty()) lumpName = "LUMP_" + std::to_string(i);
            std::string upper = lumpName;
            for (char& c : upper) if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');

            bool isMapMarker = (upper.size() == 4 && upper[0] == 'E' && std::isdigit(static_cast<unsigned char>(upper[1])) &&
                                upper[2] == 'M' && std::isdigit(static_cast<unsigned char>(upper[3]))) ||
                               (upper.size() == 5 && upper.substr(0, 3) == "MAP" &&
                                std::isdigit(static_cast<unsigned char>(upper[3])) && std::isdigit(static_cast<unsigned char>(upper[4])));

            bool isMapLump = (upper == "THINGS" || upper == "LINEDEFS" || upper == "SIDEDEFS" ||
                              upper == "VERTEXES" || upper == "SEGS" || upper == "SSECTORS" ||
                              upper == "NODES" || upper == "SECTORS" || upper == "REJECT" || upper == "BLOCKMAP");

            if (isMapMarker) {
                currentMapGroup = AsciiToWide(upper);
            } else if (!isMapLump) {
                if (upper == "PLAYPAL" || upper == "COLORMAP" || upper == "ENDOOM") {
                    currentMapGroup = L"palettes_system";
                } else if (size == 4096) {
                    currentMapGroup = L"flats";
                } else {
                    currentMapGroup = L"lumps";
                }
            }

            auto lumpSlice = r.slice(filePos, size);
            if (upper == "PLAYPAL" && lumpSlice.size() >= 768 && arc.doomPlaypal.empty()) {
                arc.doomPlaypal.assign(lumpSlice.begin(), lumpSlice.begin() + 768);
            }

            std::wstring wName = AsciiToWide(lumpName);
            ArchiveEntry e;
            e.name = wName;
            e.folder = currentMapGroup;
            e.path = (currentMapGroup == L"(root)") ? wName : (currentMapGroup + L"/" + wName);
            e.offset = filePos;
            e.compressedSize = size;
            e.uncompressedSize = size;
            e.compressionMethod = (size == 0) ? L"Marker Lump (0 B)" : L"Uncompressed Lump";
            e.crc32 = ComputeCrc32(lumpSlice);
            e.category = isMapLump || isMapMarker ? AssetCategory::Map2D : ClassifyAsset(wName, lumpSlice);
            e.customFlags = ExtractMode::Stored;
            arc.entries.push_back(std::move(e));
        }
    }
}

// ============================================================================
// 2. Quake / Half-Life PACK (.pak: PACK)
// ============================================================================
inline void ParsePakArchive(const std::wstring& path, BinaryReader& r, ParsedGameArchive& arc) {
    uint32_t dirOffset = r.read_le<uint32_t>(4);
    uint32_t dirLength = r.read_le<uint32_t>(8);
    uint32_t numEntries = dirLength / 64;

    arc.valid = true;
    arc.formatId = ArchiveFormatId::PAK;
    arc.formatName = L"Quake / Half-Life PACK Archive";
    arc.engineName = L"id Tech 1 & 2 / GoldSrc Engine";
    arc.isLittleEndian = true;
    arc.supportsRepack = true;

    AddHeaderField(arc, L"Header Signature", L"0x00000000", 4, L"PACK (50 41 43 4B)", L"Standard id Software PACK container identifier");
    AddHeaderField(arc, L"Directory Offset", L"0x00000004", 4, ToHexW(dirOffset) + L" (" + std::to_wstring(dirOffset) + L" B)", L"Absolute byte offset to the 64-byte file table");
    AddHeaderField(arc, L"Directory Size", L"0x00000008", 4, std::to_wstring(dirLength) + L" B (" + std::to_wstring(numEntries) + L" files)", L"Byte length of file table (64 bytes per entry)");

    uint32_t maxSafe = std::min<uint32_t>(numEntries, 32768);
    for (uint32_t i = 0; i < maxSafe; ++i) {
        size_t entryOfs = static_cast<size_t>(dirOffset) + static_cast<size_t>(i) * 64;
        if (entryOfs + 64 > r.size()) break;

        std::string rawPath = r.read_cstr(entryOfs, 56);
        uint32_t filePos = r.read_le<uint32_t>(entryOfs + 56);
        uint32_t fileLen = r.read_le<uint32_t>(entryOfs + 60);

        auto sp = SplitVirtualPath(AsciiToWide(rawPath.empty() ? ("entry_" + std::to_string(i) + ".lmp") : rawPath));
        auto dataSlice = r.slice(filePos, fileLen);

        ArchiveEntry e;
        e.name = sp.name;
        e.path = sp.cleanPath;
        e.folder = sp.folder;
        e.offset = filePos;
        e.compressedSize = fileLen;
        e.uncompressedSize = fileLen;
        e.compressionMethod = L"Stored (PACK Direct)";
        e.crc32 = ComputeCrc32(dataSlice);
        e.category = ClassifyAsset(sp.cleanPath, dataSlice);
        e.customFlags = ExtractMode::Stored;
        arc.entries.push_back(std::move(e));
    }
}

// ============================================================================
// 3. Valve Source VPK (.vpk: 0x55AA1234)
// ============================================================================
inline void ParseVpkArchive(const std::wstring& path, BinaryReader& r, ParsedGameArchive& arc) {
    uint32_t version = r.read_le<uint32_t>(4);
    uint32_t treeSize = r.read_le<uint32_t>(8);
    size_t headerSize = (version == 2) ? 28 : 12;
    size_t treeEnd = std::min(r.size(), headerSize + static_cast<size_t>(treeSize));
    size_t dataBaseOffset = treeEnd;

    arc.valid = true;
    arc.formatId = ArchiveFormatId::VPK;
    arc.formatName = L"Valve Pak v" + std::to_wstring(version) + L" Archive (.vpk)";
    arc.engineName = L"Valve Source Engine (Half-Life 2, TF2, Portal 2, Left 4 Dead)";
    arc.isLittleEndian = true;
    arc.supportsRepack = false;

    AddHeaderField(arc, L"VPK Signature", L"0x00000000", 4, L"0x55AA1234 (34 12 AA 55)", L"Source Engine VPK magic identifier");
    AddHeaderField(arc, L"VPK Version & Tree Size", L"0x00000004", 8,
        L"v" + std::to_wstring(version) + L" - Directory Tree " + FormatBytesW(treeSize),
        L"3-tier Extension -> Path -> Filename tree length");

    size_t pos = headerSize;
    auto readNullStr = [&]() -> std::string {
        size_t start = pos;
        while (pos < r.size() && r[pos] != 0) ++pos;
        std::string s = r.read_ascii(start, pos - start, false);
        if (pos < r.size()) ++pos;
        return s;
    };

    size_t idx = 0;
    while (pos < treeEnd && idx < 16384) {
        std::string ext = readNullStr();
        if (ext.empty()) break;
        while (pos < treeEnd && idx < 16384) {
            std::string dirStr = readNullStr();
            if (dirStr.empty()) break;
            while (pos < treeEnd && idx < 16384) {
                std::string baseName = readNullStr();
                if (baseName.empty()) break;
                if (pos + 18 > r.size()) break;

                uint32_t crc = r.read_le<uint32_t>(pos);
                uint16_t preloadBytes = r.read_le<uint16_t>(pos + 4);
                uint16_t archiveIndex = r.read_le<uint16_t>(pos + 6);
                uint32_t entryOffset = r.read_le<uint32_t>(pos + 8);
                uint32_t entryLength = r.read_le<uint32_t>(pos + 12);
                pos += 18;

                size_t preloadOffset = pos;
                auto preloadSlice = r.slice(preloadOffset, preloadBytes);
                pos += preloadBytes;

                std::string cleanDir = (dirStr == " ") ? "" : TrimA(dirStr);
                std::string fileWithExt = (ext == " ") ? baseName : (baseName + "." + ext);
                std::string fullPathStr = cleanDir.empty() ? fileWithExt : (cleanDir + "/" + fileWithExt);
                auto sp = SplitVirtualPath(AsciiToWide(fullPathStr));

                ArchiveEntry e;
                e.name = sp.name;
                e.path = sp.cleanPath;
                e.folder = sp.folder;
                e.crc32 = crc;
                e.uncompressedSize = static_cast<uint64_t>(preloadBytes) + entryLength;

                if (archiveIndex == 0x7FFF) {
                    size_t absOffset = dataBaseOffset + entryOffset;
                    if (preloadBytes > 0) {
                        auto inlineSlice = r.slice(absOffset, entryLength);
                        e.inlineData.reserve(preloadSlice.size() + inlineSlice.size());
                        e.inlineData.insert(e.inlineData.end(), preloadSlice.begin(), preloadSlice.end());
                        e.inlineData.insert(e.inlineData.end(), inlineSlice.begin(), inlineSlice.end());
                        e.offset = preloadOffset;
                        e.compressedSize = e.inlineData.size();
                        e.customFlags = ExtractMode::InlineOnly;
                    } else {
                        e.offset = absOffset;
                        e.compressedSize = entryLength;
                        e.customFlags = ExtractMode::Stored;
                    }
                    e.compressionMethod = L"VPK Inline Payload";
                } else {
                    e.inlineData.assign(preloadSlice.begin(), preloadSlice.end());
                    e.offset = preloadOffset;
                    e.compressedSize = preloadBytes;
                    wchar_t pakBuf[64]{};
                    swprintf_s(pakBuf, L"VPK Dir (pak01_%03u.vpk)", archiveIndex);
                    e.compressionMethod = pakBuf;
                    e.customFlags = ExtractMode::InlineOnly;
                }
                e.category = ClassifyAsset(e.path, preloadSlice);
                arc.entries.push_back(std::move(e));
                ++idx;
            }
        }
    }
}

// ============================================================================
// 4. ZIP / PK3 / PK4 / IWD / JAR / APPX / MSIX
// ============================================================================
inline std::string ExtractXmlAttr(const std::string& xml, const std::string& tag, const std::string& attr) {
    std::string lowerXml = ToLowerA(xml);
    std::string lowerTag = "<" + ToLowerA(tag);
    size_t pos = lowerXml.find(lowerTag);
    if (pos == std::string::npos) return {};
    size_t endTag = lowerXml.find('>', pos);
    if (endTag == std::string::npos) return {};

    std::string tagSub = xml.substr(pos, endTag - pos + 1);
    std::string lowerSub = ToLowerA(tagSub);
    std::string lowerAttr = ToLowerA(attr);

    size_t attrPos = lowerSub.find(lowerAttr);
    while (attrPos != std::string::npos) {
        size_t eqPos = attrPos + lowerAttr.size();
        while (eqPos < lowerSub.size() && (lowerSub[eqPos] == ' ' || lowerSub[eqPos] == '\t')) ++eqPos;
        if (eqPos < lowerSub.size() && lowerSub[eqPos] == '=') {
            ++eqPos;
            while (eqPos < lowerSub.size() && (lowerSub[eqPos] == ' ' || lowerSub[eqPos] == '\t')) ++eqPos;
            if (eqPos < lowerSub.size() && (tagSub[eqPos] == '"' || tagSub[eqPos] == '\'')) {
                char q = tagSub[eqPos++];
                size_t closeQ = tagSub.find(q, eqPos);
                if (closeQ != std::string::npos) {
                    return tagSub.substr(eqPos, closeQ - eqPos);
                }
            }
        }
        attrPos = lowerSub.find(lowerAttr, attrPos + 1);
    }
    return {};
}

inline std::string ExtractJsonFieldSimple(const std::string& json, const std::string& key) {
    std::string pat = "\"" + key + "\"";
    size_t pos = json.find(pat);
    if (pos == std::string::npos) return {};
    size_t colon = json.find(':', pos + pat.size());
    if (colon == std::string::npos) return {};
    size_t q1 = json.find('"', colon + 1);
    if (q1 == std::string::npos) return {};
    size_t q2 = json.find('"', q1 + 1);
    if (q2 == std::string::npos) return {};
    return json.substr(q1 + 1, q2 - q1 - 1);
}

inline void ParseZipFamilyArchive(
    const std::wstring& path,
    BinaryReader& r,
    ParsedGameArchive& arc,
    ArchiveFormatId forcedId = ArchiveFormatId::Unknown
) {
    std::wstring lowerPath = ToLowerW(path);
    ArchiveFormatId fmtId = forcedId;
    if (fmtId == ArchiveFormatId::Unknown) {
        if (EndsWithW(lowerPath, L".jar")) fmtId = ArchiveFormatId::JAR;
        else if (EndsWithW(lowerPath, L".appx") || EndsWithW(lowerPath, L".appxbundle") ||
                 EndsWithW(lowerPath, L".msix") || EndsWithW(lowerPath, L".msixbundle")) fmtId = ArchiveFormatId::APPX;
        else if (EndsWithW(lowerPath, L".pk3") || EndsWithW(lowerPath, L".pk4") || EndsWithW(lowerPath, L".iwd")) fmtId = ArchiveFormatId::PK3;
        else fmtId = ArchiveFormatId::ZIP;
    }

    arc.valid = true;
    arc.formatId = fmtId;
    arc.isLittleEndian = true;
    arc.supportsRepack = (fmtId == ArchiveFormatId::ZIP || fmtId == ArchiveFormatId::PK3 || fmtId == ArchiveFormatId::JAR);

    // Locate End of Central Directory (EOCD: 0x06054B50) near end of file
    size_t eocdPos = std::string::npos;
    size_t searchStart = (r.size() > 65557) ? (r.size() - 65557) : 0;
    if (r.size() >= 22) {
        for (size_t i = r.size() - 22; ; --i) {
            if (r.read_le<uint32_t>(i) == 0x06054B50u) {
                eocdPos = i;
                break;
            }
            if (i <= searchStart) break;
        }
    }

    if (eocdPos != std::string::npos) {
        uint16_t cdCount = r.read_le<uint16_t>(eocdPos + 10);
        uint32_t cdSize = r.read_le<uint32_t>(eocdPos + 12);
        uint32_t cdOffset = r.read_le<uint32_t>(eocdPos + 16);
        (void)cdSize;

        size_t pos = cdOffset;
        for (uint16_t i = 0; i < cdCount && pos + 46 <= r.size(); ++i) {
            if (r.read_le<uint32_t>(pos) != 0x02014B50u) break;

            uint16_t method = r.read_le<uint16_t>(pos + 10);
            uint32_t crc = r.read_le<uint32_t>(pos + 16);
            uint32_t compSize = r.read_le<uint32_t>(pos + 20);
            uint32_t uncompSize = r.read_le<uint32_t>(pos + 24);
            uint16_t nameLen = r.read_le<uint16_t>(pos + 28);
            uint16_t extraLen = r.read_le<uint16_t>(pos + 30);
            uint16_t commentLen = r.read_le<uint16_t>(pos + 32);
            uint32_t localHdrOfs = r.read_le<uint32_t>(pos + 42);

            std::string rawName = r.read_ascii(pos + 46, nameLen, false);
            pos += 46 + nameLen + extraLen + commentLen;

            if (rawName.empty() || (rawName.back() == '/' && uncompSize == 0)) {
                continue;
            }

            // Compute data offset from Local File Header (0x04034B50)
            uint64_t dataOfs = localHdrOfs + 30;
            if (localHdrOfs + 30 <= r.size() && r.read_le<uint32_t>(localHdrOfs) == 0x04034B50u) {
                uint16_t lNameLen = r.read_le<uint16_t>(localHdrOfs + 26);
                uint16_t lExtraLen = r.read_le<uint16_t>(localHdrOfs + 28);
                dataOfs = static_cast<uint64_t>(localHdrOfs) + 30 + lNameLen + lExtraLen;
            }

            auto sp = SplitVirtualPath(AsciiToWide(rawName));
            ArchiveEntry e;
            e.name = sp.name;
            e.path = sp.cleanPath;
            e.folder = sp.folder;
            e.offset = dataOfs;
            e.compressedSize = compSize;
            e.uncompressedSize = uncompSize;
            e.compressionMethod = (method == 0) ? L"Stored (0)" : ((method == 8) ? L"DEFLATE (RFC 1951)" : (L"ZIP Method " + std::to_wstring(method)));
            e.crc32 = crc;
            e.category = ClassifyAsset(sp.cleanPath);
            e.customFlags = (method == 8) ? ExtractMode::RawDeflate : ExtractMode::Stored;
            arc.entries.push_back(std::move(e));
        }
    }

    // Fallback sequential Local File Header walk if EOCD was not found or empty
    if (arc.entries.empty()) {
        size_t pos = 0;
        size_t idx = 0;
        while (pos + 30 <= r.size() && idx < 16384) {
            if (r.read_le<uint32_t>(pos) != 0x04034B50u) break;
            uint16_t flags = r.read_le<uint16_t>(pos + 6);
            uint16_t method = r.read_le<uint16_t>(pos + 8);
            uint32_t crc = r.read_le<uint32_t>(pos + 14);
            uint32_t compSize = r.read_le<uint32_t>(pos + 18);
            uint32_t uncompSize = r.read_le<uint32_t>(pos + 22);
            uint16_t nameLen = r.read_le<uint16_t>(pos + 26);
            uint16_t extraLen = r.read_le<uint16_t>(pos + 28);

            std::string rawName = r.read_ascii(pos + 30, nameLen, false);
            size_t dataOfs = pos + 30 + nameLen + extraLen;

            if ((flags & 0x08) != 0 && compSize == 0 && method == 8 && dataOfs < r.size()) {
                std::vector<uint8_t> tmpOut;
                size_t consumed = 0;
                if (InflateRaw(r.slice(dataOfs, r.size() - dataOfs), tmpOut, 0, &consumed)) {
                    compSize = static_cast<uint32_t>(consumed);
                    uncompSize = static_cast<uint32_t>(tmpOut.size());
                    crc = ComputeCrc32(tmpOut);
                }
            }

            if (!rawName.empty() && !(rawName.back() == '/' && uncompSize == 0)) {
                auto sp = SplitVirtualPath(AsciiToWide(rawName));
                ArchiveEntry e;
                e.name = sp.name;
                e.path = sp.cleanPath;
                e.folder = sp.folder;
                e.offset = dataOfs;
                e.compressedSize = compSize;
                e.uncompressedSize = uncompSize;
                e.compressionMethod = (method == 0) ? L"Stored (0)" : ((method == 8) ? L"DEFLATE (RFC 1951)" : (L"ZIP Method " + std::to_wstring(method)));
                e.crc32 = crc;
                e.category = ClassifyAsset(sp.cleanPath);
                e.customFlags = (method == 8) ? ExtractMode::RawDeflate : ExtractMode::Stored;
                arc.entries.push_back(std::move(e));
            }

            if (compSize == 0 && (flags & 0x08) != 0) break;
            pos = dataOfs + compSize;
            ++idx;
        }
    }

    auto extractEntryText = [&](const ArchiveEntry& e) -> std::string {
        auto rawSlice = r.slice(static_cast<size_t>(e.offset), static_cast<size_t>(e.compressedSize));
        if ((e.customFlags & 0xFF) == ExtractMode::RawDeflate) {
            std::vector<uint8_t> dec;
            if (InflateRaw(rawSlice, dec, static_cast<size_t>(e.uncompressedSize))) {
                return std::string(reinterpret_cast<const char*>(dec.data()), dec.size());
            }
            return {};
        }
        return std::string(reinterpret_cast<const char*>(rawSlice.data()), rawSlice.size());
    };

    if (fmtId == ArchiveFormatId::JAR) {
        std::wstring modloader = L"Vanilla / Plain Java Library";
        std::wstring modId = L"unknown";
        std::wstring modName = ExtractFileNameW(path);
        std::wstring modVersion = L"1.0";
        size_t classCount = 0;
        size_t assetCount = 0;

        for (auto& e : arc.entries) {
            std::wstring lp = ToLowerW(e.path);
            if (EndsWithW(lp, L".class")) {
                e.category = AssetCategory::Code;
                ++classCount;
            } else if (lp.rfind(L"assets/", 0) == 0) {
                ++assetCount;
            }
            if (lp == L"fabric.mod.json") {
                modloader = L"Fabric Mod";
                std::string json = extractEntryText(e);
                std::string id = ExtractJsonFieldSimple(json, "id");
                std::string nm = ExtractJsonFieldSimple(json, "name");
                std::string vr = ExtractJsonFieldSimple(json, "version");
                if (!id.empty()) modId = AsciiToWide(id);
                if (!nm.empty()) modName = AsciiToWide(nm);
                if (!vr.empty()) modVersion = AsciiToWide(vr);
            } else if (lp == L"quilt.mod.json") {
                modloader = L"Quilt Mod";
            } else if (lp == L"meta-inf/mods.toml" || lp == L"mods.toml") {
                if (modloader == L"Vanilla / Plain Java Library") modloader = L"Forge / NeoForge Mod";
            } else if (lp == L"mcmod.info") {
                if (modloader == L"Vanilla / Plain Java Library") modloader = L"Legacy Minecraft Forge";
            }
        }

        arc.formatName = L"Minecraft Mod JAR (" + modloader + L")";
        arc.engineName = L"Minecraft Java Edition (Mojang / Forge / Fabric)";
        AddHeaderField(arc, L"Container", L"0x00000000", 4, L"Java JAR / ZIP (PK\\x03\\x04)", L"Java Archive Container");
        AddHeaderField(arc, L"Modloader Target", L"META-INF", 0, modloader, L"Mod ecosystem runtime environment");
        AddHeaderField(arc, L"Mod ID & Name", L"Metadata", 0, modName + L" (" + modId + L") v" + modVersion, L"Identified manifest descriptor");
        AddHeaderField(arc, L"Class & Asset Count", L"JVM", 0, std::to_wstring(classCount) + L" classes, " + std::to_wstring(assetCount) + L" assets", L"Compiled JVM bytecode and resources");
    } else if (fmtId == ArchiveFormatId::APPX) {
        bool isBundle = EndsWithW(lowerPath, L".appxbundle") || EndsWithW(lowerPath, L".msixbundle");
        bool isMsix = EndsWithW(lowerPath, L".msix") || EndsWithW(lowerPath, L".msixbundle");
        arc.formatName = isBundle ? (isMsix ? L"MSIXBundle (Xbox / Windows GDK Bundle)" : L"AppXBundle (Xbox One / Series / UWP Bundle)")
                                  : (isMsix ? L"MSIX (Windows / Xbox GDK Package)" : L"AppX (Xbox One / Series / UWP Package)");
        arc.engineName = L"Microsoft Gaming Development Kit (GDK / Xbox / UWP)";

        AddHeaderField(arc, L"Package Container Format", L"0x00000000", 4, arc.formatName, L"Open Packaging Conventions (OPC) Deflate ZIP container");

        for (const auto& e : arc.entries) {
            std::wstring lp = ToLowerW(e.path);
            if (EndsWithW(lp, L"microsoftgame.config")) {
                std::string xml = extractEntryText(e);
                std::string titleId = ExtractXmlAttr(xml, "Game", "TitleId");
                std::string dispName = ExtractXmlAttr(xml, "ShellVisuals", "DefaultDisplayName");
                if (!dispName.empty()) AddHeaderField(arc, L"GDK Game Title", L"MicrosoftGame.config", static_cast<uint32_t>(dispName.size()), AsciiToWide(dispName), L"Xbox GDK title display name");
                if (!titleId.empty()) AddHeaderField(arc, L"Xbox Title ID", L"MicrosoftGame.config", static_cast<uint32_t>(titleId.size()), L"0x" + AsciiToWide(titleId), L"Xbox One / Series Title ID");
            } else if (EndsWithW(lp, L"appxmanifest.xml")) {
                std::string xml = extractEntryText(e);
                std::string pkgName = ExtractXmlAttr(xml, "Identity", "Name");
                std::string pkgVer = ExtractXmlAttr(xml, "Identity", "Version");
                std::string arch = ExtractXmlAttr(xml, "Identity", "ProcessorArchitecture");
                if (!pkgName.empty()) AddHeaderField(arc, L"Package Identity Name", L"AppxManifest.xml", static_cast<uint32_t>(pkgName.size()), AsciiToWide(pkgName), L"Full UWP / GDK Package identity");
                if (!pkgVer.empty()) AddHeaderField(arc, L"Package Version", L"AppxManifest.xml", static_cast<uint32_t>(pkgVer.size()), L"v" + AsciiToWide(pkgVer), L"Semantic package version");
                if (!arch.empty()) AddHeaderField(arc, L"Processor Architecture", L"AppxManifest.xml", static_cast<uint32_t>(arch.size()), AsciiToWide(arch), L"Target CPU architecture");
            }
        }
    } else {
        bool isPk3 = (fmtId == ArchiveFormatId::PK3);
        arc.formatName = isPk3 ? L"id Tech 3 / 4 Package (.PK3 / .IWD)" : L"ZIP Game Archive";
        arc.engineName = isPk3 ? L"id Tech 3 / ioquake3 / IW 2.0" : L"Universal Deflate Archive";
        AddHeaderField(arc, L"Local File Header Signature", L"0x00000000", 4, L"PK\\x03\\x04 (0x04034B50)", L"PKWARE / Virtual File System signature");
        AddHeaderField(arc, L"Extracted File Count", L"Central Dir", 4, std::to_wstring(arc.entries.size()) + L" files", L"Total indexed files in virtual hierarchy");
    }
}

// ============================================================================
// 5. RAR v4 & v5 Block Walker (.rar)
// ============================================================================
inline uint64_t ReadRar5Vint(BinaryReader& r, size_t& pos) {
    uint64_t result = 0;
    int shift = 0;
    while (pos < r.size() && shift < 64) {
        uint8_t b = r[pos++];
        result |= static_cast<uint64_t>(b & 0x7F) << shift;
        if ((b & 0x80) == 0) break;
        shift += 7;
    }
    return result;
}

inline void ParseRarArchive(const std::wstring& path, BinaryReader& r, ParsedGameArchive& arc) {
    bool isRar5 = (r.size() >= 8 && r[6] == 0x01 && r[7] == 0x00);
    arc.valid = true;
    arc.formatId = ArchiveFormatId::RAR;
    arc.formatName = isRar5 ? L"RAR v5.0 Archive Container" : L"RAR v4.x Archive Container";
    arc.engineName = L"Roshal Archive (Game Mod / Distribution Package)";
    arc.isLittleEndian = true;
    arc.supportsRepack = false;

    if (!isRar5) {
        size_t pos = 7;
        size_t idx = 0;
        while (pos + 7 <= r.size() && idx < 4096) {
            uint8_t headType = r[pos + 2];
            uint16_t headFlags = r.read_le<uint16_t>(pos + 3);
            uint16_t headSize = r.read_le<uint16_t>(pos + 5);
            if (headSize < 7 || pos + headSize > r.size()) break;

            uint32_t addSize = 0;
            if ((headFlags & 0x8000) != 0 && pos + 11 <= r.size()) {
                addSize = r.read_le<uint32_t>(pos + 7);
            }

            if (headType == 0x74 && pos + 32 <= r.size()) {
                uint32_t packSize = r.read_le<uint32_t>(pos + 7);
                uint32_t unpSize = r.read_le<uint32_t>(pos + 11);
                uint32_t fileCrc = r.read_le<uint32_t>(pos + 16);
                uint8_t method = r[pos + 25];
                uint16_t nameSize = r.read_le<uint16_t>(pos + 26);
                size_t maxName = (headSize > 32) ? std::min<size_t>(nameSize, headSize - 32) : 0;
                std::string rawName = r.read_ascii(pos + 32, maxName, true);
                size_t dataOffset = pos + headSize;

                auto sp = SplitVirtualPath(AsciiToWide(rawName.empty() ? ("rar_entry_" + std::to_string(idx) + ".bin") : rawName));
                ArchiveEntry e;
                e.name = sp.name;
                e.path = sp.cleanPath;
                e.folder = sp.folder;
                e.offset = dataOffset;
                e.compressedSize = packSize;
                e.uncompressedSize = unpSize;
                e.compressionMethod = (method == 0x30) ? L"RAR v4 Store (0x30)" : (L"RAR v4 Method " + ToHexW(method, 2));
                e.crc32 = fileCrc;
                e.category = ClassifyAsset(sp.cleanPath, r.slice(dataOffset, std::min<size_t>(packSize, 256)));
                e.customFlags = ExtractMode::Stored;
                arc.entries.push_back(std::move(e));

                pos = dataOffset + packSize;
                ++idx;
                continue;
            }
            pos += headSize + addSize;
        }
    } else {
        size_t pos = 8;
        size_t idx = 0;
        while (pos + 7 <= r.size() && idx < 4096) {
            pos += 4; // header CRC32
            uint64_t hdrSize = ReadRar5Vint(r, pos);
            if (hdrSize == 0 || pos + hdrSize > r.size()) break;
            size_t hdrStart = pos;
            size_t hdrEnd = pos + static_cast<size_t>(hdrSize);

            uint64_t hdrType = ReadRar5Vint(r, pos);
            uint64_t hdrFlags = ReadRar5Vint(r, pos);
            uint64_t extraSize = ((hdrFlags & 0x01) != 0) ? ReadRar5Vint(r, pos) : 0;
            uint64_t dataSize = ((hdrFlags & 0x02) != 0) ? ReadRar5Vint(r, pos) : 0;
            (void)extraSize;

            if (hdrType == 2 && pos < hdrEnd) { // File header
                uint64_t fileFlags = ReadRar5Vint(r, pos);
                uint64_t unpSize = ReadRar5Vint(r, pos);
                uint64_t attrs = ReadRar5Vint(r, pos);
                (void)attrs;
                if ((fileFlags & 0x02) != 0 && pos + 4 <= hdrEnd) pos += 4; // mtime
                uint32_t fileCrc = 0;
                if ((fileFlags & 0x04) != 0 && pos + 4 <= hdrEnd) {
                    fileCrc = r.read_le<uint32_t>(pos);
                    pos += 4;
                }
                uint64_t compInfo = ReadRar5Vint(r, pos);
                uint64_t hostOs = ReadRar5Vint(r, pos);
                (void)hostOs;
                uint64_t nameLen = ReadRar5Vint(r, pos);
                std::string rawName;
                if (pos + nameLen <= hdrEnd) {
                    rawName = r.read_ascii(pos, static_cast<size_t>(nameLen), false);
                }

                if ((fileFlags & 0x01) == 0) { // not a directory
                    auto sp = SplitVirtualPath(AsciiToWide(rawName.empty() ? ("rar5_entry_" + std::to_string(idx) + ".bin") : rawName));
                    ArchiveEntry e;
                    e.name = sp.name;
                    e.path = sp.cleanPath;
                    e.folder = sp.folder;
                    e.offset = hdrEnd;
                    e.compressedSize = dataSize;
                    e.uncompressedSize = unpSize;
                    uint64_t method = (compInfo >> 7) & 0x07;
                    e.compressionMethod = (method == 0) ? L"RAR v5 Store" : (L"RAR v5 Method " + std::to_wstring(method));
                    e.crc32 = fileCrc;
                    e.category = ClassifyAsset(sp.cleanPath, r.slice(hdrEnd, std::min<size_t>(static_cast<size_t>(dataSize), 256)));
                    e.customFlags = ExtractMode::Stored;
                    arc.entries.push_back(std::move(e));
                    ++idx;
                }
            }
            pos = hdrStart + static_cast<size_t>(hdrSize) + static_cast<size_t>(dataSize);
        }
    }

    AddHeaderField(arc, L"RAR Signature Marker", L"0x00000000", isRar5 ? 8 : 7,
        isRar5 ? L"Rar!\\x1A\\x07\\x01\\x00 (v5.0)" : L"Rar!\\x1A\\x07\\x00 (v4.x)", L"RAR magic marker block");
    AddHeaderField(arc, L"Parsed Block Entries", L"0x00000007", static_cast<uint32_t>(r.size() > 7 ? r.size() - 7 : 0),
        std::to_wstring(arc.entries.size()) + L" file blocks", L"File headers and payload blocks discovered in stream");
}

// ============================================================================
// 6. ISO 9660 & PlayStation 1 Raw 2352-byte CD-XA (.iso, .bin)
// ============================================================================
inline std::vector<uint8_t> ReadPs1Sectors(
    BinaryReader& r,
    uint32_t lba,
    uint32_t byteCount,
    bool isRaw2352,
    uint32_t sectorSize,
    uint32_t payloadOffsetInSector
) {
    if (!isRaw2352) {
        size_t start = static_cast<size_t>(lba) * 2048;
        return r.copy_slice(start, byteCount);
    }
    constexpr uint32_t kPayloadPerSec = 2048;
    uint32_t sectorsNeeded = (byteCount + kPayloadPerSec - 1) / kPayloadPerSec;
    std::vector<uint8_t> out(byteCount, 0);
    uint32_t written = 0;
    for (uint32_t s = 0; s < sectorsNeeded; ++s) {
        size_t secStart = static_cast<size_t>(lba + s) * sectorSize + payloadOffsetInSector;
        uint32_t take = std::min<uint32_t>(kPayloadPerSec, byteCount - written);
        if (secStart + take <= r.size()) {
            std::memcpy(out.data() + written, r.data() + secStart, take);
        }
        written += take;
    }
    return out;
}

inline void ParseIsoOrPs1Archive(const std::wstring& path, BinaryReader& r, ParsedGameArchive& arc, bool forcePs1 = false) {
    bool isRaw2352 = false;
    uint32_t sectorSize = 2048;
    size_t pvdOffset = 0x8000;
    uint32_t dataPayloadOffset = 0;

    bool hasSync0 = (r.size() >= 12 && r[0] == 0x00 && r[1] == 0xFF && r[2] == 0xFF && r[11] == 0x00);
    bool hasSync16 = (r.size() >= 16 * 2352 + 12 && r[16 * 2352] == 0x00 && r[16 * 2352 + 1] == 0xFF &&
                      r[16 * 2352 + 2] == 0xFF && r[16 * 2352 + 11] == 0x00);

    if (hasSync0 || hasSync16) {
        isRaw2352 = true;
        sectorSize = 2352;
        size_t sec16 = 16 * 2352;
        if (sec16 + 24 + 6 <= r.size() && r.read_ascii(sec16 + 24 + 1, 5, false) == "CD001") {
            dataPayloadOffset = 24;
            pvdOffset = sec16 + 24;
        } else if (sec16 + 16 + 6 <= r.size() && r.read_ascii(sec16 + 16 + 1, 5, false) == "CD001") {
            dataPayloadOffset = 16;
            pvdOffset = sec16 + 16;
        } else {
            dataPayloadOffset = 24;
            pvdOffset = sec16 + 24;
        }
    }

    std::string sysId = TrimA(r.read_ascii(pvdOffset + 8, 32));
    std::string volId = TrimA(r.read_ascii(pvdOffset + 40, 32));
    uint32_t volSpaceSize = r.read_le<uint32_t>(pvdOffset + 80);
    uint16_t blockSize = r.read_le<uint16_t>(pvdOffset + 128);
    if (blockSize == 0) blockSize = 2048;

    size_t rootRecordOffset = pvdOffset + 156;
    uint32_t rootExtentLba = r.read_le<uint32_t>(rootRecordOffset + 2);
    uint32_t rootDataLen = r.read_le<uint32_t>(rootRecordOffset + 10);

    bool isPs1 = forcePs1 || isRaw2352 || (sysId.find("PLAYSTATION") != std::string::npos);

    std::string bootExe;
    std::string gameSerial;
    std::string gameRegion;
    size_t entryCounter = 0;

    auto walkDir = [&](auto& self, uint32_t extentLba, uint32_t dataLen, const std::string& folderPrefix, int depth) -> void {
        if (depth > 8 || entryCounter >= 4000) return;
        uint32_t safeLen = std::min<uint32_t>(dataLen, 65536);
        std::vector<uint8_t> dirBytes = ReadPs1Sectors(r, extentLba, safeLen, isRaw2352, sectorSize, dataPayloadOffset);
        BinaryReader dr(dirBytes);
        size_t pos = 0;

        while (pos < dr.size() && entryCounter < 4000) {
            uint8_t recordLen = dr[pos];
            if (recordLen == 0) {
                size_t nextSec = ((pos / 2048) + 1) * 2048;
                if (nextSec <= pos) break;
                pos = nextSec;
                continue;
            }
            if (pos + recordLen > dr.size()) break;

            uint32_t fileLba = dr.read_le<uint32_t>(pos + 2);
            uint32_t fileSize = dr.read_le<uint32_t>(pos + 10);
            uint8_t flags = dr[pos + 25];
            uint8_t nameLen = dr[pos + 32];
            if (nameLen == 1 && (dr[pos + 33] == 0 || dr[pos + 33] == 1)) {
                pos += recordLen;
                continue;
            }

            std::string rawName = dr.read_ascii(pos + 33, nameLen, true);
            size_t semi = rawName.find(';');
            if (semi != std::string::npos) rawName.erase(semi);

            bool isDir = (flags & 0x02) != 0;
            if (isDir) {
                std::string subFolder = folderPrefix.empty() ? rawName : (folderPrefix + "/" + rawName);
                if (fileLba != extentLba && fileLba > 0) {
                    self(self, fileLba, fileSize, subFolder, depth + 1);
                }
            } else {
                std::string fullPath = folderPrefix.empty() ? rawName : (folderPrefix + "/" + rawName);
                std::string upperName = rawName;
                for (char& c : upperName) if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');

                if (upperName == "SYSTEM.CNF" && fileSize > 0 && fileSize <= 4096) {
                    isPs1 = true;
                    std::vector<uint8_t> cnfBytes = ReadPs1Sectors(r, fileLba, fileSize, isRaw2352, sectorSize, dataPayloadOffset);
                    std::string cnfText(reinterpret_cast<const char*>(cnfBytes.data()), cnfBytes.size());
                    size_t bootPos = cnfText.find("BOOT");
                    if (bootPos != std::string::npos) {
                        size_t colonPos = cnfText.find(':', bootPos);
                        if (colonPos != std::string::npos) {
                            size_t startExe = colonPos + 1;
                            while (startExe < cnfText.size() && (cnfText[startExe] == '\\' || cnfText[startExe] == '/' || cnfText[startExe] == ' ')) ++startExe;
                            size_t endExe = startExe;
                            while (endExe < cnfText.size() && cnfText[endExe] != ';' && cnfText[endExe] != '\r' && cnfText[endExe] != '\n') ++endExe;
                            bootExe = TrimA(cnfText.substr(startExe, endExe - startExe));
                            gameSerial = bootExe;
                            for (char& c : gameSerial) if (c == '_') c = '-';
                            if (bootExe.find("SLUS") != std::string::npos || bootExe.find("SCUS") != std::string::npos) {
                                gameRegion = "North America (NTSC-U)";
                            } else if (bootExe.find("SLES") != std::string::npos || bootExe.find("SCES") != std::string::npos) {
                                gameRegion = "Europe (PAL)";
                            } else if (bootExe.find("SLPS") != std::string::npos || bootExe.find("SLPM") != std::string::npos || bootExe.find("SCPS") != std::string::npos) {
                                gameRegion = "Japan (NTSC-J)";
                            }
                        }
                    }
                }

                auto sp = SplitVirtualPath(AsciiToWide(fullPath));
                ArchiveEntry e;
                e.name = sp.name;
                e.path = sp.cleanPath;
                e.folder = sp.folder;
                e.compressedSize = fileSize;
                e.uncompressedSize = fileSize;

                size_t rawByteStart = static_cast<size_t>(fileLba) * sectorSize + dataPayloadOffset;
                auto sampleSlice = r.slice(rawByteStart, std::min<size_t>(fileSize, 64));

                if (isRaw2352) {
                    e.offset = fileLba; // store LBA for sector extractor
                    e.customFlags = (dataPayloadOffset == 16) ? ExtractMode::Ps1Raw2352Mode1 : ExtractMode::Ps1Raw2352Mode2;
                    e.compressionMethod = L"PS1 CD-XA Raw (LBA " + std::to_wstring(fileLba) + L")";
                } else {
                    e.offset = static_cast<uint64_t>(fileLba) * blockSize;
                    e.customFlags = ExtractMode::Stored;
                    e.compressionMethod = L"ISO 9660 LBA " + std::to_wstring(fileLba);
                }

                e.crc32 = ComputeCrc32(sampleSlice);
                e.category = ClassifyAsset(sp.cleanPath, sampleSlice);
                arc.entries.push_back(std::move(e));
                ++entryCounter;
            }
            pos += recordLen;
        }
    };

    walkDir(walkDir, rootExtentLba, rootDataLen, "", 0);

    arc.valid = true;
    arc.isLittleEndian = true;
    arc.supportsRepack = false;

    if (isPs1) {
        arc.formatId = ArchiveFormatId::PS1;
        std::string label = !gameSerial.empty() ? gameSerial : (!volId.empty() ? volId : "PS1_GAME");
        arc.formatName = L"PlayStation 1 Disc Image (" + AsciiToWide(label) + L")";
        arc.engineName = !gameRegion.empty() ? (L"Sony PlayStation 1 - " + AsciiToWide(gameRegion)) : L"Sony PlayStation 1 (CD-XA / ISO 9660)";
        AddHeaderField(arc, L"PS1 Disc Sector Format", L"0x00000000", isRaw2352 ? 2352 : 2048,
            isRaw2352 ? L"Raw CD-XA Mode 2 Form 1 (2352 Bytes/Sector)" : L"Standard ISO 9660 Mode 1 (2048 Bytes)",
            L"PlayStation 1 optical media format");
        AddHeaderField(arc, L"System Identifier", ToHexW(pvdOffset + 8), 32, AsciiToWide(sysId.empty() ? "PLAYSTATION" : sysId), L"ISO9660 system identifier");
        AddHeaderField(arc, L"Volume Identifier", ToHexW(pvdOffset + 40), 32, AsciiToWide(volId.empty() ? "PS1_DISC" : volId), L"Disc volume label");
        AddHeaderField(arc, L"Boot Executable (SYSTEM.CNF)", L"SYSTEM.CNF", static_cast<uint32_t>(bootExe.size()),
            bootExe.empty() ? L"No SYSTEM.CNF found" : AsciiToWide(bootExe),
            gameRegion.empty() ? L"Boot configuration" : (L"Detected Region: " + AsciiToWide(gameRegion)));
    } else {
        arc.formatId = ArchiveFormatId::ISO;
        arc.formatName = L"ISO 9660 Optical Disc (" + AsciiToWide(volId.empty() ? "GAME_DISC" : volId) + L")";
        arc.engineName = sysId.empty() ? L"PlayStation / Console / PC CD-ROM" : (L"Optical Disc (" + AsciiToWide(sysId) + L")");
        AddHeaderField(arc, L"Primary Volume Descriptor", L"0x00008000", 7, L"01 \"CD001\" 01 (Sector 16)", L"ECMA-119 / ISO 9660 Primary Volume Descriptor");
        AddHeaderField(arc, L"System & Volume ID", L"0x00008008", 64, AsciiToWide(sysId + " / " + volId), L"Target hardware identifier and disc label");
        AddHeaderField(arc, L"Logical Block Geometry", L"0x00008050", 8, std::to_wstring(volSpaceSize) + L" sectors x " + std::to_wstring(blockSize) + L" B", L"Total optical volume space");
    }
}

// ============================================================================
// 7. Yakuza PARC (.par: PARC) & EA BIG (.big: BIGF / BIG4) & Bink (.bik/.bk2)
// ============================================================================
inline void ParseParArchive(const std::wstring& path, BinaryReader& r, ParsedGameArchive& arc) {
    uint32_t version = r.read_le<uint32_t>(4);
    uint32_t fileCount = r.read_le<uint32_t>(8);
    uint32_t dirOffset = r.read_le<uint32_t>(12);

    arc.valid = true;
    arc.formatId = ArchiveFormatId::PAR;
    arc.formatName = L"Ryu Ga Gotoku PARC Archive (.par)";
    arc.engineName = L"Sega Dragon Engine / RGG Studio";
    arc.isLittleEndian = true;
    arc.supportsRepack = false;

    uint32_t safeCount = std::min<uint32_t>(fileCount, 4096);
    AddHeaderField(arc, L"Magic Signature", L"0x00000000", 4, L"PARC (50 41 52 43)", L"PARC archive container signature");
    AddHeaderField(arc, L"Container Version", L"0x00000004", 4, ToHexW(version), L"PARC format revision");
    AddHeaderField(arc, L"Entry Table", ToHexW(dirOffset), safeCount * 64, std::to_wstring(fileCount) + L" entries at " + ToHexW(dirOffset), L"File descriptor table");

    for (uint32_t i = 0; i < safeCount; ++i) {
        size_t recOffset = static_cast<size_t>(dirOffset) + static_cast<size_t>(i) * 64;
        if (recOffset + 64 > r.size()) break;

        std::string rawPath = r.read_cstr(recOffset, 48);
        uint32_t dataOffset = r.read_le<uint32_t>(recOffset + 48);
        uint32_t compSize = r.read_le<uint32_t>(recOffset + 52);
        uint32_t uncompSize = r.read_le<uint32_t>(recOffset + 56);
        bool isCompressed = (r.read_le<uint32_t>(recOffset + 60) == 1);

        auto sp = SplitVirtualPath(AsciiToWide(rawPath.empty() ? ("entry_" + std::to_string(i) + ".bin") : rawPath));
        auto rawSlice = r.slice(dataOffset, compSize);

        ArchiveEntry e;
        e.name = sp.name;
        e.path = sp.cleanPath;
        e.folder = sp.folder;
        e.offset = dataOffset;
        e.compressedSize = compSize;
        e.uncompressedSize = uncompSize ? uncompSize : compSize;
        e.compressionMethod = isCompressed ? L"PARC Zlib Deflate" : L"PARC Stored";
        e.crc32 = ComputeCrc32(rawSlice);
        e.category = ClassifyAsset(sp.cleanPath, rawSlice);
        e.customFlags = isCompressed ? ExtractMode::ZlibStream : ExtractMode::Stored;
        arc.entries.push_back(std::move(e));
    }
}

inline void ParseBigArchive(const std::wstring& path, BinaryReader& r, ParsedGameArchive& arc) {
    std::string magic = r.read_ascii(0, 4, false);
    uint32_t archiveLen = r.read_le<uint32_t>(4);
    uint32_t numFiles = r.read_be<uint32_t>(8);
    uint32_t firstDataOffset = r.read_be<uint32_t>(12);

    arc.valid = true;
    arc.formatId = ArchiveFormatId::BIG;
    arc.formatName = L"Electronic Arts " + AsciiToWide(magic) + L" Archive";
    arc.engineName = L"SAGE Engine / EAGL (C&C Generals, BFME, NFS)";
    arc.isLittleEndian = false;
    arc.supportsRepack = false;

    AddHeaderField(arc, L"Signature", L"0x00000000", 4, AsciiToWide(magic), L"EA BIGF / BIG4 Master Archive Identifier");
    AddHeaderField(arc, L"Archive Size (LE)", L"0x00000004", 4, std::to_wstring(archiveLen) + L" B", L"Total archive length recorded in header");
    AddHeaderField(arc, L"File Count & Data Start (BE)", L"0x00000008", 8, std::to_wstring(numFiles) + L" files - Data starts at " + ToHexW(firstDataOffset), L"Big-endian directory count");

    size_t pos = 16;
    uint32_t safeCount = std::min<uint32_t>(numFiles, 8192);
    for (uint32_t i = 0; i < safeCount && pos + 9 <= r.size(); ++i) {
        uint32_t entryOffset = r.read_be<uint32_t>(pos);
        uint32_t entrySize = r.read_be<uint32_t>(pos + 4);
        pos += 8;
        size_t nameEnd = pos;
        while (nameEnd < r.size() && r[nameEnd] != 0) ++nameEnd;
        std::string rawPath = r.read_ascii(pos, nameEnd - pos, false);
        pos = nameEnd + 1;

        auto sp = SplitVirtualPath(AsciiToWide(rawPath.empty() ? ("asset_" + std::to_string(i) + ".bin") : rawPath));
        auto dataSlice = r.slice(entryOffset, entrySize);

        ArchiveEntry e;
        e.name = sp.name;
        e.path = sp.cleanPath;
        e.folder = sp.folder;
        e.offset = entryOffset;
        e.compressedSize = entrySize;
        e.uncompressedSize = entrySize;
        e.compressionMethod = AsciiToWide(magic) + L" Big-Endian Stream";
        e.crc32 = ComputeCrc32(dataSlice);
        e.category = ClassifyAsset(sp.cleanPath, dataSlice);
        e.customFlags = ExtractMode::Stored;
        arc.entries.push_back(std::move(e));
    }
}

inline void ParseBinkArchive(const std::wstring& path, BinaryReader& r, ParsedGameArchive& arc) {
    std::string magic = r.read_ascii(0, 4, false);
    bool isBink2 = (magic.rfind("KB2", 0) == 0);
    uint32_t numFrames = r.read_le<uint32_t>(8);
    uint32_t largestFrame = r.read_le<uint32_t>(12);
    uint32_t width = r.read_le<uint32_t>(20);
    uint32_t height = r.read_le<uint32_t>(24);
    uint32_t fpsNum = r.read_le<uint32_t>(28);
    uint32_t fpsDen = r.read_le<uint32_t>(32);
    uint32_t numAudioTracks = r.read_le<uint32_t>(40);

    double fps = (fpsDen > 0) ? (static_cast<double>(fpsNum) / static_cast<double>(fpsDen)) : 30.0;
    double durationSec = (fps > 0.0) ? (static_cast<double>(numFrames) / fps) : 0.0;

    arc.valid = true;
    arc.formatId = ArchiveFormatId::BIK;
    wchar_t fmtBuf[128]{};
    swprintf_s(fmtBuf, L"%s Video (%ux%u @ %.1ffps)", isBink2 ? L"Bink 2" : L"Bink 1", width, height, fps);
    arc.formatName = fmtBuf;
    arc.engineName = L"RAD Game Tools (Call of Duty, DOOM 2016, Unreal, Source)";
    arc.isLittleEndian = true;
    arc.supportsRepack = false;

    AddHeaderField(arc, L"Bink Video Signature", L"0x00000000", 4, AsciiToWide(magic) + (isBink2 ? L" (Bink 2)" : L" (Bink 1)"), L"RAD Game Tools Bink Video Codec identifier");
    AddHeaderField(arc, L"Video Resolution", L"0x00000014", 8, std::to_wstring(width) + L" x " + std::to_wstring(height), L"Native video display dimensions");
    AddHeaderField(arc, L"Frame Rate & Frames", L"0x0000001C", 8, std::to_wstring(numFrames) + L" frames", L"Total video frames in stream");
    AddHeaderField(arc, L"Audio Tracks", L"0x00000028", 4, std::to_wstring(numAudioTracks) + L" track(s)", L"Embedded multichannel Bink audio streams");
    AddHeaderField(arc, L"Largest Frame Size", L"0x0000000C", 4, std::to_wstring(largestFrame) + L" bytes", L"Peak keyframe video buffer requirement");

    size_t p = 44;
    std::string report = "// RAD Game Tools Bink Video Inspection Report\n";
    report += "// Container: " + magic + "\n";
    report += "// Resolution: " + std::to_string(width) + " x " + std::to_string(height) + "\n";
    report += "// Total Frames: " + std::to_string(numFrames) + " (Duration: " + std::to_string(static_cast<int>(durationSec)) + "s)\n";
    report += "// Audio Tracks: " + std::to_string(numAudioTracks) + "\n\n=== AUDIO TRACKS ===\n";

    if (numAudioTracks > 0 && numAudioTracks < 32 && p + numAudioTracks * 4 <= r.size()) {
        for (uint32_t a = 0; a < numAudioTracks; ++a) {
            uint32_t trackFlags = r.read_le<uint32_t>(p);
            p += 4;
            bool stereo = (trackFlags & 0x2000) != 0;
            uint32_t sr = trackFlags & 0x00FFFFFF;
            if (sr < 8000 || sr > 192000) sr = 44100;
            report += "[Track #" + std::to_string(a) + "] " + (stereo ? "Stereo" : "Mono") + " - " + std::to_string(sr) + " Hz\n";
        }
    }

    report += "\n=== FIRST 100 VIDEO FRAMES (TIMELINE SEEK TABLE) ===\n";
    uint32_t maxFrames = std::min<uint32_t>(numFrames, 100);
    if (p + (static_cast<size_t>(maxFrames) + 1) * 4 <= r.size()) {
        uint32_t prevOffset = r.read_le<uint32_t>(p) & ~1u;
        bool prevKey = (r.read_le<uint32_t>(p) & 1u) != 0;
        for (uint32_t f = 1; f <= maxFrames; ++f) {
            uint32_t rawOff = r.read_le<uint32_t>(p + f * 4);
            uint32_t off = rawOff & ~1u;
            uint32_t fSize = (off >= prevOffset) ? (off - prevOffset) : 0;
            char lineBuf[160]{};
            snprintf(lineBuf, sizeof(lineBuf), "Frame #%04u | Offset: 0x%08X | Size: %7u B | %s\n",
                f - 1, prevOffset, fSize, prevKey ? "[KEYFRAME]" : "[P-FRAME]");
            report += lineBuf;
            prevOffset = off;
            prevKey = (rawOff & 1u) != 0;
        }
    }

    std::wstring baseName = ExtractFileNameW(path);
    ArchiveEntry repEntry;
    repEntry.name = baseName + L"_timeline.txt";
    repEntry.folder = L"metadata";
    repEntry.path = L"metadata/" + repEntry.name;
    repEntry.inlineData = StringToBytes(report);
    repEntry.compressedSize = repEntry.inlineData.size();
    repEntry.uncompressedSize = repEntry.inlineData.size();
    repEntry.compressionMethod = L"Decoded Bink Seek Table";
    repEntry.crc32 = ComputeCrc32(repEntry.inlineData);
    repEntry.category = AssetCategory::Config;
    repEntry.customFlags = ExtractMode::InlineOnly;
    arc.entries.push_back(std::move(repEntry));

    ArchiveEntry vidEntry;
    vidEntry.name = baseName;
    vidEntry.folder = L"(root)";
    vidEntry.path = baseName;
    vidEntry.offset = 0;
    vidEntry.compressedSize = r.size();
    vidEntry.uncompressedSize = r.size();
    vidEntry.compressionMethod = arc.formatName;
    vidEntry.crc32 = ComputeCrc32(r.slice(0, std::min<size_t>(r.size(), 65536)));
    vidEntry.category = AssetCategory::Video;
    vidEntry.customFlags = ExtractMode::Stored;
    arc.entries.push_back(std::move(vidEntry));
}

// ============================================================================
// 8. Call of Duty FastFile (.ff: TAffu100, IWffu100, S1ffu100)
// ============================================================================
inline bool IsInlinePtr64(std::span<const uint8_t> z, size_t offset) {
    if (offset + 8 > z.size()) return false;
    return z[offset] == 0xFF && z[offset + 1] == 0xFF && z[offset + 2] == 0xFF && z[offset + 3] == 0xFF &&
           z[offset + 4] == 0x00 && z[offset + 5] == 0x00 && z[offset + 6] == 0x00 && z[offset + 7] == 0x00;
}

inline bool IsZeroPtr64(std::span<const uint8_t> z, size_t offset) {
    if (offset + 8 > z.size()) return false;
    for (size_t i = 0; i < 8; ++i) {
        if (z[offset + i] != 0) return false;
    }
    return true;
}

inline bool IsValidRawfileName(std::string_view name) {
    if (name.size() < 3 || name.size() > 250) return false;
    for (unsigned char c : name) {
        bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                  (c >= '0' && c <= '9') || c == '_' || c == '.' ||
                  c == '/' || c == '-' || c == '\\';
        if (!ok) return false;
    }
    return true;
}

inline bool IsValidLocKey(std::string_view name) {
    if (name.size() < 2 || name.size() > 128) return false;
    unsigned char f = static_cast<unsigned char>(name[0]);
    if (!((f >= 'A' && f <= 'Z') || (f >= '0' && f <= '9') || f == '_')) return false;
    for (unsigned char c : name) {
        bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
        if (!ok) return false;
    }
    return true;
}

inline bool TryInflateRawfile(std::span<const uint8_t> b, std::vector<uint8_t>& out) {
    if (b.size() < 6) return false;
    BinaryReader br(b);
    uint32_t usz = br.read_le<uint32_t>(0);
    if (usz == 0 || usz > 0x08000000u) return false;
    std::vector<uint8_t> inflated;
    if (!InflateRaw(b.subspan(4), inflated, usz)) return false;
    if (inflated.size() != usz) return false;
    out = std::move(inflated);
    return true;
}

inline std::vector<uint8_t> FormatStringTableCsvBytes(int32_t cols, int32_t rows, const std::vector<std::string>& cells) {
    std::string out;
    for (int32_t r = 0; r < rows; ++r) {
        for (int32_t c = 0; c < cols; ++c) {
            if (c > 0) out.push_back(',');
            size_t idx = static_cast<size_t>(r) * static_cast<size_t>(cols) + static_cast<size_t>(c);
            std::string cell = (idx < cells.size()) ? cells[idx] : "";
            if (cell.find(',') != std::string::npos || cell.find('"') != std::string::npos ||
                cell.find('\n') != std::string::npos || cell.find('\r') != std::string::npos) {
                out.push_back('"');
                for (char ch : cell) {
                    if (ch == '"') out += "\"\"";
                    else out.push_back(ch);
                }
                out.push_back('"');
            } else {
                out += cell;
            }
        }
        out.push_back('\n');
    }
    return StringToBytes(out);
}

inline void ParseZoneHeaderAndScan(FastFileZoneState& state) {
    std::span<const uint8_t> z(state.zone);
    BinaryReader zr(z);

    auto readStrings = [&](uint32_t count, size_t startP) -> std::pair<std::vector<std::string>, size_t> {
        std::vector<std::string> list;
        size_t p = startP;
        if (p + 8ULL * count > z.size()) return { list, p };
        std::vector<bool> isInline(count, false);
        for (uint32_t i = 0; i < count; ++i) {
            isInline[i] = IsInlinePtr64(z, p + static_cast<size_t>(i) * 8);
        }
        p += 8ULL * count;
        for (uint32_t i = 0; i < count; ++i) {
            if (isInline[i]) {
                size_t e = p;
                while (e < z.size() && z[e] != 0) ++e;
                if (e < z.size()) {
                    list.emplace_back(reinterpret_cast<const char*>(z.data() + p), e - p);
                    p = e + 1;
                } else {
                    list.emplace_back();
                }
            } else {
                list.emplace_back();
            }
        }
        return { list, p };
    };

    state.scriptStrings.clear();
    state.depends.clear();
    state.assetTypes.clear();

    if (state.isBo1) {
        size_t p = std::min<size_t>(0x24, z.size());
        std::vector<size_t> ptrOffsets;
        size_t scanLimit = (z.size() > 8) ? std::min<size_t>(z.size() - 8, 256) : 0;
        for (size_t i = 0; i <= scanLimit; ++i) {
            if (IsInlinePtr64(z, i)) {
                ptrOffsets.push_back(i);
                i += 7;
            }
        }
        uint32_t sc = 0, ac = 0;
        if (ptrOffsets.size() >= 2) {
            size_t secondLast = ptrOffsets[ptrOffsets.size() - 2];
            size_t last = ptrOffsets.back();
            if (secondLast >= 4) sc = zr.read_le<uint32_t>(secondLast - 4);
            if (last >= 4) ac = zr.read_le<uint32_t>(last - 4);
            p = last + 8;
        } else if (ptrOffsets.size() == 1) {
            if (ptrOffsets[0] >= 4) ac = zr.read_le<uint32_t>(ptrOffsets[0] - 4);
            p = ptrOffsets[0] + 8;
        }
        if (sc > 0 && sc < 100000) {
            auto r = readStrings(sc, p);
            state.scriptStrings = std::move(r.first);
            p = r.second;
        }
        if (ac > 0 && ac < 100000) {
            if (p + 16ULL * ac <= z.size()) {
                for (uint32_t i = 0; i < ac; ++i) {
                    state.assetTypes.push_back(zr.read_le<uint32_t>(p + 16ULL * i));
                }
            }
            p += 16ULL * ac;
        }
        state.dataStart = static_cast<uint32_t>(std::min(p, z.size()));
    } else {
        if (z.size() < 0x30) {
            state.dataStart = 0;
        } else {
            uint32_t sc = zr.read_le<uint32_t>(0x00);
            bool spInline = IsInlinePtr64(z, 0x08);
            uint32_t dc = zr.read_le<uint32_t>(0x10);
            bool dpInline = IsInlinePtr64(z, 0x18);
            uint32_t ac = zr.read_le<uint32_t>(0x20);
            bool apInline = IsInlinePtr64(z, 0x28);
            size_t p = 0x30;

            if (spInline && sc > 0 && sc < 100000) {
                auto r = readStrings(sc, p);
                state.scriptStrings = std::move(r.first);
                p = r.second;
            }
            if (dpInline && dc > 0 && dc < 100000) {
                auto r = readStrings(dc, p);
                state.depends = std::move(r.first);
                p = r.second;
            }
            if (apInline && ac > 0 && ac < 100000) {
                if (p + 16ULL * ac <= z.size()) {
                    for (uint32_t i = 0; i < ac; ++i) {
                        state.assetTypes.push_back(zr.read_le<uint32_t>(p + 16ULL * i));
                    }
                }
                p += 16ULL * ac;
            }
            state.dataStart = static_cast<uint32_t>(std::min(p, z.size()));
        }
    }

    // Pass 1: Scan for rawfile, script, menu via 64-bit inline pointer headers
    const size_t n = z.size();
    uint32_t itemCounter = 0;
    size_t pos = state.dataStart;
    while (pos + 28 < n) {
        if (!IsInlinePtr64(z, pos)) {
            ++pos;
            continue;
        }
        if (z[pos + 12] == 0 && z[pos + 13] == 0 && z[pos + 14] == 0 && z[pos + 15] == 0 &&
            IsInlinePtr64(z, pos + 16)) {
            uint32_t ln = zr.read_le<uint32_t>(pos + 8);
            size_t nameStart = pos + 24;
            uint8_t firstChar = z[nameStart];
            if (firstChar >= 0x21 && firstChar <= 0x7E) {
                size_t nameEnd = nameStart + 1;
                size_t maxNameEnd = std::min(n, nameStart + 254);
                while (nameEnd < maxNameEnd && z[nameEnd] >= 0x20 && z[nameEnd] <= 0x7E) {
                    ++nameEnd;
                }
                if (nameEnd < n && z[nameEnd] == 0) {
                    size_t b0 = nameEnd + 1;
                    size_t b1 = b0 + ln;
                    if (ln < 0x08000000u && b1 < n) {
                        std::string name(reinterpret_cast<const char*>(z.data() + nameStart), nameEnd - nameStart);
                        bool isGscMagic = (b0 + 4 <= n && z[b0] == 0x80 && z[b0 + 1] == 0x47 && z[b0 + 2] == 0x53 && z[b0 + 3] == 0x43);
                        bool isAllFf = (b0 + 4 <= n && z[b0] == 0xFF && z[b0 + 1] == 0xFF && z[b0 + 2] == 0xFF && z[b0 + 3] == 0xFF);

                        std::string kind;
                        bool compressed = false;
                        std::vector<uint8_t> tmpInflated;

                        if (z[b1] == 0 && isGscMagic) {
                            kind = "script";
                        } else if (!isAllFf && IsValidRawfileName(name)) {
                            if (state.isBo1 && z[b1] == 0 && (EndsWithA(name, ".gsc") || EndsWithA(name, ".csc"))) {
                                kind = "script";
                            } else if (TryInflateRawfile(z.subspan(b0, ln), tmpInflated)) {
                                kind = "rawfile";
                                compressed = true;
                            } else if (z[b1] == 0) {
                                if ((state.isBo1 && name.find('.') == std::string::npos) || EndsWithA(name, ".menu")) {
                                    kind = "menu";
                                } else {
                                    kind = "rawfile";
                                }
                            }
                        }

                        if (!kind.empty()) {
                            FastFileItemRecord rec;
                            rec.id = "ff-item-" + std::to_string(itemCounter++);
                            rec.kind = kind;
                            rec.name = name;
                            rec.hdr = static_cast<uint32_t>(pos);
                            rec.data = static_cast<uint32_t>(b0);
                            rec.size = ln;
                            rec.compressed = compressed;
                            state.items.push_back(std::move(rec));
                            pos = b1 + 1;
                            continue;
                        }
                    }
                }
            }
        }
        ++pos;
    }

    // Pass 2: Scan for localize and StringTable (.csv)
    pos = state.dataStart;
    while (pos + 24 < n) {
        if (!IsInlinePtr64(z, pos)) {
            ++pos;
            continue;
        }
        if (IsInlinePtr64(z, pos + 8)) {
            size_t s1Start = pos + 16;
            size_t s1End = s1Start;
            size_t s1Max = std::min(n, s1Start + 4096);
            while (s1End < s1Max && z[s1End] != 0 && z[s1End] != 0xFF) ++s1End;
            if (s1End < n && z[s1End] == 0) {
                size_t s2Start = s1End + 1;
                size_t s2End = s2Start;
                size_t s2Max = std::min(n, s2Start + 4096);
                while (s2End < s2Max && z[s2End] != 0 && z[s2End] != 0xFF) ++s2End;
                if (s2End < n && z[s2End] == 0) {
                    std::string str1(reinterpret_cast<const char*>(z.data() + s1Start), s1End - s1Start);
                    std::string str2(reinterpret_cast<const char*>(z.data() + s2Start), s2End - s2Start);
                    if (IsValidLocKey(str2)) {
                        FastFileItemRecord rec;
                        rec.id = "ff-loc-" + std::to_string(itemCounter++);
                        rec.kind = "localize";
                        rec.name = str2;
                        rec.hdr = static_cast<uint32_t>(pos);
                        rec.data = static_cast<uint32_t>(s1Start);
                        rec.size = static_cast<uint32_t>(s1End - s1Start);
                        state.items.push_back(std::move(rec));
                        pos = s2End + 1;
                        continue;
                    }
                    if (IsValidLocKey(str1)) {
                        FastFileItemRecord rec;
                        rec.id = "ff-loc-" + std::to_string(itemCounter++);
                        rec.kind = "localize";
                        rec.name = str1;
                        rec.hdr = static_cast<uint32_t>(pos);
                        rec.data = static_cast<uint32_t>(s2Start);
                        rec.size = static_cast<uint32_t>(s2End - s2Start);
                        state.items.push_back(std::move(rec));
                        pos = s2End + 1;
                        continue;
                    }
                }
            }
        }

        if (pos + 38 < n && IsInlinePtr64(z, pos + 16) && (IsZeroPtr64(z, pos + 24) || IsInlinePtr64(z, pos + 24))) {
            int32_t cols = zr.read_le<int32_t>(pos + 8);
            int32_t rows = zr.read_le<int32_t>(pos + 12);
            if (cols > 0 && cols < 1024 && rows > 0 && rows < 100000 && static_cast<int64_t>(cols) * rows <= 500000) {
                size_t nameStart = pos + 32;
                size_t nameEnd = nameStart;
                size_t maxName = std::min(n, nameStart + 250);
                while (nameEnd < maxName && z[nameEnd] >= 0x21 && z[nameEnd] <= 0x7E) ++nameEnd;
                if (nameEnd > nameStart + 4 && nameEnd < n && z[nameEnd] == 0) {
                    std::string csvName(reinterpret_cast<const char*>(z.data() + nameStart), nameEnd - nameStart);
                    if (EndsWithA(ToLowerA(csvName), ".csv")) {
                        size_t cnt = static_cast<size_t>(cols) * static_cast<size_t>(rows);
                        size_t p = nameEnd + 1;
                        if (p + 16ULL * cnt <= n) {
                            std::vector<uint8_t> ptrKinds(cnt, 0);
                            for (size_t i = 0; i < cnt; ++i) {
                                size_t cellPtrOff = p + 16 * i;
                                if (IsInlinePtr64(z, cellPtrOff)) ptrKinds[i] = 1;
                                else if (IsZeroPtr64(z, cellPtrOff)) ptrKinds[i] = 0;
                                else ptrKinds[i] = 2;
                            }
                            p += 16 * cnt;
                            std::vector<std::string> cells;
                            cells.reserve(cnt);
                            bool validTable = true;
                            for (size_t i = 0; i < cnt; ++i) {
                                if (ptrKinds[i] == 1) {
                                    size_t e = p;
                                    while (e < n && z[e] != 0) ++e;
                                    if (e >= n) { validTable = false; break; }
                                    cells.emplace_back(reinterpret_cast<const char*>(z.data() + p), e - p);
                                    p = e + 1;
                                } else {
                                    cells.emplace_back();
                                }
                            }
                            if (validTable) {
                                if (IsInlinePtr64(z, pos + 24)) p += 2 * cnt;
                                size_t dataStartOffset = nameEnd + 1;
                                FastFileItemRecord rec;
                                rec.id = "ff-st-" + std::to_string(itemCounter++);
                                rec.kind = "stringtable";
                                rec.name = csvName;
                                rec.hdr = static_cast<uint32_t>(pos);
                                rec.data = static_cast<uint32_t>(dataStartOffset);
                                rec.size = static_cast<uint32_t>(p > dataStartOffset ? p - dataStartOffset : 0);
                                rec.cols = cols;
                                rec.rows = rows;
                                rec.end = static_cast<uint32_t>(p);
                                rec.cells = std::move(cells);
                                state.items.push_back(std::move(rec));
                                pos = p;
                                continue;
                            }
                        }
                    }
                }
            }
        }
        ++pos;
    }

    std::sort(state.items.begin(), state.items.end(), [](const FastFileItemRecord& a, const FastFileItemRecord& b) {
        return a.hdr < b.hdr;
    });
}

inline void ParseFastFileArchive(const std::wstring& path, BinaryReader& r, ParsedGameArchive& arc) {
    std::string magic = r.read_ascii(0, 8, false);
    uint32_t version = r.read_le<uint32_t>(8);

    std::wstring gameTitle = L"Call of Duty Engine Container";
    std::wstring engineFamily = L"Call of Duty FastFile Engine";
    if (magic.rfind("TAff", 0) == 0) {
        if (version == 0x93 || version == 147) {
            gameTitle = L"Call of Duty: Black Ops II (T6)";
            engineFamily = L"Treyarch T6 Engine (BO2)";
        } else if (version >= 0x200 && version < 0x250) {
            gameTitle = L"Call of Duty: Black Ops III (T7)";
            engineFamily = L"Treyarch T7 Engine (BO3)";
        } else {
            gameTitle = L"Call of Duty: Treyarch Title";
            engineFamily = L"Treyarch Engine";
        }
    } else if (magic.rfind("IWff", 0) == 0 || magic.rfind("S1ff", 0) == 0) {
        if (version == 5 || version == 6) {
            gameTitle = L"Call of Duty 4: Modern Warfare (IW3)";
            engineFamily = L"IW 3.0 Engine (CoD4)";
        } else if (version == 372 || version == 380) {
            gameTitle = L"Call of Duty: World at War (T4)";
            engineFamily = L"Treyarch T4 Engine (WaW)";
        } else if (version == 276 || version == 277 || version == 278) {
            gameTitle = L"Call of Duty: Modern Warfare 2 (IW4)";
            engineFamily = L"IW 4.0 Engine (MW2)";
        } else if (version == 473 || version == 0x1D9) {
            gameTitle = L"Call of Duty: Black Ops (T5)";
            engineFamily = L"Treyarch T5 Engine (BO1)";
        } else if (version == 1 || version == 3) {
            gameTitle = L"Call of Duty: Modern Warfare 3 (IW5)";
            engineFamily = L"IW 5.0 Engine (MW3)";
        }
    }

    arc.valid = true;
    arc.formatId = ArchiveFormatId::FF;
    arc.formatName = L"Call of Duty FastFile (" + gameTitle + L")";
    arc.engineName = engineFamily;
    arc.isLittleEndian = true;
    arc.supportsRepack = true;

    bool isBo2 = (magic == "TAffu100");
    bool isBo1 = (magic == "IWffu100" && (version == 473 || version == 0x1D9 || version == 5 || version == 372 || version == 276));

    FastFileZoneState& state = arc.ffZoneState;
    state.magic = magic;
    state.version = version;
    state.isBo1 = !isBo2;

    bool zoneUnpacked = false;
    std::wstring decompressMethod = L"Raw / Signed Stream";

    if (isBo2 && r.size() > 20) {
        uint32_t blen = r.read_le<uint32_t>(12);
        if (blen > 0 && 16ULL + blen <= r.size()) {
            std::vector<uint8_t> xf;
            if (InflateRaw(r.slice(16, blen), xf)) {
                size_t nWords = xf.size() / 4;
                BinaryReader xfr(xf);
                for (size_t i = 0; i < nWords; ++i) {
                    state.xfile.push_back(xfr.read_le<uint32_t>(i * 4));
                }
                state.origXfile = state.xfile;
                if (nWords * 4 < xf.size()) {
                    state.xfileTail.assign(xf.begin() + static_cast<ptrdiff_t>(nWords * 4), xf.end());
                }

                size_t off = 16 + blen;
                while (off + 4 <= r.size()) {
                    uint32_t c = r.read_le<uint32_t>(off);
                    if (c == 0) break;
                    std::vector<uint8_t> chunk;
                    bool ok = false;
                    if (c <= 0x20000u && off + 4 + c <= r.size()) {
                        ok = InflateRaw(r.slice(off + 4, c), chunk);
                    }
                    if (!ok) {
                        bool resynced = false;
                        for (size_t skip = 1; skip < 64 && off + skip + 4 <= r.size(); ++skip) {
                            size_t o = off + skip;
                            uint32_t candC = r.read_le<uint32_t>(o);
                            if (candC > 0 && candC <= 0x10000u && o + 4 + candC <= r.size()) {
                                if (InflateRaw(r.slice(o + 4, candC), chunk) && !chunk.empty()) {
                                    c = candC;
                                    off = o;
                                    state.gaps++;
                                    resynced = true;
                                    break;
                                }
                            }
                        }
                        if (!resynced) break;
                    }
                    state.zone.insert(state.zone.end(), chunk.begin(), chunk.end());
                    off += 4 + c;
                    state.nblocks++;
                }
                if (!state.zone.empty()) {
                    state.origZoneLen = state.zone.size();
                    state.valid = true;
                    zoneUnpacked = true;
                    decompressMethod = L"BO2 0x7FC0 Raw Deflate Blocks (" + std::to_wstring(state.nblocks) + L" blocks)";
                }
            }
        }
    }

    if (!zoneUnpacked) {
        // Try Zlib stream at 12 or scan first 4KB
        for (size_t off = 12; off + 2 < std::min<size_t>(r.size(), 4096); ++off) {
            uint8_t b0 = r[off];
            uint8_t b1 = r[off + 1];
            if (b0 == 0x78 && (b1 == 0x01 || b1 == 0x5E || b1 == 0x9C || b1 == 0xDA) &&
                (((static_cast<uint32_t>(b0) << 8) | b1) % 31 == 0)) {
                if (DecompressZlib(r.slice(off, r.size() - off), state.zone) && state.zone.size() > 32) {
                    state.origZoneLen = state.zone.size();
                    state.isBo1 = true;
                    state.valid = (off == 12);
                    if (state.zone.size() >= 36) {
                        BinaryReader zr(state.zone);
                        for (int i = 0; i < 9; ++i) state.xfile.push_back(zr.read_le<uint32_t>(i * 4));
                        state.origXfile = state.xfile;
                    }
                    zoneUnpacked = true;
                    decompressMethod = L"Zlib Stream at " + ToHexW(off);
                    break;
                }
            }
        }
    }

    AddHeaderField(arc, L"FastFile Container Magic", L"0x00000000", 8, AsciiToWide(magic), L"FastFile magic signature identifier");
    AddHeaderField(arc, L"Engine Version Code", L"0x00000008", 4, ToHexW(version) + L" (" + std::to_wstring(version) + L")", L"Matched to: " + gameTitle);
    AddHeaderField(arc, L"Decompression Engine", L"Zone Stream", 4, decompressMethod, L"Stream decoding and asset carving state");

    if (zoneUnpacked) {
        ParseZoneHeaderAndScan(state);

        for (const auto& it : state.items) {
            std::span<const uint8_t> rawSlice(state.zone.data() + it.data, std::min<size_t>(it.size, state.zone.size() - it.data));
            std::vector<uint8_t> usableData;
            std::wstring methodLabel = L"XAsset (" + AsciiToWide(it.kind) + L")";

            if (it.kind == "rawfile" && it.compressed) {
                if (TryInflateRawfile(rawSlice, usableData)) {
                    methodLabel = L"Compressed Rawfile (Deflate -15)";
                } else {
                    usableData.assign(rawSlice.begin(), rawSlice.end());
                }
            } else if (it.kind == "stringtable" && it.cols > 0 && it.rows > 0) {
                usableData = FormatStringTableCsvBytes(it.cols, it.rows, it.cells);
                methodLabel = L"StringTable (" + std::to_wstring(it.cols) + L"x" + std::to_wstring(it.rows) + L" CSV)";
            } else {
                usableData.assign(rawSlice.begin(), rawSlice.end());
                if (it.kind == "script") {
                    methodLabel = state.isBo1 ? L"T5 Compiled GSC Bytecode" : L"T6 Compiled GSC (\\x80GSC)";
                }
            }

            std::string prefix = (it.kind == "rawfile") ? "rawfiles/" :
                                 (it.kind == "script") ? "scripts/" :
                                 (it.kind == "menu") ? "menus/" :
                                 (it.kind == "stringtable") ? "stringtables/" : "localize/";
            auto sp = SplitVirtualPath(AsciiToWide(prefix + it.name));

            ArchiveEntry e;
            e.name = sp.name;
            e.path = sp.cleanPath;
            e.folder = sp.folder;
            e.offset = it.data;
            e.compressedSize = it.size;
            e.uncompressedSize = usableData.size();
            e.compressionMethod = methodLabel;
            e.crc32 = ComputeCrc32(usableData);
            e.category = (it.kind == "script") ? AssetCategory::Code : AssetCategory::Config;
            e.customFlags = ExtractMode::InlineOnly;
            e.inlineData = std::move(usableData);
            arc.entries.push_back(std::move(e));
        }

        // Also carve any embedded DDS textures or RIFF WAVE audio inside the decompressed zone
        BinaryReader zr(state.zone);
        size_t carveCount = 0;
        for (size_t i = 0; i + 16 < state.zone.size() && carveCount < 250; ++i) {
            if (zr[i] == 0x80 && zr[i + 1] == 0x47 && zr[i + 2] == 0x53 && zr[i + 3] == 0x43 && state.items.empty()) {
                size_t end = std::min(state.zone.size(), i + 65536);
                for (size_t j = i + 16; j + 4 < end; ++j) {
                    if (zr[j] == 0x80 && zr[j + 1] == 0x47 && zr[j + 2] == 0x53 && zr[j + 3] == 0x43) {
                        end = j;
                        break;
                    }
                }
                auto slice = zr.copy_slice(i, end - i);
                ArchiveEntry e;
                e.name = L"script_" + ToHexW(i, 6) + L".gsc";
                e.folder = L"scripts";
                e.path = L"scripts/" + e.name;
                e.offset = i;
                e.compressedSize = slice.size();
                e.uncompressedSize = slice.size();
                e.compressionMethod = L"Compiled CoD Script Bytecode";
                e.crc32 = ComputeCrc32(slice);
                e.category = AssetCategory::Code;
                e.customFlags = ExtractMode::InlineOnly;
                e.inlineData = std::move(slice);
                arc.entries.push_back(std::move(e));
                i = end - 1;
                ++carveCount;
            } else if (zr[i] == 0x44 && zr[i + 1] == 0x44 && zr[i + 2] == 0x53 && zr[i + 3] == 0x20 && i + 128 <= state.zone.size()) {
                uint32_t h = zr.read_le<uint32_t>(i + 12);
                uint32_t w = zr.read_le<uint32_t>(i + 16);
                uint32_t lin = zr.read_le<uint32_t>(i + 20);
                if (w > 0 && w <= 8192 && h > 0 && h <= 8192) {
                    size_t estSize = std::max<size_t>(128, std::min<size_t>(state.zone.size() - i, 128 + (lin ? lin : w * h)));
                    auto slice = zr.copy_slice(i, estSize);
                    ArchiveEntry e;
                    e.name = L"texture_" + std::to_wstring(w) + L"x" + std::to_wstring(h) + L"_" + ToHexW(i, 6) + L".dds";
                    e.folder = L"textures";
                    e.path = L"textures/" + e.name;
                    e.offset = i;
                    e.compressedSize = slice.size();
                    e.uncompressedSize = slice.size();
                    e.compressionMethod = L"DirectDraw Surface (DDS)";
                    e.crc32 = ComputeCrc32(slice);
                    e.category = AssetCategory::Texture;
                    e.customFlags = ExtractMode::InlineOnly;
                    e.inlineData = std::move(slice);
                    arc.entries.push_back(std::move(e));
                    i += estSize - 1;
                    ++carveCount;
                }
            } else if (zr[i] == 0x52 && zr[i + 1] == 0x49 && zr[i + 2] == 0x46 && zr[i + 3] == 0x46 &&
                       zr.read_ascii(i + 8, 4, false) == "WAVE") {
                uint32_t riffSize = zr.read_le<uint32_t>(i + 4) + 8;
                size_t safeLen = std::min<size_t>(state.zone.size() - i, std::max<size_t>(44, riffSize));
                auto slice = zr.copy_slice(i, safeLen);
                ArchiveEntry e;
                e.name = L"sound_" + ToHexW(i, 6) + L".wav";
                e.folder = L"sounds";
                e.path = L"sounds/" + e.name;
                e.offset = i;
                e.compressedSize = slice.size();
                e.uncompressedSize = slice.size();
                e.compressionMethod = L"RIFF WAVE Audio Stream";
                e.crc32 = ComputeCrc32(slice);
                e.category = AssetCategory::Audio;
                e.customFlags = ExtractMode::InlineOnly;
                e.inlineData = std::move(slice);
                arc.entries.push_back(std::move(e));
                i += safeLen - 1;
                ++carveCount;
            }
        }

        // Master zone entry
        std::wstring baseFileName = ExtractFileNameW(path);
        ArchiveEntry zoneEntry;
        zoneEntry.name = baseFileName + L".zone";
        zoneEntry.folder = L"zone_stream";
        zoneEntry.path = L"zone_stream/" + zoneEntry.name;
        zoneEntry.offset = state.dataStart;
        zoneEntry.compressedSize = r.size();
        zoneEntry.uncompressedSize = state.zone.size();
        zoneEntry.compressionMethod = decompressMethod;
        zoneEntry.crc32 = ComputeCrc32(state.zone);
        zoneEntry.category = AssetCategory::Binary;
        zoneEntry.customFlags = ExtractMode::InlineOnly;
        zoneEntry.inlineData = state.zone;
        arc.entries.push_back(std::move(zoneEntry));
    } else {
        std::wstring baseFileName = ExtractFileNameW(path);
        ArchiveEntry rawEntry;
        rawEntry.name = baseFileName + L".payload";
        rawEntry.folder = L"raw";
        rawEntry.path = L"raw/" + rawEntry.name;
        rawEntry.offset = 12;
        rawEntry.compressedSize = (r.size() > 12) ? (r.size() - 12) : 0;
        rawEntry.uncompressedSize = rawEntry.compressedSize;
        rawEntry.compressionMethod = L"FastFile Raw / Signed Payload";
        rawEntry.crc32 = ComputeCrc32(r.slice(12, rawEntry.compressedSize));
        rawEntry.category = AssetCategory::Binary;
        rawEntry.customFlags = ExtractMode::Stored;
        arc.entries.push_back(std::move(rawEntry));
    }

    AddHeaderField(arc, L"Extracted Asset Count", L"Zone Assets", 4,
        std::to_wstring(arc.entries.size()) + L" assets extracted", L"Compiled scripts, rawfiles, StringTables, and textures");
}

// ============================================================================
// 9. Ren'Py Archive (.rpa: RPA-3.0 / RPA-2.0 + Python Pickle VM Protocols 0-4)
// ============================================================================
struct PickleValue {
    enum class Kind { Null, Int, String, Bytes, List, Dict, Mark };
    Kind kind = Kind::Null;
    int64_t intVal = 0;
    std::string strVal;
    std::vector<uint8_t> bytesVal;
    std::vector<PickleValue> listVal;
    std::map<std::string, PickleValue> dictVal;
};

inline int64_t DecodePickleLongLE(std::span<const uint8_t> bytes) {
    if (bytes.empty()) return 0;
    uint64_t val = 0;
    size_t n = std::min<size_t>(bytes.size(), 8);
    for (size_t i = 0; i < n; ++i) {
        val |= static_cast<uint64_t>(bytes[i]) << (i * 8);
    }
    if (bytes.size() < 8 && (bytes.back() & 0x80) != 0) {
        for (size_t i = bytes.size(); i < 8; ++i) {
            val |= 0xFFULL << (i * 8);
        }
    }
    return static_cast<int64_t>(val);
}

inline std::string UnescapePythonStr(std::string_view raw) {
    std::string s = TrimA(raw);
    if (s.size() >= 2 && ((s.front() == '\'' && s.back() == '\'') || (s.front() == '"' && s.back() == '"'))) {
        s = s.substr(1, s.size() - 2);
    }
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            char next = s[++i];
            if ((next == 'x' || next == 'X') && i + 2 < s.size()) {
                char hex[3] = { s[i + 1], s[i + 2], 0 };
                out.push_back(static_cast<char>(std::strtol(hex, nullptr, 16)));
                i += 2;
            } else if (next == 'n') out.push_back('\n');
            else if (next == 'r') out.push_back('\r');
            else if (next == 't') out.push_back('\t');
            else if (next == '0') out.push_back('\0');
            else out.push_back(next);
        } else {
            out.push_back(s[i]);
        }
    }
    return out;
}

inline bool ParsePythonPickle(std::span<const uint8_t> bytes, PickleValue& outRoot) {
    BinaryReader r(bytes);
    std::vector<PickleValue> stack;
    std::unordered_map<uint32_t, PickleValue> memo;
    size_t pos = 0;

    auto readLine = [&]() -> std::string {
        std::string line;
        while (pos < r.size()) {
            uint8_t b = r[pos++];
            if (b == 0x0A) break;
            if (b != 0x0D) line.push_back(static_cast<char>(b));
        }
        return line;
    };

    auto popMark = [&]() -> std::vector<PickleValue> {
        for (ptrdiff_t i = static_cast<ptrdiff_t>(stack.size()) - 1; i >= 0; --i) {
            if (stack[static_cast<size_t>(i)].kind == PickleValue::Kind::Mark) {
                std::vector<PickleValue> items(stack.begin() + i + 1, stack.end());
                stack.erase(stack.begin() + i, stack.end());
                return items;
            }
        }
        return {};
    };

    while (pos < r.size()) {
        uint8_t op = r[pos++];
        switch (op) {
            case 0x80: // PROTO
                if (pos < r.size()) ++pos;
                break;
            case 0x95: // FRAME (Protocol 4)
                pos += 8;
                break;
            case 0x2E: // STOP
                if (!stack.empty()) {
                    outRoot = std::move(stack.back());
                    return true;
                }
                return false;
            case 0x28: { // MARK
                PickleValue v;
                v.kind = PickleValue::Kind::Mark;
                stack.push_back(std::move(v));
                break;
            }
            case 0x30: // POP
                if (!stack.empty()) stack.pop_back();
                break;
            case 0x31: // POP_MARK
                popMark();
                break;
            case 0x32: // DUP
                if (!stack.empty()) stack.push_back(stack.back());
                break;
            case 0x7D: { // EMPTY_DICT
                PickleValue v;
                v.kind = PickleValue::Kind::Dict;
                stack.push_back(std::move(v));
                break;
            }
            case 0x5D:
            case 0x29: { // EMPTY_LIST / EMPTY_TUPLE
                PickleValue v;
                v.kind = PickleValue::Kind::List;
                stack.push_back(std::move(v));
                break;
            }
            case 0x4E: // NONE
            case 0x88: // NEWTRUE
            case 0x89: { // NEWFALSE
                PickleValue v;
                v.kind = PickleValue::Kind::Null;
                stack.push_back(std::move(v));
                break;
            }
            case 0x71: { // BINPUT
                if (pos >= r.size()) return false;
                uint32_t idx = r[pos++];
                if (!stack.empty()) memo[idx] = stack.back();
                break;
            }
            case 0x72: { // LONG_BINPUT
                if (pos + 4 > r.size()) return false;
                uint32_t idx = r.read_le<uint32_t>(pos);
                pos += 4;
                if (!stack.empty()) memo[idx] = stack.back();
                break;
            }
            case 0x70: { // PUT
                uint32_t idx = static_cast<uint32_t>(std::strtoul(readLine().c_str(), nullptr, 10));
                if (!stack.empty()) memo[idx] = stack.back();
                break;
            }
            case 0x94: { // MEMOIZE
                uint32_t idx = static_cast<uint32_t>(memo.size());
                if (!stack.empty()) memo[idx] = stack.back();
                break;
            }
            case 0x68: { // BINGET
                if (pos >= r.size()) return false;
                uint32_t idx = r[pos++];
                stack.push_back(memo[idx]);
                break;
            }
            case 0x6A: { // LONG_BINGET
                if (pos + 4 > r.size()) return false;
                uint32_t idx = r.read_le<uint32_t>(pos);
                pos += 4;
                stack.push_back(memo[idx]);
                break;
            }
            case 0x67: { // GET
                uint32_t idx = static_cast<uint32_t>(std::strtoul(readLine().c_str(), nullptr, 10));
                stack.push_back(memo[idx]);
                break;
            }
            case 0x4A: { // BININT
                if (pos + 4 > r.size()) return false;
                PickleValue v;
                v.kind = PickleValue::Kind::Int;
                v.intVal = r.read_le<int32_t>(pos);
                pos += 4;
                stack.push_back(std::move(v));
                break;
            }
            case 0x4B: { // BININT1
                if (pos >= r.size()) return false;
                PickleValue v;
                v.kind = PickleValue::Kind::Int;
                v.intVal = r[pos++];
                stack.push_back(std::move(v));
                break;
            }
            case 0x4D: { // BININT2
                if (pos + 2 > r.size()) return false;
                PickleValue v;
                v.kind = PickleValue::Kind::Int;
                v.intVal = r.read_le<uint16_t>(pos);
                pos += 2;
                stack.push_back(std::move(v));
                break;
            }
            case 0x49: // INT
            case 0x4C: { // LONG
                std::string line = TrimA(readLine());
                if (!line.empty() && line.back() == 'L') line.pop_back();
                PickleValue v;
                v.kind = PickleValue::Kind::Int;
                v.intVal = std::strtoll(line.c_str(), nullptr, 10);
                stack.push_back(std::move(v));
                break;
            }
            case 0x8A: { // LONG1
                if (pos >= r.size()) return false;
                uint8_t len = r[pos++];
                if (pos + len > r.size()) return false;
                PickleValue v;
                v.kind = PickleValue::Kind::Int;
                v.intVal = DecodePickleLongLE(r.slice(pos, len));
                pos += len;
                stack.push_back(std::move(v));
                break;
            }
            case 0x8B: { // LONG4
                if (pos + 4 > r.size()) return false;
                uint32_t len = r.read_le<uint32_t>(pos);
                pos += 4;
                if (pos + len > r.size()) return false;
                PickleValue v;
                v.kind = PickleValue::Kind::Int;
                v.intVal = DecodePickleLongLE(r.slice(pos, len));
                pos += len;
                stack.push_back(std::move(v));
                break;
            }
            case 0x58: // BINUNICODE
            case 0x54: { // BINSTRING
                if (pos + 4 > r.size()) return false;
                uint32_t len = r.read_le<uint32_t>(pos);
                pos += 4;
                if (pos + len > r.size()) return false;
                PickleValue v;
                v.kind = PickleValue::Kind::String;
                v.strVal.assign(reinterpret_cast<const char*>(r.data() + pos), len);
                v.bytesVal.assign(r.data() + pos, r.data() + pos + len);
                pos += len;
                stack.push_back(std::move(v));
                break;
            }
            case 0x8C: // SHORT_BINUNICODE
            case 0x55: { // SHORT_BINSTRING
                if (pos >= r.size()) return false;
                uint8_t len = r[pos++];
                if (pos + len > r.size()) return false;
                PickleValue v;
                v.kind = PickleValue::Kind::String;
                v.strVal.assign(reinterpret_cast<const char*>(r.data() + pos), len);
                v.bytesVal.assign(r.data() + pos, r.data() + pos + len);
                pos += len;
                stack.push_back(std::move(v));
                break;
            }
            case 0x8D: { // BINUNICODE8
                if (pos + 8 > r.size()) return false;
                uint64_t len = r.read_le<uint64_t>(pos);
                pos += 8;
                if (pos + len > r.size()) return false;
                PickleValue v;
                v.kind = PickleValue::Kind::String;
                v.strVal.assign(reinterpret_cast<const char*>(r.data() + pos), static_cast<size_t>(len));
                v.bytesVal.assign(r.data() + pos, r.data() + pos + static_cast<size_t>(len));
                pos += static_cast<size_t>(len);
                stack.push_back(std::move(v));
                break;
            }
            case 0x43: { // SHORT_BINBYTES
                if (pos >= r.size()) return false;
                uint8_t len = r[pos++];
                if (pos + len > r.size()) return false;
                PickleValue v;
                v.kind = PickleValue::Kind::Bytes;
                v.bytesVal.assign(r.data() + pos, r.data() + pos + len);
                v.strVal.assign(reinterpret_cast<const char*>(r.data() + pos), len);
                pos += len;
                stack.push_back(std::move(v));
                break;
            }
            case 0x42: { // BINBYTES
                if (pos + 4 > r.size()) return false;
                uint32_t len = r.read_le<uint32_t>(pos);
                pos += 4;
                if (pos + len > r.size()) return false;
                PickleValue v;
                v.kind = PickleValue::Kind::Bytes;
                v.bytesVal.assign(r.data() + pos, r.data() + pos + len);
                v.strVal.assign(reinterpret_cast<const char*>(r.data() + pos), len);
                pos += len;
                stack.push_back(std::move(v));
                break;
            }
            case 0x8E: { // BINBYTES8
                if (pos + 8 > r.size()) return false;
                uint64_t len = r.read_le<uint64_t>(pos);
                pos += 8;
                if (pos + len > r.size()) return false;
                PickleValue v;
                v.kind = PickleValue::Kind::Bytes;
                v.bytesVal.assign(r.data() + pos, r.data() + pos + static_cast<size_t>(len));
                pos += static_cast<size_t>(len);
                stack.push_back(std::move(v));
                break;
            }
            case 0x53: // STRING
            case 0x56: { // UNICODE
                std::string s = UnescapePythonStr(readLine());
                PickleValue v;
                v.kind = PickleValue::Kind::String;
                v.strVal = s;
                v.bytesVal.assign(s.begin(), s.end());
                stack.push_back(std::move(v));
                break;
            }
            case 0x74: // TUPLE
            case 0x6C: { // LIST
                PickleValue v;
                v.kind = PickleValue::Kind::List;
                v.listVal = popMark();
                stack.push_back(std::move(v));
                break;
            }
            case 0x85: { // TUPLE1
                if (stack.empty()) return false;
                PickleValue a = std::move(stack.back());
                stack.pop_back();
                PickleValue v;
                v.kind = PickleValue::Kind::List;
                v.listVal.push_back(std::move(a));
                stack.push_back(std::move(v));
                break;
            }
            case 0x86: { // TUPLE2
                if (stack.size() < 2) return false;
                PickleValue b = std::move(stack.back()); stack.pop_back();
                PickleValue a = std::move(stack.back()); stack.pop_back();
                PickleValue v;
                v.kind = PickleValue::Kind::List;
                v.listVal.push_back(std::move(a));
                v.listVal.push_back(std::move(b));
                stack.push_back(std::move(v));
                break;
            }
            case 0x87: { // TUPLE3
                if (stack.size() < 3) return false;
                PickleValue c = std::move(stack.back()); stack.pop_back();
                PickleValue b = std::move(stack.back()); stack.pop_back();
                PickleValue a = std::move(stack.back()); stack.pop_back();
                PickleValue v;
                v.kind = PickleValue::Kind::List;
                v.listVal.push_back(std::move(a));
                v.listVal.push_back(std::move(b));
                v.listVal.push_back(std::move(c));
                stack.push_back(std::move(v));
                break;
            }
            case 0x61: { // APPEND
                if (stack.size() < 2) return false;
                PickleValue val = std::move(stack.back());
                stack.pop_back();
                stack.back().listVal.push_back(std::move(val));
                break;
            }
            case 0x65: { // APPENDS
                auto items = popMark();
                if (!stack.empty()) {
                    for (auto& it : items) stack.back().listVal.push_back(std::move(it));
                }
                break;
            }
            case 0x64: { // DICT
                auto items = popMark();
                PickleValue v;
                v.kind = PickleValue::Kind::Dict;
                for (size_t i = 0; i + 1 < items.size(); i += 2) {
                    v.dictVal[items[i].strVal] = std::move(items[i + 1]);
                }
                stack.push_back(std::move(v));
                break;
            }
            case 0x75: { // SETITEMS
                auto items = popMark();
                if (!stack.empty()) {
                    for (size_t i = 0; i + 1 < items.size(); i += 2) {
                        stack.back().dictVal[items[i].strVal] = std::move(items[i + 1]);
                    }
                }
                break;
            }
            case 0x73: { // SETITEM
                if (stack.size() < 3) return false;
                PickleValue val = std::move(stack.back()); stack.pop_back();
                PickleValue key = std::move(stack.back()); stack.pop_back();
                stack.back().dictVal[key.strVal] = std::move(val);
                break;
            }
            case 0x63: { // GLOBAL
                std::string mod = readLine();
                std::string name = readLine();
                PickleValue v;
                v.kind = PickleValue::Kind::String;
                v.strVal = mod + "." + name;
                stack.push_back(std::move(v));
                break;
            }
            case 0x52: { // REDUCE
                if (stack.size() < 2) return false;
                PickleValue args = std::move(stack.back()); stack.pop_back();
                PickleValue fn = std::move(stack.back()); stack.pop_back();
                if (!args.listVal.empty()) {
                    PickleValue v;
                    v.kind = PickleValue::Kind::Bytes;
                    v.bytesVal = args.listVal[0].bytesVal;
                    v.strVal = args.listVal[0].strVal;
                    stack.push_back(std::move(v));
                } else {
                    stack.push_back( std::move(args));
                }
                break;
            }
            default:
                return false;
        }
    }
    return false;
}

inline void ParseRenpyArchive(const std::wstring& path, BinaryReader& r, ParsedGameArchive& arc) {
    size_t lineEnd = 0;
    while (lineEnd < std::min<size_t>(r.size(), 128) && r[lineEnd] != 0x0A) ++lineEnd;
    std::string headerLine = TrimA(r.read_ascii(0, lineEnd, false));

    std::vector<std::string> tokens;
    size_t p = 0;
    while (p < headerLine.size()) {
        while (p < headerLine.size() && (headerLine[p] == ' ' || headerLine[p] == '\t')) ++p;
        if (p >= headerLine.size()) break;
        size_t start = p;
        while (p < headerLine.size() && headerLine[p] != ' ' && headerLine[p] != '\t') ++p;
        tokens.push_back(headerLine.substr(start, p - start));
    }

    if (tokens.size() < 2) return;
    std::string version = tokens[0];
    uint64_t indexOffset = std::strtoull(tokens[1].c_str(), nullptr, 16);
    int64_t xorKey = (version == "RPA-3.0" && tokens.size() >= 3) ? static_cast<int64_t>(std::strtoull(tokens[2].c_str(), nullptr, 16)) : 0;

    arc.valid = true;
    arc.formatId = ArchiveFormatId::RPA;
    arc.formatName = L"Ren'Py Archive (" + AsciiToWide(version) + L")";
    arc.engineName = L"Ren'Py Visual Novel Engine";
    arc.isLittleEndian = true;
    arc.supportsRepack = true;

    AddHeaderField(arc, L"Format Signature", L"0x00000000", static_cast<uint32_t>(version.size()), AsciiToWide(version), L"Ren'Py Archive Container Magic");
    AddHeaderField(arc, L"Index Offset", L"Header", static_cast<uint32_t>(tokens[1].size()), ToHexW(indexOffset) + L" (" + FormatBytesW(indexOffset) + L")", L"Byte offset of Zlib-compressed Pickle file table");
    AddHeaderField(arc, L"Obfuscation Key", L"Header", 8, version == "RPA-3.0" ? ToHexW(static_cast<uint64_t>(xorKey), 8) : L"None (RPA-2.0 Plaintext)", L"XOR key applied to entry offset & length");

    if (indexOffset == 0 || indexOffset >= r.size()) return;
    std::vector<uint8_t> indexPickleBytes;
    if (!DecompressZlib(r.slice(static_cast<size_t>(indexOffset), r.size() - static_cast<size_t>(indexOffset)), indexPickleBytes)) {
        return;
    }

    PickleValue root;
    if (!ParsePythonPickle(indexPickleBytes, root) || root.kind != PickleValue::Kind::Dict) {
        return;
    }

    for (const auto& [rawPath, val] : root.dictVal) {
        if (val.listVal.empty()) continue;
        const PickleValue* tuplePtr = &val;
        if (val.listVal[0].kind == PickleValue::Kind::List) {
            tuplePtr = &val.listVal[0];
        }
        if (tuplePtr->listVal.size() < 2) continue;

        int64_t rawOfs = tuplePtr->listVal[0].intVal;
        int64_t rawLen = tuplePtr->listVal[1].intVal;
        uint64_t realOfs = static_cast<uint64_t>((version == "RPA-3.0") ? (rawOfs ^ xorKey) : rawOfs);
        uint64_t realLen = static_cast<uint64_t>((version == "RPA-3.0") ? (rawLen ^ xorKey) : rawLen);

        std::vector<uint8_t> prefix;
        if (tuplePtr->listVal.size() >= 3) {
            prefix = tuplePtr->listVal[2].bytesVal;
        }

        auto sp = SplitVirtualPath(AsciiToWide(rawPath));
        ArchiveEntry e;
        e.name = sp.name;
        e.path = sp.cleanPath;
        e.folder = sp.folder;
        e.offset = realOfs;
        e.compressedSize = realLen + prefix.size();
        e.uncompressedSize = realLen + prefix.size();
        e.compressionMethod = (version == "RPA-3.0")
            ? (prefix.empty() ? L"XOR-32 Obfuscated" : L"XOR-32 + Prefix")
            : (prefix.empty() ? L"Store" : L"Prefix + Store");

        if (!prefix.empty()) {
            auto bodySlice = r.slice(static_cast<size_t>(realOfs), static_cast<size_t>(realLen));
            e.inlineData.reserve(prefix.size() + bodySlice.size());
            e.inlineData.insert(e.inlineData.end(), prefix.begin(), prefix.end());
            e.inlineData.insert(e.inlineData.end(), bodySlice.begin(), bodySlice.end());
            e.crc32 = ComputeCrc32(e.inlineData);
            e.category = ClassifyAsset(sp.cleanPath, e.inlineData);
            e.customFlags = ExtractMode::InlineOnly;
        } else {
            auto bodySlice = r.slice(static_cast<size_t>(realOfs), static_cast<size_t>(realLen));
            e.crc32 = ComputeCrc32(bodySlice);
            e.category = ClassifyAsset(sp.cleanPath, bodySlice);
            e.customFlags = ExtractMode::Stored;
        }
        arc.entries.push_back(std::move(e));
    }

    AddHeaderField(arc, L"Total Files", L"Index", 0, std::to_wstring(arc.entries.size()) + L" assets", L"Files indexed in unpickled catalog");
}

// ============================================================================
// 10. Unreal Engine 4/5 (.pak, .uasset, .umap)
// ============================================================================
inline constexpr uint32_t kUnrealPakMagic = 0x5A6F12E1u;
inline constexpr uint32_t kUnrealPackageMagic = 0x9E2A83C1u;

inline void ParseUnrealArchive(const std::wstring& path, BinaryReader& r, ParsedGameArchive& arc) {
    bool isUAsset = (r.size() >= 4 && r.read_le<uint32_t>(0) == kUnrealPackageMagic);
    arc.valid = true;
    arc.formatId = ArchiveFormatId::UNREAL;
    arc.isLittleEndian = true;
    arc.supportsRepack = false;

    if (isUAsset) {
        int32_t fileVer = r.read_le<int32_t>(12);
        int32_t licenseeVer = r.read_le<int32_t>(16);
        int32_t folderNameLen = r.read_le<int32_t>(28);
        size_t pos = 32;
        if (folderNameLen > 0 && folderNameLen < 128 && pos + static_cast<size_t>(folderNameLen) <= r.size()) {
            pos += static_cast<size_t>(folderNameLen);
        }
        uint32_t packageFlags = r.read_le<uint32_t>(pos);
        pos += 4;
        int32_t nameCount = r.read_le<int32_t>(pos);
        int32_t nameOffset = r.read_le<int32_t>(pos + 4);
        pos += 8;
        int32_t exportCount = r.read_le<int32_t>(pos);
        int32_t importCount = r.read_le<int32_t>(pos + 8);

        bool isUmap = EndsWithW(ToLowerW(path), L".umap");
        arc.formatName = isUmap ? L"Unreal Engine Level (.umap)" : L"Unreal Engine Asset (.uasset)";
        arc.engineName = L"Unreal Engine 4 / 5 (Version " + std::to_wstring(fileVer) + L")";

        std::vector<std::string> nameStrings;
        if (nameOffset > 0 && static_cast<size_t>(nameOffset) < r.size() && nameCount > 0 && nameCount < 10000) {
            size_t np = static_cast<size_t>(nameOffset);
            for (int32_t i = 0; i < nameCount && np + 4 <= r.size(); ++i) {
                int32_t len = r.read_le<int32_t>(np);
                np += 4;
                if (len > 0 && len < 256 && np + static_cast<size_t>(len) <= r.size()) {
                    nameStrings.push_back(r.read_cstr(np, static_cast<size_t>(len)));
                    np += static_cast<size_t>(len) + 4;
                } else {
                    break;
                }
            }
        }

        AddHeaderField(arc, L"Unreal Package Header Signature", L"0x00000000", 4, L"0x9E2A83C1 (UE Asset Package)", L"Standard Unreal Engine 4 / 5 package format");
        AddHeaderField(arc, L"Engine Package File Version", L"0x0000000C", 4, L"Version " + std::to_wstring(fileVer) + L" (Licensee " + std::to_wstring(licenseeVer) + L")", L"Unreal Engine serialization build");
        AddHeaderField(arc, L"Name Table Entries", ToHexW(static_cast<uint32_t>(nameOffset)), 4, std::to_wstring(nameCount) + L" names (" + std::to_wstring(nameStrings.size()) + L" decoded)", L"Internal engine symbols and properties");
        AddHeaderField(arc, L"Export / Import Counts", L"Summary", 8, std::to_wstring(exportCount) + L" exports - " + std::to_wstring(importCount) + L" imports", L"Package object hierarchies and dependencies");

        std::wstring baseName = ExtractFileNameW(path);
        if (!nameStrings.empty()) {
            std::string report = "// Unreal Engine Asset Package: " + WideToUtf8Str(baseName) + "\n";
            report += "// Engine File Version: " + std::to_string(fileVer) + "\n";
            report += "// Total Names: " + std::to_string(nameCount) + "\n";
            report += "// Package Flags: " + ToHexA(packageFlags) + "\n\n=== NAME TABLE ENTRIES ===\n";
            for (size_t i = 0; i < nameStrings.size(); ++i) {
                char idxBuf[32]{};
                snprintf(idxBuf, sizeof(idxBuf), "[%04zu] ", i);
                report += idxBuf + nameStrings[i] + "\n";
            }
            ArchiveEntry ne;
            ne.name = baseName + L"_names.txt";
            ne.folder = L"package_meta";
            ne.path = L"package_meta/" + ne.name;
            ne.offset = static_cast<uint64_t>(std::max<int32_t>(0, nameOffset));
            ne.inlineData = StringToBytes(report);
            ne.compressedSize = ne.inlineData.size();
            ne.uncompressedSize = ne.inlineData.size();
            ne.compressionMethod = L"Decoded Name Table";
            ne.crc32 = ComputeCrc32(ne.inlineData);
            ne.category = AssetCategory::Config;
            ne.customFlags = ExtractMode::InlineOnly;
            arc.entries.push_back(std::move(ne));
        }

        ArchiveEntry rawE;
        rawE.name = baseName;
        rawE.folder = L"(root)";
        rawE.path = baseName;
        rawE.offset = 0;
        rawE.compressedSize = r.size();
        rawE.uncompressedSize = r.size();
        rawE.compressionMethod = L"Unreal Package Blob";
        rawE.crc32 = ComputeCrc32(r.slice(0, std::min<size_t>(r.size(), 65536)));
        rawE.category = isUmap ? AssetCategory::Map2D : AssetCategory::Binary;
        rawE.customFlags = ExtractMode::Stored;
        arc.entries.push_back(std::move(rawE));
        return;
    }

    // Unreal .pak footer search
    size_t magicPos = std::string::npos;
    if (r.size() >= 44) {
        size_t minSearch = (r.size() > 512) ? (r.size() - 512) : 0;
        for (size_t i = r.size() - 4; ; --i) {
            if (r.read_le<uint32_t>(i) == kUnrealPakMagic) {
                magicPos = i;
                break;
            }
            if (i <= minSearch) break;
        }
    }

    int32_t pakVersion = 8;
    int64_t indexOffset = 0;
    int64_t indexSize = 0;

    if (magicPos != std::string::npos) {
        if (magicPos >= 20) {
            pakVersion = r.read_le<int32_t>(magicPos - 20);
            indexOffset = r.read_le<int64_t>(magicPos - 16);
            indexSize = r.read_le<int64_t>(magicPos - 8);
        }
        if ((indexOffset <= 0 || static_cast<size_t>(indexOffset) >= r.size() || indexSize <= 0) && magicPos + 24 <= r.size()) {
            pakVersion = r.read_le<int32_t>(magicPos + 4);
            indexOffset = r.read_le<int64_t>(magicPos + 8);
            indexSize = r.read_le<int64_t>(magicPos + 16);
        }
    }

    arc.formatName = L"Unreal Engine PAK Archive (v" + std::to_wstring(std::abs(pakVersion) < 20 ? pakVersion : 8) + L")";
    arc.engineName = L"Epic Games Unreal Engine 4 / 5";

    AddHeaderField(arc, L"Unreal PAK Magic Signature", L"Footer", 4, L"0x5A6F12E1 (Unreal Engine PAK)", L"UE4/UE5 footer container identifier");
    AddHeaderField(arc, L"PAK Format Version", L"Footer + 4", 4, L"Version " + std::to_wstring(pakVersion), L"Unreal Engine PAK directory format version");
    AddHeaderField(arc, L"Index Table Offset", L"Footer + 8", 8, ToHexW(static_cast<uint64_t>(indexOffset)), L"Byte position of central virtual directory table");

    if (indexOffset > 0 && static_cast<size_t>(indexOffset) < r.size() && indexSize > 0) {
        auto idxSpan = r.slice(static_cast<size_t>(indexOffset), static_cast<size_t>(indexSize));
        std::vector<uint8_t> decIdx;
        if (idxSpan.size() >= 2 && idxSpan[0] == 0x78 && (idxSpan[1] == 0x9C || idxSpan[1] == 0xDA)) {
            if (DecompressZlib(idxSpan, decIdx)) {
                idxSpan = decIdx;
            }
        }
        BinaryReader ir(idxSpan);
        size_t ip = 0;
        int32_t mountLen = ir.read_le<int32_t>(ip);
        ip += 4;
        if (mountLen > 0 && mountLen < 256 && ip + static_cast<size_t>(mountLen) <= ir.size()) {
            ip += static_cast<size_t>(mountLen);
        }
        int32_t fileCount = ir.read_le<int32_t>(ip);
        ip += 4;
        if (fileCount > 0 && fileCount < 100000) {
            for (int32_t i = 0; i < fileCount && ip + 32 <= ir.size(); ++i) {
                int32_t pathLen = ir.read_le<int32_t>(ip);
                ip += 4;
                if (pathLen <= 0 || pathLen > 512 || ip + static_cast<size_t>(pathLen) > ir.size()) break;
                std::string rawPath = ir.read_cstr(ip, static_cast<size_t>(pathLen));
                ip += static_cast<size_t>(pathLen);

                uint64_t fOfs = ir.read_le<uint64_t>(ip); ip += 8;
                uint64_t fSize = ir.read_le<uint64_t>(ip); ip += 8;
                uint64_t uSize = ir.read_le<uint64_t>(ip); ip += 8;
                uint32_t compMethod = ir.read_le<uint32_t>(ip); ip += 4;
                ip += 20; // SHA1 hash
                if (pakVersion >= 3 && ip + 1 <= ir.size()) ip += 1;
                if (pakVersion >= 7 && ip + 4 <= ir.size()) ip += 4;

                auto sp = SplitVirtualPath(AsciiToWide(rawPath));
                auto fSlice = r.slice(static_cast<size_t>(fOfs), static_cast<size_t>(fSize));

                ArchiveEntry e;
                e.name = sp.name;
                e.path = sp.cleanPath;
                e.folder = sp.folder;
                e.offset = fOfs;
                e.compressedSize = fSize;
                e.uncompressedSize = uSize ? uSize : fSize;
                e.compressionMethod = (compMethod == 0) ? L"Stored" : ((compMethod == 1) ? L"Zlib" : L"LZ4 / Oodle");
                e.crc32 = ComputeCrc32(fSlice.subspan(0, std::min<size_t>(fSlice.size(), 4096)));
                e.category = ClassifyAsset(sp.cleanPath, fSlice);
                e.customFlags = (compMethod == 1) ? ExtractMode::ZlibStream : ((compMethod == 2) ? ExtractMode::Lz4Block : ExtractMode::Stored);
                arc.entries.push_back(std::move(e));
            }
        }
    }

    if (arc.entries.empty()) {
        size_t carvedIdx = 0;
        for (size_t i = 0; i + 8 <= r.size() && carvedIdx < 256; ++i) {
            if (r.read_le<uint32_t>(i) == kUnrealPackageMagic) {
                size_t len = std::min<size_t>(r.size() - i, 131072);
                ArchiveEntry e;
                e.name = L"asset_" + ToHexW(i, 6) + L".uasset";
                e.folder = L"carved_assets";
                e.path = L"carved_assets/" + e.name;
                e.offset = i;
                e.compressedSize = len;
                e.uncompressedSize = len;
                e.compressionMethod = L"Carved Unreal Package";
                e.crc32 = ComputeCrc32(r.slice(i, std::min<size_t>(len, 4096)));
                e.category = AssetCategory::Binary;
                e.customFlags = ExtractMode::Stored;
                arc.entries.push_back(std::move(e));
                i += 4095;
                ++carvedIdx;
            }
        }
    }
}

// ============================================================================
// 11. Unity Engine (.assets, .bundle, .unity3d, .sharedassets)
// ============================================================================
inline void ParseUnityArchive(const std::wstring& path, BinaryReader& r, ParsedGameArchive& arc) {
    std::string magic7 = r.read_ascii(0, 7, false);
    bool isBundle = (magic7 == "UnityFS" || magic7 == "UnityWe" || magic7 == "UnityRa");

    arc.valid = true;
    arc.formatId = ArchiveFormatId::UNITY;
    arc.isLittleEndian = !isBundle;
    arc.supportsRepack = false;

    if (isBundle) {
        uint32_t formatVersion = r.read_be<uint32_t>(8);
        size_t pos = 12;
        std::string unityVer = r.read_cstr(pos, 32);
        pos += unityVer.size() + 1;
        std::string revision = r.read_cstr(pos, 32);
        pos += revision.size() + 1;
        pos += 8; // totalFileSize
        uint32_t compMetaSize = r.read_be<uint32_t>(pos); pos += 4;
        uint32_t uncompMetaSize = r.read_be<uint32_t>(pos); pos += 4;
        uint32_t flags = r.read_be<uint32_t>(pos); pos += 4;
        (void)compMetaSize;
        (void)uncompMetaSize;

        uint32_t compType = flags & 0x3F;
        std::wstring compName = (compType == 0) ? L"None (Uncompressed)" : ((compType == 1) ? L"LZMA" : L"LZ4 / LZ4HC");

        arc.formatName = L"Unity AssetBundle (" + AsciiToWide(revision.empty() ? unityVer : revision) + L")";
        arc.engineName = L"Unity Engine - " + AsciiToWide(revision.empty() ? unityVer : revision);

        AddHeaderField(arc, L"Unity Container Magic", L"0x00000000", 8, AsciiToWide(magic7) + L" (Format Version " + std::to_wstring(formatVersion) + L")", L"Unity AssetBundle / UnityFS container format");
        AddHeaderField(arc, L"Unity Engine Build", L"0x0000000C", static_cast<uint32_t>(revision.size()), AsciiToWide(revision.empty() ? unityVer : revision), L"Target Unity runtime version");
        AddHeaderField(arc, L"Bundle Compression", L"Flags", 4, compName + L" (Flags: " + ToHexW(flags) + L")", L"Directory and payload block compression");

        size_t entryIdx = 0;
        for (size_t i = pos; i + 8 < r.size() && entryIdx < 256; ++i) {
            bool isPng = (r[i] == 0x89 && r[i + 1] == 0x50 && r[i + 2] == 0x4E && r[i + 3] == 0x47);
            bool isDds = (r[i] == 0x44 && r[i + 1] == 0x44 && r[i + 2] == 0x53 && r[i + 3] == 0x20);
            bool isOgg = (r[i] == 0x4F && r[i + 1] == 0x67 && r[i + 2] == 0x67 && r[i + 3] == 0x53);
            if (isPng || isDds || isOgg) {
                size_t len = std::min<size_t>(r.size() - i, 65536);
                std::wstring ext = isDds ? L".dds" : (isPng ? L".png" : L".ogg");
                ArchiveEntry e;
                e.name = L"bundle_asset_" + ToHexW(i, 6) + ext;
                e.folder = L"assets";
                e.path = L"assets/" + e.name;
                e.offset = i;
                e.compressedSize = len;
                e.uncompressedSize = len;
                e.compressionMethod = compName;
                e.crc32 = ComputeCrc32(r.slice(i, len));
                e.category = isOgg ? AssetCategory::Audio : AssetCategory::Texture;
                e.customFlags = ExtractMode::Stored;
                arc.entries.push_back(std::move(e));
                i += len - 1;
                ++entryIdx;
            }
        }

        if (arc.entries.empty() && pos < r.size()) {
            std::wstring baseName = ExtractFileNameW(path);
            ArchiveEntry e;
            e.name = baseName + L".bundle_stream";
            e.folder = L"raw";
            e.path = L"raw/" + e.name;
            e.offset = pos;
            e.compressedSize = r.size() - pos;
            e.uncompressedSize = (compType >= 2 && uncompMetaSize > 0) ? uncompMetaSize : (r.size() - pos);
            e.compressionMethod = compName;
            e.crc32 = ComputeCrc32(r.slice(pos, std::min<size_t>(r.size() - pos, 65536)));
            e.category = AssetCategory::Binary;
            e.customFlags = (compType >= 2 && uncompMetaSize > 0) ? ExtractMode::Lz4Block : ExtractMode::Stored;
            arc.entries.push_back(std::move(e));
        }
    } else {
        uint32_t formatVersion = r.read_be<uint32_t>(8);
        uint32_t dataOffset = r.read_be<uint32_t>(12);
        bool isBigEndian = (r[16] != 0);
        size_t p = 20;
        std::string unityVer = "Unknown";
        if (formatVersion >= 7 && p < r.size()) {
            std::string cand = r.read_cstr(p, 32);
            if (cand.size() >= 3) {
                unityVer = cand;
                p += cand.size() + 1;
            }
        }

        arc.formatName = L"Unity Serialized File (" + AsciiToWide(unityVer) + L")";
        arc.engineName = L"Unity Engine - " + AsciiToWide(unityVer);
        arc.isLittleEndian = !isBigEndian;

        AddHeaderField(arc, L"Unity Serialized File Version", L"0x00000008", 4, L"Format Version " + std::to_wstring(formatVersion), L"SerializedFile layout version");
        AddHeaderField(arc, L"Unity Version String", L"0x00000014", static_cast<uint32_t>(unityVer.size()), AsciiToWide(unityVer), L"Engine build that generated this asset archive");
        AddHeaderField(arc, L"Data Stream Offset", L"0x0000000C", 4, ToHexW(dataOffset), L"Starting byte offset of binary object blobs");

        size_t baseDataPos = std::max<size_t>(20, std::min<size_t>(r.size(), dataOffset ? dataOffset : 4096));
        size_t entryIdx = 0;
        for (size_t i = baseDataPos; i + 16 < r.size() && entryIdx < 256; ++i) {
            int32_t potentialLen = r.read_le<int32_t>(i);
            if (potentialLen > 10 && potentialLen < 262144 && i + 4 + static_cast<size_t>(potentialLen) <= r.size()) {
                bool isText = true;
                size_t checkN = std::min<size_t>(static_cast<size_t>(potentialLen), 64);
                for (size_t c = 0; c < checkN; ++c) {
                    uint8_t ch = r[i + 4 + c];
                    if ((ch < 32 || ch > 126) && ch != 9 && ch != 10 && ch != 13) {
                        isText = false;
                        break;
                    }
                }
                if (isText) {
                    auto textSlice = r.slice(i + 4, static_cast<size_t>(potentialLen));
                    std::string firstLine = r.read_ascii(i + 4, std::min<size_t>(40, textSlice.size()));
                    std::wstring ext = L".txt";
                    if (firstLine.find("Shader") != std::string::npos || firstLine.find("SubShader") != std::string::npos) ext = L".shader";
                    else if (!firstLine.empty() && (firstLine[0] == '{' || firstLine[0] == '[')) ext = L".json";
                    else if (!firstLine.empty() && firstLine[0] == '<') ext = L".xml";
                    else if (firstLine.find("using UnityEngine") != std::string::npos) ext = L".cs";

                    ArchiveEntry e;
                    e.name = L"text_asset_" + ToHexW(i, 6) + ext;
                    e.folder = L"text_assets";
                    e.path = L"text_assets/" + e.name;
                    e.offset = i + 4;
                    e.compressedSize = static_cast<uint64_t>(potentialLen);
                    e.uncompressedSize = static_cast<uint64_t>(potentialLen);
                    e.compressionMethod = L"Unity Serialized TextAsset";
                    e.crc32 = ComputeCrc32(textSlice);
                    e.category = (ext == L".shader" || ext == L".cs") ? AssetCategory::Code : AssetCategory::Config;
                    e.customFlags = ExtractMode::Stored;
                    arc.entries.push_back(std::move(e));
                    i += 4 + static_cast<size_t>(potentialLen) - 1;
                    ++entryIdx;
                    continue;
                }
            }
            if (r[i] == 0x44 && r[i + 1] == 0x44 && r[i + 2] == 0x53 && r[i + 3] == 0x20 && i + 32 <= r.size()) {
                uint32_t h = r.read_le<uint32_t>(i + 12);
                uint32_t w = r.read_le<uint32_t>(i + 16);
                if (w > 0 && w <= 4096 && h > 0 && h <= 4096) {
                    size_t len = std::min<size_t>(r.size() - i, 128 + static_cast<size_t>(w) * h);
                    ArchiveEntry e;
                    e.name = L"texture_" + std::to_wstring(w) + L"x" + std::to_wstring(h) + L"_" + ToHexW(i, 6) + L".dds";
                    e.folder = L"textures";
                    e.path = L"textures/" + e.name;
                    e.offset = i;
                    e.compressedSize = len;
                    e.uncompressedSize = len;
                    e.compressionMethod = L"Unity Texture2D (DDS)";
                    e.crc32 = ComputeCrc32(r.slice(i, len));
                    e.category = AssetCategory::Texture;
                    e.customFlags = ExtractMode::Stored;
                    arc.entries.push_back(std::move(e));
                    i += len - 1;
                    ++entryIdx;
                }
            }
        }

        if (arc.entries.empty() && baseDataPos < r.size()) {
            std::wstring baseName = ExtractFileNameW(path);
            ArchiveEntry e;
            e.name = baseName + L".serialized_data";
            e.folder = L"raw";
            e.path = L"raw/" + e.name;
            e.offset = baseDataPos;
            e.compressedSize = r.size() - baseDataPos;
            e.uncompressedSize = r.size() - baseDataPos;
            e.compressionMethod = L"Serialized Object Table";
            e.crc32 = ComputeCrc32(r.slice(baseDataPos, std::min<size_t>(r.size() - baseDataPos, 65536)));
            e.category = AssetCategory::Binary;
            e.customFlags = ExtractMode::Stored;
            arc.entries.push_back(std::move(e));
        }
    }
}

// ============================================================================
// 12. Bethesda Softworks (.bsa v103-v105 & .ba2 BTDX GNRL / DX10)
// ============================================================================
inline std::vector<uint8_t> BuildDdsHeader128(uint32_t width, uint32_t height, uint32_t mipCount, uint8_t dxgiFormat) {
    std::vector<uint8_t> hdr(128, 0);
    auto writeLe32 = [&](size_t ofs, uint32_t v) {
        hdr[ofs] = static_cast<uint8_t>(v & 0xFF);
        hdr[ofs + 1] = static_cast<uint8_t>((v >> 8) & 0xFF);
        hdr[ofs + 2] = static_cast<uint8_t>((v >> 16) & 0xFF);
        hdr[ofs + 3] = static_cast<uint8_t>((v >> 24) & 0xFF);
    };
    writeLe32(0, 0x20534444u); // "DDS "
    writeLe32(4, 124);         // dwSize
    writeLe32(8, 0x00021007u); // DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_MIPMAPCOUNT
    writeLe32(12, height);
    writeLe32(16, width);
    writeLe32(20, width * 4);
    writeLe32(28, std::max<uint32_t>(1, mipCount));
    writeLe32(76, 32);         // ddspf.dwSize
    writeLe32(80, 0x00000004u);// DDPF_FOURCC
    // Choose FourCC based on common DXGI formats (BC1=71, BC2=74, BC3=77, BC4=80, BC5=83, otherwise DX10/DXT5)
    uint32_t fourCC = 0x35545844u; // "DXT5"
    if (dxgiFormat == 70 || dxgiFormat == 71 || dxgiFormat == 72) fourCC = 0x31545844u; // "DXT1"
    else if (dxgiFormat == 73 || dxgiFormat == 74 || dxgiFormat == 75) fourCC = 0x33545844u; // "DXT3"
    writeLe32(84, fourCC);
    writeLe32(108, 0x00001000u); // DDSCAPS_TEXTURE
    return hdr;
}

inline void ParseBethesdaArchive(const std::wstring& path, BinaryReader& r, ParsedGameArchive& arc) {
    bool isBa2 = (r.size() >= 4 && r.read_ascii(0, 4, false) == "BTDX");
    arc.valid = true;
    arc.isLittleEndian = true;
    arc.supportsRepack = false;

    if (!isBa2) {
        arc.formatId = ArchiveFormatId::BSA;
        uint32_t version = r.read_le<uint32_t>(4);
        uint32_t folderRecordOffset = r.read_le<uint32_t>(8);
        uint32_t archiveFlags = r.read_le<uint32_t>(12);
        uint32_t folderCount = r.read_le<uint32_t>(16);
        uint32_t fileCount = r.read_le<uint32_t>(20);
        uint32_t totalFolderNameLen = r.read_le<uint32_t>(24);

        bool defaultCompressed = (archiveFlags & 0x0004) != 0;
        std::wstring verName = (version == 0x67) ? L"TES IV: Oblivion (v103)" :
                               (version == 0x68) ? L"Skyrim / Fallout 3 / Fallout NV (v104)" :
                               (version == 0x69) ? L"Skyrim SE (v105)" : (L"Version " + ToHexW(version, 2));

        arc.formatName = L"BSA (" + verName + L")";
        arc.engineName = L"Bethesda Creation / Gamebryo Engine";

        AddHeaderField(arc, L"Signature", L"0x00000000", 4, L"BSA\\0 (0x00415342)", L"Bethesda Softworks Archive");
        AddHeaderField(arc, L"Format Version", L"0x00000004", 4, verName, L"Engine Archive Schema");
        AddHeaderField(arc, L"Folder Count", L"0x00000010", 4, std::to_wstring(folderCount), L"Total virtual directories");
        AddHeaderField(arc, L"File Count", L"0x00000014", 4, std::to_wstring(fileCount), L"Total assets stored");

        uint32_t safeCount = std::min<uint32_t>(fileCount, 20000);
        size_t folderEntrySize = (version == 0x69) ? 24 : 16;
        size_t fileNamesOffset = static_cast<size_t>(folderRecordOffset) + static_cast<size_t>(folderCount) * folderEntrySize +
                                 static_cast<size_t>(fileCount) * 16 + totalFolderNameLen;

        std::vector<std::string> fileNames;
        if (fileNamesOffset < r.size()) {
            size_t curr = fileNamesOffset;
            for (uint32_t i = 0; i < safeCount && curr < r.size(); ++i) {
                size_t end = curr;
                while (end < r.size() && r[end] != 0) ++end;
                fileNames.push_back(r.read_ascii(curr, end - curr, false));
                curr = end + 1;
            }
        }

        size_t fileRecordPos = static_cast<size_t>(folderRecordOffset) + static_cast<size_t>(folderCount) * folderEntrySize;
        for (uint32_t i = 0; i < safeCount && fileRecordPos + 16 <= r.size(); ++i) {
            uint32_t rawSize = r.read_le<uint32_t>(fileRecordPos + 8);
            uint32_t offset = r.read_le<uint32_t>(fileRecordPos + 12);
            fileRecordPos += 16;

            bool fileCompressed = ((rawSize & (1u << 30)) != 0) ? !defaultCompressed : defaultCompressed;
            uint32_t realSize = rawSize & ~(1u << 30);

            std::string rawName = (i < fileNames.size() && !fileNames[i].empty()) ? fileNames[i] : ("asset_" + std::to_string(i) + ".dat");
            auto sp = SplitVirtualPath(AsciiToWide(rawName));

            uint32_t uncompSize = realSize;
            if (fileCompressed && offset + 4 <= r.size()) {
                uncompSize = r.read_le<uint32_t>(offset);
            }

            ArchiveEntry e;
            e.name = sp.name;
            e.path = sp.cleanPath;
            e.folder = sp.folder;
            e.offset = offset;
            e.compressedSize = realSize;
            e.uncompressedSize = uncompSize;
            e.compressionMethod = fileCompressed ? L"BSA Zlib Deflate" : L"Uncompressed";
            e.crc32 = ComputeCrc32(r.slice(offset, std::min<size_t>(realSize, 1024)));
            e.category = ClassifyAsset(sp.cleanPath);
            e.customFlags = fileCompressed ? ExtractMode::BsaZlib : ExtractMode::Stored;
            arc.entries.push_back(std::move(e));
        }
    } else {
        arc.formatId = ArchiveFormatId::BA2;
        uint32_t version = r.read_le<uint32_t>(4);
        std::string typeMagic = r.read_ascii(8, 4, false);
        uint32_t fileCount = r.read_le<uint32_t>(12);
        uint64_t nameTableOffset = r.read_le<uint64_t>(16);

        std::wstring game = (version >= 2) ? L"Starfield (Creation Engine 2)" : L"Fallout 4 (Creation Engine)";
        arc.formatName = L"BA2 (" + AsciiToWide(typeMagic) + L" - " + game + L")";
        arc.engineName = L"Bethesda Creation Engine";

        AddHeaderField(arc, L"Signature", L"0x00000000", 4, L"BTDX (0x58445442)", L"Bethesda Archive 2");
        AddHeaderField(arc, L"Archive Type", L"0x00000008", 4, typeMagic == "DX10" ? L"DX10 Texture Container" : L"GNRL General Assets", L"FourCC Asset Class");
        AddHeaderField(arc, L"Format Version", L"0x00000004", 4, L"v" + std::to_wstring(version) + L" (" + game + L")", L"BA2 Schema Revision");
        AddHeaderField(arc, L"Total Files", L"0x0000000C", 4, std::to_wstring(fileCount), L"Entry count in header table");

        std::vector<std::string> fileNames;
        if (nameTableOffset > 0 && nameTableOffset < r.size()) {
            size_t p = static_cast<size_t>(nameTableOffset);
            for (uint32_t i = 0; i < std::min<uint32_t>(fileCount, 20000) && p + 2 <= r.size(); ++i) {
                uint16_t len = r.read_le<uint16_t>(p);
                p += 2;
                if (p + len <= r.size()) {
                    fileNames.push_back(r.read_ascii(p, len, false));
                    p += len;
                } else break;
            }
        }

        uint32_t safeCount = std::min<uint32_t>(fileCount, 15000);
        size_t recordPos = 24;

        if (typeMagic == "DX10") {
            for (uint32_t i = 0; i < safeCount && recordPos + 24 <= r.size(); ++i) {
                std::string ext = TrimA(r.read_ascii(recordPos + 4, 4, true));
                uint8_t numChunks = r[recordPos + 15];
                uint16_t chunkHdrSize = r.read_le<uint16_t>(recordPos + 16);
                uint16_t texHeight = r.read_le<uint16_t>(recordPos + 18);
                uint16_t texWidth = r.read_le<uint16_t>(recordPos + 20);
                uint8_t numMips = r[recordPos + 22];
                uint8_t dxgiFmt = r[recordPos + 23];
                recordPos += 24;

                uint64_t firstOffset = 0;
                uint32_t firstPacked = 0;
                uint32_t firstUnpacked = 0;
                size_t chunkStep = (chunkHdrSize >= 24) ? chunkHdrSize : 24;
                for (uint8_t c = 0; c < numChunks && recordPos + chunkStep <= r.size(); ++c) {
                    if (c == 0) {
                        firstOffset = r.read_le<uint64_t>(recordPos);
                        firstPacked = r.read_le<uint32_t>(recordPos + 8);
                        firstUnpacked = r.read_le<uint32_t>(recordPos + 12);
                    }
                    recordPos += chunkStep;
                }

                std::string rawPath = (i < fileNames.size() && !fileNames[i].empty()) ? fileNames[i] : ("tex_" + std::to_string(i) + "." + (ext.empty() ? "dds" : ext));
                auto sp = SplitVirtualPath(AsciiToWide(rawPath));

                ArchiveEntry e;
                e.name = sp.name;
                e.path = sp.cleanPath;
                e.folder = sp.folder;
                e.offset = firstOffset;
                e.compressedSize = firstPacked ? firstPacked : firstUnpacked;
                e.uncompressedSize = 128 + firstUnpacked;
                e.compressionMethod = firstPacked ? L"BA2 DX10 Zlib Texture" : L"BA2 DX10 Raw Texture";
                e.category = AssetCategory::Texture;
                e.inlineData = BuildDdsHeader128(texWidth, texHeight, numMips, dxgiFmt);
                e.crc32 = ComputeCrc32(e.inlineData);
                e.customFlags = ExtractMode::Ba2Dx10Tex | ((firstPacked > 0 ? 1u : 0u) << 8);
                arc.entries.push_back(std::move(e));
            }
        } else {
            for (uint32_t i = 0; i < safeCount && recordPos + 36 <= r.size(); ++i) {
                std::string ext = TrimA(r.read_ascii(recordPos + 4, 4, true));
                uint64_t offset = r.read_le<uint64_t>(recordPos + 16);
                uint32_t packedSize = r.read_le<uint32_t>(recordPos + 24);
                uint32_t unpackedSize = r.read_le<uint32_t>(recordPos + 28);
                recordPos += 36;

                std::string rawPath = (i < fileNames.size() && !fileNames[i].empty()) ? fileNames[i] : ("asset_" + std::to_string(i) + "." + (ext.empty() ? "bin" : ext));
                auto sp = SplitVirtualPath(AsciiToWide(rawPath));
                bool isComp = (packedSize > 0 && packedSize != unpackedSize);

                ArchiveEntry e;
                e.name = sp.name;
                e.path = sp.cleanPath;
                e.folder = sp.folder;
                e.offset = offset;
                e.compressedSize = packedSize ? packedSize : unpackedSize;
                e.uncompressedSize = unpackedSize ? unpackedSize : packedSize;
                e.compressionMethod = isComp ? L"Deflate / Zlib" : L"Uncompressed";
                e.crc32 = ComputeCrc32(r.slice(static_cast<size_t>(offset), std::min<size_t>(static_cast<size_t>(e.compressedSize), 1024)));
                e.category = ClassifyAsset(sp.cleanPath);
                e.customFlags = isComp ? ExtractMode::ZlibStream : ExtractMode::Stored;
                arc.entries.push_back(std::move(e));
            }
        }
    }
}

// ============================================================================
// 13. Rockstar RAGE (.rpf: RPF0..RPF7) & Ubisoft FORGE (.forge: scimitar\0)
// ============================================================================
inline void ParseRockstarRpfArchive(const std::wstring& path, BinaryReader& r, ParsedGameArchive& arc) {
    std::string magic = r.read_ascii(0, 4, false);
    uint32_t entryCount = r.read_le<uint32_t>(4);
    uint32_t namesLength = r.read_le<uint32_t>(8);
    uint32_t encryption = r.read_le<uint32_t>(12);

    std::wstring rpfVer = (magic == "RPF0" || magic == "RPF2") ? L"GTA IV / EFLC (RPF2)" :
                          (magic == "RPF3") ? L"Red Dead Redemption (RPF3)" :
                          (magic == "RPF4") ? L"Max Payne 3 (RPF4)" :
                          (magic == "RPF7") ? L"Grand Theft Auto V (RPF7)" : (L"Rockstar RAGE (" + AsciiToWide(magic) + L")");

    arc.valid = true;
    arc.formatId = ArchiveFormatId::RPF;
    arc.formatName = rpfVer;
    arc.engineName = L"Rockstar RAGE Engine";
    arc.isLittleEndian = true;
    arc.supportsRepack = false;

    AddHeaderField(arc, L"Signature", L"0x00000000", 4, AsciiToWide(magic) + L" (RAGE Package)", L"Rockstar Advanced Game Engine Archive");
    AddHeaderField(arc, L"TOC Entry Count", L"0x00000004", 4, std::to_wstring(entryCount), L"Directories and asset records");
    AddHeaderField(arc, L"Names Heap Size", L"0x00000008", 4, FormatBytesW(namesLength), L"Null-terminated string pool");
    AddHeaderField(arc, L"Encryption Mode", L"0x0000000C", 4, encryption == 0 ? L"None (Plaintext)" : L"OpenIV / Platform Encrypted", L"Payload cryptographic status");

    uint32_t safeCount = std::min<uint32_t>(entryCount, 15000);
    size_t tocOffset = 16;
    size_t namesOffset = tocOffset + static_cast<size_t>(entryCount) * 16;

    for (uint32_t i = 0; i < safeCount; ++i) {
        size_t p = tocOffset + static_cast<size_t>(i) * 16;
        if (p + 16 > r.size()) break;

        uint16_t nameOfs = r.read_le<uint16_t>(p);
        bool isDir = (r[p + 3] & 0x80) != 0;
        if (isDir) continue;

        uint64_t fileOffset = static_cast<uint64_t>(r.read_le<uint32_t>(p + 4) & 0x7FFFFFFFu) * 512ULL;
        uint32_t fileSize = r.read_le<uint32_t>(p + 8);
        uint32_t uncompSize = r.read_le<uint32_t>(p + 12);
        if (uncompSize == 0) uncompSize = fileSize;

        std::string rawName = (namesOffset + nameOfs < r.size()) ? r.read_cstr(namesOffset + nameOfs, 256) : "";
        if (rawName.empty()) rawName = "rpf_entry_" + std::to_string(i) + ".dat";
        auto sp = SplitVirtualPath(AsciiToWide(rawName));

        bool isDeflated = (fileSize > 0 && fileSize != uncompSize);
        ArchiveEntry e;
        e.name = sp.name;
        e.path = sp.cleanPath;
        e.folder = (sp.folder == L"(root)") ? L"assets" : sp.folder;
        e.offset = fileOffset;
        e.compressedSize = fileSize;
        e.uncompressedSize = uncompSize;
        e.compressionMethod = isDeflated ? L"Zlib Deflate" : L"Uncompressed";
        e.crc32 = ComputeCrc32(r.slice(static_cast<size_t>(fileOffset), std::min<size_t>(fileSize, 1024)));
        e.category = ClassifyAsset(sp.cleanPath);
        e.customFlags = isDeflated ? ExtractMode::ZlibStream : ExtractMode::Stored;
        arc.entries.push_back(std::move(e));
    }
}

inline void ParseUbisoftForgeArchive(const std::wstring& path, BinaryReader& r, ParsedGameArchive& arc) {
    bool isScimitar = (r.size() >= 8 && r.read_ascii(0, 8, false) == "scimitar");
    uint32_t fileCount = 0;
    uint64_t indexOffset = 0;

    if (isScimitar && r.size() >= 0x24) {
        fileCount = r.read_le<uint32_t>(16);
        indexOffset = r.read_le<uint64_t>(20);
    } else if (r.size() >= 0x20) {
        fileCount = r.read_le<uint32_t>(8);
        indexOffset = r.read_le<uint32_t>(12);
    }
    if (indexOffset == 0 || indexOffset >= r.size()) indexOffset = 0x20;
    if (fileCount == 0 || fileCount > 50000) {
        fileCount = static_cast<uint32_t>(std::min<size_t>((r.size() - static_cast<size_t>(indexOffset)) / 24, 2000));
    }

    arc.valid = true;
    arc.formatId = ArchiveFormatId::FORGE;
    arc.formatName = L"Ubisoft FORGE (Assassin's Creed / Anvil)";
    arc.engineName = L"Ubisoft Anvil / Scimitar Engine";
    arc.isLittleEndian = true;
    arc.supportsRepack = false;

    AddHeaderField(arc, L"Signature", L"0x00000000", 8, isScimitar ? L"scimitar\\0" : L"Ubisoft FORGE", L"Ubisoft Anvil / Scimitar Engine");
    AddHeaderField(arc, L"Total Resources", L"0x00000010", 4, std::to_wstring(fileCount), L"Asset lumps in index table");
    AddHeaderField(arc, L"Index Table Offset", L"0x00000014", 8, ToHexW(indexOffset), L"File allocation table offset");

    uint32_t safeCount = std::min<uint32_t>(fileCount, 5000);
    size_t p = static_cast<size_t>(indexOffset);
    for (uint32_t i = 0; i < safeCount && p + 24 <= r.size(); ++i) {
        uint64_t resId = r.read_le<uint64_t>(p);
        uint64_t dataOfs = r.read_le<uint64_t>(p + 8);
        uint32_t compSize = r.read_le<uint32_t>(p + 16);
        uint32_t uncompSize = r.read_le<uint32_t>(p + 20);
        if (uncompSize == 0) uncompSize = compSize;
        p += 24;

        wchar_t nameBuf[64]{};
        swprintf_s(nameBuf, L"resource_%016llX.forge_res", static_cast<unsigned long long>(resId));
        wchar_t folderBuf[32]{};
        swprintf_s(folderBuf, L"lumps_%04X", static_cast<unsigned int>((resId >> 48) & 0xFFFF));

        bool isComp = (compSize > 0 && compSize != uncompSize);
        ArchiveEntry e;
        e.name = nameBuf;
        e.folder = folderBuf;
        e.path = e.folder + L"/" + e.name;
        e.offset = dataOfs;
        e.compressedSize = compSize;
        e.uncompressedSize = uncompSize;
        e.compressionMethod = isComp ? L"Deflate / LZO" : L"Raw Stream";
        e.crc32 = ComputeCrc32(r.slice(static_cast<size_t>(dataOfs), std::min<size_t>(compSize, 1024)));
        e.category = AssetCategory::Binary;
        e.customFlags = isComp ? ExtractMode::ZlibStream : ExtractMode::Stored;
        arc.entries.push_back(std::move(e));
    }
}

// ============================================================================
// 14. FromSoftware (.dcx, .bnd, .bnd4, .chrbnd, .tpf) & id Tech 6 (.resource, .bmodel)
// ============================================================================
inline void ParseBnd3Span(const std::wstring& path, std::span<const uint8_t> bytes, ParsedGameArchive& arc, bool fromDcx = false) {
    BinaryReader r(bytes);
    std::string version = TrimA(r.read_ascii(4, 8, true));
    uint8_t formatFlags = r[12];
    bool isBigEndian = (r[13] == 1);
    uint32_t fileCount = isBigEndian ? r.read_be<uint32_t>(16) : r.read_le<uint32_t>(16);

    arc.valid = true;
    arc.formatId = ArchiveFormatId::FROMSOFT;
    arc.formatName = (fromDcx ? L"DCX -> " : L"") + std::wstring(L"FromSoftware BND3 (Dark Souls / Demon's Souls)");
    arc.engineName = L"FromSoftware Soulsborne Engine";
    arc.isLittleEndian = !isBigEndian;

    AddHeaderField(arc, L"Signature", L"0x00000000", 4, L"BND3 (Binder 3)", L"FromSoftware Game Resource Package");
    AddHeaderField(arc, L"Engine Schema Revision", L"0x00000004", 8, AsciiToWide(version), L"FromSoftware binder version stamp");
    AddHeaderField(arc, L"Format Flags", L"0x0000000C", 1, ToHexW(formatFlags, 2), L"File header descriptor flags");
    AddHeaderField(arc, L"Asset Count", L"0x00000010", 4, std::to_wstring(fileCount), L"Total internal records");

    uint32_t safeCount = std::min<uint32_t>(fileCount, 10000);
    size_t currPos = 0x20;
    for (uint32_t i = 0; i < safeCount && currPos + 24 <= r.size(); ++i) {
        uint32_t compSize = isBigEndian ? r.read_be<uint32_t>(currPos + 4) : r.read_le<uint32_t>(currPos + 4);
        uint32_t dataOfs = isBigEndian ? r.read_be<uint32_t>(currPos + 8) : r.read_le<uint32_t>(currPos + 8);
        uint32_t fileId = isBigEndian ? r.read_be<uint32_t>(currPos + 12) : r.read_le<uint32_t>(currPos + 12);
        uint32_t nameOfs = isBigEndian ? r.read_be<uint32_t>(currPos + 16) : r.read_le<uint32_t>(currPos + 16);
        uint32_t uncompSize = isBigEndian ? r.read_be<uint32_t>(currPos + 20) : r.read_le<uint32_t>(currPos + 20);
        if (uncompSize == 0) uncompSize = compSize;
        currPos += 24;

        std::string rawPath = (nameOfs > 0 && nameOfs < r.size()) ? r.read_cstr(nameOfs, 256) : "";
        if (rawPath.empty()) rawPath = "asset_" + std::to_string(fileId) + ".dat";
        auto sp = SplitVirtualPath(AsciiToWide(rawPath));
        auto slice = r.slice(dataOfs, compSize);

        ArchiveEntry e;
        e.name = sp.name;
        e.path = sp.cleanPath;
        e.folder = sp.folder;
        e.offset = dataOfs;
        e.compressedSize = compSize;
        e.uncompressedSize = uncompSize;
        e.compressionMethod = L"BND3 Stream";
        e.crc32 = ComputeCrc32(slice);
        e.category = ClassifyAsset(sp.cleanPath, slice);
        if (fromDcx) {
            e.inlineData.assign(slice.begin(), slice.end());
            e.customFlags = ExtractMode::InlineOnly;
        } else {
            e.customFlags = ExtractMode::Stored;
        }
        arc.entries.push_back(std::move(e));
    }
}

inline void ParseBnd4Span(const std::wstring& path, std::span<const uint8_t> bytes, ParsedGameArchive& arc, bool fromDcx = false) {
    BinaryReader r(bytes);
    bool isBigEndian = (r[9] == 1);
    bool isUnicode = (r.size() >= 33) ? (r[32] == 1) : true;
    uint64_t fileCount = isBigEndian ? r.read_be<uint64_t>(12) : r.read_le<uint64_t>(12);
    std::string version = TrimA(r.read_ascii(24, 8, true));
    uint64_t fileHeadersOffset = isBigEndian ? r.read_be<uint64_t>(44) : r.read_le<uint64_t>(44);

    arc.valid = true;
    arc.formatId = ArchiveFormatId::FROMSOFT;
    arc.formatName = (fromDcx ? L"DCX -> " : L"") + std::wstring(L"FromSoftware BND4 (Elden Ring / DS3 / Sekiro)");
    arc.engineName = L"FromSoftware Soulsborne Engine";
    arc.isLittleEndian = !isBigEndian;

    AddHeaderField(arc, L"Signature", L"0x00000000", 4, L"BND4 (Binder 4)", L"Modern FromSoftware Universal Container");
    AddHeaderField(arc, L"Engine Generation", L"0x00000018", 8, L"BND4 (" + AsciiToWide(version) + L")", L"DS3 / Bloodborne / Elden Ring schema");
    AddHeaderField(arc, L"Asset Count", L"0x0000000C", 8, std::to_wstring(fileCount), L"Bound asset records");

    uint64_t safeCount = std::min<uint64_t>(fileCount, 15000);
    size_t currPos = (fileHeadersOffset > 0 && fileHeadersOffset < r.size()) ? static_cast<size_t>(fileHeadersOffset) : 0x40;
    for (uint64_t i = 0; i < safeCount && currPos + 36 <= r.size(); ++i) {
        uint64_t compSize = isBigEndian ? r.read_be<uint64_t>(currPos + 8) : r.read_le<uint64_t>(currPos + 8);
        uint64_t uncompSize = isBigEndian ? r.read_be<uint64_t>(currPos + 16) : r.read_le<uint64_t>(currPos + 16);
        uint64_t dataOfs = isBigEndian ? r.read_be<uint64_t>(currPos + 24) : r.read_le<uint64_t>(currPos + 24);
        uint32_t fileId = isBigEndian ? r.read_be<uint32_t>(currPos + 32) : r.read_le<uint32_t>(currPos + 32);
        uint32_t nameOfs = (currPos + 40 <= r.size()) ? (isBigEndian ? r.read_be<uint32_t>(currPos + 36) : r.read_le<uint32_t>(currPos + 36)) : 0;

        std::wstring rawWPath;
        size_t targetNamePos = (nameOfs > 0 && nameOfs < r.size()) ? nameOfs : (currPos + 36);
        if (targetNamePos < r.size()) {
            if (isUnicode) {
                rawWPath = r.read_utf16le(targetNamePos, 256);
            } else {
                rawWPath = AsciiToWide(r.read_cstr(targetNamePos, 256));
            }
        }
        if (rawWPath.empty()) rawWPath = L"asset_" + std::to_wstring(fileId) + L".dat";
        auto sp = SplitVirtualPath(rawWPath);
        auto slice = r.slice(static_cast<size_t>(dataOfs), static_cast<size_t>(compSize));

        ArchiveEntry e;
        e.name = sp.name;
        e.path = sp.cleanPath;
        e.folder = sp.folder;
        e.offset = dataOfs;
        e.compressedSize = compSize;
        e.uncompressedSize = uncompSize ? uncompSize : compSize;
        e.compressionMethod = L"BND4 Stream";
        e.crc32 = ComputeCrc32(slice);
        e.category = ClassifyAsset(sp.cleanPath, slice);
        if (fromDcx) {
            e.inlineData.assign(slice.begin(), slice.end());
            e.customFlags = ExtractMode::InlineOnly;
        } else {
            e.customFlags = ExtractMode::Stored;
        }
        arc.entries.push_back(std::move(e));
        currPos += 40;
    }
}

inline void ParseFromSoftArchive(const std::wstring& path, BinaryReader& r, ParsedGameArchive& arc) {
    std::string magic4 = r.read_ascii(0, 4, false);
    if (magic4 == "BND3") {
        ParseBnd3Span(path, r.span(), arc, false);
        return;
    }
    if (magic4 == "BND4") {
        ParseBnd4Span(path, r.span(), arc, false);
        return;
    }

    // DCX\0 container
    uint32_t uncompSize = 0;
    uint32_t compSize = 0;
    std::string compType = "DFLT";
    size_t dataOffset = 0x48;

    for (size_t i = 0; i + 8 < std::min<size_t>(r.size(), 256); ++i) {
        std::string chunk = r.read_ascii(i, 3, false);
        if (chunk == "DCS") {
            uncompSize = r.read_be<uint32_t>(i + 4);
            compSize = r.read_be<uint32_t>(i + 8);
        } else if (chunk == "DCP") {
            compType = r.read_ascii(i + 4, 4, false);
        } else if (chunk == "DCA") {
            uint32_t dcaHdrLen = r.read_be<uint32_t>(i + 4);
            dataOffset = (dcaHdrLen > 0) ? (i + 4 + dcaHdrLen) : (i + 8);
            if (dataOffset < r.size() && r[dataOffset] != 0x78) {
                for (size_t s = i + 8; s + 2 < std::min<size_t>(r.size(), i + 128); ++s) {
                    if (r[s] == 0x78 && (r[s + 1] == 0x01 || r[s + 1] == 0x5E || r[s + 1] == 0x9C || r[s + 1] == 0xDA)) {
                        dataOffset = s;
                        break;
                    }
                }
            }
            break;
        }
    }

    std::vector<uint8_t> decomp;
    bool decompOk = false;
    if (dataOffset < r.size()) {
        auto compSpan = r.slice(dataOffset, r.size() - dataOffset);
        if (compType.find("LZ4") != std::string::npos && uncompSize > 0) {
            decompOk = DecompressLz4Block(compSpan, decomp, uncompSize);
        } else {
            decompOk = DecompressZlib(compSpan, decomp, uncompSize);
            if (!decompOk) {
                decompOk = InflateRaw(compSpan, decomp, uncompSize);
            }
        }
    }

    if (decompOk && decomp.size() >= 4) {
        if (std::memcmp(decomp.data(), "BND3", 4) == 0) {
            ParseBnd3Span(path, decomp, arc, true);
            return;
        }
        if (std::memcmp(decomp.data(), "BND4", 4) == 0) {
            ParseBnd4Span(path, decomp, arc, true);
            return;
        }
    }

    arc.valid = true;
    arc.formatId = ArchiveFormatId::FROMSOFT;
    arc.formatName = L"FromSoftware DCX (" + AsciiToWide(compType) + L")";
    arc.engineName = L"FromSoftware Soulsborne Engine";
    arc.isLittleEndian = false;
    arc.supportsRepack = false;

    AddHeaderField(arc, L"Signature", L"0x00000000", 4, L"DCX\\0 (FromSoftware Container)", L"FromSoftware Compressed Container");
    AddHeaderField(arc, L"Compression Algorithm", L"DCP", 4, AsciiToWide(compType), L"Payload compression method");
    AddHeaderField(arc, L"Uncompressed Size", L"DCS", 4, FormatBytesW(uncompSize ? uncompSize : decomp.size()), L"Target buffer allocation size");

    std::wstring unwrappedName = ExtractFileNameW(path);
    if (EndsWithW(ToLowerW(unwrappedName), L".dcx")) {
        unwrappedName.erase(unwrappedName.size() - 4);
    }

    ArchiveEntry e;
    e.name = unwrappedName;
    e.folder = L"(root)";
    e.path = unwrappedName;
    e.offset = dataOffset;
    e.compressedSize = (dataOffset < r.size()) ? (r.size() - dataOffset) : 0;
    e.uncompressedSize = decompOk ? decomp.size() : (uncompSize ? uncompSize : e.compressedSize);
    e.compressionMethod = L"DCX " + AsciiToWide(compType);
    if (decompOk) {
        e.crc32 = ComputeCrc32(decomp);
        e.category = ClassifyAsset(unwrappedName, decomp);
        e.inlineData = std::move(decomp);
        e.customFlags = ExtractMode::InlineOnly;
    } else {
        auto rawSlice = r.slice(dataOffset, e.compressedSize);
        e.crc32 = ComputeCrc32(rawSlice);
        e.category = ClassifyAsset(unwrappedName, rawSlice);
        e.customFlags = ExtractMode::ZlibStream;
    }
    arc.entries.push_back(std::move(e));
}

inline void ParseIdTech6Archive(const std::wstring& path, BinaryReader& r, ParsedGameArchive& arc) {
    arc.valid = true;
    arc.formatId = ArchiveFormatId::IDTECH;
    arc.engineName = L"id Software id Tech 6 (DOOM 2016 / DOOM Eternal)";
    arc.isLittleEndian = false;
    arc.supportsRepack = false;

    std::wstring baseName = ExtractFileNameW(path);

    if (r.size() >= 8 && r[0] == 0x1B && r[1] == 0x4C && r[2] == 0x4D && r[3] == 0x42) {
        arc.formatName = L"id Tech 6 Binary Model (.bmodel)";
        int32_t meshCount = r.read_le<int32_t>(4);
        size_t p = 8;
        std::string report = "// id Tech 6 Binary Model (.bmodel): " + WideToUtf8Str(baseName) + "\n";
        report += "// Engine: DOOM (2016) / id Tech 6\n";
        report += "// Total Sub-Meshes: " + std::to_string(std::max<int32_t>(0, meshCount)) + "\n\n=== SUB-MESH & MATERIAL MAPPINGS ===\n";

        int32_t safeMeshes = std::min<int32_t>(std::max<int32_t>(0, meshCount), 128);
        for (int32_t i = 0; i < safeMeshes && p + 12 <= r.size(); ++i) {
            int32_t matLen = r.read_le<int32_t>(p);
            p += 4;
            std::string matName = "material_" + std::to_string(i);
            if (matLen > 0 && matLen < 256 && p + static_cast<size_t>(matLen) <= r.size()) {
                matName = r.read_cstr(p, static_cast<size_t>(matLen));
                p += static_cast<size_t>(matLen);
            }
            int32_t vertCount = (p + 4 <= r.size()) ? std::abs(r.read_be<int32_t>(p)) : 0;
            p += 4;
            int32_t faceCount = (p + 4 <= r.size()) ? std::abs(r.read_be<int32_t>(p)) : 0;
            p += 4;
            report += "[Mesh #" + std::to_string(i) + "] Material: " + matName +
                      " | Vertices: " + std::to_string(vertCount) + " | Faces: " + std::to_string(faceCount) + "\n";
        }

        AddHeaderField(arc, L"id Tech 6 Model Signature", L"0x00000000", 4, L"1B 4C 4D 42 (\"\\x1bLMB\")", L"DOOM (2016) pre-processed binary mesh model");
        AddHeaderField(arc, L"Sub-mesh Count", L"0x00000004", 4, std::to_wstring(std::max<int32_t>(0, meshCount)) + L" sub-meshes", L"Indexed materials and vertex buffers");

        ArchiveEntry declE;
        declE.name = baseName + L"_decl.txt";
        declE.folder = L"geometry";
        declE.path = L"geometry/" + declE.name;
        declE.inlineData = StringToBytes(report);
        declE.compressedSize = declE.inlineData.size();
        declE.uncompressedSize = declE.inlineData.size();
        declE.compressionMethod = L"Decoded BModel Structure";
        declE.crc32 = ComputeCrc32(declE.inlineData);
        declE.category = AssetCategory::Config;
        declE.customFlags = ExtractMode::InlineOnly;
        arc.entries.push_back(std::move(declE));

        ArchiveEntry rawE;
        rawE.name = baseName;
        rawE.folder = L"(root)";
        rawE.path = baseName;
        rawE.offset = 0;
        rawE.compressedSize = r.size();
        rawE.uncompressedSize = r.size();
        rawE.compressionMethod = L"id Tech 6 BModel Blob";
        rawE.crc32 = ComputeCrc32(r.slice(0, std::min<size_t>(r.size(), 65536)));
        rawE.category = AssetCategory::Model3D;
        rawE.customFlags = ExtractMode::Stored;
        arc.entries.push_back(std::move(rawE));
        return;
    }

    int32_t numEntries = (r.size() >= 12) ? r.read_le<int32_t>(8) : 0;
    uint8_t headerVersion = (r[0] == 0x52 && r[1] == 0x45 && r[2] == 0x53) ? 4 : r[0];
    arc.formatName = L"id Tech 6 Resource Container (DOOM 2016)";

    AddHeaderField(arc, L"id Tech 6 Container Magic", L"0x00000000", 4, BytesHexPreviewW(r.slice(0, 4), 4), L"DOOM 2016 .resource / .index header");
    AddHeaderField(arc, L"Index Table Entries", L"0x00000008", 4, std::to_wstring(std::max<int32_t>(0, numEntries)) + L" entries", L"Virtual assets in DOOM 2016 package");

    size_t pos = 12;
    if (numEntries > 0 && numEntries < 100000 && pos < r.size()) {
        for (int32_t i = 0; i < numEntries && pos + 24 <= r.size(); ++i) {
            int32_t id = r.read_be<int32_t>(pos); pos += 4;
            auto readLenStr = [&]() -> std::string {
                if (pos + 4 > r.size()) return {};
                int32_t len = r.read_le<int32_t>(pos); pos += 4;
                if (len > 0 && len < 512 && pos + static_cast<size_t>(len) <= r.size()) {
                    std::string s = r.read_cstr(pos, static_cast<size_t>(len));
                    pos += static_cast<size_t>(len);
                    return s;
                }
                return {};
            };
            std::string fileType = readLenStr();
            std::string name2 = readLenStr();
            std::string name3 = readLenStr();
            if (pos + 17 > r.size()) break;

            int64_t offset = r.read_be<int64_t>(pos); pos += 8;
            int32_t size = r.read_be<int32_t>(pos); pos += 4;
            int32_t compSize = r.read_be<int32_t>(pos); pos += 4;
            pos += (headerVersion <= 4 ? 8 : 4) + 1;

            std::string rawPath = !name3.empty() ? name3 : (!name2.empty() ? name2 : (!fileType.empty() ? fileType : ("res_file_" + std::to_string(id))));
            auto sp = SplitVirtualPath(AsciiToWide(rawPath));
            bool isComp = (size != compSize && compSize > 0);

            ArchiveEntry e;
            e.name = sp.name;
            e.path = sp.cleanPath;
            e.folder = sp.folder;
            e.offset = static_cast<uint64_t>(std::max<int64_t>(0, offset));
            e.compressedSize = static_cast<uint64_t>(std::max<int32_t>(0, compSize));
            e.uncompressedSize = static_cast<uint64_t>(std::max<int32_t>(0, size));
            e.compressionMethod = isComp ? L"Zlib / Deflate" : L"Stored";
            e.crc32 = ComputeCrc32(r.slice(static_cast<size_t>(e.offset), std::min<size_t>(static_cast<size_t>(e.compressedSize), 1024)));
            e.category = ClassifyAsset(sp.cleanPath);
            e.customFlags = isComp ? ExtractMode::ZlibStream : ExtractMode::Stored;
            arc.entries.push_back(std::move(e));
        }
    }

    if (arc.entries.empty()) {
        ArchiveEntry rawE;
        rawE.name = baseName;
        rawE.folder = L"(root)";
        rawE.path = baseName;
        rawE.offset = 0;
        rawE.compressedSize = r.size();
        rawE.uncompressedSize = r.size();
        rawE.compressionMethod = L"id Tech 6 Container Stream";
        rawE.crc32 = ComputeCrc32(r.slice(0, std::min<size_t>(r.size(), 65536)));
        rawE.category = AssetCategory::Binary;
        rawE.customFlags = ExtractMode::Stored;
        arc.entries.push_back(std::move(rawE));
    }
}

// ============================================================================
// 15. Final Fantasy XIII (white_data) & Square Enix (SDAT, PFS , pf8 )
// ============================================================================
inline void ParseFf13Archive(const std::wstring& path, BinaryReader& r, ParsedGameArchive& arc) {
    arc.valid = true;
    arc.formatId = ArchiveFormatId::FF13;
    arc.engineName = L"Square Enix Crystal Tools (FFXIII / FFXIII-2 / LR)";
    arc.isLittleEndian = true;
    arc.supportsRepack = false;

    size_t baseOffset = (r.size() > 44 && r.read_ascii(0, 4, false) == "CRYP") ? 32 : 0;
    uint32_t chunkInfoTableOfs = r.read_le<uint32_t>(baseOffset);
    uint32_t chunkDataStartOfs = r.read_le<uint32_t>(baseOffset + 4);
    uint32_t fileCount = r.read_le<uint32_t>(baseOffset + 8);

    bool isValidFilelist = (fileCount > 0 && fileCount < 200000 &&
                            chunkInfoTableOfs > 12 && chunkInfoTableOfs < r.size() &&
                            chunkDataStartOfs > chunkInfoTableOfs && chunkDataStartOfs < r.size());

    arc.formatName = isValidFilelist ? L"Final Fantasy XIII White Data (Filelist)" : L"Final Fantasy XIII White Data (Archive)";

    if (isValidFilelist) {
        AddHeaderField(arc, L"FFXIII Filelist Header", ToHexW(baseOffset), 12, L"White Data Filelist (" + std::to_wstring(fileCount) + L" files)", L"FFXIII white_data master filelist archive");
        AddHeaderField(arc, L"Chunk Info Table Offset", ToHexW(baseOffset), 4, ToHexW(chunkInfoTableOfs), L"Byte offset of chunk descriptor table");

        struct Ff13EntryRec { uint32_t fileCode; uint16_t chunkId; uint16_t infoPos; };
        std::vector<Ff13EntryRec> entriesList;
        uint16_t maxChunkId = 0;
        size_t entryPos = baseOffset + 12;
        for (uint32_t i = 0; i < fileCount && entryPos + 8 <= baseOffset + chunkInfoTableOfs && entryPos + 8 <= r.size(); ++i) {
            uint32_t fc = r.read_le<uint32_t>(entryPos);
            uint16_t cid = r.read_le<uint16_t>(entryPos + 4);
            uint16_t ip = r.read_le<uint16_t>(entryPos + 6);
            entriesList.push_back({ fc, cid, ip });
            if (cid > maxChunkId) maxChunkId = cid;
            entryPos += 8;
        }

        uint32_t chunkCount = static_cast<uint32_t>(maxChunkId) + 1;
        std::vector<std::string> decompressedChunks(chunkCount);
        size_t chunkInfoPos = baseOffset + chunkInfoTableOfs;
        for (uint32_t c = 0; c < chunkCount && chunkInfoPos + 12 <= r.size(); ++c) {
            uint32_t uncmpSize = r.read_le<uint32_t>(chunkInfoPos);
            uint32_t cmpSize = r.read_le<uint32_t>(chunkInfoPos + 4);
            uint32_t relPos = r.read_le<uint32_t>(chunkInfoPos + 8);
            chunkInfoPos += 12;
            size_t absPos = baseOffset + chunkDataStartOfs + relPos;
            if (absPos + cmpSize <= r.size()) {
                auto cmpSpan = r.slice(absPos, cmpSize);
                if (uncmpSize == cmpSize) {
                    decompressedChunks[c].assign(reinterpret_cast<const char*>(cmpSpan.data()), cmpSpan.size());
                } else {
                    std::vector<uint8_t> dec;
                    if (DecompressZlib(cmpSpan, dec, uncmpSize)) {
                        decompressedChunks[c].assign(reinterpret_cast<const char*>(dec.data()), dec.size());
                    }
                }
            }
        }

        for (const auto& item : entriesList) {
            if (item.chunkId >= decompressedChunks.size()) continue;
            const std::string& chunkStr = decompressedChunks[item.chunkId];
            if (item.infoPos >= chunkStr.size()) continue;
            size_t nullTerm = chunkStr.find('\0', item.infoPos);
            std::string line = (nullTerm != std::string::npos)
                ? chunkStr.substr(item.infoPos, nullTerm - item.infoPos)
                : chunkStr.substr(item.infoPos);

            size_t c1 = line.find(':');
            if (c1 == std::string::npos) continue;
            size_t c2 = line.find(':', c1 + 1);
            if (c2 == std::string::npos) continue;
            size_t c3 = line.find(':', c2 + 1);
            if (c3 == std::string::npos) continue;

            uint64_t secOfs = std::strtoull(line.substr(0, c1).c_str(), nullptr, 16);
            uint64_t uSize = std::strtoull(line.substr(c1 + 1, c2 - c1 - 1).c_str(), nullptr, 16);
            uint64_t cSize = std::strtoull(line.substr(c2 + 1, c3 - c2 - 1).c_str(), nullptr, 16);
            std::string vPath = line.substr(c3 + 1);

            auto sp = SplitVirtualPath(AsciiToWide(vPath.empty() ? ("file_" + std::to_string(item.fileCode)) : vPath));
            ArchiveEntry e;
            e.name = sp.name;
            e.path = sp.cleanPath;
            e.folder = sp.folder;
            e.offset = secOfs * 2048ULL;
            e.compressedSize = cSize;
            e.uncompressedSize = uSize;
            e.compressionMethod = (cSize == uSize) ? L"WhiteBin Sector Store (2048 B)" : L"WhiteBin Zlib Stream";
            e.crc32 = item.fileCode;
            e.category = ClassifyAsset(sp.cleanPath);
            e.customFlags = ExtractMode::Stored;
            arc.entries.push_back(std::move(e));
        }
    }

    if (arc.entries.empty()) {
        AddHeaderField(arc, L"White Data Image / Binary Stream", L"0x00000000", 8, BytesHexPreviewW(r.slice(0, 8), 8), L"FFXIII raw white_data binary container");
        size_t sectorCount = std::min<size_t>(r.size() / 2048, 2000);
        for (size_t s = 0; s < sectorCount; ++s) {
            size_t off = s * 2048;
            if (off + 4 < r.size() && r[off] == 0x78 && (r[off + 1] == 0x9C || r[off + 1] == 0xDA)) {
                std::vector<uint8_t> dec;
                size_t consumed = 0;
                if (DecompressZlib(r.slice(off, std::min<size_t>(r.size() - off, 262144)), dec, 0, &consumed) && dec.size() >= 32) {
                    AssetCategory cat = ClassifyAsset(L"block", dec);
                    std::wstring ext = (cat == AssetCategory::Texture) ? L".dds" : ((cat == AssetCategory::Audio) ? L".scd" : L".dat");
                    ArchiveEntry e;
                    e.name = L"white_sector_" + std::to_wstring(s) + ext;
                    e.folder = L"sectors";
                    e.path = L"sectors/" + e.name;
                    e.offset = off;
                    e.compressedSize = consumed ? consumed : 2048;
                    e.uncompressedSize = dec.size();
                    e.compressionMethod = L"Zlib Compressed Sector (2048 B aligned)";
                    e.crc32 = ComputeCrc32(dec);
                    e.category = cat;
                    e.inlineData = std::move(dec);
                    e.customFlags = ExtractMode::InlineOnly;
                    arc.entries.push_back(std::move(e));
                }
            }
        }
    }
}

inline void ParseSquareArchive(const std::wstring& path, BinaryReader& r, ParsedGameArchive& arc) {
    std::string magic4 = r.read_ascii(0, 4, false);
    arc.valid = true;
    arc.formatId = ArchiveFormatId::SQUARE;
    arc.isLittleEndian = true;
    arc.supportsRepack = false;

    if (magic4 == "SDAT" || EndsWithW(ToLowerW(path), L".sdat")) {
        uint16_t version = r.read_le<uint16_t>(6);
        uint32_t fileSize = r.read_le<uint32_t>(8);
        uint32_t symbOffset = r.read_le<uint32_t>(16);
        uint32_t symbSize = r.read_le<uint32_t>(20);
        uint32_t fatOffset = r.read_le<uint32_t>(32);
        uint32_t fileOffset = r.read_le<uint32_t>(40);

        arc.formatName = L"Square Enix / NDS SDAT Sound Archive";
        arc.engineName = L"Nintendo DS / Square Enix Sound Engine";

        std::unordered_map<uint32_t, std::string> symbolMap;
        if (symbOffset > 0 && symbOffset + 8 < r.size() && r.read_ascii(symbOffset, 4, false) == "SYMB") {
            size_t strSearch = symbOffset + 0x20;
            size_t searchEnd = std::min<size_t>(r.size(), symbOffset + (symbSize ? symbSize : 0x4000));
            uint32_t symIdx = 0;
            while (strSearch < searchEnd && symIdx < 1000) {
                if (r[strSearch] >= 32 && r[strSearch] <= 126) {
                    std::string s = r.read_cstr(strSearch, 128);
                    bool validSym = (s.size() >= 3);
                    for (char c : s) {
                        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')) {
                            validSym = false;
                            break;
                        }
                    }
                    if (validSym) symbolMap[symIdx++] = s;
                    strSearch += s.size() + 1;
                } else {
                    ++strSearch;
                }
            }
        }

        uint32_t fileCount = 0;
        if (fatOffset > 0 && fatOffset + 12 <= r.size()) {
            fileCount = r.read_le<uint32_t>(fatOffset + 8);
            uint32_t safeCount = std::min<uint32_t>(fileCount, 5000);
            for (uint32_t i = 0; i < safeCount; ++i) {
                size_t recPos = static_cast<size_t>(fatOffset) + 12 + static_cast<size_t>(i) * 16;
                if (recPos + 8 > r.size()) break;
                uint32_t ofs = r.read_le<uint32_t>(recPos);
                uint32_t sz = r.read_le<uint32_t>(recPos + 4);
                if (ofs == 0 || sz == 0 || ofs >= r.size()) continue;

                auto slice = r.slice(ofs, sz);
                std::string lumpMagic = (slice.size() >= 4) ? std::string(reinterpret_cast<const char*>(slice.data()), 4) : "";
                std::wstring ext = L"bin";
                std::wstring folder = L"sound";
                std::wstring desc = L"Raw Audio Lump";
                if (lumpMagic == "SSEQ") { ext = L"sseq"; folder = L"sequences"; desc = L"MIDI / Music Sequence (SSEQ)"; }
                else if (lumpMagic == "SBNK") { ext = L"sbnk"; folder = L"banks"; desc = L"Sound Bank / Instrument Patch (SBNK)"; }
                else if (lumpMagic == "SWAR") { ext = L"swar"; folder = L"wave_archives"; desc = L"Wave Archive / Sample Bank (SWAR)"; }
                else if (lumpMagic == "SWAV") { ext = L"swav"; folder = L"waves"; desc = L"Direct PCM / ADPCM Sample (SWAV)"; }

                wchar_t numBuf[32]{};
                swprintf_s(numBuf, L"sound_%04u", i);
                std::wstring base = symbolMap.count(i) ? AsciiToWide(symbolMap[i]) : std::wstring(numBuf);
                std::wstring lumpName = base + L"." + ext;

                ArchiveEntry e;
                e.name = lumpName;
                e.folder = folder;
                e.path = folder + L"/" + lumpName;
                e.offset = ofs;
                e.compressedSize = sz;
                e.uncompressedSize = sz;
                e.compressionMethod = desc;
                e.crc32 = ComputeCrc32(slice);
                e.category = AssetCategory::Audio;
                e.customFlags = ExtractMode::Stored;
                arc.entries.push_back(std::move(e));
            }
        }

        AddHeaderField(arc, L"Signature", L"0x00000000", 4, L"SDAT (Sound Data Archive)", L"Nintendo DS / Square Enix Audio Package");
        AddHeaderField(arc, L"Version", L"0x00000006", 2, ToHexW(version, 4), L"SDAT Specification Version");
        AddHeaderField(arc, L"Container Size", L"0x00000008", 4, FormatBytesW(fileSize), L"Total internal byte footprint");
        AddHeaderField(arc, L"Allocated Sounds", L"FAT", 4, std::to_wstring(fileCount), L"Music sequences, sound banks, wavebanks");
        AddHeaderField(arc, L"FILE Block Offset", L"0x00000028", 4, ToHexW(fileOffset), L"Base address for raw waveform payloads");
        return;
    }

    // Square Enix PFS / pf8 container
    uint32_t version = r.read_le<uint32_t>(4);
    uint32_t fileCount = r.read_le<uint32_t>(8);
    uint32_t dirOffset = r.read_le<uint32_t>(12);
    arc.formatName = L"Square Enix PFS Container (" + AsciiToWide(TrimA(magic4)) + L")";
    arc.engineName = L"Square Enix Crystal Tools / Luminous";

    AddHeaderField(arc, L"Signature", L"0x00000000", 4, AsciiToWide(magic4) + L" (Square Enix Container)", L"Square Enix Master Asset Container");
    AddHeaderField(arc, L"Version", L"0x00000004", 4, L"Revision " + std::to_wstring(version), L"PFS archive format version");
    AddHeaderField(arc, L"Asset Count", L"0x00000008", 4, std::to_wstring(fileCount), L"Packaged resource count");

    uint32_t safeCount = std::min<uint32_t>(fileCount, 10000);
    size_t pos = (dirOffset > 0 && dirOffset < r.size()) ? dirOffset : 0x20;
    for (uint32_t i = 0; i < safeCount && pos + 32 <= r.size(); ++i) {
        uint32_t dataOfs = r.read_le<uint32_t>(pos);
        uint32_t compSize = r.read_le<uint32_t>(pos + 4);
        uint32_t uncompSize = r.read_le<uint32_t>(pos + 8);
        uint32_t flags = r.read_le<uint32_t>(pos + 12);
        std::string rawName = TrimA(r.read_cstr(pos + 16, 16));
        if (rawName.empty()) rawName = "asset_" + std::to_string(i) + ".dat";
        pos += 32;

        auto sp = SplitVirtualPath(AsciiToWide(rawName));
        bool isComp = (compSize < uncompSize && (flags & 1) != 0);

        ArchiveEntry e;
        e.name = sp.name;
        e.path = sp.cleanPath;
        e.folder = sp.folder;
        e.offset = dataOfs;
        e.compressedSize = compSize;
        e.uncompressedSize = uncompSize ? uncompSize : compSize;
        e.compressionMethod = isComp ? L"Zlib Deflate" : L"Raw Stream";
        e.crc32 = ComputeCrc32(r.slice(dataOfs, std::min<size_t>(compSize, 1024)));
        e.category = ClassifyAsset(sp.cleanPath);
        e.customFlags = isComp ? ExtractMode::ZlibStream : ExtractMode::Stored;
        arc.entries.push_back(std::move(e));
    }
}

// ============================================================================
// 16. CRIWARE (.awb AFS2, .acb @UTF, .usm CRID, .adx, .hca HCA\0)
// ============================================================================
inline void ParseCriwareAfs2Span(std::span<const uint8_t> bytes, size_t baseOffsetInFile, ParsedGameArchive& arc, const std::wstring& folderPrefix = L"audio") {
    BinaryReader r(bytes);
    uint8_t offsetSize = (r.size() >= 6) ? r[5] : 4;
    if (offsetSize != 2 && offsetSize != 4) offsetSize = 4;
    uint32_t waveCount = (r.size() >= 12) ? r.read_le<uint32_t>(8) : 0;
    uint32_t alignment = (r.size() >= 16) ? r.read_le<uint32_t>(12) : 0x20;
    uint32_t safeCount = std::min<uint32_t>(waveCount, 10000);

    size_t offsetsTableOfs = 0x10 + static_cast<size_t>(safeCount) * 2;
    std::vector<uint32_t> offsets;
    offsets.reserve(safeCount + 1);
    for (uint32_t i = 0; i <= safeCount; ++i) {
        size_t pos = offsetsTableOfs + static_cast<size_t>(i) * offsetSize;
        uint32_t rawOfs = 0;
        if (pos + offsetSize <= r.size()) {
            rawOfs = (offsetSize == 2) ? r.read_le<uint16_t>(pos) : r.read_le<uint32_t>(pos);
        } else {
            rawOfs = static_cast<uint32_t>(r.size());
        }
        if (alignment > 1 && (rawOfs % alignment) != 0) {
            rawOfs = ((rawOfs + alignment - 1) / alignment) * alignment;
        }
        offsets.push_back(rawOfs);
    }

    for (uint32_t i = 0; i < safeCount; ++i) {
        uint32_t startOfs = offsets[i];
        uint32_t endOfs = (i + 1 < offsets.size() && offsets[i + 1] >= startOfs) ? offsets[i + 1] : static_cast<uint32_t>(r.size());
        uint32_t sz = (endOfs > startOfs && startOfs < r.size()) ? std::min<uint32_t>(endOfs - startOfs, static_cast<uint32_t>(r.size() - startOfs)) : 0;

        std::wstring ext = L"hca";
        std::wstring comp = L"CRI High Compression Audio (HCA)";
        if (startOfs + 4 <= r.size() && r[startOfs] == 0x80 && r[startOfs + 1] == 0x00) {
            ext = L"adx";
            comp = L"CRI ADX ADPCM Audio";
        }
        wchar_t trackBuf[64]{};
        swprintf_s(trackBuf, L"track_%04u.%s", i, ext.c_str());

        ArchiveEntry e;
        e.name = trackBuf;
        e.folder = folderPrefix;
        e.path = folderPrefix + L"/" + e.name;
        e.offset = baseOffsetInFile + startOfs;
        e.compressedSize = sz;
        e.uncompressedSize = sz;
        e.compressionMethod = comp;
        e.crc32 = ComputeCrc32(r.slice(startOfs, std::min<size_t>(sz, 1024)));
        e.category = AssetCategory::Audio;
        e.customFlags = ExtractMode::Stored;
        arc.entries.push_back(std::move(e));
    }
}

inline void ParseCriwareArchive(const std::wstring& path, BinaryReader& r, ParsedGameArchive& arc) {
    std::string magic4 = r.read_ascii(0, 4, false);
    std::wstring lower = ToLowerW(path);
    std::wstring baseName = ExtractFileNameW(path);

    arc.valid = true;
    arc.formatId = ArchiveFormatId::CRIWARE;
    arc.supportsRepack = false;

    if (magic4 == "AFS2" || EndsWithW(lower, L".awb")) {
        uint8_t ver = (r.size() >= 5) ? r[4] : 1;
        uint8_t offsetSize = (r.size() >= 6) ? r[5] : 4;
        uint32_t waveCount = (r.size() >= 12) ? r.read_le<uint32_t>(8) : 0;
        uint32_t alignment = (r.size() >= 16) ? r.read_le<uint32_t>(12) : 0x20;

        arc.formatName = L"CRIWARE AFS2 Wave Bank (.awb)";
        arc.engineName = L"CRI Middleware CRIWARE Atom";
        arc.isLittleEndian = true;

        AddHeaderField(arc, L"Signature", L"0x00000000", 4, L"AFS2 (Atom Wave Bank)", L"CRIWARE Atom Audio Container");
        AddHeaderField(arc, L"Container Version", L"0x00000004", 1, L"AFS2 v" + std::to_wstring(ver), L"CRI Atom Wave Format Revision");
        AddHeaderField(arc, L"Offset Field Width", L"0x00000005", 1, std::to_wstring(offsetSize) + L" bytes", L"Offset pointer integer width");
        AddHeaderField(arc, L"Waveform Count", L"0x00000008", 4, std::to_wstring(waveCount), L"Audio tracks in wavebank");
        AddHeaderField(arc, L"Byte Alignment", L"0x0000000C", 4, std::to_wstring(alignment) + L" bytes", L"Sample boundary alignment");

        ParseCriwareAfs2Span(r.span(), 0, arc, L"audio");
        return;
    }

    if (magic4 == "@UTF" || EndsWithW(lower, L".acb")) {
        uint32_t tableSize = r.read_be<uint32_t>(4);
        uint32_t stringsOffset = r.read_be<uint32_t>(12) + 8;
        uint16_t numColumns = r.read_be<uint16_t>(24);
        uint32_t numRows = r.read_be<uint32_t>(28);

        arc.formatName = L"CRIWARE Audio Cue Bank (.acb)";
        arc.engineName = L"CRI Middleware CRIWARE Atom";
        arc.isLittleEndian = false;

        AddHeaderField(arc, L"Signature", L"0x00000000", 4, L"@UTF (0x40555446)", L"CRIWARE Universal Table Format");
        AddHeaderField(arc, L"Table Size", L"0x00000004", 4, FormatBytesW(tableSize), L"Schema and metadata payload size");
        AddHeaderField(arc, L"Schema Columns", L"0x00000018", 2, std::to_wstring(numColumns), L"Database attribute descriptors");
        AddHeaderField(arc, L"Cue Record Rows", L"0x0000001C", 4, std::to_wstring(numRows), L"Audio cues and waveform descriptors");
        AddHeaderField(arc, L"Strings Heap", L"0x0000000C", 4, ToHexW(stringsOffset), L"Cue sheet names and tags offset");

        ArchiveEntry utfE;
        utfE.name = L"CueSheetTable.utf";
        utfE.folder = L"metadata";
        utfE.path = L"metadata/CueSheetTable.utf";
        utfE.offset = 0;
        utfE.compressedSize = std::min<size_t>(r.size(), static_cast<size_t>(tableSize) + 8);
        utfE.uncompressedSize = utfE.compressedSize;
        utfE.compressionMethod = L"@UTF Relational Schema";
        utfE.crc32 = ComputeCrc32(r.slice(0, std::min<size_t>(utfE.compressedSize, 4096)));
        utfE.category = AssetCategory::Config;
        utfE.customFlags = ExtractMode::Stored;
        arc.entries.push_back(std::move(utfE));

        for (size_t searchIdx = 0x20; searchIdx + 16 < r.size(); searchIdx += 4) {
            if (r[searchIdx] == 'A' && r[searchIdx + 1] == 'F' && r[searchIdx + 2] == 'S' && r[searchIdx + 3] == '2') {
                ParseCriwareAfs2Span(r.slice(searchIdx, r.size() - searchIdx), searchIdx, arc, L"internal_awb");
                break;
            }
        }
        return;
    }

    if (magic4 == "CRID" || EndsWithW(lower, L".usm")) {
        arc.formatName = L"CRI Sofdec 2 Movie Container (.usm)";
        arc.engineName = L"CRI Middleware Sofdec 2";
        arc.isLittleEndian = false;

        uint64_t videoPayload = 0;
        uint64_t audioPayload = 0;
        uint32_t packetCount = 0;
        size_t pos = 0;
        while (pos + 8 <= r.size() && packetCount < 5000) {
            std::string m = r.read_ascii(pos, 4, false);
            uint32_t sz = r.read_be<uint32_t>(pos + 4);
            if (sz == 0 || pos + 8 + sz > r.size()) break;
            if (m == "@SFV") videoPayload += sz;
            else if (m == "@SFA") audioPayload += sz;
            ++packetCount;
            pos += 8 + sz;
        }

        AddHeaderField(arc, L"Signature", L"0x00000000", 4, L"CRID (Sofdec 2 Video)", L"CRI Middleware Video Container");
        AddHeaderField(arc, L"Packets Inspected", L"0x00000000", 4, std::to_wstring(packetCount), L"Multiplexed stream chunks");
        AddHeaderField(arc, L"Video Stream Size", L"@SFV", 0, FormatBytesW(videoPayload), L"H.264 / MPEG-2 elementary video");
        AddHeaderField(arc, L"Audio Stream Size", L"@SFA", 0, FormatBytesW(audioPayload), L"HCA / ADX audio");

        ArchiveEntry vE;
        vE.name = L"video_track.m2v";
        vE.folder = L"video";
        vE.path = L"video/video_track.m2v";
        vE.offset = 0;
        vE.compressedSize = videoPayload ? videoPayload : r.size();
        vE.uncompressedSize = vE.compressedSize;
        vE.compressionMethod = L"Sofdec 2 Video (MPEG-2/H.264)";
        vE.crc32 = ComputeCrc32(r.slice(0, std::min<size_t>(r.size(), 65536)));
        vE.category = AssetCategory::Video;
        vE.customFlags = ExtractMode::Stored;
        arc.entries.push_back(std::move(vE));

        ArchiveEntry aE;
        aE.name = L"audio_track.hca";
        aE.folder = L"audio";
        aE.path = L"audio/audio_track.hca";
        aE.offset = 0;
        aE.compressedSize = audioPayload ? audioPayload : (r.size() / 5);
        aE.uncompressedSize = aE.compressedSize;
        aE.compressionMethod = L"CRI HCA / ADX Audio Track";
        aE.crc32 = vE.crc32;
        aE.category = AssetCategory::Audio;
        aE.customFlags = ExtractMode::Stored;
        arc.entries.push_back(std::move(aE));
        return;
    }

    if ((r.size() >= 2 && r[0] == 0x80 && r[1] == 0x00) || EndsWithW(lower, L".adx")) {
        uint8_t channels = (r.size() >= 8) ? r[7] : 2;
        uint32_t sampleRate = (r.size() >= 12) ? r.read_be<uint32_t>(8) : 44100;
        uint32_t sampleCount = (r.size() >= 16) ? r.read_be<uint32_t>(12) : 0;

        arc.formatName = L"CRI ADX Audio (.adx)";
        arc.engineName = L"CRI Middleware CRIWARE";
        arc.isLittleEndian = false;

        AddHeaderField(arc, L"Signature", L"0x00000000", 2, L"0x8000 (CRI ADX Header)", L"CRI ADX ADPCM Audio Stream");
        AddHeaderField(arc, L"Channel Configuration", L"0x00000007", 1, std::to_wstring(channels) + L" Channels", L"Audio channels");
        AddHeaderField(arc, L"Sampling Rate", L"0x00000008", 4, std::to_wstring(sampleRate) + L" Hz", L"Sample playback frequency");
        AddHeaderField(arc, L"Sample Count", L"0x0000000C", 4, std::to_wstring(sampleCount) + L" samples", L"Total audio duration");

        ArchiveEntry e;
        e.name = baseName;
        e.folder = L"audio";
        e.path = L"audio/" + baseName;
        e.offset = 0;
        e.compressedSize = r.size();
        e.uncompressedSize = sampleCount ? (static_cast<uint64_t>(sampleCount) * channels * 2) : (r.size() * 4);
        e.compressionMethod = L"CRI ADX ADPCM";
        e.crc32 = ComputeCrc32(r.span());
        e.category = AssetCategory::Audio;
        e.customFlags = ExtractMode::Stored;
        arc.entries.push_back(std::move(e));
        return;
    }

    // HCA audio
    uint16_t version = r.read_be<uint16_t>(4);
    uint8_t channels = (r.size() >= 0x18) ? r[0x0B] : 2;
    uint32_t sampleRate = (r.size() >= 0x18) ? ((static_cast<uint32_t>(r[0x0C]) << 16) | (static_cast<uint32_t>(r[0x0D]) << 8) | r[0x0E]) : 48000;
    uint32_t blockCount = (r.size() >= 0x18) ? r.read_be<uint32_t>(0x10) : 0;

    arc.formatName = L"CRI High Compression Audio (.hca)";
    arc.engineName = L"CRI Middleware CRIWARE Atom";
    arc.isLittleEndian = false;

    AddHeaderField(arc, L"Signature", L"0x00000000", 4, L"HCA\\0 (0x48434100)", L"CRI High Compression Audio");
    AddHeaderField(arc, L"Codec Version", L"0x00000004", 2, ToHexW(version, 4), L"HCA compression codec revision");
    AddHeaderField(arc, L"Sample Rate & Channels", L"fmt", 4, std::to_wstring(sampleRate) + L" Hz / " + std::to_wstring(channels) + L" ch", L"Playback configuration");
    AddHeaderField(arc, L"Total Audio Blocks", L"fmt", 4, std::to_wstring(blockCount), L"Coded frame blocks");

    ArchiveEntry e;
    e.name = baseName;
    e.folder = L"audio";
    e.path = L"audio/" + baseName;
    e.offset = 0;
    e.compressedSize = r.size();
    e.uncompressedSize = r.size() * 6;
    e.compressionMethod = L"CRI HCA Transform Coding";
    e.crc32 = ComputeCrc32(r.span());
    e.category = AssetCategory::Audio;
    e.customFlags = ExtractMode::Stored;
    arc.entries.push_back(std::move(e));
}


// ============================================================================
// 17. PlayStation PKG (.pkg: \x7FPKG)
// ============================================================================
inline void ParsePlaystationPkgArchive(const std::wstring& path, BinaryReader& r, ParsedGameArchive& arc) {
    if (r.size() < 64) return;
    arc.valid = true;
    arc.formatId = ArchiveFormatId::PKG;
    arc.isLittleEndian = false;

    uint16_t pkgRev = r.read_be<uint16_t>(0x04);
    uint16_t pkgType = r.read_be<uint16_t>(0x06);
    uint32_t itemCount = r.read_be<uint32_t>(0x14);
    uint64_t dataOffset = r.read_be<uint64_t>(0x20);
    uint64_t dataSize = r.read_be<uint64_t>(0x28);
    std::string contentId = r.read_ascii(0x30, 36);
    if (contentId.empty()) contentId = r.read_ascii(0x40, 36);

    std::wstring platform = (pkgType == 1) ? L"PlayStation 3" : (pkgType == 2) ? L"PlayStation Portable / Vita" : L"PlayStation 4 / 5";
    arc.formatName = L"PlayStation Package (.pkg - " + platform + L")";
    arc.engineName = L"Sony PlayStation System Package";

    AddHeaderField(arc, L"Magic Signature", L"0x00000000", 4, L"\\x7FPKG (0x7F504B47)", L"Sony PlayStation Package Container");
    AddHeaderField(arc, L"Package Revision / Type", L"0x00000004", 4,
        ToHexW(pkgRev, 4) + L" / " + platform, L"Target PlayStation hardware family");
    AddHeaderField(arc, L"Content ID", L"0x00000030", 36,
        contentId.empty() ? L"N/A" : AsciiToWide(contentId), L"NPDRM Content Identifier");
    AddHeaderField(arc, L"Item Count", L"0x00000014", 4, std::to_wstring(itemCount), L"Embedded package records");
    AddHeaderField(arc, L"Encrypted Data Offset", L"0x00000020", 8, ToHexW(dataOffset, 8), L"Payload start offset");

    // Generate package summary manifest + standard entries
    std::string summary = "=== SONY PLAYSTATION PACKAGE (.PKG) REPORT ===\n";
    summary += "Content ID   : " + (contentId.empty() ? "Unknown" : contentId) + "\n";
    summary += "Platform     : " + WideToUtf8(platform) + "\n";
    summary += "Item Count   : " + std::to_string(itemCount) + "\n";
    summary += "Data Offset  : 0x" + WideToUtf8(ToHexW(dataOffset, 8).substr(2)) + "\n";
    summary += "Data Size    : " + std::to_string(dataSize) + " bytes\n";

    auto summaryBytes = StringToBytes(summary);
    ArchiveEntry me;
    me.name = L"PkgHeaderManifest.txt";
    me.folder = L"sys";
    me.path = L"sys/PkgHeaderManifest.txt";
    me.offset = 0;
    me.compressedSize = summaryBytes.size();
    me.uncompressedSize = summaryBytes.size();
    me.compressionMethod = L"Synthesized PKG Metadata";
    me.crc32 = ComputeCrc32(summaryBytes);
    me.category = AssetCategory::Config;
    me.customFlags = ExtractMode::InlineOnly;
    me.inlineData = std::move(summaryBytes);
    arc.entries.push_back(std::move(me));

    // Carve unencrypted PNG icons or PARAM.SFO if present
    for (size_t i = 0; i + 16 < std::min<size_t>(r.size(), 0x200000); ++i) {
        if (r[i] == 0x00 && r[i + 1] == 0x50 && r[i + 2] == 0x53 && r[i + 3] == 0x46 &&
            r[i + 4] == 0x01 && r[i + 5] == 0x01 && r[i + 6] == 0x00 && r[i + 7] == 0x00) {
            size_t sfoSize = std::min<size_t>(4096, r.size() - i);
            ArchiveEntry e;
            e.name = L"PARAM.SFO";
            e.folder = L"sce_sys";
            e.path = L"sce_sys/PARAM.SFO";
            e.offset = i;
            e.compressedSize = sfoSize;
            e.uncompressedSize = sfoSize;
            e.compressionMethod = L"System File Object (PSF)";
            e.crc32 = ComputeCrc32(r.slice(i, sfoSize));
            e.category = AssetCategory::Config;
            e.customFlags = ExtractMode::Stored;
            arc.entries.push_back(std::move(e));
            i += sfoSize - 1;
        } else if (r[i] == 0x89 && r[i + 1] == 0x50 && r[i + 2] == 0x4E && r[i + 3] == 0x47) {
            size_t end = i + 8;
            while (end + 8 <= std::min<size_t>(r.size(), i + 0x200000)) {
                uint32_t clen = r.read_be<uint32_t>(end);
                std::string ctype = r.read_ascii(end + 4, 4, false);
                end += 12ULL + clen;
                if (ctype == "IEND") break;
                if (clen > 0x200000) break;
            }
            if (end <= r.size() && end > i) {
                ArchiveEntry e;
                e.name = L"ICON0_" + ToHexW(i, 6) + L".PNG";
                e.folder = L"sce_sys";
                e.path = L"sce_sys/" + e.name;
                e.offset = i;
                e.compressedSize = end - i;
                e.uncompressedSize = end - i;
                e.compressionMethod = L"Embedded Package PNG";
                e.crc32 = ComputeCrc32(r.slice(i, end - i));
                e.category = AssetCategory::Texture;
                e.customFlags = ExtractMode::Stored;
                arc.entries.push_back(std::move(e));
                i = end - 1;
            }
        }
    }
}

// ============================================================================
// 18. Xbox 360 GOD / XContent (LIVE, PIRS, CON ), GDFX, XEX2, and Xbox XVD
// ============================================================================
inline bool FindGdfxBaseOffset(BinaryReader& r, size_t& outBaseOffset, uint32_t& outRootSector, uint32_t& outRootSize) {
    static constexpr size_t kCandidates[] = { 0x10000, 0x00, 0x800, 0x8000, 0x2000, 0x10800, 0x20000 };
    for (size_t base : kCandidates) {
        size_t volOff = base + 0x10000;
        if (volOff + 32 <= r.size() && r.read_ascii(volOff, 20, false) == "MICROSOFT*XBOX*MEDIA") {
            outBaseOffset = base;
            outRootSector = r.read_le<uint32_t>(volOff + 20);
            outRootSize = r.read_le<uint32_t>(volOff + 24);
            return true;
        }
        if (base + 32 <= r.size() && r.read_ascii(base, 20, false) == "MICROSOFT*XBOX*MEDIA") {
            outBaseOffset = (base >= 0x10000) ? (base - 0x10000) : 0;
            outRootSector = r.read_le<uint32_t>(base + 20);
            outRootSize = r.read_le<uint32_t>(base + 24);
            return true;
        }
    }
    for (size_t off = 0; off + 32 <= std::min<size_t>(r.size(), 0x80000); off += 0x800) {
        if (r.read_ascii(off, 20, false) == "MICROSOFT*XBOX*MEDIA") {
            outBaseOffset = (off >= 0x10000) ? (off - 0x10000) : 0;
            outRootSector = r.read_le<uint32_t>(off + 20);
            outRootSize = r.read_le<uint32_t>(off + 24);
            return true;
        }
    }
    return false;
}

inline void WalkGdfxDirNode(BinaryReader& r, size_t baseOffset, size_t dirByteOffset, uint32_t dirSize,
                            uint32_t wordOffset, const std::string& prefix,
                            std::vector<ArchiveEntry>& entries, int depth) {
    if (depth > 64 || entries.size() >= 10000) return;
    size_t entryPos = dirByteOffset + static_cast<size_t>(wordOffset) * 4;
    if (wordOffset == 0xFFFF || entryPos + 14 > dirByteOffset + dirSize || entryPos + 14 > r.size()) return;

    uint16_t leftChild = r.read_le<uint16_t>(entryPos + 0);
    uint16_t rightChild = r.read_le<uint16_t>(entryPos + 2);
    uint32_t sectorStart = r.read_le<uint32_t>(entryPos + 4);
    uint32_t fileSize = r.read_le<uint32_t>(entryPos + 8);
    uint8_t attr = r[entryPos + 12];
    uint8_t nameLen = r[entryPos + 13];

    if (leftChild != 0 && leftChild != 0xFFFF && leftChild != wordOffset) {
        WalkGdfxDirNode(r, baseOffset, dirByteOffset, dirSize, leftChild, prefix, entries, depth + 1);
    }

    if (nameLen > 0 && entryPos + 14 + nameLen <= r.size()) {
        std::string name = r.read_ascii(entryPos + 14, nameLen, false);
        std::string fullPath = prefix.empty() ? name : (prefix + "/" + name);
        size_t dataOffset = baseOffset + static_cast<size_t>(sectorStart) * 2048ULL;
        bool isDir = (attr & 0x10) != 0;

        if (isDir) {
            if (fileSize > 0 && dataOffset + fileSize <= r.size()) {
                WalkGdfxDirNode(r, baseOffset, dataOffset, fileSize, 0, fullPath, entries, depth + 1);
            }
        } else {
            auto sp = SplitVirtualPath(AsciiToWide(fullPath));
            size_t safeOffset = std::min(dataOffset, r.size());
            size_t safeLen = std::min<size_t>(fileSize, r.size() - safeOffset);

            ArchiveEntry e;
            e.name = sp.name;
            e.path = sp.cleanPath;
            e.folder = sp.folder;
            e.offset = safeOffset;
            e.compressedSize = fileSize;
            e.uncompressedSize = fileSize;
            e.compressionMethod = L"GDFX Sector Stream";
            e.crc32 = ComputeCrc32(r.slice(safeOffset, std::min<size_t>(safeLen, 65536)));
            e.category = ClassifyFile(sp.name);
            e.customFlags = ExtractMode::Stored;
            entries.push_back(std::move(e));
        }
    }

    if (rightChild != 0 && rightChild != 0xFFFF && rightChild != wordOffset) {
        WalkGdfxDirNode(r, baseOffset, dirByteOffset, dirSize, rightChild, prefix, entries, depth + 1);
    }
}

inline void ParseXbox360GodArchive(const std::wstring& path, BinaryReader& r, ParsedGameArchive& arc) {
    arc.valid = true;
    arc.formatId = ArchiveFormatId::GOD;
    arc.isLittleEndian = false;

    std::string magic = r.read_ascii(0, 4, false);
    bool isXContent = (magic == "LIVE" || magic == "PIRS" || magic == "CON ");

    if (isXContent && r.size() >= 0x420) {
        uint32_t contentType = r.read_be<uint32_t>(0x344);
        uint32_t mediaId = r.read_be<uint32_t>(0x354);
        uint32_t titleId = r.read_be<uint32_t>(0x360);
        std::wstring titleName = r.read_utf16be(0x410, 64);
        if (titleName.empty()) titleName = L"Xbox 360 Package";

        std::wstring typeLabel = (contentType == 0x00007000) ? L"Games on Demand (GOD)" :
                                 (contentType == 0x000D0000) ? L"Xbox Live Arcade (XBLA)" :
                                 (contentType == 0x00000002) ? L"Marketplace DLC" :
                                 (contentType == 0x00000001) ? L"Saved Game" : L"XContent Package";

        arc.formatName = L"Xbox 360 " + typeLabel + L" (" + AsciiToWide(TrimA(magic)) + L")";
        arc.engineName = L"Microsoft Xbox 360 System Software (STFS / GDFX)";

        AddHeaderField(arc, L"Package Signature", L"0x00000000", 4, AsciiToWide(magic), L"Xbox 360 Signed Container (" + typeLabel + L")");
        AddHeaderField(arc, L"Title Name", L"0x00000410", 128, titleName, L"Embedded UTF-16BE Title Display Name");
        AddHeaderField(arc, L"Title ID", L"0x00000360", 4, ToHexW(titleId, 8), L"Xbox 360 Title Identifier");
        AddHeaderField(arc, L"Media ID", L"0x00000354", 4, ToHexW(mediaId, 8), L"Disc / Media Identifier");
        AddHeaderField(arc, L"Content Type", L"0x00000344", 4, ToHexW(contentType, 8) + L" (" + typeLabel + L")", L"XContent classification");

        // Carve embedded PNG thumbnail around 0x171A..0x971A
        for (size_t i = 0x1000; i + 16 < std::min<size_t>(r.size(), 0x10000); ++i) {
            if (r[i] == 0x89 && r[i + 1] == 0x50 && r[i + 2] == 0x4E && r[i + 3] == 0x47) {
                size_t end = i + 8;
                while (end + 8 <= std::min<size_t>(r.size(), i + 0x10000)) {
                    uint32_t clen = r.read_be<uint32_t>(end);
                    std::string ctype = r.read_ascii(end + 4, 4, false);
                    end += 12ULL + clen;
                    if (ctype == "IEND" || clen > 0x10000) break;
                }
                if (end <= r.size() && end > i) {
                    ArchiveEntry te;
                    te.name = L"TitleThumbnail.png";
                    te.folder = L"metadata";
                    te.path = L"metadata/TitleThumbnail.png";
                    te.offset = i;
                    te.compressedSize = end - i;
                    te.uncompressedSize = end - i;
                    te.compressionMethod = L"Embedded XContent PNG Icon";
                    te.crc32 = ComputeCrc32(r.slice(i, end - i));
                    te.category = AssetCategory::Texture;
                    te.customFlags = ExtractMode::Stored;
                    arc.entries.push_back(std::move(te));
                    break;
                }
            }
        }
    } else {
        arc.formatName = L"Xbox 360 GDFX Disc Volume";
        arc.engineName = L"Microsoft Xbox 360 GDFX Volume";
    }

    size_t gdfxBase = 0;
    uint32_t rootSector = 0, rootSize = 0;
    if (FindGdfxBaseOffset(r, gdfxBase, rootSector, rootSize)) {
        size_t rootByteOff = gdfxBase + static_cast<size_t>(rootSector) * 2048ULL;
        AddHeaderField(arc, L"GDFX Volume Magic", ToHexW(gdfxBase + 0x10000), 20, L"MICROSOFT*XBOX*MEDIA", L"Xbox Game Disc File System");
        AddHeaderField(arc, L"Root Directory Sector", ToHexW(gdfxBase + 0x10014), 4, std::to_wstring(rootSector), L"Root directory table sector");
        if (rootByteOff + rootSize <= r.size() && rootSize > 0) {
            WalkGdfxDirNode(r, gdfxBase, rootByteOff, rootSize, 0, "", arc.entries, 0);
        }
    }

    // STFS File Table Scan (64-byte entries starting at 0x971A / 0xA000 / 0xC000)
    if (isXContent && arc.entries.size() <= 1 && r.size() >= 0xB000) {
        for (size_t dirBase : { size_t(0xA000), size_t(0xB000), size_t(0xC000), size_t(0xD000) }) {
            if (dirBase + 4096 > r.size()) continue;
            for (size_t off = dirBase; off + 64 <= dirBase + 4096; off += 64) {
                uint8_t nlFlags = r[off + 0x28];
                uint8_t nameLen = nlFlags & 0x3F;
                if (nameLen == 0 || nameLen > 40) continue;
                std::string fname = r.read_ascii(off, nameLen);
                if (fname.size() != nameLen || !IsPrintableAscii(fname)) continue;
                bool isDir = (nlFlags & 0x80) != 0;
                if (isDir) continue;
                uint32_t startBlock = static_cast<uint32_t>(r[off + 0x2F]) |
                                      (static_cast<uint32_t>(r[off + 0x30]) << 8) |
                                      (static_cast<uint32_t>(r[off + 0x31]) << 16);
                uint32_t fsize = r.read_be<uint32_t>(off + 0x34);
                size_t dataOff = 0xC000ULL + static_cast<size_t>(startBlock) * 0x1000ULL;
                if (fsize > 0 && dataOff < r.size()) {
                    size_t safeLen = std::min<size_t>(fsize, r.size() - dataOff);
                    auto sp = SplitVirtualPath(AsciiToWide(fname));
                    ArchiveEntry e;
                    e.name = sp.name;
                    e.path = sp.cleanPath;
                    e.folder = sp.folder;
                    e.offset = dataOff;
                    e.compressedSize = safeLen;
                    e.uncompressedSize = fsize;
                    e.compressionMethod = L"STFS Block Stream";
                    e.crc32 = ComputeCrc32(r.slice(dataOff, std::min<size_t>(safeLen, 65536)));
                    e.category = ClassifyFile(sp.name);
                    e.customFlags = ExtractMode::Stored;
                    arc.entries.push_back(std::move(e));
                }
            }
            if (arc.entries.size() > 1) break;
        }
    }

    if (arc.entries.empty()) {
        std::string manifest = "=== XBOX 360 CONTAINER MANIFEST ===\n";
        manifest += "File Size: " + std::to_string(r.size()) + " bytes\n";
        auto mb = StringToBytes(manifest);
        ArchiveEntry e;
        e.name = L"ContentManifest.txt";
        e.folder = L"metadata";
        e.path = L"metadata/ContentManifest.txt";
        e.offset = 0;
        e.compressedSize = mb.size();
        e.uncompressedSize = mb.size();
        e.compressionMethod = L"Synthesized Report";
        e.crc32 = ComputeCrc32(mb);
        e.category = AssetCategory::Config;
        e.customFlags = ExtractMode::InlineOnly;
        e.inlineData = std::move(mb);
        arc.entries.push_back(std::move(e));
    }
}

inline void ParseXbox360XexArchive(const std::wstring& path, BinaryReader& r, ParsedGameArchive& arc) {
    if (r.size() < 24) return;
    arc.valid = true;
    arc.formatId = ArchiveFormatId::XEX;
    arc.formatName = L"Xbox 360 Executable (.xex - XEX2)";
    arc.engineName = L"Microsoft Xbox 360 Xenon (PowerPC PE)";
    arc.isLittleEndian = false;

    std::string magic = r.read_ascii(0, 4, false);
    uint32_t moduleFlags = r.read_be<uint32_t>(4);
    uint32_t peDataOffset = r.read_be<uint32_t>(8);
    uint32_t secInfoOffset = r.read_be<uint32_t>(16);
    uint32_t optHeaderCount = r.read_be<uint32_t>(20);

    uint32_t entryPoint = 0;
    uint32_t imageBase = 0;
    uint32_t titleId = 0;
    uint32_t mediaId = 0;
    std::string origPeName;

    size_t optOff = 24;
    for (uint32_t i = 0; i < std::min<uint32_t>(optHeaderCount, 128) && optOff + 8 <= r.size(); ++i) {
        uint32_t key = r.read_be<uint32_t>(optOff);
        uint32_t val = r.read_be<uint32_t>(optOff + 4);
        optOff += 8;
        if (key == 0x00010100) entryPoint = val;
        else if (key == 0x00010201) imageBase = val;
        else if (key == 0x00040006 && val + 16 <= r.size()) {
            mediaId = r.read_be<uint32_t>(val + 4);
            titleId = r.read_be<uint32_t>(val + 12);
        } else if (key == 0x000183FF && val + 4 < r.size()) {
            origPeName = r.read_cstr(val + 4, 256);
        }
    }

    AddHeaderField(arc, L"Signature", L"0x00000000", 4, AsciiToWide(magic), L"Xbox 360 Executable Header");
    AddHeaderField(arc, L"Module Flags", L"0x00000004", 4, ToHexW(moduleFlags, 8), L"Title module / DLL flags");
    AddHeaderField(arc, L"PE Payload Offset", L"0x00000008", 4, ToHexW(peDataOffset, 8), L"Base PE image stream offset");
    AddHeaderField(arc, L"Security Info Offset", L"0x00000010", 4, ToHexW(secInfoOffset, 8), L"RSA signature & section hashes");
    AddHeaderField(arc, L"Entry Point / Image Base", L"OptionalHdr", 8,
        ToHexW(entryPoint, 8) + L" / " + ToHexW(imageBase, 8), L"PowerPC execution entry address");
    if (titleId != 0) {
        AddHeaderField(arc, L"Title ID", L"ExecutionInfo", 4, ToHexW(titleId, 8), L"Xbox 360 Title Identifier");
    }

    std::string report = "=== XBOX 360 EXECUTABLE (XEX2) MANIFEST ===\n";
    report += "Signature          : " + magic + "\n";
    report += "Original PE Name   : " + (origPeName.empty() ? WideToUtf8(ExtractFileNameW(path)) : origPeName) + "\n";
    report += "Entry Point        : 0x" + WideToUtf8(ToHexW(entryPoint, 8).substr(2)) + "\n";
    report += "Image Base Address : 0x" + WideToUtf8(ToHexW(imageBase, 8).substr(2)) + "\n";
    report += "Title ID           : 0x" + WideToUtf8(ToHexW(titleId, 8).substr(2)) + "\n";
    report += "Media ID           : 0x" + WideToUtf8(ToHexW(mediaId, 8).substr(2)) + "\n";
    report += "PE Payload Offset  : 0x" + WideToUtf8(ToHexW(peDataOffset, 8).substr(2)) + "\n";

    auto repBytes = StringToBytes(report);
    ArchiveEntry me;
    me.name = L"XexManifest.txt";
    me.folder = L"headers";
    me.path = L"headers/XexManifest.txt";
    me.offset = 0;
    me.compressedSize = repBytes.size();
    me.uncompressedSize = repBytes.size();
    me.compressionMethod = L"Synthesized XEX2 Report";
    me.crc32 = ComputeCrc32(repBytes);
    me.category = AssetCategory::Config;
    me.customFlags = ExtractMode::InlineOnly;
    me.inlineData = std::move(repBytes);
    arc.entries.push_back(std::move(me));

    if (secInfoOffset < r.size()) {
        size_t certLen = std::min<size_t>(0x188, r.size() - secInfoOffset);
        ArchiveEntry ce;
        ce.name = L"XboxSecurityCertificate.bin";
        ce.folder = L"security";
        ce.path = L"security/XboxSecurityCertificate.bin";
        ce.offset = secInfoOffset;
        ce.compressedSize = certLen;
        ce.uncompressedSize = certLen;
        ce.compressionMethod = L"XEX2 Security Descriptor";
        ce.crc32 = ComputeCrc32(r.slice(secInfoOffset, certLen));
        ce.category = AssetCategory::Binary;
        ce.customFlags = ExtractMode::Stored;
        arc.entries.push_back(std::move(ce));
    }

    if (peDataOffset < r.size()) {
        size_t peLen = r.size() - peDataOffset;
        ArchiveEntry pe;
        pe.name = origPeName.empty() ? L"base_image.pe" : AsciiToWide(origPeName);
        pe.folder = L"pe_image";
        pe.path = L"pe_image/" + pe.name;
        pe.offset = peDataOffset;
        pe.compressedSize = peLen;
        pe.uncompressedSize = peLen;
        pe.compressionMethod = L"XEX2 PowerPC PE Image Stream";
        pe.crc32 = ComputeCrc32(r.slice(peDataOffset, std::min<size_t>(peLen, 65536)));
        pe.category = AssetCategory::Code;
        pe.customFlags = ExtractMode::Stored;
        arc.entries.push_back(std::move(pe));
    }
}

inline void ParseXboxXvdArchive(const std::wstring& path, BinaryReader& r, ParsedGameArchive& arc) {
    arc.valid = true;
    arc.formatId = ArchiveFormatId::APPX;
    arc.formatName = L"Xbox Virtual Disk (.xvd / .xvc)";
    arc.engineName = L"Microsoft Xbox One / Series X|S GDK";
    arc.isLittleEndian = true;

    size_t sigOff = (r.size() >= 0x208 && r.read_ascii(0x200, 8, false) == "msft-xvd") ? 0x200 : 0;
    AddHeaderField(arc, L"Signature", ToHexW(sigOff), 8, L"msft-xvd", L"Xbox Virtual Disk Container");

    std::string info = "=== XBOX VIRTUAL DISK (XVD / XVC) ===\nFile Size: " + std::to_string(r.size()) + " bytes\n";
    auto ib = StringToBytes(info);
    ArchiveEntry e;
    e.name = L"XvdHeaderInfo.txt";
    e.folder = L"metadata";
    e.path = L"metadata/XvdHeaderInfo.txt";
    e.offset = 0;
    e.compressedSize = ib.size();
    e.uncompressedSize = ib.size();
    e.compressionMethod = L"Synthesized XVD Report";
    e.crc32 = ComputeCrc32(ib);
    e.category = AssetCategory::Config;
    e.customFlags = ExtractMode::InlineOnly;
    e.inlineData = std::move(ib);
    arc.entries.push_back(std::move(e));
}

// ============================================================================
// 19. Nintendo DS (.nds, .srl) + Banner Decoder + Classic Console ROMs
// ============================================================================
inline bool DecodeNdsBannerIconToRgba(BinaryReader& r, size_t bannerOffset, std::vector<uint8_t>& outRgba) {
    if (bannerOffset + 0x240 > r.size()) return false;
    size_t tileBase = bannerOffset + 0x20;
    size_t palBase = bannerOffset + 0x220;

    uint8_t pal[16][4]{};
    for (int i = 0; i < 16; ++i) {
        uint16_t bgr = r.read_le<uint16_t>(palBase + i * 2);
        pal[i][0] = static_cast<uint8_t>(((bgr >> 0) & 0x1F) * 255 / 31);
        pal[i][1] = static_cast<uint8_t>(((bgr >> 5) & 0x1F) * 255 / 31);
        pal[i][2] = static_cast<uint8_t>(((bgr >> 10) & 0x1F) * 255 / 31);
        pal[i][3] = (i == 0) ? 0 : 255;
    }

    outRgba.assign(32 * 32 * 4, 0);
    for (int ty = 0; ty < 4; ++ty) {
        for (int tx = 0; tx < 4; ++tx) {
            size_t tileOff = tileBase + (ty * 4 + tx) * 32;
            for (int py = 0; py < 8; ++py) {
                for (int px = 0; px < 4; ++px) {
                    uint8_t byte = r[tileOff + py * 4 + px];
                    uint8_t c0 = byte & 0x0F;
                    uint8_t c1 = (byte >> 4) & 0x0F;
                    int x0 = tx * 8 + px * 2;
                    int y0 = ty * 8 + py;
                    size_t dst0 = static_cast<size_t>(y0 * 32 + x0) * 4;
                    size_t dst1 = dst0 + 4;
                    std::memcpy(&outRgba[dst0], pal[c0], 4);
                    std::memcpy(&outRgba[dst1], pal[c1], 4);
                }
            }
        }
    }
    return true;
}

inline void WalkNdsNitroFsDir(BinaryReader& r, uint32_t fntOffset, uint32_t fntSize,
                              uint32_t fatOffset, uint32_t fatSize,
                              uint16_t dirId, const std::string& currentPath,
                              std::vector<ArchiveEntry>& entries, int depth) {
    if (depth > 32 || entries.size() >= 8000) return;
    uint16_t dirIndex = dirId & 0x0FFF;
    size_t mainEntryOff = fntOffset + static_cast<size_t>(dirIndex) * 8;
    if (mainEntryOff + 8 > fntOffset + fntSize || mainEntryOff + 8 > r.size()) return;

    uint32_t subTableRel = r.read_le<uint32_t>(mainEntryOff);
    uint16_t fileId = r.read_le<uint16_t>(mainEntryOff + 4);
    size_t pos = fntOffset + subTableRel;

    while (pos < fntOffset + fntSize && pos < r.size()) {
        uint8_t typeLen = r[pos++];
        if (typeLen == 0) break;
        bool isSubDir = (typeLen & 0x80) != 0;
        uint8_t nameLen = typeLen & 0x7F;
        if (pos + nameLen > r.size()) break;
        std::string name = r.read_ascii(pos, nameLen, false);
        pos += nameLen;
        std::string fullPath = currentPath.empty() ? name : (currentPath + "/" + name);

        if (isSubDir) {
            if (pos + 2 > r.size()) break;
            uint16_t subDirId = r.read_le<uint16_t>(pos);
            pos += 2;
            WalkNdsNitroFsDir(r, fntOffset, fntSize, fatOffset, fatSize, subDirId, fullPath, entries, depth + 1);
        } else {
            size_t fatEntryOff = fatOffset + static_cast<size_t>(fileId) * 8;
            ++fileId;
            if (fatEntryOff + 8 <= fatOffset + fatSize && fatEntryOff + 8 <= r.size()) {
                uint32_t startOff = r.read_le<uint32_t>(fatEntryOff);
                uint32_t endOff = r.read_le<uint32_t>(fatEntryOff + 4);
                if (endOff >= startOff && startOff <= r.size()) {
                    size_t fsize = std::min<size_t>(endOff - startOff, r.size() - startOff);
                    auto sp = SplitVirtualPath(AsciiToWide(fullPath));
                    ArchiveEntry e;
                    e.name = sp.name;
                    e.path = sp.cleanPath;
                    e.folder = sp.folder;
                    e.offset = startOff;
                    e.compressedSize = fsize;
                    e.uncompressedSize = fsize;
                    e.compressionMethod = L"NitroFS Stored";
                    e.crc32 = ComputeCrc32(r.slice(startOff, std::min<size_t>(fsize, 65536)));
                    e.category = ClassifyFile(sp.name);
                    e.customFlags = ExtractMode::Stored;
                    entries.push_back(std::move(e));
                }
            }
        }
    }
}

inline bool IsNdsRom(BinaryReader& r, const std::wstring& lowerPath) {
    if (EndsWithW(lowerPath, L".nds") || EndsWithW(lowerPath, L".srl") || EndsWithW(lowerPath, L".dsi")) return r.size() >= 0x200;
    if (r.size() < 0x200) return false;
    return r[0x12] <= 0x18 && r[0x15D] == 0x96;
}

inline void ParseNdsRomArchive(const std::wstring& path, BinaryReader& r, ParsedGameArchive& arc) {
    if (r.size() < 0x200) return;
    arc.valid = true;
    arc.formatId = ArchiveFormatId::NDS;
    arc.formatName = L"Nintendo DS Cartridge ROM (.nds / NitroFS)";
    arc.engineName = L"Nintendo DS Nitro SDK (FNT / FAT)";
    arc.isLittleEndian = true;

    std::string gameTitle = r.read_ascii(0x00, 12);
    std::string gameCode = r.read_ascii(0x0C, 4);
    std::string makerCode = r.read_ascii(0x10, 2);
    uint32_t arm9RomOff = r.read_le<uint32_t>(0x20);
    uint32_t arm9Size = r.read_le<uint32_t>(0x2C);
    uint32_t arm7RomOff = r.read_le<uint32_t>(0x30);
    uint32_t arm7Size = r.read_le<uint32_t>(0x3C);
    uint32_t fntOff = r.read_le<uint32_t>(0x40);
    uint32_t fntSize = r.read_le<uint32_t>(0x44);
    uint32_t fatOff = r.read_le<uint32_t>(0x48);
    uint32_t fatSize = r.read_le<uint32_t>(0x4C);
    uint32_t bannerOff = r.read_le<uint32_t>(0x68);

    AddHeaderField(arc, L"Game Title", L"0x00000000", 12, AsciiToWide(gameTitle), L"Internal cartridge header title");
    AddHeaderField(arc, L"Game Code / Maker", L"0x0000000C", 6, AsciiToWide(gameCode + " / " + makerCode), L"NTR serial code & publisher ID");
    AddHeaderField(arc, L"ARM9 Binary", L"0x00000020", 16, ToHexW(arm9RomOff) + L" (" + std::to_wstring(arm9Size) + L" B)", L"Main ARM946E-S executable");
    AddHeaderField(arc, L"ARM7 Binary", L"0x00000030", 16, ToHexW(arm7RomOff) + L" (" + std::to_wstring(arm7Size) + L" B)", L"Co-processor ARM7TDMI executable");
    AddHeaderField(arc, L"NitroFS FNT / FAT", L"0x00000040", 16,
        L"FNT: " + ToHexW(fntOff) + L" / FAT: " + ToHexW(fatOff), L"File Name Table & File Allocation Table");

    // System binaries
    {
        ArchiveEntry hdr;
        hdr.name = L"cartridge_header.bin";
        hdr.folder = L"system";
        hdr.path = L"system/cartridge_header.bin";
        hdr.offset = 0;
        hdr.compressedSize = 0x200;
        hdr.uncompressedSize = 0x200;
        hdr.compressionMethod = L"NDS Cartridge Header";
        hdr.crc32 = ComputeCrc32(r.slice(0, 0x200));
        hdr.category = AssetCategory::Binary;
        hdr.customFlags = ExtractMode::Stored;
        arc.entries.push_back(std::move(hdr));
    }
    if (arm9RomOff < r.size() && arm9Size > 0) {
        size_t sz = std::min<size_t>(arm9Size, r.size() - arm9RomOff);
        ArchiveEntry e;
        e.name = L"arm9.bin";
        e.folder = L"system";
        e.path = L"system/arm9.bin";
        e.offset = arm9RomOff;
        e.compressedSize = sz;
        e.uncompressedSize = sz;
        e.compressionMethod = L"ARM9 Executable";
        e.crc32 = ComputeCrc32(r.slice(arm9RomOff, sz));
        e.category = AssetCategory::Code;
        e.customFlags = ExtractMode::Stored;
        arc.entries.push_back(std::move(e));
    }
    if (arm7RomOff < r.size() && arm7Size > 0) {
        size_t sz = std::min<size_t>(arm7Size, r.size() - arm7RomOff);
        ArchiveEntry e;
        e.name = L"arm7.bin";
        e.folder = L"system";
        e.path = L"system/arm7.bin";
        e.offset = arm7RomOff;
        e.compressedSize = sz;
        e.uncompressedSize = sz;
        e.compressionMethod = L"ARM7 Executable";
        e.crc32 = ComputeCrc32(r.slice(arm7RomOff, sz));
        e.category = AssetCategory::Code;
        e.customFlags = ExtractMode::Stored;
        arc.entries.push_back(std::move(e));
    }

    // Banner icon + multi-language titles
    if (bannerOff > 0 && bannerOff + 0x840 <= r.size()) {
        std::vector<uint8_t> rgba;
        if (DecodeNdsBannerIconToRgba(r, bannerOff, rgba)) {
            ArchiveEntry ic;
            ic.name = L"banner_icon.rgba";
            ic.folder = L"system";
            ic.path = L"system/banner_icon.rgba";
            ic.offset = bannerOff + 0x20;
            ic.compressedSize = 0x220;
            ic.uncompressedSize = rgba.size();
            ic.compressionMethod = L"NDS 32x32 4bpp BGR555 -> RGBA32";
            ic.crc32 = ComputeCrc32(rgba);
            ic.category = AssetCategory::Texture;
            ic.customFlags = ExtractMode::InlineOnly;
            ic.inlineData = std::move(rgba);
            arc.entries.push_back(std::move(ic));
        }

        static constexpr const char* kLangs[6] = { "Japanese", "English", "French", "German", "Italian", "Spanish" };
        std::string titlesReport = "=== NINTENDO DS CARTRIDGE BANNER TITLES ===\n";
        for (int i = 0; i < 6; ++i) {
            std::wstring wt = r.read_utf16le(bannerOff + 0x240 + i * 0x100, 128);
            titlesReport += std::string("[") + kLangs[i] + "]\n" + WideToUtf8(wt) + "\n\n";
        }
        auto tb = StringToBytes(titlesReport);
        ArchiveEntry te;
        te.name = L"banner_titles.txt";
        te.folder = L"system";
        te.path = L"system/banner_titles.txt";
        te.offset = bannerOff + 0x240;
        te.compressedSize = tb.size();
        te.uncompressedSize = tb.size();
        te.compressionMethod = L"UTF-16LE Banner Strings";
        te.crc32 = ComputeCrc32(tb);
        te.category = AssetCategory::Config;
        te.customFlags = ExtractMode::InlineOnly;
        te.inlineData = std::move(tb);
        arc.entries.push_back(std::move(te));
    }

    // Walk NitroFS directory tree
    if (fntOff > 0 && fntSize >= 8 && fatOff > 0 && fatSize >= 8 &&
        fntOff + fntSize <= r.size() && fatOff + fatSize <= r.size()) {
        WalkNdsNitroFsDir(r, fntOff, fntSize, fatOff, fatSize, 0xF000, "", arc.entries, 0);
    }
}

inline void ParseClassicConsoleRom(const std::wstring& path, BinaryReader& r, ParsedGameArchive& arc) {
    std::wstring lower = ToLowerW(path);
    std::wstring baseName = ExtractFileNameW(path);
    arc.valid = true;
    arc.formatId = ArchiveFormatId::ROM;
    arc.isLittleEndian = true;

    std::string report = "=== CLASSIC CONSOLE CARTRIDGE ROM ===\n";
    if (EndsWithW(lower, L".gba") || (r.size() >= 0xC0 && r[0xB2] == 0x96)) {
        std::string title = r.read_ascii(0xA0, 12);
        std::string code = r.read_ascii(0xAC, 4);
        arc.formatName = L"Game Boy Advance ROM (.gba)";
        arc.engineName = L"Nintendo Game Boy Advance (ARM7TDMI)";
        AddHeaderField(arc, L"ROM Title", L"0x000000A0", 12, AsciiToWide(title), L"GBA internal cartridge title");
        AddHeaderField(arc, L"Game Code", L"0x000000AC", 4, AsciiToWide(code), L"AGB serial identifier");
        report += "Platform : Game Boy Advance\nTitle    : " + title + "\nCode     : " + code + "\n";
    } else if (EndsWithW(lower, L".gb") || EndsWithW(lower, L".gbc")) {
        std::string title = r.read_ascii(0x134, 15);
        arc.formatName = L"Game Boy / Color ROM (.gb / .gbc)";
        arc.engineName = L"Nintendo Game Boy (Sharp LR35902)";
        AddHeaderField(arc, L"ROM Title", L"0x00000134", 15, AsciiToWide(title), L"GB/GBC cartridge header title");
        report += "Platform : Game Boy / Color\nTitle    : " + title + "\n";
    } else if (EndsWithW(lower, L".z64") || EndsWithW(lower, L".n64") || EndsWithW(lower, L".v64")) {
        std::string title = r.read_ascii(0x20, 20);
        arc.formatName = L"Nintendo 64 ROM (.z64 / .n64)";
        arc.engineName = L"Nintendo 64 (MIPS VR4300 + RCP)";
        arc.isLittleEndian = false;
        AddHeaderField(arc, L"Internal Name", L"0x00000020", 20, AsciiToWide(title), L"N64 ROM header internal name");
        report += "Platform : Nintendo 64\nTitle    : " + title + "\n";
    } else if (r.size() >= 16 && r.read_ascii(0, 4, false) == "NES\x1A") {
        uint8_t prgBanks = r[4];
        uint8_t chrBanks = r[5];
        arc.formatName = L"Nintendo Entertainment System iNES ROM (.nes)";
        arc.engineName = L"NES / Famicom (Ricoh 2A03 6502)";
        AddHeaderField(arc, L"Signature", L"0x00000000", 4, L"NES\\x1A", L"iNES ROM Header");
        AddHeaderField(arc, L"PRG / CHR Banks", L"0x00000004", 2,
            std::to_wstring(prgBanks * 16) + L" KB PRG / " + std::to_wstring(chrBanks * 8) + L" KB CHR", L"ROM bank layout");
        report += "Platform : NES (iNES)\nPRG ROM  : " + std::to_string(prgBanks * 16) + " KB\nCHR ROM  : " + std::to_string(chrBanks * 8) + " KB\n";
    } else {
        arc.formatName = L"Super Nintendo ROM (.sfc / .smc)";
        arc.engineName = L"SNES / Super Famicom (Ricoh 5A22)";
        AddHeaderField(arc, L"Cartridge Size", L"0x00000000", 4, std::to_wstring(r.size()) + L" bytes", L"SNES ROM image");
        report += "Platform : Super Nintendo (SNES)\n";
    }
    report += "ROM Size : " + std::to_string(r.size()) + " bytes\n";

    auto rb = StringToBytes(report);
    ArchiveEntry infoEntry;
    infoEntry.name = L"RomHeaderInfo.txt";
    infoEntry.folder = L"metadata";
    infoEntry.path = L"metadata/RomHeaderInfo.txt";
    infoEntry.offset = 0;
    infoEntry.compressedSize = rb.size();
    infoEntry.uncompressedSize = rb.size();
    infoEntry.compressionMethod = L"Synthesized ROM Header Report";
    infoEntry.crc32 = ComputeCrc32(rb);
    infoEntry.category = AssetCategory::Config;
    infoEntry.customFlags = ExtractMode::InlineOnly;
    infoEntry.inlineData = std::move(rb);
    arc.entries.push_back(std::move(infoEntry));

    ArchiveEntry romEntry;
    romEntry.name = baseName + L".payload";
    romEntry.folder = L"cartridge";
    romEntry.path = L"cartridge/" + romEntry.name;
    romEntry.offset = 0;
    romEntry.compressedSize = r.size();
    romEntry.uncompressedSize = r.size();
    romEntry.compressionMethod = L"Raw Cartridge Image";
    romEntry.crc32 = ComputeCrc32(r.span());
    romEntry.category = AssetCategory::Binary;
    romEntry.customFlags = ExtractMode::Stored;
    arc.entries.push_back(std::move(romEntry));
}

// ============================================================================
// 20. Zlib Stream & Embedded Asset Signature Carver
// ============================================================================
inline void ParseZlibAndCarveStreams(const std::wstring& path, BinaryReader& r, ParsedGameArchive& arc) {
    std::wstring baseName = ExtractFileNameW(path);
    arc.valid = true;
    arc.formatId = ArchiveFormatId::ZLIB_CARVED;
    arc.formatName = L"Compressed / Binary Game Container (" + baseName + L")";
    arc.engineName = L"Signature & Zlib Stream Carver";
    arc.isLittleEndian = true;

    // 1. Check if entire file is a valid Zlib stream
    if (r.size() >= 6 && r[0] == 0x78 && (r[1] == 0x01 || r[1] == 0x5E || r[1] == 0x9C || r[1] == 0xDA)) {
        std::vector<uint8_t> dec;
        if (DecompressZlib(r.span(), dec) && !dec.empty()) {
            AddHeaderField(arc, L"Zlib Header", L"0x00000000", 2, ToHexW(r.read_be<uint16_t>(0), 4), L"RFC 1950 Zlib Compressed Stream");
            AddHeaderField(arc, L"Decompressed Size", L"Stream", 4, std::to_wstring(dec.size()) + L" bytes", L"Inflated payload length");

            ArchiveEntry e;
            e.name = baseName + L".decompressed";
            e.folder = L"decompressed";
            e.path = L"decompressed/" + e.name;
            e.offset = 0;
            e.compressedSize = r.size();
            e.uncompressedSize = dec.size();
            e.compressionMethod = L"Zlib (RFC 1950)";
            e.crc32 = ComputeCrc32(dec);
            e.category = AssetCategory::Binary;
            e.customFlags = ExtractMode::InlineOnly;
            e.inlineData = std::move(dec);
            arc.entries.push_back(std::move(e));
            return;
        }
    }

    // 2. Scan first 16 MB for embedded PNGs, WAVs, DDS, or Zlib streams
    size_t scanLimit = std::min<size_t>(r.size(), 16 * 1024 * 1024);
    for (size_t i = 0; i + 16 < scanLimit && arc.entries.size() < 200; ++i) {
        if (r[i] == 0x89 && r[i + 1] == 0x50 && r[i + 2] == 0x4E && r[i + 3] == 0x47) {
            size_t end = i + 8;
            while (end + 8 <= scanLimit) {
                uint32_t clen = r.read_be<uint32_t>(end);
                std::string ctype = r.read_ascii(end + 4, 4, false);
                end += 12ULL + clen;
                if (ctype == "IEND" || clen > 0x1000000) break;
            }
            if (end <= scanLimit && end > i + 16) {
                ArchiveEntry e;
                e.name = L"carved_" + ToHexW(i, 6) + L".png";
                e.folder = L"textures";
                e.path = L"textures/" + e.name;
                e.offset = i;
                e.compressedSize = end - i;
                e.uncompressedSize = end - i;
                e.compressionMethod = L"Carved PNG Image";
                e.crc32 = ComputeCrc32(r.slice(i, end - i));
                e.category = AssetCategory::Texture;
                e.customFlags = ExtractMode::Stored;
                arc.entries.push_back(std::move(e));
                i = end - 1;
            }
        } else if (r[i] == 0x52 && r[i + 1] == 0x49 && r[i + 2] == 0x46 && r[i + 3] == 0x46 &&
                   r.read_ascii(i + 8, 4, false) == "WAVE") {
            uint32_t riffLen = r.read_le<uint32_t>(i + 4) + 8;
            size_t safeLen = std::min<size_t>(r.size() - i, std::max<size_t>(44, riffLen));
            ArchiveEntry e;
            e.name = L"carved_" + ToHexW(i, 6) + L".wav";
            e.folder = L"audio";
            e.path = L"audio/" + e.name;
            e.offset = i;
            e.compressedSize = safeLen;
            e.uncompressedSize = safeLen;
            e.compressionMethod = L"Carved RIFF WAVE";
            e.crc32 = ComputeCrc32(r.slice(i, std::min<size_t>(safeLen, 65536)));
            e.category = AssetCategory::Audio;
            e.customFlags = ExtractMode::Stored;
            arc.entries.push_back(std::move(e));
            i += safeLen - 1;
        }
    }

    if (arc.entries.empty()) {
        ArchiveEntry e;
        e.name = baseName;
        e.folder = L"raw";
        e.path = L"raw/" + baseName;
        e.offset = 0;
        e.compressedSize = r.size();
        e.uncompressedSize = r.size();
        e.compressionMethod = L"Raw Binary Stream";
        e.crc32 = ComputeCrc32(r.slice(0, std::min<size_t>(r.size(), 65536)));
        e.category = ClassifyFile(baseName);
        e.customFlags = ExtractMode::Stored;
        arc.entries.push_back(std::move(e));
    }

    AddHeaderField(arc, L"Container Size", L"0x00000000", 4, std::to_wstring(r.size()) + L" bytes", L"Total binary container size");
    AddHeaderField(arc, L"Extracted Streams", L"Scan", 4, std::to_wstring(arc.entries.size()), L"Carved or raw stream entries");
}

} // namespace gfe_detail

// ============================================================================
// Public API: Format Detection & Archive Parsing
// ============================================================================
inline bool IsSupportedArchiveOrGameFile(const std::wstring& path) {
    using namespace gfe_detail;
    std::wstring lower = ToLowerW(path);
    static constexpr const wchar_t* kSupportedExts[] = {
        L".zip", L".pk3", L".pk4", L".iwd", L".jar", L".appx", L".appxbundle", L".msix", L".msixbundle",
        L".rar", L".iso", L".bin", L".img", L".mdf", L".ff", L".wad", L".pak", L".vpk",
        L".uasset", L".umap", L".assets", L".bundle", L".unity3d", L".sharedassets",
        L".bsa", L".ba2", L".rpf", L".forge", L".dcx", L".bnd", L".bnd4", L".chrbnd",
        L".objbnd", L".partsbnd", L".anibnd", L".tpf", L".resource", L".bmodel",
        L".wbd", L".wdb", L".sdat", L".pfs", L".pf8", L".awb", L".acb", L".usm",
        L".adx", L".hca", L".par", L".big", L".bik", L".bk2", L".rpa", L".pkg",
        L".god", L".header", L".xex", L".xvd", L".xvc", L".nds", L".srl", L".dsi",
        L".gba", L".gb", L".gbc", L".z64", L".n64", L".v64", L".nes", L".sfc", L".smc",
        L".zlib"
    };
    for (const wchar_t* ext : kSupportedExts) {
        if (EndsWithW(lower, ext)) return true;
    }

    // Check magic bytes of first 64 bytes on disk (also supports Xbox 360 hash-named GOD packages)
    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                               nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return false;
    uint8_t hdr[64]{};
    DWORD bytesRead = 0;
    BOOL ok = ReadFile(hFile, hdr, sizeof(hdr), &bytesRead, nullptr);
    CloseHandle(hFile);
    if (!ok || bytesRead < 4) return false;

    std::string m4(reinterpret_cast<const char*>(hdr), 4);
    if (m4 == "PK\x03\x04" || m4 == "Rar!" || m4 == "IWAD" || m4 == "PWAD" ||
        m4 == "WAD2" || m4 == "WAD3" || m4 == "PACK" || m4 == "BTDX" ||
        m4 == "BSA\0" || m4 == "DCX\0" || m4 == "BND3" || m4 == "BND4" ||
        m4 == "AFS2" || m4 == "@UTF" || m4 == "CRID" || m4 == "HCA\0" ||
        m4 == "PARC" || m4 == "BIGF" || m4 == "BIG4" || m4 == "RPA-" ||
        m4 == "\x7FPKG" || m4 == "LIVE" || m4 == "PIRS" || m4 == "CON " ||
        m4 == "XEX2" || m4 == "XEX1" || m4 == "NES\x1A" || m4 == "SDAT" ||
        m4 == "PFS " || m4 == "pf8 " || m4 == "RES\0" || m4 == "\x1BLMB") {
        return true;
    }
    if (bytesRead >= 8) {
        std::string m8(reinterpret_cast<const char*>(hdr), 8);
        if (m8.rfind("TAff", 0) == 0 || m8.rfind("IWff", 0) == 0 || m8.rfind("S1ff", 0) == 0 ||
            m8.rfind("UnityFS", 0) == 0 || m8.rfind("scimitar", 0) == 0) {
            return true;
        }
    }
    uint32_t u32le = static_cast<uint32_t>(hdr[0]) | (static_cast<uint32_t>(hdr[1]) << 8) |
                     (static_cast<uint32_t>(hdr[2]) << 16) | (static_cast<uint32_t>(hdr[3]) << 24);
    if (u32le == 0x55AA1234u || u32le == 0x9E2A83C1u) return true;
    return false;
}

inline ParsedGameArchive ParseArchiveBytes(const std::wstring& path, std::span<const uint8_t> bytes) {
    using namespace gfe_detail;
    ParsedGameArchive arc;
    arc.filePath = path;
    arc.fileSize = bytes.size();
    if (bytes.size() < 4) return arc;

    BinaryReader r(bytes);
    std::wstring lower = ToLowerW(path);
    std::string m4 = r.read_ascii(0, 4, false);
    std::string m8 = (r.size() >= 8) ? r.read_ascii(0, 8, false) : "";
    uint32_t magicLE = r.read_le<uint32_t>(0);

    // 1. DOOM / Quake WAD
    if (m4 == "IWAD" || m4 == "PWAD" || m4 == "WAD2" || m4 == "WAD3") {
        ParseWadArchive(path, r, arc);
        return arc;
    }
    // 2. Quake PACK
    if (m4 == "PACK") {
        ParsePakArchive(path, r, arc);
        return arc;
    }
    // 3. Valve Source VPK
    if (magicLE == 0x55AA1234u || EndsWithW(lower, L".vpk")) {
        ParseVpkArchive(path, r, arc);
        return arc;
    }
    // 4. ZIP / PK3 / JAR / APPX / MSIX
    if ((r[0] == 0x50 && r[1] == 0x4B && (r[2] == 0x03 || r[2] == 0x05)) ||
        EndsWithW(lower, L".zip") || EndsWithW(lower, L".pk3") || EndsWithW(lower, L".pk4") ||
        EndsWithW(lower, L".iwd") || EndsWithW(lower, L".jar") || EndsWithW(lower, L".appx") ||
        EndsWithW(lower, L".msix")) {
        ParseZipFamilyArchive(path, r, arc);
        return arc;
    }
    // 5. RAR v4 / v5
    if (m4 == "Rar!" || EndsWithW(lower, L".rar")) {
        ParseRarArchive(path, r, arc);
        return arc;
    }
    // 6. Call of Duty FastFile (.ff)
    if (m8.rfind("TAff", 0) == 0 || m8.rfind("IWff", 0) == 0 || m8.rfind("S1ff", 0) == 0 || EndsWithW(lower, L".ff")) {
        ParseFastFileArchive(path, r, arc);
        return arc;
    }
    // 7. Ren'Py (.rpa)
    if (m8.rfind("RPA-", 0) == 0 || EndsWithW(lower, L".rpa")) {
        ParseRenpyArchive(path, r, arc);
        return arc;
    }
    // 8. Bethesda BSA / BA2
    if (m4 == "BSA\0" || m4 == "BTDX" || EndsWithW(lower, L".bsa") || EndsWithW(lower, L".ba2")) {
        ParseBethesdaArchive(path, r, arc);
        return arc;
    }
    // 9. Rockstar RPF
    if ((m4.rfind("RPF", 0) == 0 && m4[3] >= '0' && m4[3] <= '8') || EndsWithW(lower, L".rpf")) {
        ParseRockstarRpfArchive(path, r, arc);
        return arc;
    }
    // 10. Ubisoft FORGE
    if (m8.rfind("scimitar", 0) == 0 || EndsWithW(lower, L".forge")) {
        ParseUbisoftForgeArchive(path, r, arc);
        return arc;
    }
    // 11. FromSoftware DCX / BND3 / BND4
    if (m4 == "DCX\0" || m4 == "BND3" || m4 == "BND4" || EndsWithW(lower, L".dcx") ||
        EndsWithW(lower, L".bnd") || EndsWithW(lower, L".bnd4") || EndsWithW(lower, L".chrbnd") ||
        EndsWithW(lower, L".tpf")) {
        ParseFromSoftArchive(path, r, arc);
        return arc;
    }
    // 12. id Tech 6 (.resource, .bmodel)
    if (m4 == "RES\0" || (r[0] == 0x1B && r.read_ascii(1, 3, false) == "LMB") ||
        EndsWithW(lower, L".resource") || EndsWithW(lower, L".bmodel")) {
        ParseIdTech6Archive(path, r, arc);
        return arc;
    }
    // 13. CRIWARE (.awb, .acb, .usm, .adx, .hca)
    if (m4 == "AFS2" || m4 == "@UTF" || m4 == "CRID" || m4 == "HCA\0" ||
        EndsWithW(lower, L".awb") || EndsWithW(lower, L".acb") || EndsWithW(lower, L".usm") ||
        EndsWithW(lower, L".adx") || EndsWithW(lower, L".hca")) {
        ParseCriwareArchive(path, r, arc);
        return arc;
    }
    // 14. Yakuza PARC (.par)
    if (m4 == "PARC" || EndsWithW(lower, L".par")) {
        ParseParArchive(path, r, arc);
        return arc;
    }
    // 15. EA BIG (.big)
    if (m4 == "BIGF" || m4 == "BIG4" || EndsWithW(lower, L".big")) {
        ParseBigArchive(path, r, arc);
        return arc;
    }
    // 16. RAD Bink Video (.bik, .bk2)
    if ((m4.rfind("BIK", 0) == 0 || m4.rfind("KB2", 0) == 0) ||
        EndsWithW(lower, L".bik") || EndsWithW(lower, L".bk2")) {
        ParseBinkArchive(path, r, arc);
        return arc;
    }
    // 17. PlayStation PKG (.pkg)
    if ((r[0] == 0x7F && m4.substr(1) == "PKG") || EndsWithW(lower, L".pkg")) {
        ParsePlaystationPkgArchive(path, r, arc);
        return arc;
    }
    // 18. Xbox 360 XEX (.xex)
    if (m4 == "XEX2" || m4 == "XEX1" || EndsWithW(lower, L".xex")) {
        ParseXbox360XexArchive(path, r, arc);
        return arc;
    }
    // 19. Xbox 360 GOD / XContent / GDFX
    size_t dummyBase = 0;
    uint32_t dummySec = 0, dummySz = 0;
    if (m4 == "LIVE" || m4 == "PIRS" || m4 == "CON " || EndsWithW(lower, L".god") ||
        FindGdfxBaseOffset(r, dummyBase, dummySec, dummySz)) {
        ParseXbox360GodArchive(path, r, arc);
        return arc;
    }
    // 20. Xbox One / Series XVD / XVC
    if (m4 == "msft" || (r.size() >= 0x208 && r.read_ascii(0x200, 4, false) == "msft") ||
        EndsWithW(lower, L".xvd") || EndsWithW(lower, L".xvc")) {
        ParseXboxXvdArchive(path, r, arc);
        return arc;
    }
    // 21. Unreal Engine (.uasset, .umap, .pak)
    if (magicLE == 0x9E2A83C1u || EndsWithW(lower, L".uasset") || EndsWithW(lower, L".umap")) {
        ParseUnrealArchive(path, r, arc);
        return arc;
    }
    if (EndsWithW(lower, L".pak")) {
        // Check if Unreal .pak magic 0x5A6F12E1 is in footer, otherwise Quake PACK
        bool isUePak = false;
        size_t tailStart = (r.size() > 512) ? (r.size() - 512) : 0;
        for (size_t pos = r.size() >= 44 ? r.size() - 44 : 0; pos >= tailStart && pos < r.size(); --pos) {
            if (r.read_le<uint32_t>(pos) == 0x5A6F12E1u) { isUePak = true; break; }
            if (pos == 0) break;
        }
        if (isUePak) ParseUnrealArchive(path, r, arc);
        else ParsePakArchive(path, r, arc);
        return arc;
    }
    // 22. Unity Engine (.assets, .bundle, .unity3d, .sharedassets)
    if (m8.rfind("UnityFS", 0) == 0 || m8.rfind("UnityWeb", 0) == 0 || m8.rfind("UnityRaw", 0) == 0 ||
        EndsWithW(lower, L".assets") || EndsWithW(lower, L".bundle") ||
        EndsWithW(lower, L".unity3d") || EndsWithW(lower, L".sharedassets")) {
        ParseUnityArchive(path, r, arc);
        return arc;
    }
    // 23. Square Enix SDAT / PFS / pf8
    if (m4 == "SDAT" || m4 == "PFS " || m4 == "pf8 " ||
        EndsWithW(lower, L".sdat") || EndsWithW(lower, L".pfs") || EndsWithW(lower, L".pf8")) {
        ParseSquareArchive(path, r, arc);
        return arc;
    }
    // 24. Final Fantasy XIII (.wbd, .wdb)
    if (EndsWithW(lower, L".wbd") || EndsWithW(lower, L".wdb") ||
        lower.find(L"white_") != std::wstring::npos || lower.find(L"filelist") != std::wstring::npos) {
        ParseFf13Archive(path, r, arc);
        return arc;
    }
    // 25. Nintendo DS (.nds, .srl, .dsi)
    if (IsNdsRom(r, lower)) {
        ParseNdsRomArchive(path, r, arc);
        return arc;
    }
    // 26. Classic Console ROMs (.gba, .gb, .gbc, .z64, .n64, .v64, .nes, .sfc, .smc)
    if (m4 == "NES\x1A" || EndsWithW(lower, L".gba") || EndsWithW(lower, L".gb") ||
        EndsWithW(lower, L".gbc") || EndsWithW(lower, L".z64") || EndsWithW(lower, L".n64") ||
        EndsWithW(lower, L".v64") || EndsWithW(lower, L".nes") || EndsWithW(lower, L".sfc") ||
        EndsWithW(lower, L".smc")) {
        ParseClassicConsoleRom(path, r, arc);
        return arc;
    }
    // 27. ISO 9660 or PS1 Raw 2352-byte CD-XA (.iso, .bin, .img, .mdf)
    bool hasIsoSig = (r.size() > 0x8006 && r.read_ascii(0x8001, 5, false) == "CD001") ||
                     (r.size() > 0x9320 && r.read_ascii(0x9319, 5, false) == "CD001") ||
                     (r.size() >= 16 && r[0] == 0x00 && r[1] == 0xFF && r[10] == 0xFF && r[11] == 0x00);
    if (hasIsoSig || EndsWithW(lower, L".iso") || EndsWithW(lower, L".mdf")) {
        ParseIsoOrPs1Archive(path, r, arc);
        return arc;
    }
    // 28. Fallback: Zlib stream or embedded asset carver
    ParseZlibAndCarveStreams(path, r, arc);
    return arc;
}

inline ParsedGameArchive ParseArchiveFile(const std::wstring& path) {
    MappedFile mf(path);
    if (!mf.valid()) {
        ParsedGameArchive invalidArc;
        invalidArc.filePath = path;
        return invalidArc;
    }
    return ParseArchiveBytes(path, mf.span());
}

// ============================================================================
// Public API: On-Demand Entry Extraction & Specialized Decoders
// ============================================================================
inline bool ExtractArchiveEntryBytes(const ParsedGameArchive& arc, const ArchiveEntry& entry, std::vector<uint8_t>& outBytes) {
    using namespace gfe_detail;
    outBytes.clear();

    uint32_t mode = entry.customFlags & 0xFFu;

    // If inlineData is populated (and not just the 128-byte BA2 DX10 DDS header), return it directly
    if (!entry.inlineData.empty() && (mode != ExtractMode::Ba2Dx10Tex || entry.inlineData.size() > 128)) {
        outBytes = entry.inlineData;
        return true;
    }

    if (mode == ExtractMode::InlineOnly) {
        outBytes = entry.inlineData;
        return true;
    }

    MappedFile mf(arc.filePath);
    if (!mf.valid()) return false;
    BinaryReader r(mf.span());

    if (entry.offset >= r.size()) return false;
    size_t avail = r.size() - static_cast<size_t>(entry.offset);
    size_t compLen = static_cast<size_t>(std::min<uint64_t>(entry.compressedSize, avail));
    std::span<const uint8_t> rawSlice = r.slice(static_cast<size_t>(entry.offset), compLen);

    switch (mode) {
        case ExtractMode::Stored: {
            outBytes.assign(rawSlice.begin(), rawSlice.end());
            return true;
        }
        case ExtractMode::ZlibStream: {
            if (DecompressZlib(rawSlice, outBytes, static_cast<size_t>(entry.uncompressedSize))) {
                return true;
            }
            if (InflateRaw(rawSlice, outBytes, static_cast<size_t>(entry.uncompressedSize))) {
                return true;
            }
            outBytes.assign(rawSlice.begin(), rawSlice.end());
            return true;
        }
        case ExtractMode::RawDeflate: {
            if (InflateRaw(rawSlice, outBytes, static_cast<size_t>(entry.uncompressedSize))) {
                return true;
            }
            if (DecompressZlib(rawSlice, outBytes, static_cast<size_t>(entry.uncompressedSize))) {
                return true;
            }
            outBytes.assign(rawSlice.begin(), rawSlice.end());
            return true;
        }
        case ExtractMode::Lz4Block: {
            size_t targetSz = static_cast<size_t>(entry.uncompressedSize ? entry.uncompressedSize : compLen * 4);
            if (DecompressLz4Block(rawSlice, outBytes, targetSz)) {
                return true;
            }
            outBytes.assign(rawSlice.begin(), rawSlice.end());
            return true;
        }
        case ExtractMode::Ps1Raw2352Mode2:
        case ExtractMode::Ps1Raw2352Mode1: {
            size_t headerSkip = (mode == ExtractMode::Ps1Raw2352Mode2) ? 24 : 16;
            size_t targetSize = static_cast<size_t>(entry.uncompressedSize);
            outBytes.reserve(targetSize);
            size_t lbaStart = static_cast<size_t>(entry.offset / 2352ULL);
            size_t remaining = targetSize;
            for (size_t sec = 0; remaining > 0; ++sec) {
                size_t secOff = (lbaStart + sec) * 2352ULL;
                if (secOff + headerSkip >= r.size()) break;
                size_t payloadAvail = std::min<size_t>(2048, r.size() - (secOff + headerSkip));
                size_t toCopy = std::min(remaining, payloadAvail);
                auto s = r.slice(secOff + headerSkip, toCopy);
                outBytes.insert(outBytes.end(), s.begin(), s.end());
                remaining -= toCopy;
            }
            return !outBytes.empty();
        }
        case ExtractMode::BsaZlib: {
            bool embedName = (entry.customFlags & 0x100u) != 0;
            size_t pos = 0;
            if (embedName && pos < rawSlice.size()) {
                uint8_t bLen = rawSlice[pos];
                pos += 1 + bLen;
            }
            if (pos + 4 > rawSlice.size()) return false;
            uint32_t origSz = static_cast<uint32_t>(rawSlice[pos]) |
                              (static_cast<uint32_t>(rawSlice[pos + 1]) << 8) |
                              (static_cast<uint32_t>(rawSlice[pos + 2]) << 16) |
                              (static_cast<uint32_t>(rawSlice[pos + 3]) << 24);
            pos += 4;
            if (DecompressZlib(rawSlice.subspan(pos), outBytes, origSz)) {
                return true;
            }
            outBytes.assign(rawSlice.begin() + static_cast<ptrdiff_t>(pos), rawSlice.end());
            return true;
        }
        case ExtractMode::Ba2Dx10Tex: {
            // Prepend 128-byte DDS header stored in entry.inlineData
            outBytes.assign(entry.inlineData.begin(), entry.inlineData.end());
            bool isCompressed = (entry.customFlags & 0x100u) != 0;
            std::vector<uint8_t> chunkPayload;
            if (isCompressed) {
                size_t expectedChunk = (entry.uncompressedSize > 128) ? static_cast<size_t>(entry.uncompressedSize - 128) : 0;
                if (!DecompressZlib(rawSlice, chunkPayload, expectedChunk)) {
                    chunkPayload.assign(rawSlice.begin(), rawSlice.end());
                }
            } else {
                chunkPayload.assign(rawSlice.begin(), rawSlice.end());
            }
            outBytes.insert(outBytes.end(), chunkPayload.begin(), chunkPayload.end());
            return !outBytes.empty();
        }
        default: {
            outBytes.assign(rawSlice.begin(), rawSlice.end());
            return true;
        }
    }
}

// ============================================================================
// PlayStation 1 TIM Image Decoder (4bpp, 8bpp, 16bpp, 24bpp -> RGBA32)
// ============================================================================
inline bool DecodePs1TimToRgba(std::span<const uint8_t> timData, std::vector<uint8_t>& outRgba, uint32_t& outW, uint32_t& outH) {
    using namespace gfe_detail;
    outRgba.clear();
    outW = 0;
    outH = 0;
    if (timData.size() < 20) return false;

    BinaryReader r(timData);
    uint32_t magic = r.read_le<uint32_t>(0);
    if (magic != 0x00000010u) return false;

    uint32_t flags = r.read_le<uint32_t>(4);
    uint32_t pmode = flags & 0x07u;
    bool hasClut = (flags & 0x08u) != 0;

    size_t pos = 8;
    std::vector<uint16_t> clut;
    if (hasClut) {
        if (pos + 12 > r.size()) return false;
        uint32_t clutBnum = r.read_le<uint32_t>(pos);
        uint16_t cw = r.read_le<uint16_t>(pos + 8);
        uint16_t ch = r.read_le<uint16_t>(pos + 10);
        size_t clutCount = static_cast<size_t>(cw) * static_cast<size_t>(ch);
        clut.resize(clutCount, 0);
        size_t clutDataStart = pos + 12;
        for (size_t i = 0; i < clutCount && clutDataStart + i * 2 + 2 <= r.size(); ++i) {
            clut[i] = r.read_le<uint16_t>(clutDataStart + i * 2);
        }
        pos += clutBnum;
    }

    if (pos + 12 > r.size()) return false;
    uint16_t vramW = r.read_le<uint16_t>(pos + 8);
    uint16_t vramH = r.read_le<uint16_t>(pos + 10);
    pos += 12;
    if (vramW == 0 || vramH == 0 || vramW > 4096 || vramH > 4096) return false;

    auto decode1555 = [](uint16_t c, uint8_t* dst) {
        uint8_t r5 = static_cast<uint8_t>(c & 0x1F);
        uint8_t g5 = static_cast<uint8_t>((c >> 5) & 0x1F);
        uint8_t b5 = static_cast<uint8_t>((c >> 10) & 0x1F);
        uint8_t stp = static_cast<uint8_t>((c >> 15) & 0x01);
        dst[0] = static_cast<uint8_t>((r5 * 255) / 31);
        dst[1] = static_cast<uint8_t>((g5 * 255) / 31);
        dst[2] = static_cast<uint8_t>((b5 * 255) / 31);
        dst[3] = (c == 0 && stp == 0) ? 0 : 255;
    };

    if (pmode == 0) {
        // 4-bit CLUT: each 16-bit VRAM word holds 4 pixels
        outW = static_cast<uint32_t>(vramW) * 4;
        outH = vramH;
        size_t totalPixels = static_cast<size_t>(outW) * outH;
        outRgba.assign(totalPixels * 4, 0);
        size_t numBytes = totalPixels / 2;
        for (size_t i = 0; i < numBytes && pos + i < r.size(); ++i) {
            uint8_t b = r[pos + i];
            uint8_t idx0 = b & 0x0F;
            uint8_t idx1 = (b >> 4) & 0x0F;
            uint16_t c0 = (idx0 < clut.size()) ? clut[idx0] : static_cast<uint16_t>(idx0 * 0x1084);
            uint16_t c1 = (idx1 < clut.size()) ? clut[idx1] : static_cast<uint16_t>(idx1 * 0x1084);
            decode1555(c0, &outRgba[(i * 2) * 4]);
            decode1555(c1, &outRgba[(i * 2 + 1) * 4]);
        }
        return true;
    }
    if (pmode == 1) {
        // 8-bit CLUT: each 16-bit VRAM word holds 2 pixels
        outW = static_cast<uint32_t>(vramW) * 2;
        outH = vramH;
        size_t totalPixels = static_cast<size_t>(outW) * outH;
        outRgba.assign(totalPixels * 4, 0);
        for (size_t i = 0; i < totalPixels && pos + i < r.size(); ++i) {
            uint8_t idx = r[pos + i];
            uint16_t c = (idx < clut.size()) ? clut[idx] : static_cast<uint16_t>((idx >> 3) * 0x0421);
            decode1555(c, &outRgba[i * 4]);
        }
        return true;
    }
    if (pmode == 2) {
        // 16-bit direct ABGR1555
        outW = vramW;
        outH = vramH;
        size_t totalPixels = static_cast<size_t>(outW) * outH;
        outRgba.assign(totalPixels * 4, 0);
        for (size_t i = 0; i < totalPixels && pos + i * 2 + 2 <= r.size(); ++i) {
            uint16_t c = r.read_le<uint16_t>(pos + i * 2);
            decode1555(c, &outRgba[i * 4]);
        }
        return true;
    }
    if (pmode == 3) {
        // 24-bit direct RGB888
        outW = (static_cast<uint32_t>(vramW) * 2) / 3;
        outH = vramH;
        if (outW == 0) return false;
        size_t totalPixels = static_cast<size_t>(outW) * outH;
        outRgba.assign(totalPixels * 4, 255);
        for (size_t i = 0; i < totalPixels && pos + i * 3 + 3 <= r.size(); ++i) {
            outRgba[i * 4 + 0] = r[pos + i * 3 + 0];
            outRgba[i * 4 + 1] = r[pos + i * 3 + 1];
            outRgba[i * 4 + 2] = r[pos + i * 3 + 2];
            outRgba[i * 4 + 3] = 255;
        }
        return true;
    }
    return false;
}

// ============================================================================
// DOOM Flat Decoder (64x64 8-bit indexed + 768-byte PLAYPAL -> RGBA32)
// ============================================================================
inline bool DecodeDoomFlatToRgba(std::span<const uint8_t> flatData, std::span<const uint8_t> playpal,
                                 std::vector<uint8_t>& outRgba, uint32_t& outW, uint32_t& outH) {
    outRgba.clear();
    outW = 0;
    outH = 0;
    if (flatData.size() == 4096) {
        outW = 64;
        outH = 64;
    } else if (flatData.size() == 4160) {
        outW = 64;
        outH = 65;
    } else if (flatData.size() == 8192) {
        outW = 64;
        outH = 128;
    } else if (flatData.size() == 16384) {
        outW = 128;
        outH = 128;
    } else {
        return false;
    }

    size_t count = static_cast<size_t>(outW) * outH;
    outRgba.resize(count * 4);
    bool hasPal = (playpal.size() >= 768);
    for (size_t i = 0; i < count; ++i) {
        uint8_t idx = flatData[i];
        if (hasPal) {
            outRgba[i * 4 + 0] = playpal[static_cast<size_t>(idx) * 3 + 0];
            outRgba[i * 4 + 1] = playpal[static_cast<size_t>(idx) * 3 + 1];
            outRgba[i * 4 + 2] = playpal[static_cast<size_t>(idx) * 3 + 2];
            outRgba[i * 4 + 3] = 255;
        } else {
            outRgba[i * 4 + 0] = idx;
            outRgba[i * 4 + 1] = idx;
            outRgba[i * 4 + 2] = idx;
            outRgba[i * 4 + 3] = 255;
        }
    }
    return true;
}

// ============================================================================
// Call of Duty Black Ops II (T6) Compiled GSC Disassembler (\x80GSC)
// ============================================================================
inline std::wstring DisassembleT6Gsc(std::span<const uint8_t> gscData) {
    using namespace gfe_detail;
    if (gscData.size() < 64) return L"";
    if (gscData[0] != 0x80 || gscData[1] != 0x47 || gscData[2] != 0x53 || gscData[3] != 0x43) return L"";

    BinaryReader r(gscData);
    auto rstr = [&](uint32_t off) -> std::string {
        if (off == 0 || off >= r.size()) return "";
        return r.read_cstr(off, 512);
    };

    uint32_t srcCrc = r.read_le<uint32_t>(0x08);
    uint32_t includeOff = r.read_le<uint32_t>(0x0C);
    uint32_t animtreeOff = r.read_le<uint32_t>(0x10);
    uint32_t csegOff = r.read_le<uint32_t>(0x14);
    uint32_t strOff = r.read_le<uint32_t>(0x18);
    uint32_t impOff = r.read_le<uint32_t>(0x1C);
    uint32_t expOff = r.read_le<uint32_t>(0x20);
    uint32_t fixOff = r.read_le<uint32_t>(0x24);
    uint32_t profOff = r.read_le<uint32_t>(0x28);
    uint32_t csegLen = r.read_le<uint32_t>(0x2C);
    uint16_t nameOff = r.read_le<uint16_t>(0x30);
    uint16_t nStr = r.read_le<uint16_t>(0x32);
    uint16_t nExp = r.read_le<uint16_t>(0x34);
    uint16_t nImp = r.read_le<uint16_t>(0x36);
    uint16_t nFix = r.read_le<uint16_t>(0x38);
    uint16_t nProf = r.read_le<uint16_t>(0x3A);
    uint8_t nInc = r[0x3C];
    uint8_t nAnim = r[0x3D];

    std::string scriptName = rstr(nameOff);
    std::string out;
    out += "// =====================================================================\n";
    out += "//  Compiled T6 GSC Script (\x80GSC bytecode - source is not stored)\n";
    out += "//  File      : " + (scriptName.empty() ? "<unnamed>" : scriptName) + "\n";
    out += "//  Size      : " + std::to_string(r.size()) + " bytes   Code segment: " + std::to_string(csegLen) +
           " bytes @ 0x" + WideToUtf8(ToHexW(csegOff).substr(2)) + "\n";
    out += "//  Source CRC: 0x" + WideToUtf8(ToHexW(srcCrc, 8).substr(2)) + "\n";
    out += "//  Exports   : " + std::to_string(nExp) + "   Imports: " + std::to_string(nImp) +
           "   Strings: " + std::to_string(nStr) + "   Includes: " + std::to_string(nInc) +
           "   Animtrees: " + std::to_string(nAnim) + "\n";
    out += "// =====================================================================\n\n";

    // Includes
    if (nInc > 0 && includeOff + static_cast<size_t>(nInc) * 4 <= r.size()) {
        for (uint8_t i = 0; i < nInc; ++i) {
            uint32_t p = r.read_le<uint32_t>(includeOff + static_cast<size_t>(i) * 4);
            out += "#include " + rstr(p) + ";\n";
        }
        out += "\n";
    }

    // Exports
    struct ExpEntry {
        uint32_t crc;
        uint32_t start;
        std::string name;
        uint8_t params;
        uint8_t flags;
        uint32_t end;
    };
    std::vector<ExpEntry> exps;
    for (uint16_t i = 0; i < nExp; ++i) {
        size_t o = expOff + static_cast<size_t>(i) * 12;
        if (o + 12 > r.size()) break;
        ExpEntry e;
        e.crc = r.read_le<uint32_t>(o);
        e.start = r.read_le<uint32_t>(o + 4);
        e.name = rstr(r.read_le<uint16_t>(o + 8));
        e.params = r[o + 10];
        e.flags = r[o + 11];
        e.end = csegOff + csegLen;
        exps.push_back(std::move(e));
    }
    std::sort(exps.begin(), exps.end(), [](const ExpEntry& a, const ExpEntry& b) {
        return a.start < b.start;
    });
    for (size_t i = 0; i + 1 < exps.size(); ++i) {
        exps[i].end = exps[i + 1].start;
    }

    out += "// ---- Functions / exports (" + std::to_string(exps.size()) + ") ----\n";
    for (const auto& f : exps) {
        uint32_t sz = (f.end >= f.start) ? (f.end - f.start) : 0;
        std::string args;
        for (uint8_t k = 0; k < f.params; ++k) {
            if (k > 0) args += ", ";
            args += "a" + std::to_string(k);
        }
        std::string fl;
        if (f.flags & 0x01) fl += " [autoexec]";
        if (f.flags & 0x02) fl += " [private]";
        out += f.name + "(" + args + ")  // @0x" + WideToUtf8(ToHexW(f.start, 4).substr(2)) +
               "  size=" + std::to_string(sz) + "B  crc=0x" + WideToUtf8(ToHexW(f.crc, 8).substr(2)) + fl + "\n{\n";

        if (f.start + sz <= r.size() && sz > 0) {
            size_t dumpLen = std::min<size_t>(sz, 96);
            std::string hx = "    // bytecode: ";
            for (size_t k = 0; k < dumpLen; ++k) {
                char buf[4];
                std::snprintf(buf, sizeof(buf), "%02x", r[f.start + k]);
                hx += buf;
                if ((k & 3) == 3) hx.push_back(' ');
            }
            if (sz > 96) hx += "...";
            out += hx + "\n";
        }
        out += "}\n\n";
    }

    // Imports
    out += "// ---- Calls / imports (" + std::to_string(nImp) + ") ----\n";
    size_t io = impOff;
    for (uint16_t i = 0; i < nImp && io + 8 <= r.size(); ++i) {
        std::string fn = rstr(r.read_le<uint16_t>(io));
        std::string ns = rstr(r.read_le<uint16_t>(io + 2));
        uint16_t nrefs = r.read_le<uint16_t>(io + 4);
        uint8_t params = r[io + 6];
        uint8_t flags = r[io + 7];
        io += 8 + static_cast<size_t>(nrefs) * 4;
        std::string target = ns.empty() ? fn : (ns + "::" + fn);
        out += "//   " + target + "(" + std::to_string(params) + " args)   [" +
               std::to_string(nrefs) + " call site" + (nrefs == 1 ? "" : "s") +
               ", flags=0x" + WideToUtf8(ToHexW(flags, 2).substr(2)) + "]\n";
    }
    out += "\n";

    // Strings
    out += "// ---- Strings (" + std::to_string(nStr) + ") ----\n";
    size_t so = strOff;
    for (uint16_t i = 0; i < nStr && so + 4 <= r.size(); ++i) {
        std::string s = rstr(r.read_le<uint16_t>(so));
        uint8_t nrefs = r[so + 2];
        uint8_t ty = r[so + 3];
        so += 4 + static_cast<size_t>(nrefs) * 4;
        out += "//   [" + std::to_string(i) + "] \"" + s + "\"   (refs=" +
               std::to_string(nrefs) + (ty ? (", type=" + std::to_string(ty)) : "") + ")\n";
    }

    // Animtrees
    if (nAnim > 0) {
        out += "\n// ---- #using_animtree (" + std::to_string(nAnim) + ") ----\n";
        size_t ao = animtreeOff;
        for (uint8_t i = 0; i < nAnim && ao + 8 <= r.size(); ++i) {
            std::string nm = rstr(r.read_le<uint16_t>(ao));
            uint16_t nr1 = r.read_le<uint16_t>(ao + 2);
            uint16_t nr2 = r.read_le<uint16_t>(ao + 4);
            ao += 8 + static_cast<size_t>(nr1) * 4 + static_cast<size_t>(nr2) * 8;
            out += "#using_animtree(\"" + nm + "\");\n";
        }
    }

    (void)fixOff;
    (void)profOff;
    (void)nFix;
    (void)nProf;
    return Utf8ToWide(out);
}

// ============================================================================
// Game Audio to Playable RIFF WAV Converter (CRIWARE .adx, Nintendo DS .swav)
// ============================================================================
namespace gfe_audio_detail {

inline void WriteRiffWavPcm16(const std::vector<int16_t>& pcmSamples, uint32_t sampleRate, uint16_t channels, std::vector<uint8_t>& outWav) {
    uint32_t dataBytes = static_cast<uint32_t>(pcmSamples.size() * sizeof(int16_t));
    uint32_t riffSize = 36 + dataBytes;
    uint16_t blockAlign = static_cast<uint16_t>(channels * 2);
    uint32_t byteRate = sampleRate * blockAlign;

    outWav.resize(44 + dataBytes);
    auto w16 = [&](size_t off, uint16_t v) {
        outWav[off + 0] = static_cast<uint8_t>(v & 0xFF);
        outWav[off + 1] = static_cast<uint8_t>((v >> 8) & 0xFF);
    };
    auto w32 = [&](size_t off, uint32_t v) {
        outWav[off + 0] = static_cast<uint8_t>(v & 0xFF);
        outWav[off + 1] = static_cast<uint8_t>((v >> 8) & 0xFF);
        outWav[off + 2] = static_cast<uint8_t>((v >> 16) & 0xFF);
        outWav[off + 3] = static_cast<uint8_t>((v >> 24) & 0xFF);
    };

    std::memcpy(&outWav[0], "RIFF", 4);
    w32(4, riffSize);
    std::memcpy(&outWav[8], "WAVEfmt ", 8);
    w32(16, 16);
    w16(20, 1); // PCM
    w16(22, channels);
    w32(24, sampleRate);
    w32(28, byteRate);
    w16(32, blockAlign);
    w16(34, 16);
    std::memcpy(&outWav[36], "data", 4);
    w32(40, dataBytes);

    for (size_t i = 0; i < pcmSamples.size(); ++i) {
        uint16_t u = static_cast<uint16_t>(pcmSamples[i]);
        outWav[44 + i * 2 + 0] = static_cast<uint8_t>(u & 0xFF);
        outWav[44 + i * 2 + 1] = static_cast<uint8_t>((u >> 8) & 0xFF);
    }
}

inline bool DecodeCriAdxToWav(std::span<const uint8_t> src, std::vector<uint8_t>& outWav) {
    using namespace gfe_detail;
    if (src.size() < 24 || src[0] != 0x80 || src[1] != 0x00) return false;
    BinaryReader r(src);

    uint16_t copyrightOff = r.read_be<uint16_t>(2);
    size_t dataOffset = static_cast<size_t>(copyrightOff) + 4;
    if (dataOffset >= r.size()) return false;

    uint8_t encodingType = r[4];
    uint8_t blockSize = r[5];
    uint8_t bitdepth = r[6];
    uint8_t channels = r[7];
    uint32_t sampleRate = r.read_be<uint32_t>(8);
    uint32_t totalSamples = r.read_be<uint32_t>(12);
    uint16_t highpassFreq = r.read_be<uint16_t>(16);

    if ((encodingType != 3 && encodingType != 4) || blockSize != 18 || bitdepth != 4 ||
        channels == 0 || channels > 2 || sampleRate == 0 || totalSamples == 0) {
        return false;
    }

    // Standard CRI ADX IIR prediction coefficients
    double a = std::sqrt(2.0) - std::cos(2.0 * 3.14159265358979323846 * static_cast<double>(highpassFreq ? highpassFreq : 500) / static_cast<double>(sampleRate));
    double b = std::sqrt(2.0) - 1.0;
    double c = (a - std::sqrt((a + b) * (a - b))) / b;
    int32_t coef1 = static_cast<int32_t>(std::floor(c * 2.0 * 4096.0 + 0.5));
    int32_t coef2 = static_cast<int32_t>(std::floor(-(c * c) * 4096.0 + 0.5));

    size_t maxSamples = std::min<size_t>(totalSamples, 48000 * 300);
    std::vector<int16_t> pcm(maxSamples * channels, 0);
    int32_t hist1[2] = { 0, 0 };
    int32_t hist2[2] = { 0, 0 };

    size_t pos = dataOffset;
    size_t sampleIdx = 0;
    while (sampleIdx < maxSamples && pos + static_cast<size_t>(blockSize) * channels <= r.size()) {
        size_t blockSamples = std::min<size_t>(32, maxSamples - sampleIdx);
        for (uint8_t ch = 0; ch < channels; ++ch) {
            size_t blkOff = pos + static_cast<size_t>(ch) * blockSize;
            int32_t scale = (r.read_be<uint16_t>(blkOff) & 0x1FFF) + 1;
            for (size_t s = 0; s < blockSamples; ++s) {
                uint8_t byte = r[blkOff + 2 + (s >> 1)];
                int32_t nibble = ((s & 1) == 0) ? (byte >> 4) : (byte & 0x0F);
                if (nibble >= 8) nibble -= 16;

                int32_t predicted = (coef1 * hist1[ch] + coef2 * hist2[ch]) >> 12;
                int32_t sample = predicted + nibble * scale;
                sample = std::clamp(sample, -32768, 32767);
                hist2[ch] = hist1[ch];
                hist1[ch] = sample;
                pcm[(sampleIdx + s) * channels + ch] = static_cast<int16_t>(sample);
            }
        }
        pos += static_cast<size_t>(blockSize) * channels;
        sampleIdx += blockSamples;
    }

    WriteRiffWavPcm16(pcm, sampleRate, channels, outWav);
    return true;
}

inline bool DecodeNdsSwavToWav(std::span<const uint8_t> src, std::vector<uint8_t>& outWav) {
    using namespace gfe_detail;
    if (src.size() < 28) return false;
    BinaryReader r(src);

    size_t infoOff = 0;
    if (r.read_ascii(0, 4, false) == "SWAV" && r.read_ascii(16, 4, false) == "DATA") {
        infoOff = 24;
    }
    if (infoOff + 12 > r.size()) return false;

    uint8_t waveType = r[infoOff + 0]; // 0 = PCM8, 1 = PCM16, 2 = IMA-ADPCM
    uint16_t sampleRate = r.read_le<uint16_t>(infoOff + 2);
    uint16_t loopStartWords = r.read_le<uint16_t>(infoOff + 6);
    uint32_t nonLoopLenWords = r.read_le<uint32_t>(infoOff + 8);
    size_t pcmDataOff = infoOff + 12;
    if (sampleRate < 2000 || pcmDataOff >= r.size()) return false;

    size_t byteLen = std::min<size_t>(r.size() - pcmDataOff, (static_cast<size_t>(loopStartWords) + nonLoopLenWords) * 4);
    if (byteLen == 0) byteLen = r.size() - pcmDataOff;

    std::vector<int16_t> pcm;
    if (waveType == 0) {
        // 8-bit signed PCM
        pcm.resize(byteLen);
        for (size_t i = 0; i < byteLen; ++i) {
            int8_t s8 = static_cast<int8_t>(r[pcmDataOff + i]);
            pcm[i] = static_cast<int16_t>(static_cast<int32_t>(s8) * 256);
        }
    } else if (waveType == 1) {
        // 16-bit signed LE PCM
        size_t numSamples = byteLen / 2;
        pcm.resize(numSamples);
        for (size_t i = 0; i < numSamples; ++i) {
            pcm[i] = r.read_le<int16_t>(pcmDataOff + i * 2);
        }
    } else if (waveType == 2 && byteLen >= 4) {
        // 4-bit IMA-ADPCM (4-byte initial header: int16 predictor, int8 stepIndex, uint8 reserved)
        static constexpr int kIndexTable[16] = {
            -1, -1, -1, -1, 2, 4, 6, 8,
            -1, -1, -1, -1, 2, 4, 6, 8
        };
        static constexpr int kStepTable[89] = {
            7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
            50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230,
            253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963,
            1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327,
            3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442,
            11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
        };
        int32_t predictor = r.read_le<int16_t>(pcmDataOff);
        int32_t stepIdx = std::clamp<int32_t>(r[pcmDataOff + 2], 0, 88);
        size_t adpcmBytes = byteLen - 4;
        pcm.reserve(adpcmBytes * 2);

        for (size_t i = 0; i < adpcmBytes; ++i) {
            uint8_t b = r[pcmDataOff + 4 + i];
            for (int nib = 0; nib < 2; ++nib) {
                uint8_t code = (nib == 0) ? (b & 0x0F) : (b >> 4);
                int step = kStepTable[stepIdx];
                int diff = step >> 3;
                if (code & 1) diff += step >> 2;
                if (code & 2) diff += step >> 1;
                if (code & 4) diff += step;
                if (code & 8) predictor -= diff;
                else predictor += diff;
                predictor = std::clamp(predictor, -32768, 32767);
                stepIdx = std::clamp(stepIdx + kIndexTable[code], 0, 88);
                pcm.push_back(static_cast<int16_t>(predictor));
            }
        }
    } else {
        return false;
    }

    WriteRiffWavPcm16(pcm, sampleRate, 1, outWav);
    return true;
}

} // namespace gfe_audio_detail

inline bool ConvertGameAudioToWav(std::span<const uint8_t> srcData, const std::wstring& ext, std::vector<uint8_t>& outWav) {
    using namespace gfe_detail;
    outWav.clear();
    if (srcData.size() < 16) return false;

    // Already a standard RIFF WAVE file
    if (srcData[0] == 'R' && srcData[1] == 'I' && srcData[2] == 'F' && srcData[3] == 'F' &&
        srcData[8] == 'W' && srcData[9] == 'A' && srcData[10] == 'V' && srcData[11] == 'E') {
        outWav.assign(srcData.begin(), srcData.end());
        return true;
    }

    std::wstring lowerExt = ToLowerW(ext);
    if ((srcData[0] == 0x80 && srcData[1] == 0x00) || EndsWithW(lowerExt, L".adx")) {
        return gfe_audio_detail::DecodeCriAdxToWav(srcData, outWav);
    }
    if ((srcData[0] == 'S' && srcData[1] == 'W' && srcData[2] == 'A' && srcData[3] == 'V') ||
        EndsWithW(lowerExt, L".swav")) {
        return gfe_audio_detail::DecodeNdsSwavToWav(srcData, outWav);
    }
    return false;
}

// ============================================================================
// Public API: In-Place Archive Modding & Repacking
// ============================================================================
namespace gfe_repack_detail {

inline void AppendU16LE(std::vector<uint8_t>& buf, uint16_t v) {
    buf.push_back(static_cast<uint8_t>(v & 0xFF));
    buf.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
}

inline void AppendU32LE(std::vector<uint8_t>& buf, uint32_t v) {
    buf.push_back(static_cast<uint8_t>(v & 0xFF));
    buf.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    buf.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
}

inline void WriteU32LEAt(std::vector<uint8_t>& buf, size_t off, uint32_t v) {
    if (off + 4 > buf.size()) return;
    buf[off + 0] = static_cast<uint8_t>(v & 0xFF);
    buf[off + 1] = static_cast<uint8_t>((v >> 8) & 0xFF);
    buf[off + 2] = static_cast<uint8_t>((v >> 16) & 0xFF);
    buf[off + 3] = static_cast<uint8_t>((v >> 24) & 0xFF);
}

inline void AppendFixedAscii(std::vector<uint8_t>& buf, const std::string& s, size_t fixedLen) {
    for (size_t i = 0; i < fixedLen; ++i) {
        buf.push_back(i < s.size() ? static_cast<uint8_t>(s[i]) : 0);
    }
}

inline bool WriteAllBytesToDisk(const std::wstring& destPath, std::span<const uint8_t> data) {
    std::wstring tmpPath = destPath + L".speta_tmp";
    HANDLE hFile = CreateFileW(tmpPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    size_t pos = 0;
    while (pos < data.size()) {
        DWORD toWrite = static_cast<DWORD>(std::min<size_t>(data.size() - pos, 16 * 1024 * 1024));
        DWORD written = 0;
        if (!WriteFile(hFile, data.data() + pos, toWrite, &written, nullptr) || written != toWrite) {
            CloseHandle(hFile);
            DeleteFileW(tmpPath.c_str());
            return false;
        }
        pos += written;
    }
    CloseHandle(hFile);
    if (!MoveFileExW(tmpPath.c_str(), destPath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED)) {
        DeleteFileW(tmpPath.c_str());
        return false;
    }
    return true;
}

inline bool RebuildWadArchive(const ParsedGameArchive& arc, std::vector<uint8_t>& out) {
    using namespace gfe_detail;
    out.clear();
    out.reserve(static_cast<size_t>(arc.fileSize + 4096));

    std::string magic = "PWAD";
    if (!arc.headerFields.empty()) {
        std::string hdrVal = WideToUtf8(arc.headerFields[0].value);
        if (hdrVal.rfind("IWAD", 0) == 0) magic = "IWAD";
        else if (hdrVal.rfind("WAD2", 0) == 0) magic = "WAD2";
        else if (hdrVal.rfind("WAD3", 0) == 0) magic = "WAD3";
    }
    bool isQuakeWad = (magic == "WAD2" || magic == "WAD3");

    AppendFixedAscii(out, magic, 4);
    AppendU32LE(out, static_cast<uint32_t>(arc.entries.size()));
    AppendU32LE(out, 0); // placeholder for dirOffset

    struct LumpDir {
        uint32_t offset;
        uint32_t size;
        std::string name;
    };
    std::vector<LumpDir> dir;
    dir.reserve(arc.entries.size());

    for (const auto& e : arc.entries) {
        std::vector<uint8_t> payload;
        ExtractArchiveEntryBytes(arc, e, payload);
        LumpDir d;
        d.offset = static_cast<uint32_t>(out.size());
        d.size = static_cast<uint32_t>(payload.size());
        d.name = WideToUtf8(e.name);
        out.insert(out.end(), payload.begin(), payload.end());
        while ((out.size() & 3) != 0) out.push_back(0);
        dir.push_back(std::move(d));
    }

    uint32_t dirOffset = static_cast<uint32_t>(out.size());
    WriteU32LEAt(out, 8, dirOffset);

    for (const auto& d : dir) {
        if (isQuakeWad) {
            AppendU32LE(out, d.offset);
            AppendU32LE(out, d.size);
            AppendU32LE(out, d.size);
            out.push_back(0x43);
            out.push_back(0);
            AppendU16LE(out, 0);
            AppendFixedAscii(out, d.name, 16);
        } else {
            AppendU32LE(out, d.offset);
            AppendU32LE(out, d.size);
            AppendFixedAscii(out, d.name, 8);
        }
    }
    return true;
}

inline bool RebuildPakArchive(const ParsedGameArchive& arc, std::vector<uint8_t>& out) {
    using namespace gfe_detail;
    out.clear();
    AppendFixedAscii(out, "PACK", 4);
    AppendU32LE(out, 0); // dirOffset placeholder
    AppendU32LE(out, 0); // dirSize placeholder

    struct PakDir {
        std::string path;
        uint32_t offset;
        uint32_t size;
    };
    std::vector<PakDir> dir;
    for (const auto& e : arc.entries) {
        if (e.isDirectory) continue;
        std::vector<uint8_t> payload;
        ExtractArchiveEntryBytes(arc, e, payload);
        PakDir d;
        d.path = WideToUtf8(e.path);
        d.offset = static_cast<uint32_t>(out.size());
        d.size = static_cast<uint32_t>(payload.size());
        out.insert(out.end(), payload.begin(), payload.end());
        dir.push_back(std::move(d));
    }

    uint32_t dirOffset = static_cast<uint32_t>(out.size());
    uint32_t dirSize = static_cast<uint32_t>(dir.size() * 64);
    WriteU32LEAt(out, 4, dirOffset);
    WriteU32LEAt(out, 8, dirSize);

    for (const auto& d : dir) {
        AppendFixedAscii(out, d.path, 56);
        AppendU32LE(out, d.offset);
        AppendU32LE(out, d.size);
    }
    return true;
}

inline bool RebuildZipArchive(const ParsedGameArchive& arc, std::vector<uint8_t>& out) {
    using namespace gfe_detail;
    out.clear();

    struct CdRecord {
        std::string path;
        uint16_t method;
        uint32_t crc;
        uint32_t compSize;
        uint32_t uncompSize;
        uint32_t localHeaderOff;
    };
    std::vector<CdRecord> cd;

    for (const auto& e : arc.entries) {
        if (e.isDirectory) continue;
        // Skip synthesized reports from JAR/APPX parsers
        if (e.folder == L"_mod_metadata" || e.folder == L"_xbox_metadata") continue;

        std::vector<uint8_t> uncomp;
        if (!ExtractArchiveEntryBytes(arc, e, uncomp)) continue;

        uint32_t crc = ComputeCrc32(uncomp);
        std::vector<uint8_t> comp = DeflateRaw(uncomp);
        uint16_t method = 8;
        if (comp.empty() || comp.size() >= uncomp.size()) {
            method = 0;
            comp = uncomp;
        }

        std::string pathUtf8 = WideToUtf8(e.path);
        uint32_t localOff = static_cast<uint32_t>(out.size());

        // Local file header (PK\x03\x04)
        AppendU32LE(out, 0x04034B50u);
        AppendU16LE(out, 20);
        AppendU16LE(out, 0x0800); // UTF-8 flag
        AppendU16LE(out, method);
        AppendU16LE(out, 0);
        AppendU16LE(out, 0);
        AppendU32LE(out, crc);
        AppendU32LE(out, static_cast<uint32_t>(comp.size()));
        AppendU32LE(out, static_cast<uint32_t>(uncomp.size()));
        AppendU16LE(out, static_cast<uint16_t>(pathUtf8.size()));
        AppendU16LE(out, 0);
        out.insert(out.end(), pathUtf8.begin(), pathUtf8.end());
        out.insert(out.end(), comp.begin(), comp.end());

        CdRecord rec;
        rec.path = std::move(pathUtf8);
        rec.method = method;
        rec.crc = crc;
        rec.compSize = static_cast<uint32_t>(comp.size());
        rec.uncompSize = static_cast<uint32_t>(uncomp.size());
        rec.localHeaderOff = localOff;
        cd.push_back(std::move(rec));
    }

    uint32_t cdStart = static_cast<uint32_t>(out.size());
    for (const auto& rec : cd) {
        AppendU32LE(out, 0x02014B50u);
        AppendU16LE(out, 20);
        AppendU16LE(out, 20);
        AppendU16LE(out, 0x0800);
        AppendU16LE(out, rec.method);
        AppendU16LE(out, 0);
        AppendU16LE(out, 0);
        AppendU32LE(out, rec.crc);
        AppendU32LE(out, rec.compSize);
        AppendU32LE(out, rec.uncompSize);
        AppendU16LE(out, static_cast<uint16_t>(rec.path.size()));
        AppendU16LE(out, 0);
        AppendU16LE(out, 0);
        AppendU16LE(out, 0);
        AppendU16LE(out, 0);
        AppendU32LE(out, 0);
        AppendU32LE(out, rec.localHeaderOff);
        out.insert(out.end(), rec.path.begin(), rec.path.end());
    }
    uint32_t cdSize = static_cast<uint32_t>(out.size()) - cdStart;

    // EOCD (PK\x05\x06)
    AppendU32LE(out, 0x06054B50u);
    AppendU16LE(out, 0);
    AppendU16LE(out, 0);
    AppendU16LE(out, static_cast<uint16_t>(cd.size()));
    AppendU16LE(out, static_cast<uint16_t>(cd.size()));
    AppendU32LE(out, cdSize);
    AppendU32LE(out, cdStart);
    AppendU16LE(out, 0);
    return true;
}

inline bool RebuildRenpyRpaArchive(const ParsedGameArchive& arc, std::vector<uint8_t>& out) {
    using namespace gfe_detail;
    out.clear();

    // Reserve 34-byte RPA-3.0 header ("RPA-3.0 %016llx %08x\n")
    uint32_t xorKey = 0xDEADBEEFu;
    out.resize(34, ' ');

    struct RpaItem {
        std::string path;
        uint64_t offset;
        uint32_t size;
    };
    std::vector<RpaItem> items;

    for (const auto& e : arc.entries) {
        if (e.isDirectory) continue;
        std::vector<uint8_t> data;
        if (!ExtractArchiveEntryBytes(arc, e, data)) continue;

        RpaItem it;
        it.path = WideToUtf8(e.path);
        it.offset = out.size();
        it.size = static_cast<uint32_t>(data.size());
        out.insert(out.end(), data.begin(), data.end());
        items.push_back(std::move(it));
    }

    uint64_t indexOffset = out.size();

    // Build Python Pickle Protocol 2 dict: { path: [ (offset ^ key, size ^ key, b"") ] }
    std::vector<uint8_t> pickle;
    pickle.push_back(0x80);
    pickle.push_back(0x02);
    pickle.push_back(0x7D); // EMPTY_DICT
    pickle.push_back(0x28); // MARK

    auto pushPickleInt = [&](uint64_t val) {
        if (val <= 0x7FFFFFFFULL) {
            pickle.push_back(0x4A); // BININT
            AppendU32LE(pickle, static_cast<uint32_t>(val));
        } else {
            pickle.push_back(0x8A); // LONG1
            pickle.push_back(8);
            for (int b = 0; b < 8; ++b) pickle.push_back(static_cast<uint8_t>((val >> (b * 8)) & 0xFF));
        }
    };

    for (const auto& it : items) {
        // Key: BINUNICODE (0x58)
        pickle.push_back(0x58);
        AppendU32LE(pickle, static_cast<uint32_t>(it.path.size()));
        pickle.insert(pickle.end(), it.path.begin(), it.path.end());

        // Value: [ (offset ^ xorKey, size ^ xorKey, "") ]
        pickle.push_back(0x5D); // EMPTY_LIST
        pushPickleInt(it.offset ^ xorKey);
        pushPickleInt(static_cast<uint64_t>(it.size) ^ xorKey);
        pickle.push_back(0x55); // SHORT_BINSTRING ""
        pickle.push_back(0x00);
        pickle.push_back(0x87); // TUPLE3
        pickle.push_back(0x61); // APPEND
    }
    pickle.push_back(0x75); // SETITEMS
    pickle.push_back(0x2E); // STOP

    std::vector<uint8_t> zIndex = CompressZlib(pickle);
    out.insert(out.end(), zIndex.begin(), zIndex.end());

    char hdr[35]{};
    std::snprintf(hdr, sizeof(hdr), "RPA-3.0 %016llx %08x\n",
                  static_cast<unsigned long long>(indexOffset), xorKey);
    std::memcpy(out.data(), hdr, 34);
    return true;
}

inline bool RebuildFastFileArchive(const ParsedGameArchive& arc, std::vector<uint8_t>& out) {
    using namespace gfe_detail;
    const FastFileZoneState& st = arc.ffZoneState;
    if (!st.valid || st.zone.empty()) return false;

    // Apply any modified entry.inlineData back into a working copy of the zone stream
    std::vector<uint8_t> zone = st.zone;

    for (const auto& e : arc.entries) {
        if (e.folder == L"zone_stream" && e.inlineData.size() != st.zone.size() && !e.inlineData.empty()) {
            zone = e.inlineData;
            break;
        }
    }

    // Also check individual rawfile/script/menu items for edits
    for (const auto& it : st.items) {
        std::string prefix = (it.kind == "rawfile") ? "rawfiles/" :
                             (it.kind == "script") ? "scripts/" :
                             (it.kind == "menu") ? "menus/" :
                             (it.kind == "stringtable") ? "stringtables/" : "localize/";
        std::wstring expectedPath = SplitVirtualPath(AsciiToWide(prefix + it.name)).cleanPath;
        for (const auto& e : arc.entries) {
            if (e.path == expectedPath && !e.inlineData.empty()) {
                if (it.kind == "rawfile" && it.compressed) {
                    std::vector<uint8_t> comp = DeflateRaw(e.inlineData);
                    std::vector<uint8_t> packed;
                    AppendU32LE(packed, static_cast<uint32_t>(e.inlineData.size()));
                    packed.insert(packed.end(), comp.begin(), comp.end());
                    if (packed.size() <= it.size && it.data + it.size <= zone.size()) {
                        std::memset(zone.data() + it.data, 0, it.size);
                        std::memcpy(zone.data() + it.data, packed.data(), packed.size());
                        WriteU32LEAt(zone, it.hdr + 8, static_cast<uint32_t>(packed.size()));
                    }
                } else if (it.kind != "stringtable" && e.inlineData.size() <= it.size && it.data + it.size <= zone.size()) {
                    std::memset(zone.data() + it.data, 0, it.size);
                    std::memcpy(zone.data() + it.data, e.inlineData.data(), e.inlineData.size());
                    if (it.kind == "rawfile" || it.kind == "script" || it.kind == "menu") {
                        WriteU32LEAt(zone, it.hdr + 8, static_cast<uint32_t>(e.inlineData.size()));
                    }
                }
                break;
            }
        }
    }

    out.clear();
    AppendFixedAscii(out, st.magic.empty() ? "TAffu100" : st.magic, 8);
    AppendU32LE(out, st.version);

    if (st.isBo1) {
        std::vector<uint8_t> zStream = CompressZlib(zone);
        out.insert(out.end(), zStream.begin(), zStream.end());
        return true;
    }

    // BO2 (TAffu100): deflate xfile preamble + 0x7FC0 chunked raw deflate blocks
    std::vector<uint8_t> xfBytes;
    for (uint32_t w : st.xfile) AppendU32LE(xfBytes, w);
    xfBytes.insert(xfBytes.end(), st.xfileTail.begin(), st.xfileTail.end());
    std::vector<uint8_t> xfComp = DeflateRaw(xfBytes);
    AppendU32LE(out, static_cast<uint32_t>(xfComp.size()));
    out.insert(out.end(), xfComp.begin(), xfComp.end());

    static constexpr size_t kBo2ChunkSize = 0x7FC0;
    size_t pos = 0;
    while (pos < zone.size()) {
        size_t chunkLen = std::min<size_t>(kBo2ChunkSize, zone.size() - pos);
        std::vector<uint8_t> compChunk = DeflateRaw(std::span<const uint8_t>(zone.data() + pos, chunkLen));
        AppendU32LE(out, static_cast<uint32_t>(compChunk.size()));
        out.insert(out.end(), compChunk.begin(), compChunk.end());
        pos += chunkLen;
    }
    AppendU32LE(out, 0); // terminating block
    return true;
}

} // namespace gfe_repack_detail

inline bool SaveModifiedArchive(const ParsedGameArchive& arc, const std::wstring& destPath) {
    using namespace gfe_repack_detail;
    std::vector<uint8_t> rebuilt;
    bool ok = false;

    switch (arc.formatId) {
        case ArchiveFormatId::WAD:
            ok = RebuildWadArchive(arc, rebuilt);
            break;
        case ArchiveFormatId::PAK:
            ok = RebuildPakArchive(arc, rebuilt);
            break;
        case ArchiveFormatId::ZIP:
        case ArchiveFormatId::PK3:
        case ArchiveFormatId::JAR:
        case ArchiveFormatId::APPX:
            ok = RebuildZipArchive(arc, rebuilt);
            break;
        case ArchiveFormatId::RPA:
            ok = RebuildRenpyRpaArchive(arc, rebuilt);
            break;
        case ArchiveFormatId::FF:
            ok = RebuildFastFileArchive(arc, rebuilt);
            break;
        default:
            return false;
    }

    if (!ok || rebuilt.empty()) return false;
    return WriteAllBytesToDisk(destPath.empty() ? arc.filePath : destPath, rebuilt);
}

inline bool CreateZipArchiveFromDisk(const std::vector<std::wstring>& sourcePaths, const std::wstring& destZipPath) {
    using namespace gfe_detail;
    if (sourcePaths.empty() || destZipPath.empty()) return false;

    ParsedGameArchive synthArc;
    synthArc.valid = true;
    synthArc.formatId = ArchiveFormatId::ZIP;
    synthArc.supportsRepack = true;

    auto addDiskFile = [&](const std::wstring& diskPath, const std::wstring& relVirtualPath) {
        MappedFile mf(diskPath);
        if (!mf.valid()) return;
        auto sp = SplitVirtualPath(relVirtualPath);
        ArchiveEntry e;
        e.name = sp.name;
        e.path = sp.cleanPath;
        e.folder = sp.folder;
        e.uncompressedSize = mf.size();
        e.compressedSize = mf.size();
        e.category = ClassifyFile(sp.name);
        e.customFlags = ExtractMode::InlineOnly;
        e.inlineData.assign(mf.span().begin(), mf.span().end());
        synthArc.entries.push_back(std::move(e));
    };

    for (const auto& src : sourcePaths) {
        std::error_code ec;
        std::filesystem::path fsPath(src);
        if (!std::filesystem::exists(fsPath, ec)) continue;

        if (std::filesystem::is_directory(fsPath, ec)) {
            std::filesystem::path parentPath = fsPath.parent_path();
            for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(fsPath, ec)) {
                if (dirEntry.is_regular_file(ec)) {
                    std::filesystem::path rel = std::filesystem::relative(dirEntry.path(), parentPath, ec);
                    addDiskFile(dirEntry.path().wstring(), rel.wstring());
                }
            }
        } else if (std::filesystem::is_regular_file(fsPath, ec)) {
            addDiskFile(fsPath.wstring(), fsPath.filename().wstring());
        }
    }

    if (synthArc.entries.empty()) return false;
    return SaveModifiedArchive(synthArc, destZipPath);
}

} // namespace speta

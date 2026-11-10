#pragma once

#ifndef _ALLOW_COMPILER_AND_STL_VERSION_MISMATCH
#define _ALLOW_COMPILER_AND_STL_VERSION_MISMATCH
#endif

#include <cstdint>
#include <cstddef>
#include <vector>
#include <span>
#include <array>
#include <algorithm>
#include <cstring>

namespace speta {

namespace detail {

inline constexpr std::array<uint32_t, 256> BuildCrc32Table() {
    std::array<uint32_t, 256> table{};
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t c = i;
        for (int k = 0; k < 8; ++k) {
            c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
        }
        table[i] = c;
    }
    return table;
}

inline constexpr auto kCrc32Table = BuildCrc32Table();

struct BitReader {
    std::span<const uint8_t> data;
    size_t bytePos = 0;
    uint64_t bitBuf = 0;
    int bitCount = 0;

    explicit BitReader(std::span<const uint8_t> src) : data(src) {}

    inline void Fill() {
        while (bitCount <= 56 && bytePos < data.size()) {
            bitBuf |= static_cast<uint64_t>(data[bytePos++]) << bitCount;
            bitCount += 8;
        }
    }

    inline bool EnsureBits(int n) {
        if (bitCount < n) {
            Fill();
        }
        return bitCount >= n;
    }

    inline uint32_t PeekBits(int n) {
        return static_cast<uint32_t>(bitBuf & ((1ULL << n) - 1ULL));
    }

    inline void DropBits(int n) {
        bitBuf >>= n;
        bitCount -= n;
    }

    inline bool ReadBits(int n, uint32_t& outVal) {
        if (n == 0) {
            outVal = 0;
            return true;
        }
        if (!EnsureBits(n)) return false;
        outVal = PeekBits(n);
        DropBits(n);
        return true;
    }

    inline void AlignToByte() {
        int drop = bitCount & 7;
        if (drop > 0) {
            DropBits(drop);
        }
    }

    inline size_t ConsumedBytes() const {
        size_t unreadBytes = static_cast<size_t>(bitCount / 8);
        return (bytePos >= unreadBytes) ? (bytePos - unreadBytes) : 0;
    }

    inline bool ReadAlignedBytes(uint8_t* dst, size_t count) {
        AlignToByte();
        while (bitCount >= 8 && count > 0) {
            *dst++ = static_cast<uint8_t>(bitBuf & 0xFF);
            DropBits(8);
            --count;
        }
        if (count == 0) return true;
        if (bytePos + count > data.size()) return false;
        std::memcpy(dst, data.data() + bytePos, count);
        bytePos += count;
        return true;
    }
};

struct HuffmanTree {
    static constexpr int kMaxBits = 15;
    static constexpr int kFastBits = 9;
    static constexpr int kFastSize = 1 << kFastBits;

    // Fast lookup table: low 12 bits = symbol, high 4 bits = code length (0 if > kFastBits or invalid)
    std::array<uint16_t, kFastSize> fastTable{};
    std::array<uint16_t, 320> symbols{};
    std::array<uint16_t, kMaxBits + 1> counts{};
    std::array<int32_t, kMaxBits + 1> firstCode{};
    std::array<int32_t, kMaxBits + 1> firstSymbol{};
    int maxLen = 0;

    static inline uint32_t ReverseBits(uint32_t code, int len) {
        uint32_t res = 0;
        for (int i = 0; i < len; ++i) {
            res = (res << 1) | (code & 1);
            code >>= 1;
        }
        return res;
    }

    bool Build(std::span<const uint8_t> lengths) {
        fastTable.fill(0);
        counts.fill(0);
        firstCode.fill(0);
        firstSymbol.fill(0);
        maxLen = 0;

        for (uint8_t len : lengths) {
            if (len > kMaxBits) return false;
            if (len > 0) {
                counts[len]++;
                if (len > maxLen) maxLen = len;
            }
        }

        if (maxLen == 0) {
            return true;
        }

        int32_t code = 0;
        int32_t symOffset = 0;
        std::array<int32_t, kMaxBits + 1> nextCode{};
        for (int bits = 1; bits <= kMaxBits; ++bits) {
            code = (code + counts[bits - 1]) << 1;
            if (code + counts[bits] > (1 << bits)) {
                return false;
            }
            firstCode[bits] = code;
            firstSymbol[bits] = symOffset;
            nextCode[bits] = code;
            symOffset += counts[bits];
        }

        std::array<int32_t, kMaxBits + 1> symWrite = firstSymbol;
        for (size_t sym = 0; sym < lengths.size(); ++sym) {
            uint8_t len = lengths[sym];
            if (len > 0) {
                int32_t idx = symWrite[len]++;
                symbols[idx] = static_cast<uint16_t>(sym);

                int32_t c = nextCode[len]++;
                if (len <= kFastBits) {
                    uint32_t rev = ReverseBits(static_cast<uint32_t>(c), len);
                    uint16_t entry = static_cast<uint16_t>((len << 12) | (sym & 0x0FFF));
                    uint32_t step = 1u << len;
                    for (uint32_t fill = rev; fill < kFastSize; fill += step) {
                        fastTable[fill] = entry;
                    }
                }
            }
        }
        return true;
    }

    inline bool Decode(BitReader& br, uint16_t& outSym) const {
        if (maxLen == 0) return false;
        br.Fill();
        if (br.bitCount >= kFastBits) {
            uint16_t fast = fastTable[br.bitBuf & (kFastSize - 1)];
            if (fast != 0) {
                int len = fast >> 12;
                outSym = fast & 0x0FFF;
                br.DropBits(len);
                return true;
            }
        }

        int32_t code = 0;
        for (int len = 1; len <= maxLen; ++len) {
            if (br.bitCount < len) {
                if (!br.EnsureBits(len)) return false;
            }
            code = (code << 1) | static_cast<int32_t>((br.bitBuf >> (len - 1)) & 1);
            int32_t count = counts[len];
            int32_t first = firstCode[len];
            if (code >= first && code - first < count) {
                outSym = symbols[firstSymbol[len] + (code - first)];
                br.DropBits(len);
                return true;
            }
        }
        return false;
    }
};

struct BitWriter {
    std::vector<uint8_t> out;
    uint64_t bitBuf = 0;
    int bitCount = 0;

    inline void WriteBits(uint32_t val, int n) {
        if (n <= 0) return;
        bitBuf |= (static_cast<uint64_t>(val) & ((1ULL << n) - 1ULL)) << bitCount;
        bitCount += n;
        while (bitCount >= 8) {
            out.push_back(static_cast<uint8_t>(bitBuf & 0xFF));
            bitBuf >>= 8;
            bitCount -= 8;
        }
    }

    inline void WriteHuffman(uint32_t canonicalCode, int len) {
        uint32_t rev = HuffmanTree::ReverseBits(canonicalCode, len);
        WriteBits(rev, len);
    }

    inline void FlushToByte() {
        if (bitCount > 0) {
            out.push_back(static_cast<uint8_t>(bitBuf & 0xFF));
            bitBuf = 0;
            bitCount = 0;
        }
    }
};

inline void WriteFixedLitLen(BitWriter& bw, uint16_t sym) {
    if (sym <= 143) {
        bw.WriteHuffman(0x30 + sym, 8);
    } else if (sym <= 255) {
        bw.WriteHuffman(0x190 + (sym - 144), 9);
    } else if (sym <= 279) {
        bw.WriteHuffman(sym - 256, 7);
    } else {
        bw.WriteHuffman(0xC0 + (sym - 280), 8);
    }
}

inline void WriteFixedLengthMatch(BitWriter& bw, uint16_t length, uint16_t distance) {
    static constexpr std::array<uint16_t, 29> kLenBase = {
        3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
        35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258
    };
    static constexpr std::array<uint8_t, 29> kLenExtra = {
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
        3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0
    };
    static constexpr std::array<uint16_t, 30> kDistBase = {
        1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
        257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577
    };
    static constexpr std::array<uint8_t, 30> kDistExtra = {
        0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
        7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13
    };

    int lenCode = 28;
    for (int i = 0; i < 29; ++i) {
        if (i == 28 || (length >= kLenBase[i] && length < kLenBase[i + 1])) {
            lenCode = i;
            break;
        }
    }
    WriteFixedLitLen(bw, static_cast<uint16_t>(257 + lenCode));
    if (kLenExtra[lenCode] > 0) {
        bw.WriteBits(length - kLenBase[lenCode], kLenExtra[lenCode]);
    }

    int distCode = 29;
    for (int i = 0; i < 30; ++i) {
        if (i == 29 || (distance >= kDistBase[i] && distance < kDistBase[i + 1])) {
            distCode = i;
            break;
        }
    }
    bw.WriteHuffman(static_cast<uint32_t>(distCode), 5);
    if (kDistExtra[distCode] > 0) {
        bw.WriteBits(distance - kDistBase[distCode], kDistExtra[distCode]);
    }
}

} // namespace detail

inline uint32_t ComputeCrc32(std::span<const uint8_t> data) {
    uint32_t crc = 0xFFFFFFFFu;
    for (uint8_t b : data) {
        crc = detail::kCrc32Table[(crc ^ b) & 0xFFu] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFu;
}

inline uint32_t ComputeAdler32(std::span<const uint8_t> data) {
    constexpr uint32_t kModAdler = 65521u;
    uint32_t a = 1;
    uint32_t b = 0;
    size_t len = data.size();
    const uint8_t* ptr = data.data();

    while (len > 0) {
        size_t tlen = (len > 5552) ? 5552 : len;
        len -= tlen;
        do {
            a += *ptr++;
            b += a;
        } while (--tlen);
        a %= kModAdler;
        b %= kModAdler;
    }
    return (b << 16) | a;
}

inline bool InflateRaw(
    std::span<const uint8_t> src,
    std::vector<uint8_t>& out,
    size_t expectedSize = 0,
    size_t* bytesConsumed = nullptr
) {
    out.clear();
    if (src.empty()) return false;
    if (expectedSize > 0 && expectedSize <= 256 * 1024 * 1024) {
        out.reserve(expectedSize);
    } else {
        out.reserve(std::min<size_t>(src.size() * 2, 16 * 1024 * 1024));
    }

    static constexpr std::array<uint16_t, 29> kLenBase = {
        3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
        35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258
    };
    static constexpr std::array<uint8_t, 29> kLenExtra = {
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
        3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0
    };
    static constexpr std::array<uint16_t, 30> kDistBase = {
        1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
        257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577
    };
    static constexpr std::array<uint8_t, 30> kDistExtra = {
        0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
        7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13
    };
    static constexpr std::array<uint8_t, 19> kCodeLenOrder = {
        16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
    };

    detail::BitReader br(src);
    detail::HuffmanTree litLenTree;
    detail::HuffmanTree distTree;
    detail::HuffmanTree codeLenTree;

    uint32_t bfinal = 0;
    while (bfinal == 0) {
        uint32_t btype = 0;
        if (!br.ReadBits(1, bfinal) || !br.ReadBits(2, btype)) {
            return false;
        }

        if (btype == 0) {
            br.AlignToByte();
            uint32_t len = 0;
            uint32_t nlen = 0;
            if (!br.ReadBits(16, len) || !br.ReadBits(16, nlen)) {
                return false;
            }
            if ((len ^ 0xFFFFu) != nlen) {
                return false;
            }
            size_t curSize = out.size();
            out.resize(curSize + len);
            if (len > 0 && !br.ReadAlignedBytes(out.data() + curSize, len)) {
                return false;
            }
        } else if (btype == 1 || btype == 2) {
            if (btype == 1) {
                std::array<uint8_t, 288> fixedLit{};
                for (int i = 0; i <= 143; ++i) fixedLit[i] = 8;
                for (int i = 144; i <= 255; ++i) fixedLit[i] = 9;
                for (int i = 256; i <= 279; ++i) fixedLit[i] = 7;
                for (int i = 280; i <= 287; ++i) fixedLit[i] = 8;

                std::array<uint8_t, 32> fixedDist{};
                fixedDist.fill(5);

                if (!litLenTree.Build(fixedLit) || !distTree.Build(fixedDist)) {
                    return false;
                }
            } else {
                uint32_t hlit = 0, hdist = 0, hclen = 0;
                if (!br.ReadBits(5, hlit) || !br.ReadBits(5, hdist) || !br.ReadBits(4, hclen)) {
                    return false;
                }
                hlit += 257;
                hdist += 1;
                hclen += 4;
                if (hlit > 286 || hdist > 32) {
                    return false;
                }

                std::array<uint8_t, 19> clLengths{};
                for (uint32_t i = 0; i < hclen; ++i) {
                    uint32_t cl = 0;
                    if (!br.ReadBits(3, cl)) return false;
                    clLengths[kCodeLenOrder[i]] = static_cast<uint8_t>(cl);
                }

                if (!codeLenTree.Build(clLengths)) {
                    return false;
                }

                std::array<uint8_t, 320> allLengths{};
                uint32_t totalCodes = hlit + hdist;
                uint32_t idx = 0;
                while (idx < totalCodes) {
                    uint16_t sym = 0;
                    if (!codeLenTree.Decode(br, sym)) {
                        return false;
                    }
                    if (sym <= 15) {
                        allLengths[idx++] = static_cast<uint8_t>(sym);
                    } else if (sym == 16) {
                        if (idx == 0) return false;
                        uint32_t rep = 0;
                        if (!br.ReadBits(2, rep)) return false;
                        rep += 3;
                        uint8_t prev = allLengths[idx - 1];
                        if (idx + rep > totalCodes) return false;
                        for (uint32_t r = 0; r < rep; ++r) allLengths[idx++] = prev;
                    } else if (sym == 17) {
                        uint32_t rep = 0;
                        if (!br.ReadBits(3, rep)) return false;
                        rep += 3;
                        if (idx + rep > totalCodes) return false;
                        for (uint32_t r = 0; r < rep; ++r) allLengths[idx++] = 0;
                    } else if (sym == 18) {
                        uint32_t rep = 0;
                        if (!br.ReadBits(7, rep)) return false;
                        rep += 11;
                        if (idx + rep > totalCodes) return false;
                        for (uint32_t r = 0; r < rep; ++r) allLengths[idx++] = 0;
                    } else {
                        return false;
                    }
                }

                if (allLengths[256] == 0) {
                    return false;
                }
                if (!litLenTree.Build(std::span<const uint8_t>(allLengths.data(), hlit))) {
                    return false;
                }
                if (!distTree.Build(std::span<const uint8_t>(allLengths.data() + hlit, hdist))) {
                    return false;
                }
            }

            while (true) {
                uint16_t sym = 0;
                if (!litLenTree.Decode(br, sym)) {
                    return false;
                }
                if (sym < 256) {
                    out.push_back(static_cast<uint8_t>(sym));
                } else if (sym == 256) {
                    break;
                } else {
                    uint32_t lenIdx = sym - 257;
                    if (lenIdx >= 29) return false;
                    uint32_t extraBits = 0;
                    if (kLenExtra[lenIdx] > 0 && !br.ReadBits(kLenExtra[lenIdx], extraBits)) {
                        return false;
                    }
                    uint32_t matchLen = kLenBase[lenIdx] + extraBits;

                    uint16_t distSym = 0;
                    if (!distTree.Decode(br, distSym) || distSym >= 30) {
                        return false;
                    }
                    uint32_t distExtra = 0;
                    if (kDistExtra[distSym] > 0 && !br.ReadBits(kDistExtra[distSym], distExtra)) {
                        return false;
                    }
                    uint32_t matchDist = kDistBase[distSym] + distExtra;

                    if (matchDist == 0 || matchDist > out.size()) {
                        return false;
                    }

                    size_t dstPos = out.size();
                    out.resize(dstPos + matchLen);
                    uint8_t* dstPtr = out.data() + dstPos;
                    const uint8_t* srcPtr = dstPtr - matchDist;

                    if (matchDist >= matchLen) {
                        std::memcpy(dstPtr, srcPtr, matchLen);
                    } else if (matchDist == 1) {
                        std::memset(dstPtr, *srcPtr, matchLen);
                    } else {
                        for (uint32_t i = 0; i < matchLen; ++i) {
                            dstPtr[i] = srcPtr[i];
                        }
                    }
                }
            }
        } else {
            return false;
        }
    }

    if (bytesConsumed) {
        *bytesConsumed = br.ConsumedBytes();
    }
    return true;
}

inline bool DecompressZlib(
    std::span<const uint8_t> src,
    std::vector<uint8_t>& out,
    size_t expectedSize = 0,
    size_t* bytesConsumed = nullptr
) {
    out.clear();
    if (src.size() < 2) return false;

    uint8_t b0 = src[0];
    uint8_t b1 = src[1];
    if (((static_cast<uint32_t>(b0) << 8) | b1) % 31 != 0) {
        return false;
    }
    if ((b0 & 0x0F) != 8) {
        return false;
    }
    if (((b0 >> 4) & 0x0F) > 7) {
        return false;
    }

    size_t headerLen = 2;
    if ((b1 & 0x20) != 0) {
        if (src.size() < 6) return false;
        headerLen = 6;
    }

    size_t rawConsumed = 0;
    if (!InflateRaw(src.subspan(headerLen), out, expectedSize, &rawConsumed)) {
        return false;
    }

    size_t totalConsumed = headerLen + rawConsumed;
    if (totalConsumed + 4 <= src.size()) {
        totalConsumed += 4;
    }
    if (bytesConsumed) {
        *bytesConsumed = totalConsumed;
    }
    return true;
}

inline std::vector<uint8_t> DeflateRaw(std::span<const uint8_t> src) {
    detail::BitWriter bw;
    bw.out.reserve(src.size() + 32);

    if (src.empty()) {
        // Final block (1), Fixed Huffman (01), End-of-block symbol 256 (0000000)
        bw.WriteBits(1, 1);
        bw.WriteBits(1, 2);
        detail::WriteFixedLitLen(bw, 256);
        bw.FlushToByte();
        return std::move(bw.out);
    }

    // Encode as a single Final Fixed-Huffman + LZ77 block
    bw.WriteBits(1, 1); // BFINAL = 1
    bw.WriteBits(1, 2); // BTYPE = 01 (Fixed Huffman)

    constexpr size_t kHashBits = 14;
    constexpr size_t kHashSize = 1 << kHashBits;
    constexpr size_t kWindowSize = 32768;
    constexpr size_t kMaxChain = 32;

    std::vector<int32_t> head(kHashSize, -1);
    std::vector<int32_t> prev(kWindowSize, -1);

    auto hash3 = [](const uint8_t* p) -> uint32_t {
        uint32_t v = (static_cast<uint32_t>(p[0]) << 16) |
                     (static_cast<uint32_t>(p[1]) << 8) |
                     static_cast<uint32_t>(p[2]);
        return ((v * 2654435761u) >> (32 - kHashBits)) & (kHashSize - 1);
    };

    const uint8_t* data = src.data();
    const size_t n = src.size();
    size_t pos = 0;

    while (pos < n) {
        uint16_t bestLen = 0;
        uint16_t bestDist = 0;

        if (pos + 3 <= n) {
            uint32_t h = hash3(data + pos);
            int32_t matchPos = head[h];
            head[h] = static_cast<int32_t>(pos);
            prev[pos & (kWindowSize - 1)] = matchPos;

            size_t chainLen = 0;
            size_t maxLen = std::min<size_t>(258, n - pos);

            while (matchPos >= 0 && chainLen < kMaxChain) {
                size_t dist = pos - static_cast<size_t>(matchPos);
                if (dist > kWindowSize || dist == 0) break;

                if (data[matchPos + bestLen] == data[pos + bestLen] &&
                    data[matchPos] == data[pos] &&
                    data[matchPos + 1] == data[pos + 1] &&
                    data[matchPos + 2] == data[pos + 2]) {
                    size_t len = 3;
                    while (len < maxLen && data[matchPos + len] == data[pos + len]) {
                        ++len;
                    }
                    if (len > bestLen) {
                        bestLen = static_cast<uint16_t>(len);
                        bestDist = static_cast<uint16_t>(dist);
                        if (bestLen == maxLen) break;
                    }
                }
                matchPos = prev[static_cast<size_t>(matchPos) & (kWindowSize - 1)];
                ++chainLen;
            }
        }

        if (bestLen >= 3) {
            detail::WriteFixedLengthMatch(bw, bestLen, bestDist);
            for (uint16_t step = 1; step < bestLen; ++step) {
                size_t advPos = pos + step;
                if (advPos + 3 <= n) {
                    uint32_t h = hash3(data + advPos);
                    prev[advPos & (kWindowSize - 1)] = head[h];
                    head[h] = static_cast<int32_t>(advPos);
                }
            }
            pos += bestLen;
        } else {
            detail::WriteFixedLitLen(bw, data[pos]);
            ++pos;
        }
    }

    detail::WriteFixedLitLen(bw, 256);
    bw.FlushToByte();
    return std::move(bw.out);
}

inline std::vector<uint8_t> CompressZlib(std::span<const uint8_t> src) {
    std::vector<uint8_t> raw = DeflateRaw(src);
    std::vector<uint8_t> out;
    out.reserve(2 + raw.size() + 4);

    // RFC 1950 Zlib Header: CMF = 0x78 (Deflate, 32K window), FLG = 0x9C (Default compression, check bits)
    out.push_back(0x78);
    out.push_back(0x9C);
    out.insert(out.end(), raw.begin(), raw.end());

    uint32_t adler = ComputeAdler32(src);
    out.push_back(static_cast<uint8_t>((adler >> 24) & 0xFF));
    out.push_back(static_cast<uint8_t>((adler >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((adler >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>(adler & 0xFF));
    return out;
}

inline bool DecompressLz4Block(
    std::span<const uint8_t> src,
    std::vector<uint8_t>& out,
    size_t uncompressedSize
) {
    out.clear();
    if (uncompressedSize == 0) return true;
    if (src.empty()) return false;
    out.reserve(std::min<size_t>(uncompressedSize, 64 * 1024 * 1024));

    size_t pos = 0;
    const size_t srcLen = src.size();

    while (pos < srcLen && out.size() < uncompressedSize) {
        uint8_t token = src[pos++];
        size_t litLen = token >> 4;
        if (litLen == 15) {
            while (pos < srcLen) {
                uint8_t extra = src[pos++];
                litLen += extra;
                if (extra != 255) break;
            }
        }

        if (pos + litLen > srcLen) return false;
        if (out.size() + litLen > uncompressedSize) {
            litLen = uncompressedSize - out.size();
        }
        out.insert(out.end(), src.data() + pos, src.data() + pos + litLen);
        pos += litLen;

        if (pos >= srcLen || out.size() >= uncompressedSize) {
            break;
        }

        if (pos + 2 > srcLen) return false;
        uint16_t offset = static_cast<uint16_t>(src[pos]) | (static_cast<uint16_t>(src[pos + 1]) << 8);
        pos += 2;
        if (offset == 0 || offset > out.size()) return false;

        size_t matchLen = (token & 0x0F) + 4;
        if ((token & 0x0F) == 15) {
            while (pos < srcLen) {
                uint8_t extra = src[pos++];
                matchLen += extra;
                if (extra != 255) break;
            }
        }

        if (out.size() + matchLen > uncompressedSize) {
            matchLen = uncompressedSize - out.size();
        }

        size_t dstStart = out.size();
        out.resize(dstStart + matchLen);
        uint8_t* dstPtr = out.data() + dstStart;
        const uint8_t* srcPtr = dstPtr - offset;
        for (size_t i = 0; i < matchLen; ++i) {
            dstPtr[i] = srcPtr[i];
        }
    }

    return out.size() == uncompressedSize;
}

} // namespace speta

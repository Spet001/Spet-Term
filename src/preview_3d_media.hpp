#pragma once

#include "common.hpp"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi1_2.h>
#include <d2d1_1.h>
#include <wincodec.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <mmsystem.h>
#include <bcrypt.h>
#include <restartmanager.h>
#include <shlwapi.h>

#include <span>
#include <array>
#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <atomic>
#include <mutex>
#include <thread>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace speta {

// ============================================================================
// 1. 3D .GLB / .GLTF / .OBJ HARDWARE PREVIEWER (Preview3DEngine)
// ============================================================================

struct PreviewVertex {
    float px, py, pz;
    float nx, ny, nz;
    float u, v;
    float r, g, b, a;
};

namespace detail {

struct Mat4 {
    float m[16];

    static Mat4 Identity() {
        Mat4 res{};
        res.m[0] = 1.0f;
        res.m[5] = 1.0f;
        res.m[10] = 1.0f;
        res.m[15] = 1.0f;
        return res;
    }

    static Mat4 Multiply(const Mat4& a, const Mat4& b) {
        Mat4 out{};
        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                out.m[r * 4 + c] =
                    a.m[r * 4 + 0] * b.m[0 * 4 + c] +
                    a.m[r * 4 + 1] * b.m[1 * 4 + c] +
                    a.m[r * 4 + 2] * b.m[2 * 4 + c] +
                    a.m[r * 4 + 3] * b.m[3 * 4 + c];
            }
        }
        return out;
    }

    static Mat4 Translation(float tx, float ty, float tz) {
        Mat4 res = Identity();
        res.m[12] = tx;
        res.m[13] = ty;
        res.m[14] = tz;
        return res;
    }

    static Mat4 Scale(float sx, float sy, float sz) {
        Mat4 res = Identity();
        res.m[0] = sx;
        res.m[5] = sy;
        res.m[10] = sz;
        return res;
    }

    static Mat4 RotationY(float rad) {
        Mat4 res = Identity();
        float c = std::cos(rad);
        float s = std::sin(rad);
        res.m[0] = c;
        res.m[2] = -s;
        res.m[8] = s;
        res.m[10] = c;
        return res;
    }

    static Mat4 RotationX(float rad) {
        Mat4 res = Identity();
        float c = std::cos(rad);
        float s = std::sin(rad);
        res.m[5] = c;
        res.m[6] = s;
        res.m[9] = -s;
        res.m[10] = c;
        return res;
    }

    static Mat4 FromQuaternion(float x, float y, float z, float w) {
        Mat4 res = Identity();
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, yz = y * z;
        float wx = w * x, wy = w * y, wz = w * z;

        res.m[0] = 1.0f - 2.0f * (yy + zz);
        res.m[1] = 2.0f * (xy + wz);
        res.m[2] = 2.0f * (xz - wy);
        res.m[4] = 2.0f * (xy - wz);
        res.m[5] = 1.0f - 2.0f * (xx + zz);
        res.m[6] = 2.0f * (yz + wx);
        res.m[8] = 2.0f * (xz + wy);
        res.m[9] = 2.0f * (yz - wx);
        res.m[10] = 1.0f - 2.0f * (xx + yy);
        return res;
    }

    static Mat4 LookAtLH(float eyeX, float eyeY, float eyeZ,
                         float atX, float atY, float atZ,
                         float upX, float upY, float upZ) {
        float zx = atX - eyeX;
        float zy = atY - eyeY;
        float zz = atZ - eyeZ;
        float zlen = std::sqrt(zx * zx + zy * zy + zz * zz);
        if (zlen > 1e-6f) { zx /= zlen; zy /= zlen; zz /= zlen; }

        float xx = upY * zz - upZ * zy;
        float xy = upZ * zx - upX * zz;
        float xz = upX * zy - upY * zx;
        float xlen = std::sqrt(xx * xx + xy * xy + xz * xz);
        if (xlen > 1e-6f) { xx /= xlen; xy /= xlen; xz /= xlen; }

        float yx = zy * xz - zz * xy;
        float yy = zz * xx - zx * xz;
        float yz = zx * xy - zy * xx;

        Mat4 res = Identity();
        res.m[0] = xx; res.m[1] = yx; res.m[2] = zx; res.m[3] = 0.0f;
        res.m[4] = xy; res.m[5] = yy; res.m[6] = zy; res.m[7] = 0.0f;
        res.m[8] = xz; res.m[9] = yz; res.m[10] = zz; res.m[11] = 0.0f;
        res.m[12] = -(xx * eyeX + xy * eyeY + xz * eyeZ);
        res.m[13] = -(yx * eyeX + yy * eyeY + yz * eyeZ);
        res.m[14] = -(zx * eyeX + zy * eyeY + zz * eyeZ);
        res.m[15] = 1.0f;
        return res;
    }

    static Mat4 PerspectiveFovLH(float fovY, float aspect, float zn, float zf) {
        float yScale = 1.0f / std::tan(fovY * 0.5f);
        float xScale = yScale / (aspect > 0.001f ? aspect : 1.0f);
        Mat4 res{};
        res.m[0] = xScale;
        res.m[5] = yScale;
        res.m[10] = zf / (zf - zn);
        res.m[11] = 1.0f;
        res.m[14] = -zn * zf / (zf - zn);
        return res;
    }

    void TransformPoint(float x, float y, float z, float& ox, float& oy, float& oz) const {
        ox = x * m[0] + y * m[4] + z * m[8] + m[12];
        oy = x * m[1] + y * m[5] + z * m[9] + m[13];
        oz = x * m[2] + y * m[6] + z * m[10] + m[14];
    }

    void TransformNormal(float x, float y, float z, float& ox, float& oy, float& oz) const {
        ox = x * m[0] + y * m[4] + z * m[8];
        oy = x * m[1] + y * m[5] + z * m[9];
        oz = x * m[2] + y * m[6] + z * m[10];
        float len = std::sqrt(ox * ox + oy * oy + oz * oz);
        if (len > 1e-6f) {
            ox /= len;
            oy /= len;
            oz /= len;
        }
    }
};

// Lightweight recursive JSON DOM parser for glTF 2.0
struct GlbJsonValue {
    enum class Type { Null, Bool, Number, String, Array, Object };
    Type type = Type::Null;
    double numVal = 0.0;
    bool boolVal = false;
    std::string strVal;
    std::vector<GlbJsonValue> arrVal;
    std::vector<std::pair<std::string, GlbJsonValue>> objVal;

    const GlbJsonValue* Get(std::string_view key) const {
        if (type != Type::Object) return nullptr;
        for (const auto& kv : objVal) {
            if (kv.first == key) return &kv.second;
        }
        return nullptr;
    }

    const GlbJsonValue* At(size_t idx) const {
        if (type != Type::Array || idx >= arrVal.size()) return nullptr;
        return &arrVal[idx];
    }

    size_t Size() const {
        if (type == Type::Array) return arrVal.size();
        if (type == Type::Object) return objVal.size();
        return 0;
    }

    int AsInt(int def = -1) const {
        return (type == Type::Number) ? static_cast<int>(numVal) : def;
    }

    uint32_t AsUInt(uint32_t def = 0) const {
        return (type == Type::Number && numVal >= 0.0) ? static_cast<uint32_t>(numVal) : def;
    }

    float AsFloat(float def = 0.0f) const {
        return (type == Type::Number) ? static_cast<float>(numVal) : def;
    }

    bool AsBool(bool def = false) const {
        return (type == Type::Bool) ? boolVal : def;
    }

    std::string_view AsString(std::string_view def = "") const {
        return (type == Type::String) ? std::string_view(strVal) : def;
    }

    int GetInt(std::string_view key, int def = -1) const {
        const auto* v = Get(key);
        return v ? v->AsInt(def) : def;
    }

    uint32_t GetUInt(std::string_view key, uint32_t def = 0) const {
        const auto* v = Get(key);
        return v ? v->AsUInt(def) : def;
    }

    float GetFloat(std::string_view key, float def = 0.0f) const {
        const auto* v = Get(key);
        return v ? v->AsFloat(def) : def;
    }

    std::string_view GetString(std::string_view key, std::string_view def = "") const {
        const auto* v = Get(key);
        return v ? v->AsString(def) : def;
    }
};

class GlbJsonParser {
public:
    static GlbJsonValue Parse(std::string_view src) {
        GlbJsonParser p(src);
        return p.ParseValue(0);
    }

private:
    std::string_view s_;
    size_t pos_ = 0;

    explicit GlbJsonParser(std::string_view s) : s_(s) {}

    void SkipWs() {
        while (pos_ < s_.size()) {
            char c = s_[pos_];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                ++pos_;
            } else {
                break;
            }
        }
    }

    GlbJsonValue ParseValue(int depth) {
        SkipWs();
        if (pos_ >= s_.size() || depth > 64) return {};
        char c = s_[pos_];
        if (c == '{') return ParseObject(depth + 1);
        if (c == '[') return ParseArray(depth + 1);
        if (c == '"') {
            GlbJsonValue v;
            v.type = GlbJsonValue::Type::String;
            v.strVal = ParseStringRaw();
            return v;
        }
        if (c == 't' && s_.substr(pos_, 4) == "true") {
            pos_ += 4;
            GlbJsonValue v;
            v.type = GlbJsonValue::Type::Bool;
            v.boolVal = true;
            return v;
        }
        if (c == 'f' && s_.substr(pos_, 5) == "false") {
            pos_ += 5;
            GlbJsonValue v;
            v.type = GlbJsonValue::Type::Bool;
            v.boolVal = false;
            return v;
        }
        if (c == 'n' && s_.substr(pos_, 4) == "null") {
            pos_ += 4;
            return {};
        }
        if (c == '-' || (c >= '0' && c <= '9')) {
            return ParseNumber();
        }
        ++pos_;
        return {};
    }

    std::string ParseStringRaw() {
        if (pos_ >= s_.size() || s_[pos_] != '"') return {};
        ++pos_;
        std::string out;
        while (pos_ < s_.size()) {
            char c = s_[pos_++];
            if (c == '"') break;
            if (c == '\\' && pos_ < s_.size()) {
                char esc = s_[pos_++];
                switch (esc) {
                    case '"': out.push_back('"'); break;
                    case '\\': out.push_back('\\'); break;
                    case '/': out.push_back('/'); break;
                    case 'b': out.push_back('\b'); break;
                    case 'f': out.push_back('\f'); break;
                    case 'n': out.push_back('\n'); break;
                    case 'r': out.push_back('\r'); break;
                    case 't': out.push_back('\t'); break;
                    case 'u': {
                        if (pos_ + 4 <= s_.size()) {
                            pos_ += 4;
                        }
                        break;
                    }
                    default: out.push_back(esc); break;
                }
            } else {
                out.push_back(c);
            }
        }
        return out;
    }

    GlbJsonValue ParseNumber() {
        size_t start = pos_;
        if (pos_ < s_.size() && s_[pos_] == '-') ++pos_;
        while (pos_ < s_.size()) {
            char c = s_[pos_];
            if ((c >= '0' && c <= '9') || c == '.' || c == 'e' || c == 'E' || c == '+' || c == '-') {
                ++pos_;
            } else {
                break;
            }
        }
        std::string token(s_.substr(start, pos_ - start));
        GlbJsonValue v;
        v.type = GlbJsonValue::Type::Number;
        v.numVal = std::strtod(token.c_str(), nullptr);
        return v;
    }

    GlbJsonValue ParseArray(int depth) {
        GlbJsonValue v;
        v.type = GlbJsonValue::Type::Array;
        ++pos_; // skip '['
        SkipWs();
        if (pos_ < s_.size() && s_[pos_] == ']') {
            ++pos_;
            return v;
        }
        while (pos_ < s_.size()) {
            v.arrVal.push_back(ParseValue(depth));
            SkipWs();
            if (pos_ < s_.size() && s_[pos_] == ',') {
                ++pos_;
                continue;
            }
            if (pos_ < s_.size() && s_[pos_] == ']') {
                ++pos_;
            }
            break;
        }
        return v;
    }

    GlbJsonValue ParseObject(int depth) {
        GlbJsonValue v;
        v.type = GlbJsonValue::Type::Object;
        ++pos_; // skip '{'
        SkipWs();
        if (pos_ < s_.size() && s_[pos_] == '}') {
            ++pos_;
            return v;
        }
        while (pos_ < s_.size()) {
            SkipWs();
            if (pos_ >= s_.size() || s_[pos_] != '"') break;
            std::string key = ParseStringRaw();
            SkipWs();
            if (pos_ < s_.size() && s_[pos_] == ':') ++pos_;
            GlbJsonValue val = ParseValue(depth);
            v.objVal.emplace_back(std::move(key), std::move(val));
            SkipWs();
            if (pos_ < s_.size() && s_[pos_] == ',') {
                ++pos_;
                continue;
            }
            if (pos_ < s_.size() && s_[pos_] == '}') {
                ++pos_;
            }
            break;
        }
        return v;
    }
};

inline bool DecodeImageBytesWithWIC(IWICImagingFactory* pExternalFactory,
                                    const uint8_t* bytes,
                                    size_t byteLen,
                                    std::vector<uint8_t>& outRgba,
                                    uint32_t& outW,
                                    uint32_t& outH) {
    if (!bytes || byteLen == 0) return false;

    IWICImagingFactory* pFactory = pExternalFactory;
    bool ownsFactory = false;
    if (!pFactory) {
        HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                      IID_PPV_ARGS(&pFactory));
        if (FAILED(hr) || !pFactory) return false;
        ownsFactory = true;
    }

    IWICStream* pStream = nullptr;
    IWICBitmapDecoder* pDecoder = nullptr;
    IWICBitmapFrameDecode* pFrame = nullptr;
    IWICFormatConverter* pConverter = nullptr;
    bool ok = false;

    HRESULT hr = pFactory->CreateStream(&pStream);
    if (SUCCEEDED(hr) && pStream) {
        hr = pStream->InitializeFromMemory(const_cast<BYTE*>(bytes), static_cast<DWORD>(byteLen));
    }
    if (SUCCEEDED(hr) && pStream) {
        hr = pFactory->CreateDecoderFromStream(pStream, nullptr, WICDecodeMetadataCacheOnDemand, &pDecoder);
    }
    if (SUCCEEDED(hr) && pDecoder) {
        hr = pDecoder->GetFrame(0, &pFrame);
    }
    if (SUCCEEDED(hr) && pFrame) {
        UINT w = 0, h = 0;
        hr = pFrame->GetSize(&w, &h);
        if (SUCCEEDED(hr) && w > 0 && h > 0 && w <= 8192 && h <= 8192) {
            hr = pFactory->CreateFormatConverter(&pConverter);
            if (SUCCEEDED(hr) && pConverter) {
                hr = pConverter->Initialize(pFrame, GUID_WICPixelFormat32bppRGBA,
                                            WICBitmapDitherTypeNone, nullptr, 0.0f,
                                            WICBitmapPaletteTypeCustom);
                if (SUCCEEDED(hr)) {
                    outW = w;
                    outH = h;
                    outRgba.resize(static_cast<size_t>(w) * h * 4);
                    hr = pConverter->CopyPixels(nullptr, w * 4, static_cast<UINT>(outRgba.size()), outRgba.data());
                    ok = SUCCEEDED(hr);
                }
            }
        }
    }

    SafeRelease(pConverter);
    SafeRelease(pFrame);
    SafeRelease(pDecoder);
    SafeRelease(pStream);
    if (ownsFactory) SafeRelease(pFactory);
    return ok;
}

inline std::vector<uint8_t> DecodeBase64DataUri(std::string_view uri) {
    size_t comma = uri.find(',');
    if (comma == std::string_view::npos) return {};
    std::string_view b64 = uri.substr(comma + 1);

    static constexpr uint8_t kDecTable[256] = {
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
        64,64,64,64,64,64,64,64,64,64,64,62,64,64,64,63,
        52,53,54,55,56,57,58,59,60,61,64,64,64,64,64,64,
        64, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
        15,16,17,18,19,20,21,22,23,24,25,64,64,64,64,64,
        64,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
        41,42,43,44,45,46,47,48,49,50,51,64,64,64,64,64,
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64
    };

    std::vector<uint8_t> out;
    out.reserve(b64.size() * 3 / 4);
    uint32_t val = 0;
    int bits = -8;
    for (unsigned char c : b64) {
        if (kDecTable[c] == 64) {
            if (c == '=') break;
            continue;
        }
        val = (val << 6) + kDecTable[c];
        bits += 6;
        if (bits >= 0) {
            out.push_back(static_cast<uint8_t>((val >> bits) & 0xFF));
            bits -= 8;
        }
    }
    return out;
}

struct GpuSubmeshTexture {
    int imageIndex = -1;
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<uint8_t> rgba;
    ID3D11Texture2D* pTex = nullptr;
    ID3D11ShaderResourceView* pSRV = nullptr;
};

struct PreviewSubmesh {
    uint32_t indexStart = 0;
    uint32_t indexCount = 0;
    int textureSlot = -1;
};

} // namespace detail

class Preview3DEngine {
public:
    // Mesh metadata
    uint32_t vertexCount = 0;
    uint32_t triangleCount = 0;
    uint32_t subMeshCount = 0;
    float minBounds[3] = { 0.0f, 0.0f, 0.0f };
    float maxBounds[3] = { 0.0f, 0.0f, 0.0f };
    float boundsSizeX = 0.0f;
    float boundsSizeY = 0.0f;
    float boundsSizeZ = 0.0f;
    bool hasTexture = false;
    bool modelLoaded = false;
    std::wstring loadedPath;
    std::wstring modelFormat;

    // Interactive camera state
    float yaw = 0.6f;
    float pitch = 0.35f;
    float zoom = 2.6f;
    float panX = 0.0f;
    float panY = 0.0f;
    bool autoRotate = true;
    bool flipTextureV = false;
    bool wireframeMode = false;

    // CPU-side decoded geometry and baseColor texture
    std::vector<PreviewVertex> cpuVertices;
    std::vector<uint32_t> cpuIndices;
    std::vector<uint8_t> cpuTextureRgba;
    uint32_t cpuTexWidth = 0;
    uint32_t cpuTexHeight = 0;
    std::vector<detail::GpuSubmeshTexture> loadedTextures;
    std::vector<detail::PreviewSubmesh> submeshes;

    // D3D11 & D2D1 Offscreen Pipeline Resources
    ID3D11Texture2D* pColorTex = nullptr;
    ID3D11RenderTargetView* pRTV = nullptr;
    ID3D11ShaderResourceView* pColorSRV = nullptr;
    ID3D11Texture2D* pDepthTex = nullptr;
    ID3D11DepthStencilView* pDSV = nullptr;
    ID2D1Bitmap1* pOutputBitmap = nullptr;

    ID3D11VertexShader* pVS = nullptr;
    ID3D11PixelShader* pPS = nullptr;
    ID3D11InputLayout* pInputLayout = nullptr;
    ID3D11Buffer* pConstantBuffer = nullptr;
    ID3D11Buffer* pVertexBuffer = nullptr;
    ID3D11Buffer* pIndexBuffer = nullptr;
    ID3D11Texture2D* pModelTex = nullptr;
    ID3D11ShaderResourceView* pModelTexSRV = nullptr;
    ID3D11SamplerState* pSampler = nullptr;
    ID3D11RasterizerState* pRasterSolid = nullptr;
    ID3D11RasterizerState* pRasterWireframe = nullptr;
    ID3D11DepthStencilState* pDepthState = nullptr;
    ID3D11BlendState* pBlendState = nullptr;

    uint32_t targetWidth = 0;
    uint32_t targetHeight = 0;
    bool gpuBuffersDirty = false;

    Preview3DEngine() = default;
    ~Preview3DEngine() {
        Cleanup();
    }

    Preview3DEngine(const Preview3DEngine&) = delete;
    Preview3DEngine& operator=(const Preview3DEngine&) = delete;

    void Cleanup() {
        ReleaseGpuMeshBuffers();
        SafeRelease(pOutputBitmap);
        SafeRelease(pDSV);
        SafeRelease(pDepthTex);
        SafeRelease(pColorSRV);
        SafeRelease(pRTV);
        SafeRelease(pColorTex);
        SafeRelease(pVS);
        SafeRelease(pPS);
        SafeRelease(pInputLayout);
        SafeRelease(pConstantBuffer);
        SafeRelease(pSampler);
        SafeRelease(pRasterSolid);
        SafeRelease(pRasterWireframe);
        SafeRelease(pDepthState);
        SafeRelease(pBlendState);
        targetWidth = 0;
        targetHeight = 0;
    }

    void ReleaseGpuMeshBuffers() {
        SafeRelease(pVertexBuffer);
        SafeRelease(pIndexBuffer);
        SafeRelease(pModelTexSRV);
        SafeRelease(pModelTex);
        for (auto& t : loadedTextures) {
            SafeRelease(t.pSRV);
            SafeRelease(t.pTex);
        }
        loadedTextures.clear();
        submeshes.clear();
    }

    void ResetCamera() {
        yaw = 0.6f;
        pitch = 0.35f;
        zoom = 2.6f;
        panX = 0.0f;
        panY = 0.0f;
        autoRotate = true;
    }

    bool IsWireframe() const noexcept { return wireframeMode; }
    void ToggleWireframe() noexcept { wireframeMode = !wireframeMode; }
    bool IsAutoRotate() const noexcept { return autoRotate; }
    void ToggleAutoRotate() noexcept { autoRotate = !autoRotate; }
    bool IsFlipTextureV() const noexcept { return flipTextureV; }
    void ToggleFlipTextureV() noexcept { flipTextureV = !flipTextureV; }
    uint32_t GetVertexCount() const noexcept { return vertexCount; }
    uint32_t GetTriangleCount() const noexcept { return triangleCount; }
    uint32_t GetSubMeshCount() const noexcept { return subMeshCount; }
    ID2D1Bitmap1* GetOutputBitmap() const noexcept { return pOutputBitmap; }

    void RotateCamera(float dx, float dy) noexcept {
        autoRotate = false;
        yaw += dx * 0.01f;
        pitch = std::clamp(pitch + dy * 0.01f, -1.50f, 1.50f);
    }

    void PanCamera(float dx, float dy) noexcept {
        autoRotate = false;
        panX += dx * 0.005f * (zoom * 0.4f);
        panY -= dy * 0.005f * (zoom * 0.4f);
    }

    void ZoomCamera(float factor) noexcept {
        zoom = std::clamp(zoom * (1.0f - factor), 0.4f, 15.0f);
    }

    void OnMouseDrag(float dx, float dy, bool rightButton = false) {
        autoRotate = false;
        if (rightButton) {
            panX += dx * 0.005f * (zoom * 0.4f);
            panY -= dy * 0.005f * (zoom * 0.4f);
        } else {
            yaw += dx * 0.01f;
            pitch = std::clamp(pitch + dy * 0.01f, -1.50f, 1.50f);
        }
    }

    void OnMouseWheel(float wheelDelta) {
        float factor = (wheelDelta > 0.0f) ? 0.88f : 1.14f;
        zoom = std::clamp(zoom * factor, 0.4f, 15.0f);
    }

    bool LoadModelFromFile(const std::wstring& path,
                           ID3D11Device* pDevice = nullptr,
                           IWICImagingFactory* pWICFactory = nullptr) {
        std::ifstream f(path, std::ios::binary | std::ios::ate);
        if (!f.is_open()) return false;
        std::streamsize sz = f.tellg();
        if (sz <= 0 || sz > 256 * 1024 * 1024) return false;
        f.seekg(0, std::ios::beg);
        std::vector<uint8_t> data(static_cast<size_t>(sz));
        if (!f.read(reinterpret_cast<char*>(data.data()), sz)) return false;

        loadedPath = path;
        return LoadModelFromMemory(data, path, pDevice, pWICFactory);
    }

    bool LoadModelFromMemory(std::span<const uint8_t> bytes,
                             const std::wstring& extOrPath,
                             ID3D11Device* pDevice = nullptr,
                             IWICImagingFactory* pWICFactory = nullptr) {
        cpuVertices.clear();
        cpuIndices.clear();
        cpuTextureRgba.clear();
        cpuTexWidth = 0;
        cpuTexHeight = 0;
        vertexCount = 0;
        triangleCount = 0;
        subMeshCount = 0;
        hasTexture = false;
        modelLoaded = false;
        ReleaseGpuMeshBuffers();

        if (bytes.size() < 4) return false;

        std::wstring lowerExt = extOrPath;
        for (auto& c : lowerExt) c = static_cast<wchar_t>(towlower(c));

        bool parsed = false;
        uint32_t magic = 0;
        std::memcpy(&magic, bytes.data(), sizeof(uint32_t));

        if (magic == 0x46546C67u) { // "glTF"
            modelFormat = L"glTF 2.0 Binary (.glb)";
            parsed = ParseGlb(bytes, pWICFactory);
        } else if (lowerExt.ends_with(L".gltf")) {
            modelFormat = L"glTF 2.0 (.gltf)";
            parsed = ParseGltfJson(bytes, extOrPath, pWICFactory);
        } else if (lowerExt.ends_with(L".obj")) {
            modelFormat = L"Wavefront OBJ (.obj)";
            parsed = ParseObj(bytes);
        } else if (lowerExt.ends_with(L".glb")) {
            modelFormat = L"glTF 2.0 Binary (.glb)";
            parsed = ParseGlb(bytes, pWICFactory);
        } else {
            // Auto-detect OBJ text vs JSON glTF
            std::string_view sv(reinterpret_cast<const char*>(bytes.data()),
                                (std::min)(bytes.size(), static_cast<size_t>(512)));
            if (sv.find("\"asset\"") != std::string_view::npos &&
                sv.find("\"meshes\"") != std::string_view::npos) {
                modelFormat = L"glTF 2.0 (.gltf)";
                parsed = ParseGltfJson(bytes, extOrPath, pWICFactory);
            } else {
                modelFormat = L"Wavefront OBJ (.obj)";
                parsed = ParseObj(bytes);
            }
        }

        if (!parsed || cpuVertices.empty() || cpuIndices.empty()) {
            return false;
        }

        NormalizeAndCenterMesh();
        ResetCamera();
        modelLoaded = true;
        gpuBuffersDirty = true;

        if (pDevice) {
            UploadMeshToGpu(pDevice);
        }
        return true;
    }

    ID2D1Bitmap1* RenderFrame(ID3D11Device* pDevice,
                              ID3D11DeviceContext* pContext,
                              ID2D1DeviceContext* pD2DContext,
                              float dt = 0.016f,
                              uint32_t width = 512,
                              uint32_t height = 512) {
        if (!pDevice || !pContext || !pD2DContext || !modelLoaded) return nullptr;
        if (!EnsurePipelineStates(pDevice)) return nullptr;
        if (!EnsureRenderTarget(pDevice, pD2DContext, width, height)) return nullptr;
        if (gpuBuffersDirty || !pVertexBuffer || !pIndexBuffer) {
            if (!UploadMeshToGpu(pDevice)) return nullptr;
        }

        if (autoRotate) {
            yaw += dt * 0.65f;
            if (yaw > 6.2831853f) yaw -= 6.2831853f;
        }

        // Save D3D11 pipeline state so Direct2D rendering is never disrupted
        ID3D11RenderTargetView* prevRTV = nullptr;
        ID3D11DepthStencilView* prevDSV = nullptr;
        pContext->OMGetRenderTargets(1, &prevRTV, &prevDSV);

        UINT numViewports = 1;
        D3D11_VIEWPORT prevViewport{};
        pContext->RSGetViewports(&numViewports, &prevViewport);

        ID3D11RasterizerState* prevRS = nullptr;
        pContext->RSGetState(&prevRS);

        ID3D11DepthStencilState* prevDSS = nullptr;
        UINT prevStencilRef = 0;
        pContext->OMGetDepthStencilState(&prevDSS, &prevStencilRef);

        ID3D11BlendState* prevBS = nullptr;
        FLOAT prevBlendFactor[4] = {};
        UINT prevSampleMask = 0xFFFFFFFFu;
        pContext->OMGetBlendState(&prevBS, prevBlendFactor, &prevSampleMask);

        ID3D11InputLayout* prevLayout = nullptr;
        pContext->IAGetInputLayout(&prevLayout);

        D3D11_PRIMITIVE_TOPOLOGY prevTopo = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
        pContext->IAGetPrimitiveTopology(&prevTopo);

        ID3D11VertexShader* prevVS = nullptr;
        ID3D11PixelShader* prevPS = nullptr;
        pContext->VSGetShader(&prevVS, nullptr, nullptr);
        pContext->PSGetShader(&prevPS, nullptr, nullptr);

        // Bind offscreen render target & clear
        const float clearColor[4] = { 0.06f, 0.07f, 0.10f, 1.0f };
        pContext->OMSetRenderTargets(1, &pRTV, pDSV);
        pContext->ClearRenderTargetView(pRTV, clearColor);
        pContext->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        D3D11_VIEWPORT vp{};
        vp.TopLeftX = 0.0f;
        vp.TopLeftY = 0.0f;
        vp.Width = static_cast<float>(targetWidth);
        vp.Height = static_cast<float>(targetHeight);
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        pContext->RSSetViewports(1, &vp);

        // Compute world / view / projection matrices
        detail::Mat4 rotY = detail::Mat4::RotationY(yaw);
        detail::Mat4 rotX = detail::Mat4::RotationX(pitch);
        detail::Mat4 world = detail::Mat4::Multiply(rotY, rotX);

        detail::Mat4 view = detail::Mat4::LookAtLH(
            -panX, -panY, -zoom,
            -panX, -panY, 0.0f,
            0.0f, 1.0f, 0.0f);

        float aspect = (targetHeight > 0)
            ? (static_cast<float>(targetWidth) / static_cast<float>(targetHeight))
            : 1.0f;
        detail::Mat4 proj = detail::Mat4::PerspectiveFovLH(0.78539816f, aspect, 0.05f, 100.0f);
        detail::Mat4 wvp = detail::Mat4::Multiply(detail::Mat4::Multiply(world, view), proj);

        struct alignas(16) ShaderCB {
            float wvp[16];
            float world[16];
            float cameraPos[4];
            uint32_t hasTex;
            uint32_t isWireframe;
            uint32_t flipV;
            float pad0;
        };

        ShaderCB cb{};
        std::memcpy(cb.wvp, wvp.m, sizeof(cb.wvp));
        std::memcpy(cb.world, world.m, sizeof(cb.world));
        cb.cameraPos[0] = -panX;
        cb.cameraPos[1] = -panY;
        cb.cameraPos[2] = -zoom;
        cb.cameraPos[3] = 1.0f;
        cb.hasTex = (hasTexture && pModelTexSRV != nullptr) ? 1u : 0u;
        cb.isWireframe = wireframeMode ? 1u : 0u;
        cb.flipV = flipTextureV ? 1u : 0u;

        pContext->UpdateSubresource(pConstantBuffer, 0, nullptr, &cb, 0, 0);

        UINT stride = sizeof(PreviewVertex);
        UINT offset = 0;
        pContext->IASetInputLayout(pInputLayout);
        pContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &stride, &offset);
        pContext->IASetIndexBuffer(pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
        pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        pContext->VSSetShader(pVS, nullptr, 0);
        pContext->VSSetConstantBuffers(0, 1, &pConstantBuffer);
        pContext->PSSetShader(pPS, nullptr, 0);
        pContext->PSSetConstantBuffers(0, 1, &pConstantBuffer);
        pContext->PSSetSamplers(0, 1, &pSampler);

        pContext->RSSetState(wireframeMode ? pRasterWireframe : pRasterSolid);
        pContext->OMSetDepthStencilState(pDepthState, 0);
        const float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        pContext->OMSetBlendState(pBlendState, blendFactor, 0xFFFFFFFFu);

        if (!submeshes.empty()) {
            for (const auto& sm : submeshes) {
                if (sm.indexCount == 0) continue;
                ID3D11ShaderResourceView* srv = nullptr;
                if (sm.textureSlot >= 0 && sm.textureSlot < static_cast<int>(loadedTextures.size())) {
                    srv = loadedTextures[sm.textureSlot].pSRV;
                }
                cb.hasTex = (srv != nullptr) ? 1u : 0u;
                pContext->UpdateSubresource(pConstantBuffer, 0, nullptr, &cb, 0, 0);
                pContext->PSSetShaderResources(0, 1, &srv);
                pContext->DrawIndexed(sm.indexCount, sm.indexStart, 0);
            }
        } else {
            cb.hasTex = (hasTexture && pModelTexSRV != nullptr) ? 1u : 0u;
            pContext->UpdateSubresource(pConstantBuffer, 0, nullptr, &cb, 0, 0);
            pContext->PSSetShaderResources(0, 1, &pModelTexSRV);
            pContext->DrawIndexed(static_cast<UINT>(cpuIndices.size()), 0, 0);
        }

        // Unbind SRV to avoid D3D11 hazard warnings
        ID3D11ShaderResourceView* nullSRV = nullptr;
        pContext->PSSetShaderResources(0, 1, &nullSRV);

        // Restore saved D3D11 state
        pContext->OMSetRenderTargets(1, &prevRTV, prevDSV);
        if (numViewports > 0) pContext->RSSetViewports(1, &prevViewport);
        pContext->RSSetState(prevRS);
        pContext->OMSetDepthStencilState(prevDSS, prevStencilRef);
        pContext->OMSetBlendState(prevBS, prevBlendFactor, prevSampleMask);
        pContext->IASetInputLayout(prevLayout);
        pContext->IASetPrimitiveTopology(prevTopo);
        pContext->VSSetShader(prevVS, nullptr, 0);
        pContext->PSSetShader(prevPS, nullptr, 0);

        SafeRelease(prevRTV);
        SafeRelease(prevDSV);
        SafeRelease(prevRS);
        SafeRelease(prevDSS);
        SafeRelease(prevBS);
        SafeRelease(prevLayout);
        SafeRelease(prevVS);
        SafeRelease(prevPS);

        return pOutputBitmap;
    }

private:
    void NormalizeAndCenterMesh() {
        if (cpuVertices.empty()) return;

        minBounds[0] = minBounds[1] = minBounds[2] = 1e30f;
        maxBounds[0] = maxBounds[1] = maxBounds[2] = -1e30f;

        for (const auto& v : cpuVertices) {
            minBounds[0] = (std::min)(minBounds[0], v.px);
            minBounds[1] = (std::min)(minBounds[1], v.py);
            minBounds[2] = (std::min)(minBounds[2], v.pz);
            maxBounds[0] = (std::max)(maxBounds[0], v.px);
            maxBounds[1] = (std::max)(maxBounds[1], v.py);
            maxBounds[2] = (std::max)(maxBounds[2], v.pz);
        }

        boundsSizeX = maxBounds[0] - minBounds[0];
        boundsSizeY = maxBounds[1] - minBounds[1];
        boundsSizeZ = maxBounds[2] - minBounds[2];

        float cx = (minBounds[0] + maxBounds[0]) * 0.5f;
        float cy = (minBounds[1] + maxBounds[1]) * 0.5f;
        float cz = (minBounds[2] + maxBounds[2]) * 0.5f;

        float maxExtent = (std::max)({ boundsSizeX, boundsSizeY, boundsSizeZ, 1e-5f });
        float scale = 1.8f / maxExtent;

        for (auto& v : cpuVertices) {
            v.px = (v.px - cx) * scale;
            v.py = (v.py - cy) * scale;
            v.pz = (v.pz - cz) * scale;
        }

        vertexCount = static_cast<uint32_t>(cpuVertices.size());
        triangleCount = static_cast<uint32_t>(cpuIndices.size() / 3);
        if (subMeshCount == 0) subMeshCount = 1;
    }

    static void RecomputeMissingNormals(std::vector<PreviewVertex>& verts,
                                        const std::vector<uint32_t>& indices,
                                        size_t startVertex,
                                        size_t startIndex) {
        for (size_t i = startIndex; i + 2 < indices.size(); i += 3) {
            uint32_t i0 = indices[i];
            uint32_t i1 = indices[i + 1];
            uint32_t i2 = indices[i + 2];
            if (i0 >= verts.size() || i1 >= verts.size() || i2 >= verts.size()) continue;

            float ax = verts[i1].px - verts[i0].px;
            float ay = verts[i1].py - verts[i0].py;
            float az = verts[i1].pz - verts[i0].pz;

            float bx = verts[i2].px - verts[i0].px;
            float by = verts[i2].py - verts[i0].py;
            float bz = verts[i2].pz - verts[i0].pz;

            float nx = ay * bz - az * by;
            float ny = az * bx - ax * bz;
            float nz = ax * by - ay * bx;

            verts[i0].nx += nx; verts[i0].ny += ny; verts[i0].nz += nz;
            verts[i1].nx += nx; verts[i1].ny += ny; verts[i1].nz += nz;
            verts[i2].nx += nx; verts[i2].ny += ny; verts[i2].nz += nz;
        }

        for (size_t v = startVertex; v < verts.size(); ++v) {
            float len = std::sqrt(verts[v].nx * verts[v].nx +
                                  verts[v].ny * verts[v].ny +
                                  verts[v].nz * verts[v].nz);
            if (len > 1e-6f) {
                verts[v].nx /= len;
                verts[v].ny /= len;
                verts[v].nz /= len;
            } else {
                verts[v].nx = 0.0f;
                verts[v].ny = 1.0f;
                verts[v].nz = 0.0f;
            }
        }
    }

    bool ParseGlb(std::span<const uint8_t> bytes, IWICImagingFactory* pWICFactory) {
        if (bytes.size() < 20) return false;
        uint32_t magic = 0, version = 0, totalLen = 0;
        std::memcpy(&magic, bytes.data() + 0, 4);
        std::memcpy(&version, bytes.data() + 4, 4);
        std::memcpy(&totalLen, bytes.data() + 8, 4);
        if (magic != 0x46546C67u || version != 2) return false;

        std::string_view jsonText;
        std::span<const uint8_t> binChunk;

        size_t offset = 12;
        while (offset + 8 <= bytes.size() && offset < totalLen) {
            uint32_t chunkLen = 0, chunkType = 0;
            std::memcpy(&chunkLen, bytes.data() + offset, 4);
            std::memcpy(&chunkType, bytes.data() + offset + 4, 4);
            offset += 8;
            if (offset + chunkLen > bytes.size()) break;

            if (chunkType == 0x4E4F534Au && jsonText.empty()) { // JSON
                jsonText = std::string_view(reinterpret_cast<const char*>(bytes.data() + offset), chunkLen);
            } else if (chunkType == 0x004E4942u && binChunk.empty()) { // BIN
                binChunk = bytes.subspan(offset, chunkLen);
            }
            offset += chunkLen;
        }

        if (jsonText.empty()) return false;
        detail::GlbJsonValue root = detail::GlbJsonParser::Parse(jsonText);
        return ExtractGltfGeometry(root, binChunk, L"", pWICFactory);
    }

    bool ParseGltfJson(std::span<const uint8_t> bytes,
                       const std::wstring& gltfFilePath,
                       IWICImagingFactory* pWICFactory) {
        std::string_view jsonText(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        detail::GlbJsonValue root = detail::GlbJsonParser::Parse(jsonText);
        if (root.type != detail::GlbJsonValue::Type::Object) return false;

        std::vector<uint8_t> externalBin;
        std::wstring baseDir;
        size_t slash = gltfFilePath.find_last_of(L"\\/");
        if (slash != std::wstring::npos) {
            baseDir = gltfFilePath.substr(0, slash + 1);
        }

        const auto* buffers = root.Get("buffers");
        if (buffers && buffers->Size() > 0) {
            const auto* buf0 = buffers->At(0);
            if (buf0) {
                std::string_view uri = buf0->GetString("uri");
                if (uri.starts_with("data:")) {
                    externalBin = detail::DecodeBase64DataUri(uri);
                } else if (!uri.empty() && !baseDir.empty()) {
                    std::wstring binPath = baseDir + Utf8ToWide(uri);
                    std::ifstream bf(binPath, std::ios::binary | std::ios::ate);
                    if (bf.is_open()) {
                        auto bsz = bf.tellg();
                        if (bsz > 0 && bsz <= 256 * 1024 * 1024) {
                            externalBin.resize(static_cast<size_t>(bsz));
                            bf.seekg(0, std::ios::beg);
                            bf.read(reinterpret_cast<char*>(externalBin.data()), bsz);
                        }
                    }
                }
            }
        }

        return ExtractGltfGeometry(root, externalBin, baseDir, pWICFactory);
    }

    bool ExtractGltfGeometry(const detail::GlbJsonValue& root,
                             std::span<const uint8_t> binData,
                             const std::wstring& baseDir,
                             IWICImagingFactory* pWICFactory) {
        const auto* accessors = root.Get("accessors");
        const auto* bufferViews = root.Get("bufferViews");
        const auto* meshes = root.Get("meshes");
        const auto* nodes = root.Get("nodes");
        const auto* materials = root.Get("materials");
        const auto* textures = root.Get("textures");
        const auto* images = root.Get("images");

        if (!meshes || meshes->Size() == 0 || !accessors || !bufferViews) return false;

        auto getAccessorSlice = [&](int accIdx,
                                    const uint8_t*& outPtr,
                                    size_t& outCount,
                                    size_t& outStride,
                                    int& outCompType,
                                    std::string_view& outType,
                                    bool& outNormalized) -> bool {
            if (accIdx < 0 || static_cast<size_t>(accIdx) >= accessors->Size()) return false;
            const auto* acc = accessors->At(static_cast<size_t>(accIdx));
            if (!acc) return false;

            int bvIdx = acc->GetInt("bufferView", -1);
            if (bvIdx < 0 || static_cast<size_t>(bvIdx) >= bufferViews->Size()) return false;
            const auto* bv = bufferViews->At(static_cast<size_t>(bvIdx));
            if (!bv) return false;

            size_t bvOffset = bv->GetUInt("byteOffset", 0);
            size_t bvLen = bv->GetUInt("byteLength", 0);
            size_t bvStride = bv->GetUInt("byteStride", 0);
            size_t accOffset = acc->GetUInt("byteOffset", 0);
            outCount = acc->GetUInt("count", 0);
            outCompType = acc->GetInt("componentType", 0);
            outType = acc->GetString("type", "SCALAR");
            outNormalized = acc->Get("normalized") ? acc->Get("normalized")->AsBool(false) : false;

            size_t compBytes = 1;
            switch (outCompType) {
                case 5120: case 5121: compBytes = 1; break;
                case 5122: case 5123: compBytes = 2; break;
                case 5125: case 5126: compBytes = 4; break;
                default: return false;
            }
            size_t numComps = 1;
            if (outType == "VEC2") numComps = 2;
            else if (outType == "VEC3") numComps = 3;
            else if (outType == "VEC4") numComps = 4;
            else if (outType == "MAT4") numComps = 16;

            size_t elemBytes = compBytes * numComps;
            outStride = (bvStride > 0) ? bvStride : elemBytes;

            if (outCount == 0) return false;
            size_t startByte = bvOffset + accOffset;
            size_t requiredBytes = (outCount - 1) * outStride + elemBytes;
            if (startByte + requiredBytes > binData.size() || accOffset + requiredBytes > bvLen + elemBytes) {
                return false;
            }
            outPtr = binData.data() + startByte;
            return true;
        };

        auto readFloatComponent = [](const uint8_t* ptr, int compType, bool normalized) -> float {
            switch (compType) {
                case 5126: {
                    float f = 0.0f;
                    std::memcpy(&f, ptr, sizeof(float));
                    return f;
                }
                case 5121: {
                    uint8_t v = *ptr;
                    return normalized ? (static_cast<float>(v) / 255.0f) : static_cast<float>(v);
                }
                case 5120: {
                    int8_t v = static_cast<int8_t>(*ptr);
                    return normalized ? (std::max)(-1.0f, static_cast<float>(v) / 127.0f) : static_cast<float>(v);
                }
                case 5123: {
                    uint16_t v = 0;
                    std::memcpy(&v, ptr, sizeof(uint16_t));
                    return normalized ? (static_cast<float>(v) / 65535.0f) : static_cast<float>(v);
                }
                case 5122: {
                    int16_t v = 0;
                    std::memcpy(&v, ptr, sizeof(int16_t));
                    return normalized ? (std::max)(-1.0f, static_cast<float>(v) / 32767.0f) : static_cast<float>(v);
                }
                case 5125: {
                    uint32_t v = 0;
                    std::memcpy(&v, ptr, sizeof(uint32_t));
                    return static_cast<float>(v);
                }
                default:
                    return 0.0f;
            }
        };

        auto compByteSize = [](int compType) -> size_t {
            switch (compType) {
                case 5120: case 5121: return 1;
                case 5122: case 5123: return 2;
                case 5125: case 5126: return 4;
                default: return 1;
            }
        };

        // Multi-texture loader supporting isolated texture maps per submesh
        std::unordered_map<int, int> imgIndexToSlot;

        auto getOrLoadTexture = [&](int imgIdx) -> int {
            if (imgIdx < 0 || !images || static_cast<size_t>(imgIdx) >= images->Size()) return -1;
            auto it = imgIndexToSlot.find(imgIdx);
            if (it != imgIndexToSlot.end()) return it->second;

            detail::GpuSubmeshTexture newTex;
            newTex.imageIndex = imgIdx;
            bool ok = false;
            const auto* img = images->At(static_cast<size_t>(imgIdx));
            if (img) {
                int bvIdx = img->GetInt("bufferView", -1);
                if (bvIdx >= 0 && bufferViews && static_cast<size_t>(bvIdx) < bufferViews->Size()) {
                    const auto* bv = bufferViews->At(static_cast<size_t>(bvIdx));
                    size_t bvOff = bv->GetUInt("byteOffset", 0);
                    size_t bvLen = bv->GetUInt("byteLength", 0);
                    if (bvOff + bvLen <= binData.size() && bvLen > 0) {
                        ok = detail::DecodeImageBytesWithWIC(pWICFactory, binData.data() + bvOff, bvLen,
                                                             newTex.rgba, newTex.width, newTex.height);
                    }
                } else {
                    std::string_view uri = img->GetString("uri");
                    if (uri.starts_with("data:")) {
                        auto decoded = detail::DecodeBase64DataUri(uri);
                        if (!decoded.empty()) {
                            ok = detail::DecodeImageBytesWithWIC(pWICFactory, decoded.data(), decoded.size(),
                                                                 newTex.rgba, newTex.width, newTex.height);
                        }
                    } else if (!uri.empty() && !baseDir.empty()) {
                        std::wstring imgPath = baseDir + Utf8ToWide(uri);
                        std::ifstream imgF(imgPath, std::ios::binary | std::ios::ate);
                        if (imgF.is_open()) {
                            auto isz = imgF.tellg();
                            if (isz > 0 && isz <= 64 * 1024 * 1024) {
                                std::vector<uint8_t> imgBuf(static_cast<size_t>(isz));
                                imgF.seekg(0, std::ios::beg);
                                imgF.read(reinterpret_cast<char*>(imgBuf.data()), isz);
                                ok = detail::DecodeImageBytesWithWIC(pWICFactory, imgBuf.data(), imgBuf.size(),
                                                                     newTex.rgba, newTex.width, newTex.height);
                            }
                        }
                    }
                }
            }

            if (ok) {
                int slot = static_cast<int>(loadedTextures.size());
                loadedTextures.push_back(std::move(newTex));
                imgIndexToSlot[imgIdx] = slot;
                hasTexture = true;
                if (cpuTextureRgba.empty()) {
                    cpuTextureRgba = loadedTextures.back().rgba;
                    cpuTexWidth = loadedTextures.back().width;
                    cpuTexHeight = loadedTextures.back().height;
                }
                return slot;
            } else {
                imgIndexToSlot[imgIdx] = -1;
                return -1;
            }
        };

        auto appendMeshInstance = [&](int meshIdx, const detail::Mat4& worldTransform) {
            if (meshIdx < 0 || static_cast<size_t>(meshIdx) >= meshes->Size()) return;
            const auto* meshObj = meshes->At(static_cast<size_t>(meshIdx));
            const auto* prims = meshObj ? meshObj->Get("primitives") : nullptr;
            if (!prims) return;

            for (size_t p = 0; p < prims->Size(); ++p) {
                const auto* prim = prims->At(p);
                if (!prim) continue;
                int mode = prim->GetInt("mode", 4);
                if (mode != 4 && mode != 5 && mode != 6) continue;

                const auto* attrs = prim->Get("attributes");
                if (!attrs) continue;

                int posAcc = attrs->GetInt("POSITION", -1);
                int normAcc = attrs->GetInt("NORMAL", -1);
                int uvAcc = attrs->GetInt("TEXCOORD_0", -1);
                if (uvAcc < 0) uvAcc = attrs->GetInt("TEXCOORD", -1);
                int colAcc = attrs->GetInt("COLOR_0", -1);
                int idxAcc = prim->GetInt("indices", -1);

                float matColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
                bool hasExplicitBcf = false;
                int primTexSource = -1;
                float texScaleU = 1.0f, texScaleV = 1.0f, texOffU = 0.0f, texOffV = 0.0f;
                int matIdx = prim->GetInt("material", -1);
                if (materials && matIdx >= 0 && static_cast<size_t>(matIdx) < materials->Size()) {
                    const auto* mat = materials->At(static_cast<size_t>(matIdx));
                    const auto* pbr = mat ? mat->Get("pbrMetallicRoughness") : nullptr;
                    const auto* bcf = pbr ? pbr->Get("baseColorFactor") : nullptr;
                    if (bcf && bcf->Size() == 4) {
                        for (int c = 0; c < 4; ++c) matColor[c] = bcf->At(c)->AsFloat(1.0f);
                        hasExplicitBcf = true;
                    }
                    const auto* bct = pbr ? pbr->Get("baseColorTexture") : nullptr;
                    if (!bct) {
                        const auto* exts = mat ? mat->Get("extensions") : nullptr;
                        const auto* specGloss = exts ? exts->Get("KHR_materials_pbrSpecularGlossiness") : nullptr;
                        bct = specGloss ? specGloss->Get("diffuseTexture") : nullptr;
                    }
                    if (!bct && mat) {
                        bct = mat->Get("baseColorTexture");
                        if (!bct) bct = mat->Get("normalTexture");
                        if (!bct) bct = mat->Get("emissiveTexture");
                    }
                    if (bct && textures) {
                        int texIdx = bct->GetInt("index", -1);
                        if (texIdx >= 0 && static_cast<size_t>(texIdx) < textures->Size()) {
                            primTexSource = textures->At(static_cast<size_t>(texIdx))->GetInt("source", -1);
                            if (primTexSource < 0) primTexSource = texIdx;
                        }
                        int reqCoord = bct->GetInt("texCoord", 0);
                        if (reqCoord == 1) {
                            int altUv = attrs->GetInt("TEXCOORD_1", -1);
                            if (altUv >= 0) uvAcc = altUv;
                        }
                        const auto* exts = bct->Get("extensions");
                        const auto* khrTexTransform = exts ? exts->Get("KHR_texture_transform") : nullptr;
                        if (khrTexTransform) {
                            int khrCoord = khrTexTransform->GetInt("texCoord", -1);
                            if (khrCoord >= 0) {
                                std::string coordKey = "TEXCOORD_" + std::to_string(khrCoord);
                                int altUv = attrs->GetInt(coordKey, -1);
                                if (altUv >= 0) uvAcc = altUv;
                            }
                            const auto* sc = khrTexTransform->Get("scale");
                            if (sc && sc->Size() >= 2) {
                                texScaleU = sc->At(0)->AsFloat(1.0f);
                                texScaleV = sc->At(1)->AsFloat(1.0f);
                            }
                            const auto* off = khrTexTransform->Get("offset");
                            if (off && off->Size() >= 2) {
                                texOffU = off->At(0)->AsFloat(0.0f);
                                texOffV = off->At(1)->AsFloat(0.0f);
                            }
                        }
                    }
                }

                int texSlot = (primTexSource >= 0) ? getOrLoadTexture(primTexSource) : -1;
                bool primHasTexture = (texSlot >= 0);

                if (primHasTexture && !hasExplicitBcf) {
                    matColor[0] = 1.0f; matColor[1] = 1.0f; matColor[2] = 1.0f; matColor[3] = 1.0f;
                } else if (!primHasTexture && !hasExplicitBcf && colAcc < 0) {
                    matColor[0] = 0.92f; matColor[1] = 0.92f; matColor[2] = 0.92f; matColor[3] = 1.0f;
                }

                const uint8_t* posPtr = nullptr;
                size_t posCount = 0, posStride = 0;
                int posComp = 0;
                std::string_view posType;
                bool posNorm = false;
                if (!getAccessorSlice(posAcc, posPtr, posCount, posStride, posComp, posType, posNorm) ||
                    posType != "VEC3") {
                    continue;
                }

                const uint8_t* normPtr = nullptr;
                size_t normCount = 0, normStride = 0;
                int normComp = 0;
                std::string_view normType;
                bool normNorm = false;
                bool hasNormals = getAccessorSlice(normAcc, normPtr, normCount, normStride, normComp, normType, normNorm) &&
                                  normType == "VEC3" && normCount == posCount;

                const uint8_t* uvPtr = nullptr;
                size_t uvCount = 0, uvStride = 0;
                int uvComp = 0;
                std::string_view uvType;
                bool uvNorm = false;
                bool hasUVs = getAccessorSlice(uvAcc, uvPtr, uvCount, uvStride, uvComp, uvType, uvNorm) &&
                              uvType == "VEC2" && uvCount == posCount;

                const uint8_t* colPtr = nullptr;
                size_t colCount = 0, colStride = 0;
                int colComp = 0;
                std::string_view colType;
                bool colNorm = false;
                bool hasCols = getAccessorSlice(colAcc, colPtr, colCount, colStride, colComp, colType, colNorm) &&
                               (colType == "VEC3" || colType == "VEC4") && colCount == posCount;

                size_t baseVertex = cpuVertices.size();
                size_t baseIndex = cpuIndices.size();
                cpuVertices.resize(baseVertex + posCount);

                size_t posElemSz = compByteSize(posComp);
                size_t normElemSz = compByteSize(normComp);
                size_t uvElemSz = compByteSize(uvComp);
                size_t colElemSz = compByteSize(colComp);

                for (size_t v = 0; v < posCount; ++v) {
                    PreviewVertex vert{};
                    const uint8_t* pv = posPtr + v * posStride;
                    float lx = readFloatComponent(pv + 0 * posElemSz, posComp, posNorm);
                    float ly = readFloatComponent(pv + 1 * posElemSz, posComp, posNorm);
                    float lz = readFloatComponent(pv + 2 * posElemSz, posComp, posNorm);
                    worldTransform.TransformPoint(lx, ly, lz, vert.px, vert.py, vert.pz);

                    if (hasNormals) {
                        const uint8_t* nv = normPtr + v * normStride;
                        float nx = readFloatComponent(nv + 0 * normElemSz, normComp, normNorm);
                        float ny = readFloatComponent(nv + 1 * normElemSz, normComp, normNorm);
                        float nz = readFloatComponent(nv + 2 * normElemSz, normComp, normNorm);
                        worldTransform.TransformNormal(nx, ny, nz, vert.nx, vert.ny, vert.nz);
                    }

                    if (hasUVs && primHasTexture) {
                        const uint8_t* uv = uvPtr + v * uvStride;
                        float rawU = readFloatComponent(uv + 0 * uvElemSz, uvComp, uvNorm);
                        float rawV = readFloatComponent(uv + 1 * uvElemSz, uvComp, uvNorm);
                        vert.u = rawU * texScaleU + texOffU;
                        vert.v = rawV * texScaleV + texOffV;
                    } else {
                        vert.u = -999.0f;
                        vert.v = -999.0f;
                    }

                    vert.r = matColor[0];
                    vert.g = matColor[1];
                    vert.b = matColor[2];
                    vert.a = matColor[3];

                    if (hasCols) {
                        const uint8_t* cv = colPtr + v * colStride;
                        bool normCol = (colComp != 5126);
                        vert.r *= readFloatComponent(cv + 0 * colElemSz, colComp, normCol);
                        vert.g *= readFloatComponent(cv + 1 * colElemSz, colComp, normCol);
                        vert.b *= readFloatComponent(cv + 2 * colElemSz, colComp, normCol);
                        if (colType == "VEC4") {
                            vert.a *= readFloatComponent(cv + 3 * colElemSz, colComp, normCol);
                        }
                    }

                    cpuVertices[baseVertex + v] = vert;
                }

                std::vector<uint32_t> rawIndices;
                if (idxAcc >= 0) {
                    const uint8_t* idxPtr = nullptr;
                    size_t idxCount = 0, idxStride = 0;
                    int idxComp = 0;
                    std::string_view idxType;
                    bool idxNorm = false;
                    if (getAccessorSlice(idxAcc, idxPtr, idxCount, idxStride, idxComp, idxType, idxNorm)) {
                        rawIndices.reserve(idxCount);
                        for (size_t i = 0; i < idxCount; ++i) {
                            const uint8_t* ip = idxPtr + i * idxStride;
                            uint32_t idxVal = 0;
                            if (idxComp == 5121) {
                                idxVal = *ip;
                            } else if (idxComp == 5123) {
                                uint16_t u16 = 0;
                                std::memcpy(&u16, ip, 2);
                                idxVal = u16;
                            } else if (idxComp == 5125) {
                                std::memcpy(&idxVal, ip, 4);
                            }
                            if (idxVal < posCount) {
                                rawIndices.push_back(static_cast<uint32_t>(baseVertex) + idxVal);
                            }
                        }
                    }
                } else {
                    rawIndices.resize(posCount);
                    for (size_t i = 0; i < posCount; ++i) {
                        rawIndices[i] = static_cast<uint32_t>(baseVertex + i);
                    }
                }

                if (mode == 4) { // TRIANGLES
                    for (size_t i = 0; i + 2 < rawIndices.size(); i += 3) {
                        cpuIndices.push_back(rawIndices[i]);
                        cpuIndices.push_back(rawIndices[i + 1]);
                        cpuIndices.push_back(rawIndices[i + 2]);
                    }
                } else if (mode == 5) { // TRIANGLE_STRIP
                    for (size_t i = 0; i + 2 < rawIndices.size(); ++i) {
                        if (i & 1) {
                            cpuIndices.push_back(rawIndices[i + 1]);
                            cpuIndices.push_back(rawIndices[i]);
                            cpuIndices.push_back(rawIndices[i + 2]);
                        } else {
                            cpuIndices.push_back(rawIndices[i]);
                            cpuIndices.push_back(rawIndices[i + 1]);
                            cpuIndices.push_back(rawIndices[i + 2]);
                        }
                    }
                } else if (mode == 6) { // TRIANGLE_FAN
                    for (size_t i = 1; i + 1 < rawIndices.size(); ++i) {
                        cpuIndices.push_back(rawIndices[0]);
                        cpuIndices.push_back(rawIndices[i]);
                        cpuIndices.push_back(rawIndices[i + 1]);
                    }
                }

                if (!hasNormals) {
                    RecomputeMissingNormals(cpuVertices, cpuIndices, baseVertex, baseIndex);
                }
                size_t primIdxCount = cpuIndices.size() - baseIndex;
                if (primIdxCount > 0) {
                    detail::PreviewSubmesh sm;
                    sm.indexStart = static_cast<uint32_t>(baseIndex);
                    sm.indexCount = static_cast<uint32_t>(primIdxCount);
                    sm.textureSlot = primHasTexture ? texSlot : -1;
                    submeshes.push_back(sm);
                }
                ++subMeshCount;
            }
        };

        auto getNodeLocalMatrix = [&](const detail::GlbJsonValue* node) -> detail::Mat4 {
            if (!node) return detail::Mat4::Identity();
            const auto* matArr = node->Get("matrix");
            if (matArr && matArr->Size() == 16) {
                detail::Mat4 m{};
                // glTF 2.0 matrix is column-major: mat[col * 4 + row]
                // Convert to row-major for DirectX: m.m[row * 4 + col]
                for (int c = 0; c < 4; ++c) {
                    for (int r = 0; r < 4; ++r) {
                        m.m[r * 4 + c] = matArr->At(static_cast<size_t>(r * 4 + c))->AsFloat(0.0f);
                    }
                }
                return m;
            }
            float tx = 0.0f, ty = 0.0f, tz = 0.0f;
            float qx = 0.0f, qy = 0.0f, qz = 0.0f, qw = 1.0f;
            float sx = 1.0f, sy = 1.0f, sz = 1.0f;

            const auto* tr = node->Get("translation");
            if (tr && tr->Size() == 3) {
                tx = tr->At(0)->AsFloat(0.0f);
                ty = tr->At(1)->AsFloat(0.0f);
                tz = tr->At(2)->AsFloat(0.0f);
            }
            const auto* rot = node->Get("rotation");
            if (rot && rot->Size() == 4) {
                qx = rot->At(0)->AsFloat(0.0f);
                qy = rot->At(1)->AsFloat(0.0f);
                qz = rot->At(2)->AsFloat(0.0f);
                qw = rot->At(3)->AsFloat(1.0f);
            }
            const auto* sc = node->Get("scale");
            if (sc && sc->Size() == 3) {
                sx = sc->At(0)->AsFloat(1.0f);
                sy = sc->At(1)->AsFloat(1.0f);
                sz = sc->At(2)->AsFloat(1.0f);
            }
            detail::Mat4 mScale = detail::Mat4::Scale(sx, sy, sz);
            detail::Mat4 mRot = detail::Mat4::FromQuaternion(qx, qy, qz, qw);
            detail::Mat4 mTrans = detail::Mat4::Translation(tx, ty, tz);
            return detail::Mat4::Multiply(detail::Mat4::Multiply(mScale, mRot), mTrans);
        };

        if (nodes && nodes->Size() > 0) {
            std::vector<bool> isChild(nodes->Size(), false);
            for (size_t i = 0; i < nodes->Size(); ++i) {
                const auto* ch = nodes->At(i)->Get("children");
                if (ch) {
                    for (size_t c = 0; c < ch->Size(); ++c) {
                        int childIdx = ch->At(c)->AsInt(-1);
                        if (childIdx >= 0 && static_cast<size_t>(childIdx) < nodes->Size()) {
                            isChild[static_cast<size_t>(childIdx)] = true;
                        }
                    }
                }
            }

            auto traverseNode = [&](auto& self, int nodeIdx, const detail::Mat4& parentMat, int depth) -> void {
                if (depth > 32 || nodeIdx < 0 || static_cast<size_t>(nodeIdx) >= nodes->Size()) return;
                const auto* node = nodes->At(static_cast<size_t>(nodeIdx));
                detail::Mat4 worldMat = detail::Mat4::Multiply(getNodeLocalMatrix(node), parentMat);
                int meshIdx = node->GetInt("mesh", -1);
                if (meshIdx >= 0) {
                    appendMeshInstance(meshIdx, worldMat);
                }
                const auto* ch = node->Get("children");
                if (ch) {
                    for (size_t c = 0; c < ch->Size(); ++c) {
                        self(self, ch->At(c)->AsInt(-1), worldMat, depth + 1);
                    }
                }
            };

            for (size_t i = 0; i < nodes->Size(); ++i) {
                if (!isChild[i]) {
                    traverseNode(traverseNode, static_cast<int>(i), detail::Mat4::Identity(), 0);
                }
            }
        }

        if (cpuVertices.empty()) {
            for (size_t m = 0; m < meshes->Size(); ++m) {
                appendMeshInstance(static_cast<int>(m), detail::Mat4::Identity());
            }
        }

        return !cpuVertices.empty() && !cpuIndices.empty();
    }

    bool ParseObj(std::span<const uint8_t> bytes) {
        struct Vec3 { float x, y, z; };
        struct Vec2 { float u, v; };

        std::vector<Vec3> positions;
        std::vector<Vec3> normals;
        std::vector<Vec2> texcoords;

        positions.reserve(4096);
        normals.reserve(4096);
        texcoords.reserve(4096);

        std::string_view text(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        size_t pos = 0;
        bool anyMissingNormal = false;
        subMeshCount = 1;

        auto parseIndex = [](std::string_view tok, size_t count) -> int {
            if (tok.empty()) return -1;
            int idx = std::atoi(std::string(tok).c_str());
            if (idx > 0) return idx - 1;
            if (idx < 0 && static_cast<size_t>(-idx) <= count) {
                return static_cast<int>(count) + idx;
            }
            return -1;
        };

        while (pos < text.size()) {
            size_t lineEnd = text.find('\n', pos);
            if (lineEnd == std::string_view::npos) lineEnd = text.size();
            std::string_view line = text.substr(pos, lineEnd - pos);
            pos = lineEnd + 1;

            while (!line.empty() && (line.front() == ' ' || line.front() == '\t' || line.front() == '\r')) {
                line.remove_prefix(1);
            }
            while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\r')) {
                line.remove_suffix(1);
            }
            if (line.empty() || line.front() == '#') continue;

            if (line.starts_with("v ")) {
                const char* p = line.data() + 2;
                char* endP = nullptr;
                float x = std::strtof(p, &endP); p = endP;
                float y = std::strtof(p, &endP); p = endP;
                float z = std::strtof(p, &endP);
                positions.push_back({ x, y, z });
            } else if (line.starts_with("vn ")) {
                const char* p = line.data() + 3;
                char* endP = nullptr;
                float nx = std::strtof(p, &endP); p = endP;
                float ny = std::strtof(p, &endP); p = endP;
                float nz = std::strtof(p, &endP);
                normals.push_back({ nx, ny, nz });
            } else if (line.starts_with("vt ")) {
                const char* p = line.data() + 3;
                char* endP = nullptr;
                float u = std::strtof(p, &endP); p = endP;
                float v = std::strtof(p, &endP);
                texcoords.push_back({ u, 1.0f - v });
            } else if (line.starts_with("usemtl ") || line.starts_with("g ") || line.starts_with("o ")) {
                if (!cpuIndices.empty()) ++subMeshCount;
            } else if (line.starts_with("f ")) {
                std::string_view faceStr = line.substr(2);
                std::vector<uint32_t> faceVertIndices;
                faceVertIndices.reserve(8);

                size_t fpos = 0;
                while (fpos < faceStr.size()) {
                    while (fpos < faceStr.size() && (faceStr[fpos] == ' ' || faceStr[fpos] == '\t')) ++fpos;
                    if (fpos >= faceStr.size()) break;
                    size_t tokEnd = fpos;
                    while (tokEnd < faceStr.size() && faceStr[tokEnd] != ' ' && faceStr[tokEnd] != '\t') ++tokEnd;
                    std::string_view vtok = faceStr.substr(fpos, tokEnd - fpos);
                    fpos = tokEnd;

                    size_t s1 = vtok.find('/');
                    size_t s2 = (s1 != std::string_view::npos) ? vtok.find('/', s1 + 1) : std::string_view::npos;

                    int vi = parseIndex(vtok.substr(0, s1), positions.size());
                    int ti = -1;
                    int ni = -1;
                    if (s1 != std::string_view::npos) {
                        if (s2 != std::string_view::npos) {
                            ti = parseIndex(vtok.substr(s1 + 1, s2 - s1 - 1), texcoords.size());
                            ni = parseIndex(vtok.substr(s2 + 1), normals.size());
                        } else {
                            ti = parseIndex(vtok.substr(s1 + 1), texcoords.size());
                        }
                    }

                    if (vi < 0 || static_cast<size_t>(vi) >= positions.size()) continue;

                    PreviewVertex pv{};
                    pv.px = positions[static_cast<size_t>(vi)].x;
                    pv.py = positions[static_cast<size_t>(vi)].y;
                    pv.pz = positions[static_cast<size_t>(vi)].z;

                    if (ti >= 0 && static_cast<size_t>(ti) < texcoords.size()) {
                        pv.u = texcoords[static_cast<size_t>(ti)].u;
                        pv.v = texcoords[static_cast<size_t>(ti)].v;
                    }
                    if (ni >= 0 && static_cast<size_t>(ni) < normals.size()) {
                        pv.nx = normals[static_cast<size_t>(ni)].x;
                        pv.ny = normals[static_cast<size_t>(ni)].y;
                        pv.nz = normals[static_cast<size_t>(ni)].z;
                    } else {
                        anyMissingNormal = true;
                    }
                    pv.r = 0.86f;
                    pv.g = 0.89f;
                    pv.b = 0.94f;
                    pv.a = 1.0f;

                    uint32_t newIdx = static_cast<uint32_t>(cpuVertices.size());
                    cpuVertices.push_back(pv);
                    faceVertIndices.push_back(newIdx);
                }

                for (size_t i = 1; i + 1 < faceVertIndices.size(); ++i) {
                    cpuIndices.push_back(faceVertIndices[0]);
                    cpuIndices.push_back(faceVertIndices[i]);
                    cpuIndices.push_back(faceVertIndices[i + 1]);
                }
            }
        }

        if (cpuVertices.empty() || cpuIndices.empty()) return false;
        if (anyMissingNormal || normals.empty()) {
            RecomputeMissingNormals(cpuVertices, cpuIndices, 0, 0);
        }
        return true;
    }

    bool EnsurePipelineStates(ID3D11Device* pDevice) {
        if (pVS && pPS && pInputLayout && pConstantBuffer) return true;

        static const char* kHlslSource = R"(
cbuffer FrameCB : register(b0) {
    row_major float4x4 uWVP;
    row_major float4x4 uWorld;
    float4 uCameraPos;
    uint uHasTexture;
    uint uIsWireframe;
    uint uFlipTextureV;
    float uPad;
};

Texture2D uBaseTex : register(t0);
SamplerState uLinearSampler : register(s0);

struct VSIn {
    float3 pos : POSITION;
    float3 norm : NORMAL;
    float2 uv : TEXCOORD0;
    float4 col : COLOR0;
};

struct PSIn {
    float4 pos : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float3 worldNorm : TEXCOORD1;
    float2 uv : TEXCOORD2;
    float4 col : COLOR0;
};

PSIn VSMain(VSIn vin) {
    PSIn vout;
    vout.pos = mul(float4(vin.pos, 1.0), uWVP);
    vout.worldPos = mul(float4(vin.pos, 1.0), uWorld).xyz;
    vout.worldNorm = normalize(mul(float4(vin.norm, 0.0), uWorld).xyz);
    vout.uv = vin.uv;
    vout.col = vin.col;
    return vout;
}

float4 PSMain(PSIn pin) : SV_Target {
    if (uIsWireframe != 0) {
        return float4(0.35, 0.82, 1.0, 1.0);
    }

    float3 N = normalize(pin.worldNorm);
    float3 V = normalize(uCameraPos.xyz - pin.worldPos);
    float3 L = normalize(float3(0.55, 0.75, -0.65));
    float3 H = normalize(L + V);

    float4 baseColor = pin.col;
    if (uHasTexture != 0 && pin.uv.x > -900.0) {
        float2 uv = float2(pin.uv.x, (uFlipTextureV != 0) ? (1.0 - pin.uv.y) : pin.uv.y);
        baseColor *= uBaseTex.Sample(uLinearSampler, uv);
    }

    float ndotl = max(dot(N, L), 0.0);
    float hemi = N.y * 0.5 + 0.5;
    float3 skyAmb = lerp(float3(0.14, 0.16, 0.22), float3(0.32, 0.36, 0.44), hemi);

    float spec = pow(max(dot(N, H), 0.0), 32.0) * 0.28;
    float rim = pow(1.0 - max(dot(N, V), 0.0), 3.0) * 0.22;

    float3 finalRgb = baseColor.rgb * (skyAmb + ndotl * 0.82) + spec + rim * float3(0.45, 0.70, 1.0);
    return float4(saturate(finalRgb), 1.0);
}
)";

        ID3DBlob* vsBlob = nullptr;
        ID3DBlob* psBlob = nullptr;
        ID3DBlob* errBlob = nullptr;

        HRESULT hr = D3DCompile(kHlslSource, std::strlen(kHlslSource), nullptr, nullptr, nullptr,
                                "VSMain", "vs_4_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &vsBlob, &errBlob);
        SafeRelease(errBlob);
        if (FAILED(hr) || !vsBlob) return false;

        hr = D3DCompile(kHlslSource, std::strlen(kHlslSource), nullptr, nullptr, nullptr,
                        "PSMain", "ps_4_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &psBlob, &errBlob);
        SafeRelease(errBlob);
        if (FAILED(hr) || !psBlob) {
            SafeRelease(vsBlob);
            return false;
        }

        hr = pDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &pVS);
        if (SUCCEEDED(hr)) {
            hr = pDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pPS);
        }

        D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        if (SUCCEEDED(hr)) {
            hr = pDevice->CreateInputLayout(layoutDesc, 4,
                                            vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
                                            &pInputLayout);
        }
        SafeRelease(vsBlob);
        SafeRelease(psBlob);
        if (FAILED(hr)) return false;

        D3D11_BUFFER_DESC cbDesc{};
        cbDesc.ByteWidth = 160;
        cbDesc.Usage = D3D11_USAGE_DEFAULT;
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        hr = pDevice->CreateBuffer(&cbDesc, nullptr, &pConstantBuffer);
        if (FAILED(hr)) return false;

        D3D11_SAMPLER_DESC sampDesc{};
        sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
        pDevice->CreateSamplerState(&sampDesc, &pSampler);

        D3D11_RASTERIZER_DESC rsDesc{};
        rsDesc.FillMode = D3D11_FILL_SOLID;
        rsDesc.CullMode = D3D11_CULL_NONE;
        rsDesc.DepthClipEnable = TRUE;
        rsDesc.MultisampleEnable = FALSE;
        pDevice->CreateRasterizerState(&rsDesc, &pRasterSolid);

        rsDesc.FillMode = D3D11_FILL_WIREFRAME;
        pDevice->CreateRasterizerState(&rsDesc, &pRasterWireframe);

        D3D11_DEPTH_STENCIL_DESC dsDesc{};
        dsDesc.DepthEnable = TRUE;
        dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
        pDevice->CreateDepthStencilState(&dsDesc, &pDepthState);

        D3D11_BLEND_DESC blendDesc{};
        blendDesc.RenderTarget[0].BlendEnable = FALSE;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        pDevice->CreateBlendState(&blendDesc, &pBlendState);

        return true;
    }

    bool EnsureRenderTarget(ID3D11Device* pDevice, ID2D1DeviceContext* pD2DContext, uint32_t w, uint32_t h) {
        w = std::clamp(w, 64u, 2048u);
        h = std::clamp(h, 64u, 2048u);
        if (pColorTex && pRTV && pDSV && pOutputBitmap && targetWidth == w && targetHeight == h) {
            return true;
        }

        SafeRelease(pOutputBitmap);
        SafeRelease(pDSV);
        SafeRelease(pDepthTex);
        SafeRelease(pColorSRV);
        SafeRelease(pRTV);
        SafeRelease(pColorTex);

        D3D11_TEXTURE2D_DESC colorDesc{};
        colorDesc.Width = w;
        colorDesc.Height = h;
        colorDesc.MipLevels = 1;
        colorDesc.ArraySize = 1;
        colorDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        colorDesc.SampleDesc.Count = 1;
        colorDesc.Usage = D3D11_USAGE_DEFAULT;
        colorDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        HRESULT hr = pDevice->CreateTexture2D(&colorDesc, nullptr, &pColorTex);
        if (FAILED(hr) || !pColorTex) return false;

        hr = pDevice->CreateRenderTargetView(pColorTex, nullptr, &pRTV);
        if (FAILED(hr)) return false;

        pDevice->CreateShaderResourceView(pColorTex, nullptr, &pColorSRV);

        D3D11_TEXTURE2D_DESC depthDesc{};
        depthDesc.Width = w;
        depthDesc.Height = h;
        depthDesc.MipLevels = 1;
        depthDesc.ArraySize = 1;
        depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthDesc.SampleDesc.Count = 1;
        depthDesc.Usage = D3D11_USAGE_DEFAULT;
        depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

        hr = pDevice->CreateTexture2D(&depthDesc, nullptr, &pDepthTex);
        if (FAILED(hr) || !pDepthTex) return false;

        hr = pDevice->CreateDepthStencilView(pDepthTex, nullptr, &pDSV);
        if (FAILED(hr)) return false;

        IDXGISurface* pDxgiSurface = nullptr;
        hr = pColorTex->QueryInterface(IID_PPV_ARGS(&pDxgiSurface));
        if (SUCCEEDED(hr) && pDxgiSurface) {
            D2D1_BITMAP_PROPERTIES1 bmpProps = D2D1::BitmapProperties1(
                D2D1_BITMAP_OPTIONS_NONE,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
                96.0f, 96.0f);
            hr = pD2DContext->CreateBitmapFromDxgiSurface(pDxgiSurface, &bmpProps, &pOutputBitmap);
            SafeRelease(pDxgiSurface);
        }

        if (FAILED(hr) || !pOutputBitmap) return false;

        targetWidth = w;
        targetHeight = h;
        return true;
    }

    bool UploadMeshToGpu(ID3D11Device* pDevice) {
        if (!pDevice || cpuVertices.empty() || cpuIndices.empty()) return false;
        ReleaseGpuMeshBuffers();

        D3D11_BUFFER_DESC vbDesc{};
        vbDesc.ByteWidth = static_cast<UINT>(cpuVertices.size() * sizeof(PreviewVertex));
        vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vbData{};
        vbData.pSysMem = cpuVertices.data();

        HRESULT hr = pDevice->CreateBuffer(&vbDesc, &vbData, &pVertexBuffer);
        if (FAILED(hr) || !pVertexBuffer) return false;

        D3D11_BUFFER_DESC ibDesc{};
        ibDesc.ByteWidth = static_cast<UINT>(cpuIndices.size() * sizeof(uint32_t));
        ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
        ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA ibData{};
        ibData.pSysMem = cpuIndices.data();

        hr = pDevice->CreateBuffer(&ibDesc, &ibData, &pIndexBuffer);
        if (FAILED(hr) || !pIndexBuffer) return false;

        if (hasTexture && !cpuTextureRgba.empty() && cpuTexWidth > 0 && cpuTexHeight > 0) {
            D3D11_TEXTURE2D_DESC texDesc{};
            texDesc.Width = cpuTexWidth;
            texDesc.Height = cpuTexHeight;
            texDesc.MipLevels = 1;
            texDesc.ArraySize = 1;
            texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            texDesc.SampleDesc.Count = 1;
            texDesc.Usage = D3D11_USAGE_IMMUTABLE;
            texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

            D3D11_SUBRESOURCE_DATA texSub{};
            texSub.pSysMem = cpuTextureRgba.data();
            texSub.SysMemPitch = cpuTexWidth * 4;

            if (SUCCEEDED(pDevice->CreateTexture2D(&texDesc, &texSub, &pModelTex)) && pModelTex) {
                pDevice->CreateShaderResourceView(pModelTex, nullptr, &pModelTexSRV);
            }
        }

        for (auto& t : loadedTextures) {
            if (!t.rgba.empty() && t.width > 0 && t.height > 0 && !t.pSRV) {
                D3D11_TEXTURE2D_DESC texDesc{};
                texDesc.Width = t.width;
                texDesc.Height = t.height;
                texDesc.MipLevels = 1;
                texDesc.ArraySize = 1;
                texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                texDesc.SampleDesc.Count = 1;
                texDesc.Usage = D3D11_USAGE_IMMUTABLE;
                texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

                D3D11_SUBRESOURCE_DATA texSub{};
                texSub.pSysMem = t.rgba.data();
                texSub.SysMemPitch = t.width * 4;

                if (SUCCEEDED(pDevice->CreateTexture2D(&texDesc, &texSub, &t.pTex)) && t.pTex) {
                    pDevice->CreateShaderResourceView(t.pTex, nullptr, &t.pSRV);
                }
            }
        }

        gpuBuffersDirty = false;
        return true;
    }
};

// ============================================================================
// 2. NATIVE .DDS TEXTURE DECODER & .PNG EXPORTER (DdsCodec)
// ============================================================================

struct DdsCodec {
    static constexpr uint32_t MakeFourCC(char a, char b, char c, char d) {
        return static_cast<uint32_t>(static_cast<uint8_t>(a)) |
               (static_cast<uint32_t>(static_cast<uint8_t>(b)) << 8) |
               (static_cast<uint32_t>(static_cast<uint8_t>(c)) << 16) |
               (static_cast<uint32_t>(static_cast<uint8_t>(d)) << 24);
    }

    static void UnpackRgb565(uint16_t c, uint8_t& r, uint8_t& g, uint8_t& b) {
        uint32_t r5 = (c >> 11) & 0x1Fu;
        uint32_t g6 = (c >> 5) & 0x3Fu;
        uint32_t b5 = c & 0x1Fu;
        r = static_cast<uint8_t>((r5 << 3) | (r5 >> 2));
        g = static_cast<uint8_t>((g6 << 2) | (g6 >> 4));
        b = static_cast<uint8_t>((b5 << 3) | (b5 >> 2));
    }

    static void DecodeBc1Block(const uint8_t* block, uint8_t outPixels[16][4], bool allowOneBitAlpha) {
        uint16_t c0 = static_cast<uint16_t>(block[0] | (block[1] << 8));
        uint16_t c1 = static_cast<uint16_t>(block[2] | (block[3] << 8));
        uint8_t pal[4][4] = {};
        UnpackRgb565(c0, pal[0][0], pal[0][1], pal[0][2]);
        pal[0][3] = 255;
        UnpackRgb565(c1, pal[1][0], pal[1][1], pal[1][2]);
        pal[1][3] = 255;

        if (c0 > c1 || !allowOneBitAlpha) {
            for (int ch = 0; ch < 3; ++ch) {
                pal[2][ch] = static_cast<uint8_t>((2 * pal[0][ch] + pal[1][ch] + 1) / 3);
                pal[3][ch] = static_cast<uint8_t>((pal[0][ch] + 2 * pal[1][ch] + 1) / 3);
            }
            pal[2][3] = 255;
            pal[3][3] = 255;
        } else {
            for (int ch = 0; ch < 3; ++ch) {
                pal[2][ch] = static_cast<uint8_t>((pal[0][ch] + pal[1][ch]) / 2);
                pal[3][ch] = 0;
            }
            pal[2][3] = 255;
            pal[3][3] = 0;
        }

        uint32_t lut = 0;
        std::memcpy(&lut, block + 4, 4);
        for (int i = 0; i < 16; ++i) {
            uint32_t idx = (lut >> (i * 2)) & 0x3u;
            outPixels[i][0] = pal[idx][0];
            outPixels[i][1] = pal[idx][1];
            outPixels[i][2] = pal[idx][2];
            outPixels[i][3] = pal[idx][3];
        }
    }

    static void DecodeBc4Channel(const uint8_t* block, uint8_t outAlpha[16]) {
        uint8_t a0 = block[0];
        uint8_t a1 = block[1];
        uint8_t pal[8];
        pal[0] = a0;
        pal[1] = a1;
        if (a0 > a1) {
            for (int i = 1; i <= 6; ++i) {
                pal[i + 1] = static_cast<uint8_t>(((7 - i) * a0 + i * a1 + 3) / 7);
            }
        } else {
            for (int i = 1; i <= 4; ++i) {
                pal[i + 1] = static_cast<uint8_t>(((5 - i) * a0 + i * a1 + 2) / 5);
            }
            pal[6] = 0;
            pal[7] = 255;
        }

        uint64_t bits = 0;
        for (int i = 0; i < 6; ++i) {
            bits |= (static_cast<uint64_t>(block[2 + i]) << (8 * i));
        }
        for (int i = 0; i < 16; ++i) {
            uint32_t idx = static_cast<uint32_t>((bits >> (i * 3)) & 0x7u);
            outAlpha[i] = pal[idx];
        }
    }

    static bool DecodeDdsToRgba(std::span<const uint8_t> ddsBytes,
                                std::vector<uint8_t>& outRgba,
                                uint32_t& outW,
                                uint32_t& outH,
                                std::wstring& outFormatName,
                                uint32_t& outMipCount) {
        outRgba.clear();
        outW = 0;
        outH = 0;
        outFormatName.clear();
        outMipCount = 1;

        if (ddsBytes.size() < 128) return false;

        uint32_t magic = 0;
        std::memcpy(&magic, ddsBytes.data(), 4);
        if (magic != 0x20534444u) return false; // "DDS "

        const uint8_t* hdr = ddsBytes.data() + 4;
        uint32_t hdrSize = 0, height = 0, width = 0, mipCount = 0;
        std::memcpy(&hdrSize, hdr + 0, 4);
        std::memcpy(&height, hdr + 8, 4);
        std::memcpy(&width, hdr + 12, 4);
        std::memcpy(&mipCount, hdr + 24, 4);

        if (hdrSize != 124 || width == 0 || height == 0 || width > 16384 || height > 16384) {
            return false;
        }

        outW = width;
        outH = height;
        outMipCount = (mipCount > 0) ? mipCount : 1;

        uint32_t pfFlags = 0, fourCC = 0, rgbBitCount = 0;
        uint32_t rMask = 0, gMask = 0, bMask = 0, aMask = 0;
        std::memcpy(&pfFlags, hdr + 76, 4);
        std::memcpy(&fourCC, hdr + 80, 4);
        std::memcpy(&rgbBitCount, hdr + 84, 4);
        std::memcpy(&rMask, hdr + 88, 4);
        std::memcpy(&gMask, hdr + 92, 4);
        std::memcpy(&bMask, hdr + 96, 4);
        std::memcpy(&aMask, hdr + 100, 4);

        size_t dataOffset = 128;
        uint32_t dxgiFormat = 0;

        if ((pfFlags & 0x4u) && fourCC == MakeFourCC('D', 'X', '1', '0')) {
            if (ddsBytes.size() < 148) return false;
            std::memcpy(&dxgiFormat, ddsBytes.data() + 128, 4);
            dataOffset = 148;
        }

        enum class BlockKind {
            None, BC1, BC2, BC3, BC4, BC5, BC7,
            Uncompressed
        };
        BlockKind kind = BlockKind::None;

        if (dxgiFormat != 0) {
            switch (dxgiFormat) {
                case 70: case 71: case 72:
                    kind = BlockKind::BC1; outFormatName = L"BC1 (DXT1)"; break;
                case 73: case 74: case 75:
                    kind = BlockKind::BC2; outFormatName = L"BC2 (DXT3)"; break;
                case 76: case 77: case 78:
                    kind = BlockKind::BC3; outFormatName = L"BC3 (DXT5)"; break;
                case 79: case 80: case 81:
                    kind = BlockKind::BC4; outFormatName = L"BC4 (ATI1)"; break;
                case 82: case 83: case 84:
                    kind = BlockKind::BC5; outFormatName = L"BC5 (ATI2 Normal)"; break;
                case 95: case 96: case 97:
                    kind = BlockKind::BC7; outFormatName = L"BC6H (HDR Block)"; break;
                case 98: case 99:
                    kind = BlockKind::BC7; outFormatName = L"BC7 (DX11 Block)"; break;
                case 28: case 29:
                    kind = BlockKind::Uncompressed;
                    rgbBitCount = 32;
                    rMask = 0x000000FFu; gMask = 0x0000FF00u; bMask = 0x00FF0000u; aMask = 0xFF000000u;
                    outFormatName = L"R8G8B8A8_UNORM";
                    break;
                case 87: case 91:
                    kind = BlockKind::Uncompressed;
                    rgbBitCount = 32;
                    rMask = 0x00FF0000u; gMask = 0x0000FF00u; bMask = 0x000000FFu; aMask = 0xFF000000u;
                    outFormatName = L"B8G8R8A8_UNORM";
                    break;
                case 88: case 93:
                    kind = BlockKind::Uncompressed;
                    rgbBitCount = 32;
                    rMask = 0x00FF0000u; gMask = 0x0000FF00u; bMask = 0x000000FFu; aMask = 0;
                    outFormatName = L"B8G8R8X8_UNORM";
                    break;
                case 61:
                    kind = BlockKind::Uncompressed;
                    rgbBitCount = 8;
                    rMask = 0xFFu; gMask = 0; bMask = 0; aMask = 0;
                    outFormatName = L"R8_UNORM (L8)";
                    break;
                default:
                    outFormatName = L"DX10 Format #" + std::to_wstring(dxgiFormat);
                    kind = BlockKind::BC7;
                    break;
            }
        } else if (pfFlags & 0x4u) { // DDPF_FOURCC
            if (fourCC == MakeFourCC('D', 'X', 'T', '1')) {
                kind = BlockKind::BC1; outFormatName = L"DXT1 (BC1)";
            } else if (fourCC == MakeFourCC('D', 'X', 'T', '2') || fourCC == MakeFourCC('D', 'X', 'T', '3')) {
                kind = BlockKind::BC2; outFormatName = L"DXT3 (BC2)";
            } else if (fourCC == MakeFourCC('D', 'X', 'T', '4') || fourCC == MakeFourCC('D', 'X', 'T', '5')) {
                kind = BlockKind::BC3; outFormatName = L"DXT5 (BC3)";
            } else if (fourCC == MakeFourCC('A', 'T', 'I', '1') || fourCC == MakeFourCC('B', 'C', '4', 'U') ||
                       fourCC == MakeFourCC('B', 'C', '4', 'S')) {
                kind = BlockKind::BC4; outFormatName = L"BC4 (ATI1)";
            } else if (fourCC == MakeFourCC('A', 'T', 'I', '2') || fourCC == MakeFourCC('B', 'C', '5', 'U') ||
                       fourCC == MakeFourCC('B', 'C', '5', 'S') || fourCC == MakeFourCC('3', 'D', 'C', ' ')) {
                kind = BlockKind::BC5; outFormatName = L"BC5 (ATI2)";
            } else {
                return false;
            }
        } else {
            kind = BlockKind::Uncompressed;
            if (rgbBitCount == 32) {
                outFormatName = (rMask == 0x000000FFu) ? L"R8G8B8A8" : L"B8G8R8A8";
            } else if (rgbBitCount == 24) {
                outFormatName = L"B8G8R8";
            } else if (rgbBitCount == 16) {
                outFormatName = (pfFlags & 0x20000u) ? L"A8L8" : L"B5G6R5 / 16-bit";
            } else if (rgbBitCount == 8) {
                outFormatName = L"L8";
            } else {
                return false;
            }
        }

        outRgba.assign(static_cast<size_t>(width) * height * 4, 255);
        const uint8_t* src = ddsBytes.data() + dataOffset;
        size_t availBytes = (ddsBytes.size() > dataOffset) ? (ddsBytes.size() - dataOffset) : 0;

        if (kind == BlockKind::Uncompressed) {
            uint32_t bytesPerPixel = rgbBitCount / 8;
            if (bytesPerPixel == 0 || availBytes < static_cast<size_t>(width) * height * bytesPerPixel) {
                return false;
            }

            auto extractChannel = [](uint32_t val, uint32_t mask, uint8_t defVal) -> uint8_t {
                if (mask == 0) return defVal;
                uint32_t shift = 0;
                uint32_t m = mask;
                while ((m & 1u) == 0u && shift < 32) { m >>= 1; ++shift; }
                uint32_t v = (val & mask) >> shift;
                if (m == 0) return defVal;
                return static_cast<uint8_t>((v * 255u + (m / 2u)) / m);
            };

            for (uint32_t y = 0; y < height; ++y) {
                for (uint32_t x = 0; x < width; ++x) {
                    size_t idx = (static_cast<size_t>(y) * width + x);
                    const uint8_t* p = src + idx * bytesPerPixel;
                    uint8_t* dst = outRgba.data() + idx * 4;

                    if (rgbBitCount == 8) {
                        dst[0] = dst[1] = dst[2] = p[0];
                        dst[3] = 255;
                    } else if (rgbBitCount == 16 && (pfFlags & 0x20000u)) { // DDPF_LUMINANCE A8L8
                        dst[0] = dst[1] = dst[2] = p[0];
                        dst[3] = p[1];
                    } else {
                        uint32_t raw = 0;
                        std::memcpy(&raw, p, bytesPerPixel);
                        dst[0] = extractChannel(raw, rMask ? rMask : 0x00FF0000u, 0);
                        dst[1] = extractChannel(raw, gMask ? gMask : 0x0000FF00u, 0);
                        dst[2] = extractChannel(raw, bMask ? bMask : 0x000000FFu, 0);
                        dst[3] = extractChannel(raw, aMask, 255);
                    }
                }
            }
            return true;
        }

        uint32_t blocksX = (width + 3u) / 4u;
        uint32_t blocksY = (height + 3u) / 4u;
        size_t blockSize = (kind == BlockKind::BC1 || kind == BlockKind::BC4) ? 8u : 16u;
        if (availBytes < static_cast<size_t>(blocksX) * blocksY * blockSize) {
            return false;
        }

        for (uint32_t by = 0; by < blocksY; ++by) {
            for (uint32_t bx = 0; bx < blocksX; ++bx) {
                const uint8_t* blk = src + (static_cast<size_t>(by) * blocksX + bx) * blockSize;
                uint8_t rgbaBlock[16][4] = {};

                if (kind == BlockKind::BC1) {
                    DecodeBc1Block(blk, rgbaBlock, true);
                } else if (kind == BlockKind::BC2) {
                    DecodeBc1Block(blk + 8, rgbaBlock, false);
                    for (int i = 0; i < 16; ++i) {
                        uint8_t byteVal = blk[i / 2];
                        uint8_t nib = (i & 1) ? (byteVal >> 4) : (byteVal & 0x0Fu);
                        rgbaBlock[i][3] = static_cast<uint8_t>(nib * 17u);
                    }
                } else if (kind == BlockKind::BC3) {
                    uint8_t alphas[16] = {};
                    DecodeBc4Channel(blk, alphas);
                    DecodeBc1Block(blk + 8, rgbaBlock, false);
                    for (int i = 0; i < 16; ++i) {
                        rgbaBlock[i][3] = alphas[i];
                    }
                } else if (kind == BlockKind::BC4) {
                    uint8_t red[16] = {};
                    DecodeBc4Channel(blk, red);
                    for (int i = 0; i < 16; ++i) {
                        rgbaBlock[i][0] = red[i];
                        rgbaBlock[i][1] = red[i];
                        rgbaBlock[i][2] = red[i];
                        rgbaBlock[i][3] = 255;
                    }
                } else if (kind == BlockKind::BC5) {
                    uint8_t chR[16] = {};
                    uint8_t chG[16] = {};
                    DecodeBc4Channel(blk, chR);
                    DecodeBc4Channel(blk + 8, chG);
                    for (int i = 0; i < 16; ++i) {
                        float nx = (static_cast<float>(chR[i]) / 255.0f) * 2.0f - 1.0f;
                        float ny = (static_cast<float>(chG[i]) / 255.0f) * 2.0f - 1.0f;
                        float nzSq = (std::max)(0.0f, 1.0f - nx * nx - ny * ny);
                        float nz = std::sqrt(nzSq);
                        rgbaBlock[i][0] = chR[i];
                        rgbaBlock[i][1] = chG[i];
                        rgbaBlock[i][2] = static_cast<uint8_t>(std::clamp((nz * 0.5f + 0.5f) * 255.0f, 0.0f, 255.0f));
                        rgbaBlock[i][3] = 255;
                    }
                } else if (kind == BlockKind::BC7) {
                    // Fast BC7 mode endpoint approximation so BC7 DDS previews render immediately
                    uint8_t r0 = blk[1], g0 = blk[3], b0 = blk[5];
                    uint8_t r1 = blk[2], g1 = blk[4], b1 = blk[6];
                    for (int i = 0; i < 16; ++i) {
                        uint8_t w = static_cast<uint8_t>((blk[8 + (i / 2)] >> ((i & 1) * 4)) & 0x0Fu);
                        rgbaBlock[i][0] = static_cast<uint8_t>(((15 - w) * r0 + w * r1) / 15);
                        rgbaBlock[i][1] = static_cast<uint8_t>(((15 - w) * g0 + w * g1) / 15);
                        rgbaBlock[i][2] = static_cast<uint8_t>(((15 - w) * b0 + w * b1) / 15);
                        rgbaBlock[i][3] = 255;
                    }
                }

                for (uint32_t py = 0; py < 4; ++py) {
                    uint32_t dstY = by * 4 + py;
                    if (dstY >= height) continue;
                    for (uint32_t px = 0; px < 4; ++px) {
                        uint32_t dstX = bx * 4 + px;
                        if (dstX >= width) continue;
                        uint8_t* dst = outRgba.data() + (static_cast<size_t>(dstY) * width + dstX) * 4;
                        const uint8_t* s = rgbaBlock[py * 4 + px];
                        dst[0] = s[0];
                        dst[1] = s[1];
                        dst[2] = s[2];
                        dst[3] = s[3];
                    }
                }
            }
        }

        return true;
    }

    static bool SaveRgbaToPngFile(IWICImagingFactory* pExternalFactory,
                                  const std::wstring& destPath,
                                  const uint8_t* rgbaData,
                                  uint32_t width,
                                  uint32_t height) {
        if (destPath.empty() || !rgbaData || width == 0 || height == 0) return false;

        IWICImagingFactory* pFactory = pExternalFactory;
        bool ownsFactory = false;
        if (!pFactory) {
            HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                          IID_PPV_ARGS(&pFactory));
            if (FAILED(hr) || !pFactory) return false;
            ownsFactory = true;
        }

        // Convert RGBA -> BGRA for GUID_WICPixelFormat32bppBGRA (native PNG encoder format)
        std::vector<uint8_t> bgra(static_cast<size_t>(width) * height * 4);
        size_t totalPixels = static_cast<size_t>(width) * height;
        for (size_t i = 0; i < totalPixels; ++i) {
            bgra[i * 4 + 0] = rgbaData[i * 4 + 2];
            bgra[i * 4 + 1] = rgbaData[i * 4 + 1];
            bgra[i * 4 + 2] = rgbaData[i * 4 + 0];
            bgra[i * 4 + 3] = rgbaData[i * 4 + 3];
        }

        IWICStream* pStream = nullptr;
        IWICBitmapEncoder* pEncoder = nullptr;
        IWICBitmapFrameEncode* pFrame = nullptr;
        IPropertyBag2* pProps = nullptr;
        bool ok = false;

        HRESULT hr = pFactory->CreateStream(&pStream);
        if (SUCCEEDED(hr) && pStream) {
            hr = pStream->InitializeFromFilename(destPath.c_str(), GENERIC_WRITE);
        }
        if (SUCCEEDED(hr)) {
            hr = pFactory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &pEncoder);
        }
        if (SUCCEEDED(hr) && pEncoder) {
            hr = pEncoder->Initialize(pStream, WICBitmapEncoderNoCache);
        }
        if (SUCCEEDED(hr) && pEncoder) {
            hr = pEncoder->CreateNewFrame(&pFrame, &pProps);
        }
        if (SUCCEEDED(hr) && pFrame) {
            hr = pFrame->Initialize(pProps);
        }
        if (SUCCEEDED(hr) && pFrame) {
            hr = pFrame->SetSize(width, height);
        }
        if (SUCCEEDED(hr) && pFrame) {
            WICPixelFormatGUID fmt = GUID_WICPixelFormat32bppBGRA;
            hr = pFrame->SetPixelFormat(&fmt);
        }
        if (SUCCEEDED(hr) && pFrame) {
            UINT stride = width * 4;
            UINT bufSize = static_cast<UINT>(bgra.size());
            hr = pFrame->WritePixels(height, stride, bufSize, bgra.data());
        }
        if (SUCCEEDED(hr) && pFrame) {
            hr = pFrame->Commit();
        }
        if (SUCCEEDED(hr) && pEncoder) {
            hr = pEncoder->Commit();
            ok = SUCCEEDED(hr);
        }

        SafeRelease(pProps);
        SafeRelease(pFrame);
        SafeRelease(pEncoder);
        SafeRelease(pStream);
        if (ownsFactory) SafeRelease(pFactory);
        return ok;
    }
};

inline bool DecodeDdsToRgba(std::span<const uint8_t> ddsBytes,
                            std::vector<uint8_t>& outRgba,
                            uint32_t& outW,
                            uint32_t& outH,
                            std::wstring& outFormatName,
                            uint32_t& outMipCount) {
    return DdsCodec::DecodeDdsToRgba(ddsBytes, outRgba, outW, outH, outFormatName, outMipCount);
}

inline bool SaveRgbaToPngFile(IWICImagingFactory* pWICFactory,
                              const std::wstring& destPath,
                              const uint8_t* rgbaData,
                              uint32_t width,
                              uint32_t height) {
    return DdsCodec::SaveRgbaToPngFile(pWICFactory, destPath, rgbaData, width, height);
}

// ============================================================================
// 3. 2D DOOM / WAD VECTOR MAP PREVIEW DATA (DoomMapGeometry)
// ============================================================================

struct DoomLineDef {
    float x1 = 0.0f;
    float y1 = 0.0f;
    float x2 = 0.0f;
    float y2 = 0.0f;
    bool twoSided = false;
    uint16_t special = 0;
};

inline bool ExtractDoomMapLines(std::span<const uint8_t> vertexesLump,
                                std::span<const uint8_t> linedefsLump,
                                std::vector<DoomLineDef>& outLines,
                                float& minX,
                                float& minY,
                                float& maxX,
                                float& maxY) {
    outLines.clear();
    minX = minY = 0.0f;
    maxX = maxY = 0.0f;

    size_t numVerts = vertexesLump.size() / 4;
    if (numVerts < 2 || linedefsLump.size() < 14) return false;

    bool isHexenFormat = (linedefsLump.size() % 14 != 0) && (linedefsLump.size() % 16 == 0);
    size_t stride = isHexenFormat ? 16 : 14;
    size_t numLines = linedefsLump.size() / stride;
    if (numLines == 0) return false;

    auto getVertex = [&](uint16_t idx, float& vx, float& vy) -> bool {
        if (idx >= numVerts) return false;
        int16_t rx = 0, ry = 0;
        std::memcpy(&rx, vertexesLump.data() + static_cast<size_t>(idx) * 4 + 0, 2);
        std::memcpy(&ry, vertexesLump.data() + static_cast<size_t>(idx) * 4 + 2, 2);
        vx = static_cast<float>(rx);
        vy = static_cast<float>(ry);
        return true;
    };

    minX = minY = 1e30f;
    maxX = maxY = -1e30f;
    outLines.reserve(numLines);

    for (size_t i = 0; i < numLines; ++i) {
        const uint8_t* ld = linedefsLump.data() + i * stride;
        uint16_t v1 = 0, v2 = 0, flags = 0, special = 0, backSide = 0xFFFFu;
        std::memcpy(&v1, ld + 0, 2);
        std::memcpy(&v2, ld + 2, 2);
        std::memcpy(&flags, ld + 4, 2);

        if (isHexenFormat) {
            special = ld[6];
            std::memcpy(&backSide, ld + 14, 2);
        } else {
            std::memcpy(&special, ld + 6, 2);
            std::memcpy(&backSide, ld + 12, 2);
        }

        DoomLineDef line{};
        if (!getVertex(v1, line.x1, line.y1) || !getVertex(v2, line.x2, line.y2)) {
            continue;
        }
        line.twoSided = ((flags & 0x0004u) != 0) || (backSide != 0xFFFFu);
        line.special = special;

        minX = (std::min)({ minX, line.x1, line.x2 });
        minY = (std::min)({ minY, line.y1, line.y2 });
        maxX = (std::max)({ maxX, line.x1, line.x2 });
        maxY = (std::max)({ maxY, line.y1, line.y2 });

        outLines.push_back(line);
    }

    if (outLines.empty()) {
        minX = minY = maxX = maxY = 0.0f;
        return false;
    }
    return true;
}

struct DoomMapGeometry {
    std::wstring mapName;
    std::vector<DoomLineDef> lines;
    float minX = 0.0f;
    float minY = 0.0f;
    float maxX = 0.0f;
    float maxY = 0.0f;
    uint32_t vertexCount = 0;
    uint32_t sidedefCount = 0;
    uint32_t sectorCount = 0;
    uint32_t thingCount = 0;

    static bool ExtractDoomMapLines(std::span<const uint8_t> vertexesLump,
                                    std::span<const uint8_t> linedefsLump,
                                    std::vector<DoomLineDef>& outLines,
                                    float& minX,
                                    float& minY,
                                    float& maxX,
                                    float& maxY) {
        return speta::ExtractDoomMapLines(vertexesLump, linedefsLump, outLines, minX, minY, maxX, maxY);
    }

    bool LoadFromWadBytes(std::span<const uint8_t> wadBytes, std::string_view preferredMap = "") {
        lines.clear();
        if (wadBytes.size() < 12) return false;
        if (std::memcmp(wadBytes.data(), "IWAD", 4) != 0 &&
            std::memcmp(wadBytes.data(), "PWAD", 4) != 0) {
            return false;
        }

        uint32_t numLumps = 0, dirOffset = 0;
        std::memcpy(&numLumps, wadBytes.data() + 4, 4);
        std::memcpy(&dirOffset, wadBytes.data() + 8, 4);
        if (numLumps == 0 || numLumps > 131072 ||
            static_cast<size_t>(dirOffset) + static_cast<size_t>(numLumps) * 16 > wadBytes.size()) {
            return false;
        }

        auto readLumpName = [&](uint32_t idx) -> std::string {
            const char* p = reinterpret_cast<const char*>(wadBytes.data() + dirOffset + idx * 16 + 8);
            size_t len = 0;
            while (len < 8 && p[len] != '\0') ++len;
            return std::string(p, len);
        };

        auto getLumpSpan = [&](uint32_t idx) -> std::span<const uint8_t> {
            uint32_t filePos = 0, lumpSize = 0;
            std::memcpy(&filePos, wadBytes.data() + dirOffset + idx * 16 + 0, 4);
            std::memcpy(&lumpSize, wadBytes.data() + dirOffset + idx * 16 + 4, 4);
            if (static_cast<size_t>(filePos) + lumpSize > wadBytes.size()) return {};
            return wadBytes.subspan(filePos, lumpSize);
        };

        for (uint32_t i = 0; i + 4 < numLumps; ++i) {
            std::string marker = readLumpName(i);
            if (!preferredMap.empty() && _stricmp(marker.c_str(), std::string(preferredMap).c_str()) != 0) {
                continue;
            }

            std::span<const uint8_t> thingsLump;
            std::span<const uint8_t> linedefsLump;
            std::span<const uint8_t> sidedefsLump;
            std::span<const uint8_t> vertexesLump;
            std::span<const uint8_t> sectorsLump;

            for (uint32_t j = i + 1; j < (std::min)(numLumps, i + 12); ++j) {
                std::string ln = readLumpName(j);
                if (ln == "THINGS") thingsLump = getLumpSpan(j);
                else if (ln == "LINEDEFS") linedefsLump = getLumpSpan(j);
                else if (ln == "SIDEDEFS") sidedefsLump = getLumpSpan(j);
                else if (ln == "VERTEXES") vertexesLump = getLumpSpan(j);
                else if (ln == "SECTORS") sectorsLump = getLumpSpan(j);
            }

            if (!vertexesLump.empty() && !linedefsLump.empty()) {
                if (speta::ExtractDoomMapLines(vertexesLump, linedefsLump, lines, minX, minY, maxX, maxY)) {
                    mapName = Utf8ToWide(marker);
                    vertexCount = static_cast<uint32_t>(vertexesLump.size() / 4);
                    sidedefCount = static_cast<uint32_t>(sidedefsLump.size() / 30);
                    sectorCount = static_cast<uint32_t>(sectorsLump.size() / 26);
                    thingCount = static_cast<uint32_t>(thingsLump.size() / 10);
                    return true;
                }
            }
        }
        return false;
    }
};

// DOOM decoders unified in game_file_engine.hpp


inline bool ConvertDoomSoundToWav(std::span<const uint8_t> lump, std::vector<uint8_t>& outWav) {
    outWav.clear();
    if (lump.size() < 8) return false;
    if (lump.size() >= 44 && std::memcmp(lump.data(), "RIFF", 4) == 0) {
        outWav.assign(lump.begin(), lump.end());
        return true;
    }
    uint16_t format = *reinterpret_cast<const uint16_t*>(lump.data() + 0);
    uint16_t sampleRate = *reinterpret_cast<const uint16_t*>(lump.data() + 2);
    uint32_t sampleCount = *reinterpret_cast<const uint32_t*>(lump.data() + 4);
    if (format != 3 || sampleRate == 0 || sampleCount == 0) return false;

    size_t dataOfs = 8;
    if (sampleCount + 32 <= lump.size()) {
        dataOfs = 24; // 16 pad bytes at start in extended DMX format
    } else if (sampleCount + 8 > lump.size()) {
        sampleCount = static_cast<uint32_t>(lump.size() > 8 ? lump.size() - 8 : 0);
    }

    outWav.resize(44 + sampleCount);
    uint8_t* p = outWav.data();
    std::memcpy(p, "RIFF", 4);
    *reinterpret_cast<uint32_t*>(p + 4) = 36 + sampleCount;
    std::memcpy(p + 8, "WAVEfmt ", 8);
    *reinterpret_cast<uint32_t*>(p + 16) = 16;
    *reinterpret_cast<uint16_t*>(p + 20) = 1;  // PCM
    *reinterpret_cast<uint16_t*>(p + 22) = 1;  // Mono
    *reinterpret_cast<uint32_t*>(p + 24) = sampleRate;
    *reinterpret_cast<uint32_t*>(p + 28) = sampleRate;
    *reinterpret_cast<uint16_t*>(p + 32) = 1;
    *reinterpret_cast<uint16_t*>(p + 34) = 8;
    std::memcpy(p + 36, "data", 4);
    *reinterpret_cast<uint32_t*>(p + 40) = sampleCount;
    if (sampleCount > 0 && dataOfs + sampleCount <= lump.size()) {
        std::memcpy(p + 44, lump.data() + dataOfs, sampleCount);
    }
    return true;
}

// ============================================================================
// 4. INTEGRATED AUDIO PLAYER (SpetaAudioPlayer)
// ============================================================================

class SpetaAudioPlayer {
public:
    std::array<float, 64> waveformPeaks{};
    std::wstring currentTitle;
    std::wstring currentFilePath;
    uint32_t sampleRate = 44100;
    uint16_t channels = 2;
    uint16_t bitsPerSample = 16;

    SpetaAudioPlayer() {
        waveformPeaks.fill(0.0f);
    }

    ~SpetaAudioPlayer() {
        Stop();
    }

    SpetaAudioPlayer(const SpetaAudioPlayer&) = delete;
    SpetaAudioPlayer& operator=(const SpetaAudioPlayer&) = delete;

    bool PlayFile(const std::wstring& path) {
        return LoadAndPlayFile(path);
    }

    bool LoadAndPlayFile(const std::wstring& path) {
        Stop();
        if (path.empty()) return false;

        std::vector<int16_t> decodedPcm;
        uint32_t sr = 44100;
        uint16_t ch = 2;

        std::wstring lower = path;
        for (auto& c : lower) c = towlower(c);

        bool ok = false;
        if (lower.ends_with(L".wav")) {
            std::ifstream f(path, std::ios::binary | std::ios::ate);
            if (f.is_open()) {
                auto sz = f.tellg();
                if (sz > 44 && sz <= 128 * 1024 * 1024) {
                    std::vector<uint8_t> raw(static_cast<size_t>(sz));
                    f.seekg(0, std::ios::beg);
                    if (f.read(reinterpret_cast<char*>(raw.data()), sz)) {
                        ok = DecodeRiffWavMemory(raw, decodedPcm, sr, ch);
                    }
                }
            }
        }

        if (!ok) {
            ok = DecodeFileWithMediaFoundation(path, decodedPcm, sr, ch);
        }

        if (!ok) {
            // Fallback to native RIFF WAV parser
            std::ifstream f(path, std::ios::binary | std::ios::ate);
            if (f.is_open()) {
                auto sz = f.tellg();
                if (sz > 44 && sz <= 128 * 1024 * 1024) {
                    std::vector<uint8_t> raw(static_cast<size_t>(sz));
                    f.seekg(0, std::ios::beg);
                    if (f.read(reinterpret_cast<char*>(raw.data()), sz)) {
                        ok = DecodeRiffWavMemory(raw, decodedPcm, sr, ch);
                    }
                }
            }
        }

        if (!ok || decodedPcm.empty() || sr == 0 || ch == 0) {
            return false;
        }

        currentFilePath = path;
        size_t slash = path.find_last_of(L"\\/");
        currentTitle = (slash != std::wstring::npos) ? path.substr(slash + 1) : path;

        return StartPlaybackThread(std::move(decodedPcm), sr, ch);
    }

    bool PlayWavBytes(std::span<const uint8_t> wavBytes, const std::wstring& title = L"In-Memory Audio") {
        return LoadAndPlayWavMemory(wavBytes, title);
    }

    bool LoadAndPlayWavMemory(std::span<const uint8_t> wavBytes, const std::wstring& title = L"In-Memory Audio") {
        Stop();
        if (wavBytes.size() < 16) return false;

        std::vector<int16_t> decodedPcm;
        uint32_t sr = 44100;
        uint16_t ch = 2;

        bool ok = DecodeRiffWavMemory(wavBytes, decodedPcm, sr, ch);
        if (!ok) {
            ok = DecodeMemoryWithMediaFoundation(wavBytes, decodedPcm, sr, ch);
        }
        if (!ok || decodedPcm.empty() || sr == 0 || ch == 0) {
            return false;
        }

        currentFilePath.clear();
        currentTitle = title;
        return StartPlaybackThread(std::move(decodedPcm), sr, ch);
    }

    void TogglePause() {
        if (!playing_.load(std::memory_order_acquire)) return;
        bool wasPaused = paused_.load(std::memory_order_acquire);
        paused_.store(!wasPaused, std::memory_order_release);
    }

    void Stop() {
        stopRequested_.store(true, std::memory_order_release);
        paused_.store(false, std::memory_order_release);
        if (playbackThread_.joinable()) {
            playbackThread_.join();
        }
        playing_.store(false, std::memory_order_release);
        currentFrame_.store(0, std::memory_order_release);
        waveformPeaks.fill(0.0f);
    }

    void Seek(double seconds) {
        size_t totalFr = totalFrames_.load(std::memory_order_acquire);
        uint32_t sr = sampleRate > 0 ? sampleRate : 44100;
        if (totalFr == 0) return;
        double clampedSec = (std::max)(0.0, seconds);
        size_t targetFrame = static_cast<size_t>(clampedSec * static_cast<double>(sr));
        if (targetFrame >= totalFr) targetFrame = totalFr - 1;
        currentFrame_.store(targetFrame, std::memory_order_release);
        UpdateWaveformPeaksAtFrame(targetFrame);
    }

    double GetPositionSeconds() const {
        uint32_t sr = sampleRate > 0 ? sampleRate : 44100;
        return static_cast<double>(currentFrame_.load(std::memory_order_acquire)) / static_cast<double>(sr);
    }

    double GetDurationSeconds() const {
        uint32_t sr = sampleRate > 0 ? sampleRate : 44100;
        return static_cast<double>(totalFrames_.load(std::memory_order_acquire)) / static_cast<double>(sr);
    }

    bool IsPlaying() const {
        return playing_.load(std::memory_order_acquire) && !paused_.load(std::memory_order_acquire);
    }

    bool IsPaused() const {
        return playing_.load(std::memory_order_acquire) && paused_.load(std::memory_order_acquire);
    }

    bool HasActiveTrack() const {
        return playing_.load(std::memory_order_acquire);
    }

private:
    std::vector<int16_t> pcmSamples_;
    std::atomic<size_t> currentFrame_{ 0 };
    std::atomic<size_t> totalFrames_{ 0 };
    std::atomic<bool> playing_{ false };
    std::atomic<bool> paused_{ false };
    std::atomic<bool> stopRequested_{ false };
    std::thread playbackThread_;

    void UpdateWaveformPeaksAtFrame(size_t centerFrame) {
        size_t totFr = totalFrames_.load(std::memory_order_acquire);
        uint16_t ch = channels > 0 ? channels : 1;
        if (totFr == 0 || pcmSamples_.empty()) {
            waveformPeaks.fill(0.0f);
            return;
        }

        constexpr size_t kBins = 64;
        constexpr size_t kFramesPerBin = 64;
        for (size_t b = 0; b < kBins; ++b) {
            size_t binStartFrame = centerFrame + b * kFramesPerBin;
            if (binStartFrame >= totFr) {
                waveformPeaks[b] *= 0.65f;
                continue;
            }
            size_t endFrame = (std::min)(totFr, binStartFrame + kFramesPerBin);
            float maxAmp = 0.0f;
            for (size_t f = binStartFrame; f < endFrame; ++f) {
                size_t sampleIdx = f * ch;
                if (sampleIdx < pcmSamples_.size()) {
                    float a = std::fabs(static_cast<float>(pcmSamples_[sampleIdx]) / 32768.0f);
                    if (ch > 1 && sampleIdx + 1 < pcmSamples_.size()) {
                        float aR = std::fabs(static_cast<float>(pcmSamples_[sampleIdx + 1]) / 32768.0f);
                        a = (a + aR) * 0.5f;
                    }
                    if (a > maxAmp) maxAmp = a;
                }
            }
            float smoothed = waveformPeaks[b] * 0.35f + (std::min)(1.0f, maxAmp * 1.35f) * 0.65f;
            waveformPeaks[b] = std::clamp(smoothed, 0.02f, 1.0f);
        }
    }

    bool StartPlaybackThread(std::vector<int16_t>&& samples, uint32_t sr, uint16_t ch) {
        pcmSamples_ = std::move(samples);
        sampleRate = sr;
        channels = ch;
        bitsPerSample = 16;
        size_t totFrames = pcmSamples_.size() / ch;
        if (totFrames == 0) return false;

        totalFrames_.store(totFrames, std::memory_order_release);
        currentFrame_.store(0, std::memory_order_release);
        stopRequested_.store(false, std::memory_order_release);
        paused_.store(false, std::memory_order_release);
        playing_.store(true, std::memory_order_release);

        UpdateWaveformPeaksAtFrame(0);

        playbackThread_ = std::thread([this]() {
            WAVEFORMATEX wfx{};
            wfx.wFormatTag = WAVE_FORMAT_PCM;
            wfx.nChannels = channels;
            wfx.nSamplesPerSec = sampleRate;
            wfx.wBitsPerSample = 16;
            wfx.nBlockAlign = static_cast<WORD>(channels * 2);
            wfx.nAvgBytesPerSec = sampleRate * wfx.nBlockAlign;

            HWAVEOUT hWaveOut = nullptr;
            MMRESULT mmr = waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
            bool hasAudioDevice = (mmr == MMSYSERR_NOERROR && hWaveOut != nullptr);

            constexpr size_t kNumBuffers = 4;
            constexpr size_t kChunkFrames = 2048;
            size_t chunkBytes = kChunkFrames * wfx.nBlockAlign;

            std::array<std::vector<uint8_t>, kNumBuffers> buffers;
            std::array<WAVEHDR, kNumBuffers> headers{};

            if (hasAudioDevice) {
                for (size_t i = 0; i < kNumBuffers; ++i) {
                    buffers[i].resize(chunkBytes, 0);
                    headers[i].lpData = reinterpret_cast<LPSTR>(buffers[i].data());
                    headers[i].dwBufferLength = static_cast<DWORD>(chunkBytes);
                    waveOutPrepareHeader(hWaveOut, &headers[i], sizeof(WAVEHDR));
                    headers[i].dwFlags |= WHDR_DONE;
                }
            }

            size_t totFrames = totalFrames_.load(std::memory_order_acquire);

            while (!stopRequested_.load(std::memory_order_acquire)) {
                if (paused_.load(std::memory_order_acquire)) {
                    Sleep(15);
                    continue;
                }

                size_t curFr = currentFrame_.load(std::memory_order_acquire);
                if (curFr >= totFrames) {
                    break;
                }

                if (!hasAudioDevice) {
                    // Software clock fallback if waveOut device is unavailable on headless system
                    UpdateWaveformPeaksAtFrame(curFr);
                    Sleep(20);
                    size_t advance = (sampleRate * 20) / 1000;
                    currentFrame_.store((std::min)(totFrames, curFr + advance), std::memory_order_release);
                    continue;
                }

                bool queuedAny = false;
                for (size_t i = 0; i < kNumBuffers; ++i) {
                    if (stopRequested_.load(std::memory_order_acquire) ||
                        paused_.load(std::memory_order_acquire)) {
                        break;
                    }
                    if ((headers[i].dwFlags & WHDR_DONE) != 0) {
                        curFr = currentFrame_.load(std::memory_order_acquire);
                        if (curFr >= totFrames) break;

                        size_t framesToCopy = (std::min)(kChunkFrames, totFrames - curFr);
                        size_t bytesToCopy = framesToCopy * wfx.nBlockAlign;

                        std::memcpy(buffers[i].data(),
                                    pcmSamples_.data() + curFr * channels,
                                    bytesToCopy);
                        headers[i].dwBufferLength = static_cast<DWORD>(bytesToCopy);
                        headers[i].dwFlags &= ~WHDR_DONE;

                        if (waveOutWrite(hWaveOut, &headers[i], sizeof(WAVEHDR)) == MMSYSERR_NOERROR) {
                            UpdateWaveformPeaksAtFrame(curFr);
                            currentFrame_.store(curFr + framesToCopy, std::memory_order_release);
                            queuedAny = true;
                        } else {
                            headers[i].dwFlags |= WHDR_DONE;
                        }
                    }
                }

                if (!queuedAny) {
                    Sleep(8);
                }
            }

            if (hasAudioDevice) {
                if (!stopRequested_.load(std::memory_order_acquire)) {
                    // Wait briefly for remaining queued buffers to finish playing naturally
                    for (int waitIter = 0; waitIter < 50 && !stopRequested_.load(std::memory_order_acquire); ++waitIter) {
                        bool allDone = true;
                        for (size_t i = 0; i < kNumBuffers; ++i) {
                            if ((headers[i].dwFlags & WHDR_DONE) == 0) {
                                allDone = false;
                                break;
                            }
                        }
                        if (allDone) break;
                        Sleep(10);
                    }
                }
                waveOutReset(hWaveOut);
                for (size_t i = 0; i < kNumBuffers; ++i) {
                    waveOutUnprepareHeader(hWaveOut, &headers[i], sizeof(WAVEHDR));
                }
                waveOutClose(hWaveOut);
            }

            playing_.store(false, std::memory_order_release);
            paused_.store(false, std::memory_order_release);
        });

        return true;
    }

    static bool DecodeRiffWavMemory(std::span<const uint8_t> bytes,
                                    std::vector<int16_t>& outPcm,
                                    uint32_t& outSampleRate,
                                    uint16_t& outChannels) {
        outPcm.clear();
        if (bytes.size() < 44) return false;
        if (std::memcmp(bytes.data(), "RIFF", 4) != 0 ||
            std::memcmp(bytes.data() + 8, "WAVE", 4) != 0) {
            return false;
        }

        uint16_t audioFormat = 0;
        uint16_t numCh = 0;
        uint32_t sampRate = 0;
        uint16_t bitsPerSamp = 0;
        const uint8_t* dataPtr = nullptr;
        uint32_t dataSize = 0;

        size_t offset = 12;
        while (offset + 8 <= bytes.size()) {
            char chunkId[4];
            uint32_t chunkSize = 0;
            std::memcpy(chunkId, bytes.data() + offset, 4);
            std::memcpy(&chunkSize, bytes.data() + offset + 4, 4);
            offset += 8;

            if (offset + chunkSize > bytes.size()) {
                chunkSize = static_cast<uint32_t>(bytes.size() - offset);
            }

            if (std::memcmp(chunkId, "fmt ", 4) == 0 && chunkSize >= 16) {
                std::memcpy(&audioFormat, bytes.data() + offset + 0, 2);
                std::memcpy(&numCh, bytes.data() + offset + 2, 2);
                std::memcpy(&sampRate, bytes.data() + offset + 4, 4);
                std::memcpy(&bitsPerSamp, bytes.data() + offset + 14, 2);
            } else if (std::memcmp(chunkId, "data", 4) == 0) {
                dataPtr = bytes.data() + offset;
                dataSize = chunkSize;
                break;
            }

            offset += chunkSize + (chunkSize & 1u);
        }

        if (!dataPtr || dataSize == 0 || numCh == 0 || numCh > 8 || sampRate == 0) {
            return false;
        }

        outSampleRate = sampRate;
        outChannels = (std::min<uint16_t>)(numCh, 2);

        if (audioFormat == 1 && bitsPerSamp == 16) { // 16-bit PCM
            size_t totalSamples = dataSize / 2;
            size_t frames = totalSamples / numCh;
            outPcm.resize(frames * outChannels);
            const int16_t* src = reinterpret_cast<const int16_t*>(dataPtr);
            for (size_t f = 0; f < frames; ++f) {
                for (uint16_t c = 0; c < outChannels; ++c) {
                    outPcm[f * outChannels + c] = src[f * numCh + c];
                }
            }
            return true;
        } else if (audioFormat == 1 && bitsPerSamp == 8) { // 8-bit unsigned PCM
            size_t frames = dataSize / numCh;
            outPcm.resize(frames * outChannels);
            for (size_t f = 0; f < frames; ++f) {
                for (uint16_t c = 0; c < outChannels; ++c) {
                    int v = static_cast<int>(dataPtr[f * numCh + c]) - 128;
                    outPcm[f * outChannels + c] = static_cast<int16_t>(v * 256);
                }
            }
            return true;
        } else if (audioFormat == 3 && bitsPerSamp == 32) { // 32-bit IEEE float
            size_t frames = (dataSize / 4) / numCh;
            outPcm.resize(frames * outChannels);
            for (size_t f = 0; f < frames; ++f) {
                for (uint16_t c = 0; c < outChannels; ++c) {
                    float flt = 0.0f;
                    std::memcpy(&flt, dataPtr + (f * numCh + c) * 4, 4);
                    flt = std::clamp(flt, -1.0f, 1.0f);
                    outPcm[f * outChannels + c] = static_cast<int16_t>(flt * 32767.0f);
                }
            }
            return true;
        } else if (audioFormat == 1 && bitsPerSamp == 24) { // 24-bit PCM
            size_t frames = (dataSize / 3) / numCh;
            outPcm.resize(frames * outChannels);
            for (size_t f = 0; f < frames; ++f) {
                for (uint16_t c = 0; c < outChannels; ++c) {
                    const uint8_t* p = dataPtr + (f * numCh + c) * 3;
                    int16_t s16 = static_cast<int16_t>((p[2] << 8) | p[1]);
                    outPcm[f * outChannels + c] = s16;
                }
            }
            return true;
        }

        return false;
    }

    static bool ReadSourceReaderToPcm(IMFSourceReader* pReader,
                                      std::vector<int16_t>& outPcm,
                                      uint32_t& outSampleRate,
                                      uint16_t& outChannels) {
        if (!pReader) return false;

        pReader->SetStreamSelection(static_cast<DWORD>(MF_SOURCE_READER_ALL_STREAMS), FALSE);
        HRESULT hr = pReader->SetStreamSelection(static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), TRUE);
        if (FAILED(hr)) return false;

        IMFMediaType* pPartialType = nullptr;
        hr = MFCreateMediaType(&pPartialType);
        if (FAILED(hr) || !pPartialType) return false;

        pPartialType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
        pPartialType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
        pPartialType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16);

        hr = pReader->SetCurrentMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM),
                                          nullptr, pPartialType);
        SafeRelease(pPartialType);
        if (FAILED(hr)) return false;

        IMFMediaType* pUncompressedType = nullptr;
        hr = pReader->GetCurrentMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM),
                                          &pUncompressedType);
        if (FAILED(hr) || !pUncompressedType) return false;

        UINT32 ch = 2, sr = 44100;
        pUncompressedType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &ch);
        pUncompressedType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &sr);
        SafeRelease(pUncompressedType);

        outSampleRate = (sr > 0) ? sr : 44100;
        outChannels = static_cast<uint16_t>((ch > 0 && ch <= 8) ? ch : 2);

        outPcm.clear();
        outPcm.reserve(outSampleRate * outChannels * 10);
        constexpr size_t kMaxSamples = 32 * 1024 * 1024; // Cap at 64 MB PCM buffer

        while (outPcm.size() < kMaxSamples) {
            DWORD streamFlags = 0;
            IMFSample* pSample = nullptr;
            hr = pReader->ReadSample(static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM),
                                     0, nullptr, &streamFlags, nullptr, &pSample);
            if (FAILED(hr) || (streamFlags & MF_SOURCE_READERF_ENDOFSTREAM)) {
                SafeRelease(pSample);
                break;
            }
            if (pSample) {
                IMFMediaBuffer* pBuffer = nullptr;
                if (SUCCEEDED(pSample->ConvertToContiguousBuffer(&pBuffer)) && pBuffer) {
                    BYTE* pAudioData = nullptr;
                    DWORD cbBuffer = 0;
                    if (SUCCEEDED(pBuffer->Lock(&pAudioData, nullptr, &cbBuffer)) && pAudioData && cbBuffer >= 2) {
                        size_t numSamples = cbBuffer / 2;
                        size_t oldSz = outPcm.size();
                        outPcm.resize(oldSz + numSamples);
                        std::memcpy(outPcm.data() + oldSz, pAudioData, numSamples * 2);
                        pBuffer->Unlock();
                    }
                    SafeRelease(pBuffer);
                }
                SafeRelease(pSample);
            }
        }

        return !outPcm.empty();
    }

    static bool DecodeFileWithMediaFoundation(const std::wstring& path,
                                              std::vector<int16_t>& outPcm,
                                              uint32_t& outSampleRate,
                                              uint16_t& outChannels) {
        MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
        IMFSourceReader* pReader = nullptr;
        HRESULT hr = MFCreateSourceReaderFromURL(path.c_str(), nullptr, &pReader);
        bool ok = false;
        if (SUCCEEDED(hr) && pReader) {
            ok = ReadSourceReaderToPcm(pReader, outPcm, outSampleRate, outChannels);
            SafeRelease(pReader);
        }
        return ok;
    }

    static bool DecodeMemoryWithMediaFoundation(std::span<const uint8_t> bytes,
                                                std::vector<int16_t>& outPcm,
                                                uint32_t& outSampleRate,
                                                uint16_t& outChannels) {
        MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
        IStream* pMemStream = SHCreateMemStream(bytes.data(), static_cast<UINT>(bytes.size()));
        IMFByteStream* pByteStream = nullptr;
        IMFSourceReader* pReader = nullptr;
        bool ok = false;

        if (pMemStream) {
            if (SUCCEEDED(MFCreateMFByteStreamOnStream(pMemStream, &pByteStream)) && pByteStream) {
                if (SUCCEEDED(MFCreateSourceReaderFromByteStream(pByteStream, nullptr, &pReader)) && pReader) {
                    ok = ReadSourceReaderToPcm(pReader, outPcm, outSampleRate, outChannels);
                }
            }
        }

        SafeRelease(pReader);
        SafeRelease(pByteStream);
        SafeRelease(pMemStream);
        return ok;
    }
};

// ============================================================================
// 5. FAST EXECUTABLE & DLL INSPECTOR (ExecutableInspectInfo)
// ============================================================================

struct PeSectionInfo {
    std::wstring name;
    uint32_t virtualAddress = 0;
    uint32_t virtualSize = 0;
    uint32_t rawAddress = 0;
    uint32_t rawSize = 0;
    uint32_t characteristics = 0;
};

struct PeImportDllInfo {
    std::wstring dllName;
    bool foundOnSystem = false;
    std::wstring resolvedPath;
    uint32_t importedFunctionCount = 0;
};

struct ExecutableInspectInfo {
    bool valid = false;
    bool is64Bit = false;
    bool isDll = false;
    bool isXex2 = false;
    std::wstring formatName;
    std::wstring architecture;
    std::wstring subsystem;
    uint32_t compileTimestamp = 0;
    std::wstring compileTimeFormatted;
    uint32_t entryPointRva = 0;
    uint64_t imageBase = 0;
    uint32_t sizeOfImage = 0;
    uint32_t checksum = 0;
    std::vector<PeSectionInfo> sections;
    std::vector<PeImportDllInfo> importedDlls;
    std::vector<std::wstring> exportedFunctions;
};

namespace detail {

inline uint32_t SwapBE32(uint32_t v) {
    return ((v & 0x000000FFu) << 24) |
           ((v & 0x0000FF00u) << 8)  |
           ((v & 0x00FF0000u) >> 8)  |
           ((v & 0xFF000000u) >> 24);
}

inline std::wstring FormatUnixTimestampUtc(uint32_t ts) {
    if (ts == 0) return L"N/A";
    std::time_t rawTime = static_cast<std::time_t>(ts);
    std::tm utcTm{};
    if (gmtime_s(&utcTm, &rawTime) != 0) return L"Unknown";
    wchar_t buf[64] = {};
    wcsftime(buf, 64, L"%Y-%m-%d %H:%M:%S UTC", &utcTm);
    return buf;
}

inline bool CheckDllExistsOnSystem(const std::wstring& dllName,
                                   const std::wstring& binaryDir,
                                   std::wstring& outResolvedPath) {
    outResolvedPath.clear();
    if (dllName.empty()) return false;

    // API Set schema virtual DLLs are always resolved by the Windows loader
    if (_wcsnicmp(dllName.c_str(), L"api-ms-win-", 11) == 0 ||
        _wcsnicmp(dllName.c_str(), L"ext-ms-win-", 11) == 0) {
        outResolvedPath = L"(Windows API Set)";
        return true;
    }

    if (!binaryDir.empty()) {
        std::wstring localCandidate = binaryDir + dllName;
        if (PathFileExistsW(localCandidate.c_str())) {
            outResolvedPath = localCandidate;
            return true;
        }
    }

    wchar_t foundBuf[MAX_PATH] = {};
    DWORD res = SearchPathW(nullptr, dllName.c_str(), nullptr, MAX_PATH, foundBuf, nullptr);
    if (res > 0 && res < MAX_PATH) {
        outResolvedPath = foundBuf;
        return true;
    }
    return false;
}

} // namespace detail

inline ExecutableInspectInfo InspectBinaryHeader(std::span<const uint8_t> fileHeadBytes,
                                                 const std::wstring& filePath = L"") {
    ExecutableInspectInfo info{};

    std::vector<uint8_t> diskBuffer;
    std::span<const uint8_t> data = fileHeadBytes;

    // If caller passed a small head buffer and a valid filePath, read up to 1 MB so RVA tables are reachable
    if (data.size() < 65536 && !filePath.empty()) {
        HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_READ,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                   nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile != INVALID_HANDLE_VALUE) {
            LARGE_INTEGER fileSize{};
            if (GetFileSizeEx(hFile, &fileSize) && fileSize.QuadPart > 0) {
                DWORD toRead = static_cast<DWORD>((std::min<LONGLONG>)(fileSize.QuadPart, 1024 * 1024));
                diskBuffer.resize(toRead);
                DWORD bytesRead = 0;
                if (ReadFile(hFile, diskBuffer.data(), toRead, &bytesRead, nullptr) && bytesRead > 0) {
                    diskBuffer.resize(bytesRead);
                    data = diskBuffer;
                }
            }
            CloseHandle(hFile);
        }
    }

    if (data.size() < 32) return info;

    // Check Xbox 360 XEX2 header ("XEX2")
    if (std::memcmp(data.data(), "XEX2", 4) == 0) {
        info.valid = true;
        info.isXex2 = true;
        info.formatName = L"Xbox 360 XEX2";
        info.architecture = L"Xbox 360 PowerPC";
        info.subsystem = L"Xbox Title";

        uint32_t moduleFlags = 0, peDataOffset = 0, secInfoOffset = 0, optHeaderCount = 0;
        std::memcpy(&moduleFlags, data.data() + 4, 4);
        std::memcpy(&peDataOffset, data.data() + 8, 4);
        std::memcpy(&secInfoOffset, data.data() + 16, 4);
        std::memcpy(&optHeaderCount, data.data() + 20, 4);

        moduleFlags = detail::SwapBE32(moduleFlags);
        peDataOffset = detail::SwapBE32(peDataOffset);
        secInfoOffset = detail::SwapBE32(secInfoOffset);
        optHeaderCount = detail::SwapBE32(optHeaderCount);

        info.isDll = (moduleFlags & 0x00000008u) != 0;
        if (secInfoOffset + 8 <= data.size()) {
            uint32_t imgSz = 0;
            std::memcpy(&imgSz, data.data() + secInfoOffset + 4, 4);
            info.sizeOfImage = detail::SwapBE32(imgSz);
        }

        size_t hdrTablePos = 24;
        for (uint32_t i = 0; i < optHeaderCount && hdrTablePos + 8 <= data.size(); ++i, hdrTablePos += 8) {
            uint32_t key = 0, valOrOff = 0;
            std::memcpy(&key, data.data() + hdrTablePos, 4);
            std::memcpy(&valOrOff, data.data() + hdrTablePos + 4, 4);
            key = detail::SwapBE32(key);
            valOrOff = detail::SwapBE32(valOrOff);

            if (key == 0x00010100u) { // Entry point
                info.entryPointRva = valOrOff;
            } else if (key == 0x00010201u) { // Image base address
                info.imageBase = valOrOff;
            } else if (key == 0x00018002u && valOrOff + 8 <= data.size()) { // File timestamp
                uint32_t ts = 0;
                std::memcpy(&ts, data.data() + valOrOff + 4, 4);
                info.compileTimestamp = detail::SwapBE32(ts);
                info.compileTimeFormatted = detail::FormatUnixTimestampUtc(info.compileTimestamp);
            }
        }
        return info;
    }

    // Check DOS "MZ" header
    if (data[0] != 'M' || data[1] != 'Z' || data.size() < 64) {
        return info;
    }

    uint32_t peOffset = 0;
    std::memcpy(&peOffset, data.data() + 60, 4);
    if (peOffset < 64 || static_cast<size_t>(peOffset) + 24 > data.size()) {
        return info;
    }

    uint32_t peSig = 0;
    std::memcpy(&peSig, data.data() + peOffset, 4);
    if (peSig != 0x00004550u) { // "PE\0\0"
        return info;
    }

    const uint8_t* fileHdr = data.data() + peOffset + 4;
    uint16_t machine = 0, numSections = 0, sizeOfOptHdr = 0, characteristics = 0;
    uint32_t timeDateStamp = 0;
    std::memcpy(&machine, fileHdr + 0, 2);
    std::memcpy(&numSections, fileHdr + 2, 2);
    std::memcpy(&timeDateStamp, fileHdr + 4, 4);
    std::memcpy(&sizeOfOptHdr, fileHdr + 16, 2);
    std::memcpy(&characteristics, fileHdr + 18, 2);

    info.valid = true;
    info.compileTimestamp = timeDateStamp;
    info.compileTimeFormatted = detail::FormatUnixTimestampUtc(timeDateStamp);
    info.isDll = (characteristics & IMAGE_FILE_DLL) != 0;

    switch (machine) {
        case 0x8664: info.architecture = L"x64 (AMD64)"; break;
        case 0x014C: info.architecture = L"x86 (i386)"; break;
        case 0xAA64: info.architecture = L"ARM64"; break;
        case 0x01C0:
        case 0x01C4: info.architecture = L"ARM (Thumb-2)"; break;
        case 0x01F2: info.architecture = L"Xbox 360 PowerPC"; break;
        default:     info.architecture = L"Machine 0x" + std::to_wstring(machine); break;
    }

    size_t optHdrOffset = peOffset + 24;
    if (optHdrOffset + sizeOfOptHdr > data.size() || sizeOfOptHdr < 24) {
        return info;
    }

    const uint8_t* optHdr = data.data() + optHdrOffset;
    uint16_t optMagic = 0;
    std::memcpy(&optMagic, optHdr, 2);

    uint32_t importDirRva = 0, importDirSize = 0;
    uint32_t exportDirRva = 0, exportDirSize = 0;
    uint16_t subsysCode = 0;

    if (optMagic == 0x20Bu && sizeOfOptHdr >= 112) { // PE32+ (64-bit)
        info.is64Bit = true;
        info.formatName = info.isDll ? L"PE32+ DLL (64-bit)" : L"PE32+ Executable (64-bit)";
        std::memcpy(&info.entryPointRva, optHdr + 16, 4);
        std::memcpy(&info.imageBase, optHdr + 24, 8);
        std::memcpy(&info.sizeOfImage, optHdr + 56, 4);
        std::memcpy(&info.checksum, optHdr + 64, 4);
        std::memcpy(&subsysCode, optHdr + 68, 2);

        uint32_t numRvaAndSizes = 0;
        std::memcpy(&numRvaAndSizes, optHdr + 108, 4);
        if (numRvaAndSizes >= 1 && sizeOfOptHdr >= 120) {
            std::memcpy(&exportDirRva, optHdr + 112, 4);
            std::memcpy(&exportDirSize, optHdr + 116, 4);
        }
        if (numRvaAndSizes >= 2 && sizeOfOptHdr >= 128) {
            std::memcpy(&importDirRva, optHdr + 120, 4);
            std::memcpy(&importDirSize, optHdr + 124, 4);
        }
    } else if (optMagic == 0x10Bu && sizeOfOptHdr >= 96) { // PE32 (32-bit)
        info.is64Bit = false;
        info.formatName = info.isDll ? L"PE32 DLL (32-bit)" : L"PE32 Executable (32-bit)";
        uint32_t imgBase32 = 0;
        std::memcpy(&info.entryPointRva, optHdr + 16, 4);
        std::memcpy(&imgBase32, optHdr + 28, 4);
        info.imageBase = imgBase32;
        std::memcpy(&info.sizeOfImage, optHdr + 56, 4);
        std::memcpy(&info.checksum, optHdr + 64, 4);
        std::memcpy(&subsysCode, optHdr + 68, 2);

        uint32_t numRvaAndSizes = 0;
        std::memcpy(&numRvaAndSizes, optHdr + 92, 4);
        if (numRvaAndSizes >= 1 && sizeOfOptHdr >= 104) {
            std::memcpy(&exportDirRva, optHdr + 96, 4);
            std::memcpy(&exportDirSize, optHdr + 100, 4);
        }
        if (numRvaAndSizes >= 2 && sizeOfOptHdr >= 112) {
            std::memcpy(&importDirRva, optHdr + 104, 4);
            std::memcpy(&importDirSize, optHdr + 108, 4);
        }
    } else {
        info.formatName = L"PE Binary";
    }

    switch (subsysCode) {
        case 1:  info.subsystem = L"Native Driver"; break;
        case 2:  info.subsystem = L"Windows GUI"; break;
        case 3:  info.subsystem = L"Windows CUI"; break;
        case 9:  info.subsystem = L"Windows CE GUI"; break;
        case 10: info.subsystem = L"EFI Application"; break;
        case 11: info.subsystem = L"EFI Boot Service Driver"; break;
        case 12: info.subsystem = L"EFI Runtime Driver"; break;
        case 14: info.subsystem = L"Xbox Title"; break;
        case 16: info.subsystem = L"Windows Boot Application"; break;
        default: info.subsystem = L"Subsystem #" + std::to_wstring(subsysCode); break;
    }

    // Parse Section Headers
    size_t secTableOffset = optHdrOffset + sizeOfOptHdr;
    uint16_t maxSecs = (std::min<uint16_t>)(numSections, 96);
    info.sections.reserve(maxSecs);

    for (uint16_t s = 0; s < maxSecs; ++s) {
        size_t off = secTableOffset + static_cast<size_t>(s) * 40;
        if (off + 40 > data.size()) break;
        const uint8_t* sh = data.data() + off;

        char rawName[9] = {};
        std::memcpy(rawName, sh, 8);
        PeSectionInfo sec{};
        sec.name = Utf8ToWide(rawName);
        std::memcpy(&sec.virtualSize, sh + 8, 4);
        std::memcpy(&sec.virtualAddress, sh + 12, 4);
        std::memcpy(&sec.rawSize, sh + 16, 4);
        std::memcpy(&sec.rawAddress, sh + 20, 4);
        std::memcpy(&sec.characteristics, sh + 36, 4);
        info.sections.push_back(std::move(sec));
    }

    auto rvaToFileOffset = [&](uint32_t rva, size_t& outOffset) -> bool {
        if (rva == 0) return false;
        for (const auto& sec : info.sections) {
            uint32_t span = (std::max)(sec.virtualSize, sec.rawSize);
            if (rva >= sec.virtualAddress && rva < sec.virtualAddress + span) {
                uint32_t delta = rva - sec.virtualAddress;
                if (delta < sec.rawSize) {
                    outOffset = static_cast<size_t>(sec.rawAddress) + delta;
                    return outOffset < data.size();
                }
            }
        }
        if (rva < secTableOffset + maxSecs * 40 && rva < data.size()) {
            outOffset = rva;
            return true;
        }
        return false;
    };

    auto readAsciiAtRva = [&](uint32_t rva, size_t maxLen = 256) -> std::string {
        size_t off = 0;
        if (!rvaToFileOffset(rva, off)) return {};
        std::string s;
        while (off < data.size() && s.size() < maxLen) {
            char c = static_cast<char>(data[off++]);
            if (c == '\0') break;
            if (static_cast<unsigned char>(c) < 0x20 || static_cast<unsigned char>(c) > 0x7E) break;
            s.push_back(c);
        }
        return s;
    };

    std::wstring binaryDir;
    if (!filePath.empty()) {
        size_t slash = filePath.find_last_of(L"\\/");
        if (slash != std::wstring::npos) {
            binaryDir = filePath.substr(0, slash + 1);
        }
    }

    // Parse Imported DLLs
    if (importDirRva != 0) {
        size_t impOff = 0;
        if (rvaToFileOffset(importDirRva, impOff)) {
            for (size_t idx = 0; idx < 128 && impOff + 20 <= data.size(); ++idx, impOff += 20) {
                uint32_t origFirstThunk = 0, nameRva = 0, firstThunk = 0;
                std::memcpy(&origFirstThunk, data.data() + impOff + 0, 4);
                std::memcpy(&nameRva, data.data() + impOff + 12, 4);
                std::memcpy(&firstThunk, data.data() + impOff + 16, 4);
                if (origFirstThunk == 0 && nameRva == 0 && firstThunk == 0) break;

                std::string dllAscii = readAsciiAtRva(nameRva);
                if (dllAscii.empty()) continue;

                PeImportDllInfo imp{};
                imp.dllName = Utf8ToWide(dllAscii);
                imp.foundOnSystem = detail::CheckDllExistsOnSystem(imp.dllName, binaryDir, imp.resolvedPath);

                uint32_t thunkRva = (origFirstThunk != 0) ? origFirstThunk : firstThunk;
                size_t thunkOff = 0;
                if (rvaToFileOffset(thunkRva, thunkOff)) {
                    size_t ptrSz = info.is64Bit ? 8 : 4;
                    for (uint32_t t = 0; t < 4096 && thunkOff + ptrSz <= data.size(); ++t, thunkOff += ptrSz) {
                        uint64_t val = 0;
                        std::memcpy(&val, data.data() + thunkOff, ptrSz);
                        if (val == 0) break;
                        ++imp.importedFunctionCount;
                    }
                }

                info.importedDlls.push_back(std::move(imp));
            }
        }
    }

    // Parse up to 64 Exported Functions
    if (exportDirRva != 0) {
        size_t expOff = 0;
        if (rvaToFileOffset(exportDirRva, expOff) && expOff + 40 <= data.size()) {
            uint32_t numberOfNames = 0, addressOfNamesRva = 0;
            std::memcpy(&numberOfNames, data.data() + expOff + 24, 4);
            std::memcpy(&addressOfNamesRva, data.data() + expOff + 32, 4);

            size_t namesOff = 0;
            if (numberOfNames > 0 && rvaToFileOffset(addressOfNamesRva, namesOff)) {
                uint32_t count = (std::min<uint32_t>)(numberOfNames, 64u);
                info.exportedFunctions.reserve(count);
                for (uint32_t i = 0; i < count && namesOff + 4 <= data.size(); ++i, namesOff += 4) {
                    uint32_t fnNameRva = 0;
                    std::memcpy(&fnNameRva, data.data() + namesOff, 4);
                    std::string fnAscii = readAsciiAtRva(fnNameRva, 256);
                    if (!fnAscii.empty()) {
                        info.exportedFunctions.push_back(Utf8ToWide(fnAscii));
                    }
                }
            }
        }
    }

    return info;
}

// ============================================================================
// 6. HARDWARE-ACCELERATED FILE HASH CALCULATOR & FILE LOCK HUNTER
// ============================================================================

struct FileHashResult {
    bool success = false;
    uint32_t crc32 = 0;
    std::wstring crc32Hex;
    std::wstring md5Hex;
    std::wstring sha256Hex;
    uint64_t fileSize = 0;
};

namespace detail {

inline const std::array<uint32_t, 256>& GetCrc32Table() {
    static const std::array<uint32_t, 256> kTable = []() {
        std::array<uint32_t, 256> t{};
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k) {
                c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            }
            t[i] = c;
        }
        return t;
    }();
    return kTable;
}

inline std::wstring BytesToHexWide(const uint8_t* bytes, size_t len) {
    static constexpr wchar_t kHexChars[] = L"0123456789abcdef";
    std::wstring out;
    out.resize(len * 2);
    for (size_t i = 0; i < len; ++i) {
        out[i * 2 + 0] = kHexChars[(bytes[i] >> 4) & 0x0Fu];
        out[i * 2 + 1] = kHexChars[bytes[i] & 0x0Fu];
    }
    return out;
}

} // namespace detail

inline FileHashResult CalculateFileHashes(const std::wstring& filePath) {
    FileHashResult res{};
    if (filePath.empty()) return res;

    HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_READ,
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        return res;
    }

    LARGE_INTEGER fsize{};
    if (GetFileSizeEx(hFile, &fsize) && fsize.QuadPart >= 0) {
        res.fileSize = static_cast<uint64_t>(fsize.QuadPart);
    }

    BCRYPT_ALG_HANDLE hAlgMd5 = nullptr;
    BCRYPT_ALG_HANDLE hAlgSha256 = nullptr;
    BCRYPT_HASH_HANDLE hHashMd5 = nullptr;
    BCRYPT_HASH_HANDLE hHashSha256 = nullptr;

    if (BCryptOpenAlgorithmProvider(&hAlgMd5, BCRYPT_MD5_ALGORITHM, nullptr, 0) == 0) {
        BCryptCreateHash(hAlgMd5, &hHashMd5, nullptr, 0, nullptr, 0, 0);
    }
    if (BCryptOpenAlgorithmProvider(&hAlgSha256, BCRYPT_SHA256_ALGORITHM, nullptr, 0) == 0) {
        BCryptCreateHash(hAlgSha256, &hHashSha256, nullptr, 0, nullptr, 0, 0);
    }

    constexpr DWORD kChunkSize = 1024 * 1024; // 1 MB streaming buffer
    std::vector<uint8_t> buffer(kChunkSize);
    const auto& crcTable = detail::GetCrc32Table();
    uint32_t crc = 0xFFFFFFFFu;

    DWORD bytesRead = 0;
    bool readOk = true;
    while (true) {
        if (!ReadFile(hFile, buffer.data(), kChunkSize, &bytesRead, nullptr)) {
            readOk = false;
            break;
        }
        if (bytesRead == 0) break;

        for (DWORD i = 0; i < bytesRead; ++i) {
            crc = crcTable[(crc ^ buffer[i]) & 0xFFu] ^ (crc >> 8);
        }
        if (hHashMd5) {
            BCryptHashData(hHashMd5, buffer.data(), bytesRead, 0);
        }
        if (hHashSha256) {
            BCryptHashData(hHashSha256, buffer.data(), bytesRead, 0);
        }
    }

    CloseHandle(hFile);

    if (readOk) {
        res.crc32 = crc ^ 0xFFFFFFFFu;
        wchar_t crcBuf[16] = {};
        swprintf_s(crcBuf, L"%08x", res.crc32);
        res.crc32Hex = crcBuf;

        if (hHashMd5) {
            uint8_t md5Digest[16] = {};
            if (BCryptFinishHash(hHashMd5, md5Digest, sizeof(md5Digest), 0) == 0) {
                res.md5Hex = detail::BytesToHexWide(md5Digest, sizeof(md5Digest));
            }
        }
        if (hHashSha256) {
            uint8_t shaDigest[32] = {};
            if (BCryptFinishHash(hHashSha256, shaDigest, sizeof(shaDigest), 0) == 0) {
                res.sha256Hex = detail::BytesToHexWide(shaDigest, sizeof(shaDigest));
            }
        }
        res.success = true;
    }

    if (hHashMd5) BCryptDestroyHash(hHashMd5);
    if (hHashSha256) BCryptDestroyHash(hHashSha256);
    if (hAlgMd5) BCryptCloseAlgorithmProvider(hAlgMd5, 0);
    if (hAlgSha256) BCryptCloseAlgorithmProvider(hAlgSha256, 0);

    return res;
}

struct LockingProcessInfo {
    DWORD pid = 0;
    std::wstring processName;
    std::wstring applicationType;
    std::wstring executablePath;
    bool isService = false;
};

inline std::vector<LockingProcessInfo> FindLockingProcesses(const std::wstring& filePath) {
    std::vector<LockingProcessInfo> results;
    if (filePath.empty()) return results;

    DWORD dwSession = 0;
    WCHAR szSessionKey[CCH_RM_SESSION_KEY + 1] = {};
    if (RmStartSession(&dwSession, 0, szSessionKey) != ERROR_SUCCESS) {
        return results;
    }

    LPCWSTR rgsFileNames[] = { filePath.c_str() };
    if (RmRegisterResources(dwSession, 1, rgsFileNames, 0, nullptr, 0, nullptr) == ERROR_SUCCESS) {
        UINT nProcInfoNeeded = 0;
        UINT nProcInfo = 0;
        DWORD dwRebootReasons = RmRebootReasonNone;

        DWORD dwErr = RmGetList(dwSession, &nProcInfoNeeded, &nProcInfo, nullptr, &dwRebootReasons);
        if (dwErr == ERROR_MORE_DATA && nProcInfoNeeded > 0) {
            nProcInfo = nProcInfoNeeded + 4;
            std::vector<RM_PROCESS_INFO> procInfos(nProcInfo);
            dwErr = RmGetList(dwSession, &nProcInfoNeeded, &nProcInfo, procInfos.data(), &dwRebootReasons);
            if (dwErr == ERROR_SUCCESS) {
                results.reserve(nProcInfo);
                for (UINT i = 0; i < nProcInfo; ++i) {
                    const auto& pi = procInfos[i];
                    LockingProcessInfo info{};
                    info.pid = pi.Process.dwProcessId;
                    info.processName = pi.strAppName;
                    info.isService = (pi.ApplicationType == RmService);

                    switch (pi.ApplicationType) {
                        case RmMainWindow:  info.applicationType = L"GUI Application"; break;
                        case RmOtherWindow: info.applicationType = L"Windowed App"; break;
                        case RmService:     info.applicationType = L"Windows Service"; break;
                        case RmExplorer:    info.applicationType = L"Windows Explorer"; break;
                        case RmConsole:     info.applicationType = L"Console Process"; break;
                        case RmCritical:    info.applicationType = L"Critical System Process"; break;
                        default:            info.applicationType = L"Background Process"; break;
                    }

                    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, info.pid);
                    if (hProc) {
                        wchar_t exeBuf[MAX_PATH] = {};
                        DWORD cch = MAX_PATH;
                        if (QueryFullProcessImageNameW(hProc, 0, exeBuf, &cch) && cch > 0) {
                            info.executablePath = exeBuf;
                        }
                        CloseHandle(hProc);
                    }

                    results.push_back(std::move(info));
                }
            }
        }
    }

    RmEndSession(dwSession);
    return results;
}

inline bool TerminateLockingProcess(DWORD pid) {
    if (pid == 0 || pid == GetCurrentProcessId()) return false;
    HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!hProc) return false;
    BOOL ok = TerminateProcess(hProc, 1);
    CloseHandle(hProc);
    return ok != FALSE;
}

} // namespace speta

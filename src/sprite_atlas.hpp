#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d2d1.h>
#include <wincodec.h>
#include <shlwapi.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <algorithm>
#include <cstdint>
#include <cmath>

namespace speta {

enum class SpriteId {
    AudioSpeaker,
    BlueFolder,
    CrtMonitor,
    CrtTerminal,
    FloppyDisk,
    GbaCartridge,
    GbaConsole,
    GreenChip,
    GreenChip24,
    GreenChipClean,
    IaChip,
    IaChipTrans,
    Paintbrush,
    PortalWormhole,
    RadarSonar,
    RadioTower,
    SaturnFolder,
    SaturnFolder24,
    SaturnFolderClean,
    ServerRack,
    TerminalWindow,
    WadDoc,
    WadDocTrans
};

inline const char* GetSpriteFilename(SpriteId id) {
    switch (id) {
    case SpriteId::AudioSpeaker:      return "audio_speaker.png";
    case SpriteId::BlueFolder:        return "blue_folder.png";
    case SpriteId::CrtMonitor:        return "crt_monitor.png";
    case SpriteId::CrtTerminal:       return "crt_terminal.png";
    case SpriteId::FloppyDisk:        return "floppy_disk.png";
    case SpriteId::GbaCartridge:      return "gba_cartridge.png";
    case SpriteId::GbaConsole:        return "gba_console.png";
    case SpriteId::GreenChip:         return "green_chip.png";
    case SpriteId::GreenChip24:       return "green_chip_24.png";
    case SpriteId::GreenChipClean:    return "green_chip_clean.png";
    case SpriteId::IaChip:            return "ia_chip.png";
    case SpriteId::IaChipTrans:       return "ia_chip_trans.png";
    case SpriteId::Paintbrush:        return "paintbrush.png";
    case SpriteId::PortalWormhole:    return "portal_wormhole.png";
    case SpriteId::RadarSonar:        return "radar_sonar.png";
    case SpriteId::RadioTower:        return "radio_tower.png";
    case SpriteId::SaturnFolder:      return "saturn_folder.png";
    case SpriteId::SaturnFolder24:    return "saturn_folder_24.png";
    case SpriteId::SaturnFolderClean: return "saturn_folder_clean.png";
    case SpriteId::ServerRack:        return "server_rack.png";
    case SpriteId::TerminalWindow:    return "terminal_window.png";
    case SpriteId::WadDoc:            return "wad_doc.png";
    case SpriteId::WadDocTrans:       return "wad_doc_trans.png";
    default:                          return "terminal_window.png";
    }
}

class SpriteAtlas {
public:
    static SpriteAtlas& Instance() {
        static SpriteAtlas instance;
        return instance;
    }

    static SpriteAtlas& Get() {
        return Instance();
    }

    size_t GetSpriteCount() const {
        return 23;
    }

    void Initialize(IWICImagingFactory* pWic) {
        pWicFactory = pWic;
    }

    static std::wstring ResolveSpritePath(const std::string& name) {
        std::wstring wname(name.begin(), name.end());

        // 1. Check relative to executable directory assets/win98 and assets/spet_sprites
        wchar_t exePath[MAX_PATH] = { 0 };
        if (GetModuleFileNameW(nullptr, exePath, MAX_PATH) > 0) {
            PathRemoveFileSpecW(exePath);
            std::wstring candidateWin98 = std::wstring(exePath) + L"\\assets\\win98\\" + wname;
            if (PathFileExistsW(candidateWin98.c_str())) return candidateWin98;
            std::wstring candidateSpet = std::wstring(exePath) + L"\\assets\\spet_sprites\\" + wname;
            if (PathFileExistsW(candidateSpet.c_str())) return candidateSpet;
        }

        // 2. Check relative to current working directory
        std::wstring cwdWin98 = L"assets\\win98\\" + wname;
        if (PathFileExistsW(cwdWin98.c_str())) return cwdWin98;

        std::wstring cwdCandidate = L"assets\\spet_sprites\\" + wname;
        if (PathFileExistsW(cwdCandidate.c_str())) return cwdCandidate;

        // 3. Fallback to original portfolio assets folder
        std::wstring fallback = L"C:\\Users\\josem\\Desktop\\Portfolio\\Portfolio web\\assets\\icons\\spet_sprites\\" + wname;
        if (PathFileExistsW(fallback.c_str())) return fallback;

        return cwdWin98;
    }

    static bool DeCheckerboardRGBA(uint8_t* pixels, uint32_t width, uint32_t height, const std::string& filename = "crt_terminal.png") {
        if (!pixels || width == 0 || height == 0) return false;

        bool isCleanBorderCrop = (filename.find("saturn_folder_clean") != std::string::npos ||
                                  filename.find("green_chip_clean") != std::string::npos);

        if (isCleanBorderCrop) {
            for (uint32_t y = 0; y < height; ++y) {
                for (uint32_t x = 0; x < width; ++x) {
                    if (x < 14 || x >= width - 14 || y < 14 || y >= height - 14) {
                        size_t idx = (static_cast<size_t>(y) * width + x) * 4;
                        pixels[idx + 0] = 0;
                        pixels[idx + 1] = 0;
                        pixels[idx + 2] = 0;
                        pixels[idx + 3] = 0;
                    }
                }
            }
        }

        bool isBigCheckerboard = (filename.find("crt_terminal") != std::string::npos ||
                                  filename.find("ia_chip") != std::string::npos ||
                                  filename.find("gba_console") != std::string::npos);

        if (isBigCheckerboard) {
            std::vector<bool> visited(static_cast<size_t>(width) * height, false);
            std::queue<std::pair<uint32_t, uint32_t>> q;

            auto isRemovable = [&](uint32_t x, uint32_t y) -> bool {
                size_t idx = (static_cast<size_t>(y) * width + x) * 4;
                uint8_t b = pixels[idx + 0];
                uint8_t g = pixels[idx + 1];
                uint8_t r = pixels[idx + 2];
                uint8_t a = pixels[idx + 3];
                if (a == 0) return true;

                int mx = (std::max)(static_cast<int>(r), (std::max)(static_cast<int>(g), static_cast<int>(b)));
                int mn = (std::min)(static_cast<int>(r), (std::min)(static_cast<int>(g), static_cast<int>(b)));

                // Check outer boundary (corners and top/bottom edges)
                bool isOuter = (x < 42 || x >= width - 42 || y < 35 || y >= height - 35);
                if (isOuter) {
                    // Outer dark navy / blue-gray anti-aliased edge or background corner
                    if (r <= 85 && g <= 105 && b >= 45) return true;
                    // Neutral/light gray or white checkerboard in outer area
                    if ((mx - mn) < 28 && mx > 135) return true;
                }

                // Neutral/light gray or white checkerboard anywhere
                if ((mx - mn) < 24 && mx > 145) return true;

                return false;
            };

            auto tryEnqueue = [&](uint32_t x, uint32_t y) {
                size_t lin = static_cast<size_t>(y) * width + x;
                if (!visited[lin] && isRemovable(x, y)) {
                    visited[lin] = true;
                    q.push({x, y});
                    size_t idx = lin * 4;
                    pixels[idx + 0] = 0;
                    pixels[idx + 1] = 0;
                    pixels[idx + 2] = 0;
                    pixels[idx + 3] = 0;
                }
            };

            for (uint32_t x = 0; x < width; ++x) {
                tryEnqueue(x, 0);
                tryEnqueue(x, height - 1);
            }
            for (uint32_t y = 0; y < height; ++y) {
                tryEnqueue(0, y);
                tryEnqueue(width - 1, y);
            }

            const int dx[4] = { 1, -1, 0, 0 };
            const int dy[4] = { 0, 0, 1, -1 };

            while (!q.empty()) {
                auto [cx, cy] = q.front();
                q.pop();

                for (int i = 0; i < 4; ++i) {
                    int nx = static_cast<int>(cx) + dx[i];
                    int ny = static_cast<int>(cy) + dy[i];
                    if (nx >= 0 && nx < static_cast<int>(width) && ny >= 0 && ny < static_cast<int>(height)) {
                        uint32_t unx = static_cast<uint32_t>(nx);
                        uint32_t uny = static_cast<uint32_t>(ny);
                        size_t lin = static_cast<size_t>(uny) * width + unx;
                        if (!visited[lin] && isRemovable(unx, uny)) {
                            visited[lin] = true;
                            size_t idx = lin * 4;
                            pixels[idx + 0] = 0;
                            pixels[idx + 1] = 0;
                            pixels[idx + 2] = 0;
                            pixels[idx + 3] = 0;
                            q.push({unx, uny});
                        }
                    }
                }
            }
        }

        // Premultiply B, G, R by Alpha for ALL sprites so pixels with A=0 NEVER leak hidden RGB checkerboard values!
        for (size_t i = 0; i < static_cast<size_t>(width) * height; ++i) {
            size_t idx = i * 4;
            uint8_t a = pixels[idx + 3];
            if (a == 0) {
                pixels[idx + 0] = 0;
                pixels[idx + 1] = 0;
                pixels[idx + 2] = 0;
            } else if (a < 255) {
                pixels[idx + 0] = static_cast<uint8_t>((static_cast<uint32_t>(pixels[idx + 0]) * a + 127) / 255);
                pixels[idx + 1] = static_cast<uint8_t>((static_cast<uint32_t>(pixels[idx + 1]) * a + 127) / 255);
                pixels[idx + 2] = static_cast<uint8_t>((static_cast<uint32_t>(pixels[idx + 2]) * a + 127) / 255);
            }
        }

        return true;
    }

    static bool DeCheckerboardRGBA(std::vector<uint8_t>& pixels, uint32_t width, uint32_t height, const std::string& filename = "crt_terminal.png") {
        if (pixels.empty() || width == 0 || height == 0) return false;
        return DeCheckerboardRGBA(pixels.data(), width, height, filename);
    }

    static bool LoadImagePixels(
        const std::wstring& path,
        std::vector<uint8_t>& outPixels,
        uint32_t& outW,
        uint32_t& outH,
        IWICImagingFactory* pFactory = nullptr
    ) {
        if (!PathFileExistsW(path.c_str())) return false;
        IWICImagingFactory* factory = pFactory;
        bool localFactory = false;
        if (!factory) {
            HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
            if (FAILED(hr) || !factory) return false;
            localFactory = true;
        }

        IWICBitmapDecoder* pDecoder = nullptr;
        HRESULT hr = factory->CreateDecoderFromFilename(
            path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &pDecoder
        );
        if (FAILED(hr) || !pDecoder) {
            if (localFactory) factory->Release();
            return false;
        }

        IWICBitmapFrameDecode* pFrame = nullptr;
        hr = pDecoder->GetFrame(0, &pFrame);
        if (FAILED(hr) || !pFrame) {
            pDecoder->Release();
            if (localFactory) factory->Release();
            return false;
        }

        IWICFormatConverter* pConv = nullptr;
        hr = factory->CreateFormatConverter(&pConv);
        if (FAILED(hr) || !pConv) {
            pFrame->Release();
            pDecoder->Release();
            if (localFactory) factory->Release();
            return false;
        }

        hr = pConv->Initialize(
            pFrame, GUID_WICPixelFormat32bppBGRA,
            WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom
        );
        if (FAILED(hr)) {
            pConv->Release();
            pFrame->Release();
            pDecoder->Release();
            if (localFactory) factory->Release();
            return false;
        }

        UINT w = 0, h = 0;
        pConv->GetSize(&w, &h);
        if (w == 0 || h == 0) {
            pConv->Release();
            pFrame->Release();
            pDecoder->Release();
            if (localFactory) factory->Release();
            return false;
        }

        outW = w;
        outH = h;
        outPixels.resize(static_cast<size_t>(w) * h * 4);
        hr = pConv->CopyPixels(nullptr, w * 4, static_cast<UINT>(outPixels.size()), outPixels.data());

        pConv->Release();
        pFrame->Release();
        pDecoder->Release();
        if (localFactory) factory->Release();

        return SUCCEEDED(hr);
    }

    ID2D1Bitmap* GetBitmap(ID2D1RenderTarget* pTarget, const std::string& name) {
        if (!pTarget) return nullptr;

        auto it = cachedBitmaps.find(name);
        if (it != cachedBitmaps.end() && it->second) {
            return it->second;
        }

        if (!pWicFactory) {
            CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pWicFactory));
            if (!pWicFactory) return nullptr;
        }

        std::wstring resolved = ResolveSpritePath(name);
        std::vector<uint8_t> pixels;
        uint32_t w = 0, h = 0;
        if (!LoadImagePixels(resolved, pixels, w, h, pWicFactory)) return nullptr;

        DeCheckerboardRGBA(pixels, w, h, name);

        ID2D1Bitmap* pBitmap = nullptr;
        D2D1_BITMAP_PROPERTIES props = D2D1::BitmapProperties(
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
        );
        HRESULT hr = pTarget->CreateBitmap(D2D1::SizeU(w, h), pixels.data(), w * 4, props, &pBitmap);
        if (SUCCEEDED(hr) && pBitmap) {
            cachedBitmaps[name] = pBitmap;
            return pBitmap;
        }

        return nullptr;
    }

    ID2D1Bitmap* GetBitmap(ID2D1RenderTarget* pTarget, SpriteId id) {
        return GetBitmap(pTarget, GetSpriteFilename(id));
    }

    void DrawSprite(ID2D1RenderTarget* pTarget, const std::string& name, const D2D1_RECT_F& dstRect, float opacity = 1.0f) {
        if (!pTarget) return;
        ID2D1Bitmap* bmp = GetBitmap(pTarget, name);
        if (!bmp) return;

        bool isSmoothKeycap = (name.rfind("key_", 0) == 0);
        D2D1_BITMAP_INTERPOLATION_MODE mode = isSmoothKeycap
            ? D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
            : D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR;
        pTarget->DrawBitmap(bmp, dstRect, opacity, mode, nullptr);
    }

    void DrawSprite(ID2D1RenderTarget* pTarget, SpriteId id, const D2D1_RECT_F& dstRect, float opacity = 1.0f) {
        DrawSprite(pTarget, GetSpriteFilename(id), dstRect, opacity);
    }

    void DrawSpriteCentered(ID2D1RenderTarget* pTarget, const std::string& name, const D2D1_RECT_F& container, float sw, float sh, float opacity = 1.0f) {
        float cx = (container.left + container.right) * 0.5f;
        float cy = (container.top + container.bottom) * 0.5f;
        D2D1_RECT_F dst = D2D1::RectF(
            std::floor(cx - sw * 0.5f),
            std::floor(cy - sh * 0.5f),
            std::floor(cx - sw * 0.5f) + sw,
            std::floor(cy - sh * 0.5f) + sh
        );
        DrawSprite(pTarget, name, dst, opacity);
    }

    void DrawSpriteCentered(ID2D1RenderTarget* pTarget, SpriteId id, const D2D1_RECT_F& container, float sw, float sh, float opacity = 1.0f) {
        DrawSpriteCentered(pTarget, GetSpriteFilename(id), container, sw, sh, opacity);
    }

    float DrawKeycap(ID2D1RenderTarget* pTarget, const std::string& keyFile, float x, float y, float size = 20.0f, float opacity = 1.0f) {
        if (!pTarget) return 0.0f;
        std::string fn = (keyFile.rfind("key_", 0) == 0) ? keyFile : ("key_" + keyFile);
        if (fn.find(".png") == std::string::npos) fn += ".png";
        D2D1_RECT_F dst = D2D1::RectF(std::floor(x), std::floor(y), std::floor(x) + size, std::floor(y) + size);
        DrawSprite(pTarget, fn, dst, opacity);
        return size;
    }

    float DrawKeyCombo(ID2D1RenderTarget* pTarget, const std::vector<std::string>& keys, float x, float y, float size = 20.0f, float opacity = 1.0f) {
        float curX = x;
        for (size_t i = 0; i < keys.size(); ++i) {
            curX += DrawKeycap(pTarget, keys[i], curX, y, size, opacity);
            if (i + 1 < keys.size()) {
                curX += 1.0f;
            }
        }
        return curX - x;
    }

    void Render(ID2D1RenderTarget* pTarget, const std::string& name, float x, float y, float w, float h) {
        DrawSprite(pTarget, name, D2D1::RectF(x, y, x + w, y + h));
    }

    void Render(ID2D1RenderTarget* pTarget, SpriteId id, float x, float y, float w, float h) {
        DrawSprite(pTarget, id, D2D1::RectF(x, y, x + w, y + h));
    }

    void DiscardDeviceResources() {
        for (auto& [k, bmp] : cachedBitmaps) {
            if (bmp) {
                bmp->Release();
                bmp = nullptr;
            }
        }
        cachedBitmaps.clear();
    }

    void Shutdown() {
        DiscardDeviceResources();
        if (pWicFactory) {
            pWicFactory->Release();
            pWicFactory = nullptr;
        }
    }

    ~SpriteAtlas() {
        DiscardDeviceResources();
    }

private:
    SpriteAtlas() = default;
    IWICImagingFactory* pWicFactory = nullptr;
    std::unordered_map<std::string, ID2D1Bitmap*> cachedBitmaps;
};

} // namespace speta

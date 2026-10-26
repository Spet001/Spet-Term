#pragma once

#include "common.hpp"
#include <d3d11.h>
#include <d3d10_1.h>
#include <dxgi1_2.h>
#include <d2d1_1.h>
#include <wincodec.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <propvarutil.h>
#include <shlwapi.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>

namespace speta {

enum class WallpaperType {
    None,
    Static,
    Gif,
    Video
};

inline D2D1_RECT_F CalculateCoverRect(float winW, float winH, float imgW, float imgH) {
    if (winW <= 0.0f || winH <= 0.0f || imgW <= 0.0f || imgH <= 0.0f) {
        return D2D1::RectF(0.0f, 0.0f, (std::max)(0.0f, winW), (std::max)(0.0f, winH));
    }
    float scaleX = winW / imgW;
    float scaleY = winH / imgH;
    float scale = (scaleX > scaleY) ? scaleX : scaleY;
    float dstW = imgW * scale;
    float dstH = imgH * scale;
    float left = (winW - dstW) * 0.5f;
    float top = (winH - dstH) * 0.5f;
    return D2D1::RectF(left, top, left + dstW, top + dstH);
}

inline WallpaperType DetectWallpaperType(const std::wstring& path) {
    if (path.empty() || _wcsicmp(path.c_str(), L"none") == 0) return WallpaperType::None;
    size_t dot = path.find_last_of(L'.');
    if (dot == std::wstring::npos) return WallpaperType::None;
    std::wstring ext = path.substr(dot);
    for (auto& c : ext) c = static_cast<wchar_t>(towlower(c));

    if (ext == L".png" || ext == L".jpg" || ext == L".jpeg" || ext == L".bmp" || ext == L".webp") {
        return WallpaperType::Static;
    }
    if (ext == L".gif") {
        return WallpaperType::Gif;
    }
    if (ext == L".mp4" || ext == L".webm" || ext == L".mov" || ext == L".mkv" || ext == L".avi") {
        return WallpaperType::Video;
    }
    return WallpaperType::None;
}

struct WallpaperItem {
    std::wstring path;
    std::wstring displayName;
    WallpaperType type = WallpaperType::None;
    std::wstring thumbPath;
};

inline std::wstring GetSampleFriendlyName(const std::wstring& rawPath) {
    if (rawPath.empty() || _wcsicmp(rawPath.c_str(), L"none") == 0) return L"None";
    wchar_t fname[MAX_PATH];
    wcscpy_s(fname, rawPath.c_str());
    PathStripPathW(fname);
    std::wstring fn = fname;
    for (auto& c : fn) c = static_cast<wchar_t>(towlower(c));
    if (fn == L"galaxy_within.mp4" || fn == L"galaxy_within") return L"Anime (Galaxy Within)";
    if (fn == L"singularity.mp4" || fn == L"singularity") return L"Galaxy (Singularity)";
    if (fn == L"cafe.mp4" || fn == L"cafe") return L"Anime (Cafe)";
    if (fn == L"dark_souls_3.mp4" || fn == L"dark_souls_3") return L"Dark Souls 3";
    if (fn == L"overwatch_hanamura.mp4" || fn == L"overwatch_hanamura") return L"Overwatch (Hanamura)";
    if (fn == L"cod_blood_of_the_dead.mp4" || fn == L"cod_blood_of_the_dead") return L"COD (Blood of the Dead)";

    size_t dot = fn.find_last_of(L'.');
    if (dot != std::wstring::npos) return fn.substr(0, dot);
    return fname;
}

inline std::wstring ResolveWallpaperPath(const std::wstring& rawPath, const std::wstring& baseDir) {
    if (rawPath.empty() || _wcsicmp(rawPath.c_str(), L"none") == 0) return L"";
    if (PathFileExistsW(rawPath.c_str())) return rawPath;

    std::wstring combined = baseDir + L"\\" + rawPath;
    if (PathFileExistsW(combined.c_str())) return combined;

    wchar_t cwd[MAX_PATH];
    GetCurrentDirectoryW(MAX_PATH, cwd);
    std::wstring fromCwd = std::wstring(cwd) + L"\\" + rawPath;
    if (PathFileExistsW(fromCwd.c_str())) return fromCwd;

    return rawPath;
}

inline std::vector<WallpaperItem> DiscoverWallpapers(const std::wstring& baseDir, const std::vector<std::wstring>& customWallpapers) {
    std::vector<WallpaperItem> result;

    auto ScanDir = [&](const std::wstring& dir, bool isSampleDir) {
        if (!PathFileExistsW(dir.c_str())) return;
        WIN32_FIND_DATAW fd;
        std::wstring pattern = dir + L"\\*.*";
        HANDLE hFind = FindFirstFileW(pattern.c_str(), &fd);
        if (hFind == INVALID_HANDLE_VALUE) return;
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                WallpaperType t = DetectWallpaperType(fd.cFileName);
                if (t != WallpaperType::None) {
                    WallpaperItem item;
                    item.path = dir + L"\\" + fd.cFileName;
                    item.type = t;
                    item.displayName = isSampleDir ? GetSampleFriendlyName(fd.cFileName) : GetSampleFriendlyName(fd.cFileName);
                    result.push_back(item);
                }
            }
        } while (FindNextFileW(hFind, &fd));
        FindClose(hFind);
    };

    ScanDir(baseDir + L"\\assets\\wallpapers\\samples", true);
    if (result.empty()) {
        ScanDir(L"assets\\wallpapers\\samples", true);
    }

    ScanDir(baseDir + L"\\assets\\wallpapers", false);
    if (result.empty()) {
        ScanDir(L"assets\\wallpapers", false);
    }

    for (const auto& cw : customWallpapers) {
        if (PathFileExistsW(cw.c_str())) {
            WallpaperType t = DetectWallpaperType(cw);
            if (t != WallpaperType::None) {
                bool exists = false;
                for (const auto& existing : result) {
                    if (_wcsicmp(existing.path.c_str(), cw.c_str()) == 0) {
                        exists = true;
                        break;
                    }
                }
                if (!exists) {
                    WallpaperItem item;
                    item.path = cw;
                    item.type = t;
                    wchar_t fname[MAX_PATH];
                    wcscpy_s(fname, cw.c_str());
                    PathStripPathW(fname);
                    item.displayName = GetSampleFriendlyName(fname);
                    result.push_back(item);
                }
            }
        }
    }

    return result;
}

class WallpaperEngine {
public:
    WallpaperEngine() = default;
    ~WallpaperEngine() {
        Shutdown();
    }

    void Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, ID2D1DeviceContext* pD2DContext, IWICImagingFactory* pWicFactory, HWND hwnd = nullptr) {
        m_pD3DDevice = pDevice;
        m_pD3DContext = pContext;
        m_pD2DContext = pD2DContext;
        m_pWicFactory = pWicFactory;
        m_hwnd = hwnd;
    }

    void SetHwnd(HWND hwnd) {
        m_hwnd = hwnd;
    }

    void Shutdown() {
        UnloadCurrent();
    }

    bool Load(const std::wstring& path) {
        UnloadCurrent();

        if (path.empty() || _wcsicmp(path.c_str(), L"none") == 0) {
            m_currentType = WallpaperType::None;
            m_currentPath.clear();
            return true;
        }

        if (!PathFileExistsW(path.c_str())) {
            m_currentType = WallpaperType::None;
            m_currentPath.clear();
            return false;
        }

        m_currentType = DetectWallpaperType(path);
        m_currentPath = path;

        switch (m_currentType) {
        case WallpaperType::Static:
            return LoadStatic(path);
        case WallpaperType::Gif:
            return LoadGif(path);
        case WallpaperType::Video:
            return LoadVideo(path);
        default:
            m_currentType = WallpaperType::None;
            m_currentPath.clear();
            return false;
        }
    }

    void Tick() {
        if (m_paused) return;

        if (m_currentType == WallpaperType::Gif && !m_gifFrames.empty() && m_gifTotalDurationMs > 0) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_gifStartTime).count();
            int curTime = static_cast<int>(elapsed % m_gifTotalDurationMs);

            int accum = 0;
            for (size_t i = 0; i < m_gifFrames.size(); ++i) {
                accum += m_gifFrames[i].delayMs;
                if (curTime < accum) {
                    m_gifActiveIndex = i;
                    break;
                }
            }
        }
    }

    ID2D1Bitmap* GetCurrentBitmap() {
        switch (m_currentType) {
        case WallpaperType::Static:
            return m_pStaticBitmap;
        case WallpaperType::Gif:
            if (!m_gifFrames.empty() && m_gifActiveIndex < m_gifFrames.size()) {
                return m_gifFrames[m_gifActiveIndex].pBitmap;
            }
            return nullptr;
        case WallpaperType::Video:
            return m_pVideoBitmap;
        default:
            return nullptr;
        }
    }

    void Pause() {
        m_paused = true;
    }

    void Resume() {
        m_paused = false;
        if (m_currentType == WallpaperType::Gif) {
            m_gifStartTime = std::chrono::steady_clock::now();
        }
    }

    bool IsPaused() const {
        return m_paused;
    }

    WallpaperType GetCurrentType() const {
        return m_currentType;
    }

    const std::wstring& GetCurrentPath() const {
        return m_currentPath;
    }

private:
    struct GifFrame {
        ID2D1Bitmap* pBitmap = nullptr;
        int delayMs = 100;
    };

    void UnloadCurrent() {
        // Stop video thread if active
        if (m_videoRunning) {
            m_videoStop = true;
            if (m_videoThread.joinable()) {
                m_videoThread.join();
            }
            m_videoRunning = false;
        }

        SafeRelease(m_pVideoBitmap);
        SafeRelease(m_pVideoDestTexture);
        SafeRelease(m_pSourceReader);
        SafeRelease(m_pDXGIManager);

        if (m_mfStarted) {
            MFShutdown();
            m_mfStarted = false;
        }

        // Clean static bitmap
        SafeRelease(m_pStaticBitmap);

        // Clean GIF frames
        for (auto& f : m_gifFrames) {
            SafeRelease(f.pBitmap);
        }
        m_gifFrames.clear();
        m_gifTotalDurationMs = 0;
        m_gifActiveIndex = 0;

        m_currentType = WallpaperType::None;
        m_currentPath.clear();
    }

    bool LoadStatic(const std::wstring& path) {
        if (!m_pD2DContext || !m_pWicFactory) return false;

        IWICBitmapDecoder* pDecoder = nullptr;
        HRESULT hr = m_pWicFactory->CreateDecoderFromFilename(
            path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &pDecoder
        );
        if (FAILED(hr) || !pDecoder) return false;

        IWICBitmapFrameDecode* pFrame = nullptr;
        hr = pDecoder->GetFrame(0, &pFrame);
        if (FAILED(hr) || !pFrame) {
            pDecoder->Release();
            return false;
        }

        IWICFormatConverter* pConverter = nullptr;
        hr = m_pWicFactory->CreateFormatConverter(&pConverter);
        if (SUCCEEDED(hr) && pConverter) {
            hr = pConverter->Initialize(
                pFrame, GUID_WICPixelFormat32bppPBGRA,
                WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom
            );
            if (SUCCEEDED(hr)) {
                hr = m_pD2DContext->CreateBitmapFromWicBitmap(pConverter, nullptr, &m_pStaticBitmap);
            }
            pConverter->Release();
        }

        pFrame->Release();
        pDecoder->Release();

        return SUCCEEDED(hr) && (m_pStaticBitmap != nullptr);
    }

    bool LoadGif(const std::wstring& path) {
        if (!m_pD2DContext || !m_pWicFactory) return false;

        IWICBitmapDecoder* pDecoder = nullptr;
        HRESULT hr = m_pWicFactory->CreateDecoderFromFilename(
            path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &pDecoder
        );
        if (FAILED(hr) || !pDecoder) return false;

        UINT frameCount = 0;
        hr = pDecoder->GetFrameCount(&frameCount);
        if (FAILED(hr) || frameCount == 0) {
            pDecoder->Release();
            return false;
        }

        // Limit frame count if excessively large
        frameCount = (std::min)(frameCount, 120u);

        m_gifTotalDurationMs = 0;
        for (UINT i = 0; i < frameCount; ++i) {
            IWICBitmapFrameDecode* pFrame = nullptr;
            if (FAILED(pDecoder->GetFrame(i, &pFrame)) || !pFrame) continue;

            int delayMs = 100;
            IWICMetadataQueryReader* pReader = nullptr;
            if (SUCCEEDED(pFrame->GetMetadataQueryReader(&pReader)) && pReader) {
                PROPVARIANT prop;
                PropVariantInit(&prop);
                if (SUCCEEDED(pReader->GetMetadataByName(L"/grctlext/Delay", &prop))) {
                    if (prop.vt == VT_UI2 && prop.uiVal > 0) {
                        delayMs = prop.uiVal * 10;
                    }
                    PropVariantClear(&prop);
                }
                pReader->Release();
            }
            if (delayMs < 20) delayMs = 100;

            IWICFormatConverter* pConverter = nullptr;
            if (SUCCEEDED(m_pWicFactory->CreateFormatConverter(&pConverter)) && pConverter) {
                if (SUCCEEDED(pConverter->Initialize(pFrame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom))) {
                    ID2D1Bitmap* pBmp = nullptr;
                    if (SUCCEEDED(m_pD2DContext->CreateBitmapFromWicBitmap(pConverter, nullptr, &pBmp)) && pBmp) {
                        m_gifFrames.push_back({ pBmp, delayMs });
                        m_gifTotalDurationMs += delayMs;
                    }
                }
                pConverter->Release();
            }
            pFrame->Release();
        }

        pDecoder->Release();

        m_gifStartTime = std::chrono::steady_clock::now();
        m_gifActiveIndex = 0;

        return !m_gifFrames.empty();
    }

    bool LoadVideo(const std::wstring& path) {
        if (!m_pD3DDevice || !m_pD3DContext || !m_pD2DContext) return false;

        if (!m_mfStarted) {
            HRESULT hrStartup = MFStartup(MF_VERSION, MFSTARTUP_LITE);
            if (SUCCEEDED(hrStartup)) {
                m_mfStarted = true;
            } else {
                return false;
            }
        }

        IMFAttributes* pAttributes = nullptr;
        MFCreateAttributes(&pAttributes, 2);
        pAttributes->SetUINT32(MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, TRUE);
        pAttributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);

        HRESULT hr = MFCreateSourceReaderFromURL(path.c_str(), pAttributes, &m_pSourceReader);
        pAttributes->Release();
        if (FAILED(hr) || !m_pSourceReader) {
            return false;
        }

        m_pSourceReader->SetStreamSelection(MF_SOURCE_READER_ALL_STREAMS, FALSE);
        m_pSourceReader->SetStreamSelection(MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE);

        IMFMediaType* pPartialType = nullptr;
        MFCreateMediaType(&pPartialType);
        pPartialType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        pPartialType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
        hr = m_pSourceReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, pPartialType);
        pPartialType->Release();
        if (FAILED(hr)) {
            SafeRelease(m_pSourceReader);
            return false;
        }

        IMFMediaType* pActualType = nullptr;
        m_pSourceReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, &pActualType);
        UINT32 vWidth = 0, vHeight = 0;
        MFGetAttributeSize(pActualType, MF_MT_FRAME_SIZE, &vWidth, &vHeight);
        UINT32 frNum = 0, frDen = 0;
        MFGetAttributeRatio(pActualType, MF_MT_FRAME_RATE, &frNum, &frDen);
        pActualType->Release();

        if (vWidth == 0 || vHeight == 0) {
            SafeRelease(m_pSourceReader);
            return false;
        }

        m_videoFps = (frDen > 0) ? (static_cast<float>(frNum) / static_cast<float>(frDen)) : 30.0f;
        if (m_videoFps <= 0.0f || m_videoFps > 60.0f) m_videoFps = 30.0f;
        m_videoWidth = vWidth;
        m_videoHeight = vHeight;

        // Create Destination Texture (B8G8R8X8_UNORM matches MFVideoFormat_RGB32)
        D3D11_TEXTURE2D_DESC td = {};
        td.Width = vWidth;
        td.Height = vHeight;
        td.MipLevels = 1;
        td.ArraySize = 1;
        td.Format = DXGI_FORMAT_B8G8R8X8_UNORM;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        hr = m_pD3DDevice->CreateTexture2D(&td, nullptr, &m_pVideoDestTexture);
        if (FAILED(hr) || !m_pVideoDestTexture) {
            SafeRelease(m_pSourceReader);
            return false;
        }

        IDXGISurface* pSurface = nullptr;
        hr = m_pVideoDestTexture->QueryInterface(__uuidof(IDXGISurface), reinterpret_cast<void**>(&pSurface));
        if (SUCCEEDED(hr) && pSurface) {
            D2D1_BITMAP_PROPERTIES1 bp = D2D1::BitmapProperties1(
                D2D1_BITMAP_OPTIONS_NONE,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8X8_UNORM, D2D1_ALPHA_MODE_IGNORE)
            );
            hr = m_pD2DContext->CreateBitmapFromDxgiSurface(pSurface, &bp, &m_pVideoBitmap);
            pSurface->Release();
        }

        if (FAILED(hr) || !m_pVideoBitmap) {
            SafeRelease(m_pVideoDestTexture);
            SafeRelease(m_pSourceReader);
            return false;
        }

        // Start video decode worker thread
        m_videoStop = false;
        m_videoRunning = true;
        m_videoThread = std::thread([this]() {
            this->VideoWorkerLoop();
        });

        return true;
    }

    void VideoWorkerLoop() {
        int frameIntervalMs = static_cast<int>(1000.0f / m_videoFps);
        if (frameIntervalMs < 16) frameIntervalMs = 16;

        struct QueuedFrame {
            LONGLONG ts = 0;
            std::vector<BYTE> pixels;
        };

        std::vector<QueuedFrame> buffer;
        std::vector<std::vector<BYTE>> freePool;
        const size_t REORDER_DEPTH = 6;
        size_t expectedSize = static_cast<size_t>(m_videoWidth) * m_videoHeight * 4;
        bool endOfStream = false;

        auto FetchOne = [&]() {
            if (endOfStream || !m_pSourceReader) return;
            DWORD streamIndex = 0;
            DWORD flags = 0;
            LONGLONG timeStamp = 0;
            IMFSample* pSample = nullptr;
            HRESULT hr = m_pSourceReader->ReadSample(
                MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &streamIndex, &flags, &timeStamp, &pSample
            );
            if (FAILED(hr)) return;
            if (flags & MF_SOURCE_READERF_ENDOFSTREAM) {
                endOfStream = true;
                return;
            }
            if (pSample) {
                LONGLONG sampleTime = 0;
                pSample->GetSampleTime(&sampleTime);
                IMFMediaBuffer* pBuffer = nullptr;
                pSample->GetBufferByIndex(0, &pBuffer);
                if (pBuffer) {
                    BYTE* ptr = nullptr;
                    DWORD maxLen = 0, curLen = 0;
                    if (SUCCEEDED(pBuffer->Lock(&ptr, &maxLen, &curLen)) && ptr) {
                        QueuedFrame qf;
                        qf.ts = sampleTime;
                        if (!freePool.empty()) {
                            qf.pixels = std::move(freePool.back());
                            freePool.pop_back();
                        } else {
                            qf.pixels.resize(expectedSize);
                        }
                        size_t copyBytes = (std::min)(static_cast<size_t>(curLen), qf.pixels.size());
                        memcpy(qf.pixels.data(), ptr, copyBytes);
                        pBuffer->Unlock();
                        buffer.push_back(std::move(qf));
                    }
                    pBuffer->Release();
                }
                pSample->Release();
            }
        };

        // Pre-fill reorder buffer to ensure presentation chronological order
        while (buffer.size() < REORDER_DEPTH && !endOfStream && !m_videoStop) {
            FetchOne();
        }

        while (!m_videoStop) {
            if (m_paused) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }

            auto startTime = std::chrono::steady_clock::now();

            FetchOne();

            if (buffer.empty()) {
                if (endOfStream) {
                    PROPVARIANT var;
                    PropVariantInit(&var);
                    var.vt = VT_I8;
                    var.hVal.QuadPart = 0;
                    if (m_pSourceReader) {
                        m_pSourceReader->SetCurrentPosition(GUID_NULL, var);
                    }
                    PropVariantClear(&var);
                    endOfStream = false;

                    while (buffer.size() < REORDER_DEPTH && !endOfStream && !m_videoStop) {
                        FetchOne();
                    }
                    continue;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            // Always pop sample with lowest PTS (strictly monotonic presentation order)
            auto minIt = std::min_element(buffer.begin(), buffer.end(), [](const QueuedFrame& a, const QueuedFrame& b) {
                return a.ts < b.ts;
            });

            QueuedFrame popped = std::move(*minIt);
            buffer.erase(minIt);

            if (!popped.pixels.empty() && m_pD3DContext && m_pVideoDestTexture) {
                UINT rowPitch = m_videoWidth * 4;
                ID3D10Multithread* pMt = nullptr;
                if (m_pD3DContext) m_pD3DContext->QueryInterface(__uuidof(ID3D10Multithread), reinterpret_cast<void**>(&pMt));
                else if (m_pD3DDevice) m_pD3DDevice->QueryInterface(__uuidof(ID3D10Multithread), reinterpret_cast<void**>(&pMt));
                if (pMt) pMt->Enter();
                m_pD3DContext->UpdateSubresource(m_pVideoDestTexture, 0, nullptr, popped.pixels.data(), rowPitch, 0);
                if (pMt) {
                    pMt->Leave();
                    pMt->Release();
                }
                if (m_hwnd) {
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                }
            }

            // Recycle buffer into freePool to prevent heap allocations
            freePool.push_back(std::move(popped.pixels));

            auto endTime = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
            int sleepTime = frameIntervalMs - static_cast<int>(elapsed);
            if (sleepTime > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
            }
        }

        buffer.clear();
        freePool.clear();
    }

    template <typename T>
    static void SafeRelease(T*& p) {
        if (p) {
            p->Release();
            p = nullptr;
        }
    }

    ID3D11Device* m_pD3DDevice = nullptr;
    ID3D11DeviceContext* m_pD3DContext = nullptr;
    ID2D1DeviceContext* m_pD2DContext = nullptr;
    IWICImagingFactory* m_pWicFactory = nullptr;
    HWND m_hwnd = nullptr;

    WallpaperType m_currentType = WallpaperType::None;
    std::wstring m_currentPath;
    std::atomic<bool> m_paused{ false };

    // Static image
    ID2D1Bitmap* m_pStaticBitmap = nullptr;

    // GIF animation
    std::vector<GifFrame> m_gifFrames;
    int m_gifTotalDurationMs = 0;
    size_t m_gifActiveIndex = 0;
    std::chrono::steady_clock::time_point m_gifStartTime;

    // Video playback
    IMFDXGIDeviceManager* m_pDXGIManager = nullptr;
    IMFSourceReader* m_pSourceReader = nullptr;
    ID3D11Texture2D* m_pVideoDestTexture = nullptr;
    ID2D1Bitmap1* m_pVideoBitmap = nullptr;
    float m_videoFps = 30.0f;
    UINT m_videoWidth = 0;
    UINT m_videoHeight = 0;
    std::thread m_videoThread;
    std::atomic<bool> m_videoStop{ false };
    std::atomic<bool> m_videoRunning{ false };
    bool m_mfStarted = false;
};

} // namespace speta

# Spet@term

Spet@term is an ultra-fast native Windows terminal and micro-IDE written in modern C++20.

It combines hardware-accelerated Direct2D and DirectComposition rendering with low resource usage and built-in developer workflows.

## Key Features

- Ultra-Low Footprint: ~3.6 MB idle working set memory, outperforming default terminal hosts.
- Direct2D and DirectComposition Pipeline: Sub-millisecond text rendering with DirectWrite and GPU acceleration.
- Modern ConPTY Integration: Full compatibility with Windows Command Prompt (CMD) and PowerShell.
- Command Blocks Stream: Distinct visual outputs for commands, exit codes, and elapsed execution timing.
- Animated and Static Wallpapers: Support for video loops (MP4 via Media Foundation), animated GIFs, and static images with hardware-accelerated decode and low CPU overhead.
- Native Git Integration: In-process index and status detection without subprocess overhead.
- Retro and Modern Aesthetics: Customizable themes, typography, and optional retro UI styling.

## Architecture

Spet@term is built directly on native Win32 APIs without web runtimes or heavy framework layers:

- `src/main.cpp`: Application entry point, window management, message dispatching.
- `src/renderer_d2d.hpp`: Direct2D, DirectWrite, and DirectComposition rendering pipeline.
- `src/conpty_engine.hpp`: ConPTY pseudo-console lifecycle and asynchronous I/O pump.
- `src/wallpaper.hpp`: Background media decoder supporting Media Foundation DXVA and WIC.
- `src/git_engine.hpp`: In-process Git repository discovery and working tree inspection.
- `src/settings.hpp`: Configuration persistence and schema defaults.

## Building from Source

### Prerequisites

- Windows 10 (Version 1809 or higher) or Windows 11
- Clang / LLVM (supporting C++20)
- Windows SDK (DirectX, Direct2D, Media Foundation headers)

### Compilation

Run the PowerShell build script:

```powershell
.\build.ps1
```

To build and run the test suite:

```powershell
clang++ tests/test_suite.cpp -o tests/test_suite.exe -std=c++20 -O2 -DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN -DNOMINMAX -D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH -ld2d1 -ldwrite -lwindowscodecs -lole32 -luuid -luser32 -lgdi32 -lshell32 -lshlwapi -lpsapi -ldwmapi -lcomdlg32 -ladvapi32 -ld3d11 -ldxgi -ldcomp -lpdh -lmfplat -lmfreadwrite -lmfuuid
.\tests\test_suite.exe
```

## License

Internal proprietary / Experimental research project.

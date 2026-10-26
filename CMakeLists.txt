cmake_minimum_required(VERSION 3.20)
project(SpetaTerm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN -DNOMINMAX -D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH)

add_executable(spetaterm WIN32
    src/main.cpp
    src/common.hpp
    src/sprite_atlas.hpp
    src/themes.hpp
    src/git_engine.hpp
    src/syntax_autocomplete.hpp
    src/error_doctor.hpp
    src/builtins.hpp
    src/conpty_engine.hpp
    src/renderer_d2d.hpp
    src/settings.hpp
    src/terminal_tab.hpp
    src/i18n.hpp
    src/workfolder.hpp
)

target_link_libraries(spetaterm PRIVATE
    d2d1
    dwrite
    windowscodecs
    ole32
    uuid
    user32
    gdi32
    shell32
    shlwapi
    psapi
    dwmapi
    comdlg32
    advapi32
    d3d11
    dxgi
    dcomp
)

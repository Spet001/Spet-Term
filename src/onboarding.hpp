#pragma once

#include "common.hpp"
#include <d2d1.h>
#include <vector>
#include <string>

namespace speta {

struct OnboardingState {
    bool open = false;
    int step = 0; // 0 to 6

    // Navigation hit rects
    D2D1_RECT_F backBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F nextBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F skipBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F modalRect{ 0, 0, 0, 0 };

    // Interactive rects for step controls
    std::vector<std::pair<D2D1_RECT_F, std::string>> languageBtns;
    D2D1_RECT_F shellPwBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F shellCmdBtnRect{ 0, 0, 0, 0 };
    std::vector<std::pair<D2D1_RECT_F, std::wstring>> themeBtns;
    std::vector<std::pair<D2D1_RECT_F, std::wstring>> fontBtns;
    D2D1_RECT_F fontMinusBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F fontPlusBtnRect{ 0, 0, 0, 0 };
    std::vector<std::pair<D2D1_RECT_F, std::wstring>> wallpaperBtns;
    D2D1_RECT_F wallpaperAddBtnRect{ 0, 0, 0, 0 };
    D2D1_RECT_F wallpaperDimTrackRect{ 0, 0, 0, 0 };
    D2D1_RECT_F wallpaperDimThumbRect{ 0, 0, 0, 0 };
    std::vector<std::pair<D2D1_RECT_F, bool>> warnPasteBtns;
    std::vector<std::pair<D2D1_RECT_F, bool>> rightClickPasteBtns;
    std::vector<std::pair<D2D1_RECT_F, bool>> sysCursorBtns;
    D2D1_RECT_F registerTermBtnRect{ 0, 0, 0, 0 };

    void Next() {
        if (step < 6) step++;
    }

    void Prev() {
        if (step > 0) step--;
    }

    void Reset() {
        step = 0;
        open = true;
    }

    void Close() {
        open = false;
    }
};

} // namespace speta

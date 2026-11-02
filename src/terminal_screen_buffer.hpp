#pragma once

#include "common.hpp"
#include <vector>
#include <string>
#include <algorithm>

namespace speta {

struct TerminalCell {
    wchar_t ch = L' ';
    D2D1_COLOR_F fg{ 0.9f, 0.9f, 0.9f, 1.0f };
    D2D1_COLOR_F bg{ 0.0f, 0.0f, 0.0f, 0.0f };
    bool bold = false;
    bool italic = false;
    bool underline = false;
};

class TerminalScreenBuffer {
public:
    int cols = 120;
    int rows = 32;
    int cursorCol = 0;
    int cursorRow = 0;
    bool cursorVisible = true;
    bool isAltBuffer = false;

    int savedPrimaryRow = 0;
    int savedPrimaryCol = 0;

    D2D1_COLOR_F defaultFg{ 0.9f, 0.9f, 0.9f, 1.0f };
    D2D1_COLOR_F defaultBg{ 0.0f, 0.0f, 0.0f, 0.0f };

    std::vector<std::vector<TerminalCell>> primaryGrid;
    std::vector<std::vector<TerminalCell>> altGrid;
    std::vector<StyledLine> scrollback;

    TerminalScreenBuffer(int c = 120, int r = 32) : cols(c), rows(r) {
        Reset();
    }

    void SetDefaultColors(D2D1_COLOR_F fg, D2D1_COLOR_F bg) {
        defaultFg = fg;
        defaultBg = bg;
    }

    TerminalCell MakeEmptyCell() const {
        TerminalCell cell;
        cell.ch = L' ';
        cell.fg = defaultFg;
        cell.bg = defaultBg;
        cell.bold = false;
        cell.italic = false;
        cell.underline = false;
        return cell;
    }

    void Reset() {
        cursorCol = 0;
        cursorRow = 0;
        cursorVisible = true;
        isAltBuffer = false;
        savedPrimaryRow = 0;
        savedPrimaryCol = 0;
        scrollback.clear();

        primaryGrid.assign(rows, std::vector<TerminalCell>(cols, MakeEmptyCell()));
        altGrid.assign(rows, std::vector<TerminalCell>(cols, MakeEmptyCell()));
    }

    void Resize(int newCols, int newRows) {
        if (newCols <= 0 || newRows <= 0) return;
        cols = newCols;
        rows = newRows;

        primaryGrid.resize(rows);
        for (auto& row : primaryGrid) {
            row.resize(cols, MakeEmptyCell());
        }

        altGrid.resize(rows);
        for (auto& row : altGrid) {
            row.resize(cols, MakeEmptyCell());
        }

        if (cursorRow >= rows) cursorRow = rows - 1;
        if (cursorCol >= cols) cursorCol = cols - 1;
    }

    void ScrollUp() {
        auto& grid = isAltBuffer ? altGrid : primaryGrid;
        if (!isAltBuffer) {
            StyledLine sl = ConvertGridRowToStyledLine(grid[0]);
            scrollback.push_back(sl);
            if (scrollback.size() > 5000) {
                scrollback.erase(scrollback.begin(), scrollback.begin() + 200);
            }
        }
        for (int r = 0; r < rows - 1; ++r) {
            grid[r] = std::move(grid[r + 1]);
        }
        grid[rows - 1] = std::vector<TerminalCell>(cols, MakeEmptyCell());
        cursorRow = rows - 1;
    }

    void WriteChar(wchar_t c, const D2D1_COLOR_F& fg, const D2D1_COLOR_F& bg, bool bold, bool italic, bool underline) {
        auto& grid = isAltBuffer ? altGrid : primaryGrid;
        if (cursorRow < 0) cursorRow = 0;
        if (cursorRow >= rows) {
            ScrollUp();
        }
        if (cursorCol >= cols) {
            cursorCol = 0;
            cursorRow++;
            if (cursorRow >= rows) {
                ScrollUp();
            }
        }

        TerminalCell& cell = grid[cursorRow][cursorCol];
        cell.ch = c;
        cell.fg = fg;
        cell.bg = bg;
        cell.bold = bold;
        cell.italic = italic;
        cell.underline = underline;
        cursorCol++;
    }

    void CarriageReturn() {
        cursorCol = 0;
    }

    void NewLine() {
        cursorRow++;
        if (cursorRow >= rows) {
            ScrollUp();
        }
    }

    void Backspace() {
        if (cursorCol > 0) {
            cursorCol--;
        }
    }

    void Tab() {
        int nextTab = (cursorCol / 8 + 1) * 8;
        if (nextTab >= cols) nextTab = cols - 1;
        cursorCol = nextTab;
    }

    void SetCursorPosition(int r, int c) {
        // 1-based ANSI coordinates
        int targetR = (std::clamp)(r - 1, 0, rows - 1);
        int targetC = (std::clamp)(c - 1, 0, cols - 1);
        cursorRow = targetR;
        cursorCol = targetC;
    }

    void MoveCursor(int deltaRow, int deltaCol) {
        cursorRow = (std::clamp)(cursorRow + deltaRow, 0, rows - 1);
        cursorCol = (std::clamp)(cursorCol + deltaCol, 0, cols - 1);
    }

    void ClearLine(int mode) {
        auto& grid = isAltBuffer ? altGrid : primaryGrid;
        if (cursorRow < 0 || cursorRow >= rows) return;

        int startCol = 0;
        int endCol = cols - 1;
        if (mode == 0) {
            // Cursor to end of line
            startCol = (std::clamp)(cursorCol, 0, cols - 1);
        } else if (mode == 1) {
            // Start of line to cursor
            endCol = (std::clamp)(cursorCol, 0, cols - 1);
        } else {
            // Entire line (mode 2)
            startCol = 0;
            endCol = cols - 1;
        }

        for (int c = startCol; c <= endCol; ++c) {
            grid[cursorRow][c] = MakeEmptyCell();
        }
    }

    void ClearDisplay(int mode) {
        auto& grid = isAltBuffer ? altGrid : primaryGrid;
        if (mode == 0) {
            // From cursor to end of screen
            ClearLine(0);
            for (int r = cursorRow + 1; r < rows; ++r) {
                std::fill(grid[r].begin(), grid[r].end(), MakeEmptyCell());
            }
        } else if (mode == 1) {
            // From beginning to cursor
            for (int r = 0; r < cursorRow; ++r) {
                std::fill(grid[r].begin(), grid[r].end(), MakeEmptyCell());
            }
            ClearLine(1);
        } else if (mode == 2) {
            // Entire screen
            for (int r = 0; r < rows; ++r) {
                std::fill(grid[r].begin(), grid[r].end(), MakeEmptyCell());
            }
        } else if (mode == 3) {
            // Scrollback clear
            scrollback.clear();
        }
    }

    void EnterAlternateBuffer() {
        if (!isAltBuffer) {
            savedPrimaryRow = cursorRow;
            savedPrimaryCol = cursorCol;
            isAltBuffer = true;
            for (int r = 0; r < rows; ++r) {
                std::fill(altGrid[r].begin(), altGrid[r].end(), MakeEmptyCell());
            }
            cursorRow = 0;
            cursorCol = 0;
        }
    }

    void ExitAlternateBuffer() {
        if (isAltBuffer) {
            isAltBuffer = false;
            cursorRow = savedPrimaryRow;
            cursorCol = savedPrimaryCol;
        }
    }

    static StyledLine ConvertGridRowToStyledLine(const std::vector<TerminalCell>& rowCells) {
        StyledLine line;
        int lastNonSpace = -1;
        for (int c = static_cast<int>(rowCells.size()) - 1; c >= 0; --c) {
            if (rowCells[c].ch != L' ' || rowCells[c].bg.a > 0.01f) {
                lastNonSpace = c;
                break;
            }
        }
        if (lastNonSpace == -1) {
            return line;
        }

        StyledSpan currentSpan;
        bool hasSpan = false;

        for (int c = 0; c <= lastNonSpace; ++c) {
            const auto& cell = rowCells[c];
            line.rawText += cell.ch;

            if (!hasSpan) {
                currentSpan.text += cell.ch;
                currentSpan.fg = cell.fg;
                currentSpan.bg = cell.bg;
                currentSpan.bold = cell.bold;
                currentSpan.italic = cell.italic;
                currentSpan.underline = cell.underline;
                hasSpan = true;
            } else if (
                currentSpan.fg.r == cell.fg.r && currentSpan.fg.g == cell.fg.g && currentSpan.fg.b == cell.fg.b &&
                currentSpan.bg.r == cell.bg.r && currentSpan.bg.g == cell.bg.g && currentSpan.bg.b == cell.bg.b &&
                currentSpan.bold == cell.bold && currentSpan.italic == cell.italic && currentSpan.underline == cell.underline
            ) {
                currentSpan.text += cell.ch;
            } else {
                line.spans.push_back(currentSpan);
                currentSpan = StyledSpan();
                currentSpan.text += cell.ch;
                currentSpan.fg = cell.fg;
                currentSpan.bg = cell.bg;
                currentSpan.bold = cell.bold;
                currentSpan.italic = cell.italic;
                currentSpan.underline = cell.underline;
            }
        }
        if (hasSpan && !currentSpan.text.empty()) {
            line.spans.push_back(currentSpan);
        }
        return line;
    }

    void ExportToLines(std::vector<StyledLine>& outLines) const {
        outLines.clear();
        if (isAltBuffer) {
            int maxR = rows - 1;
            while (maxR >= 0) {
                bool hasContent = false;
                for (const auto& cell : altGrid[maxR]) {
                    if (cell.ch != L' ' || cell.bg.a > 0.01f) {
                        hasContent = true;
                        break;
                    }
                }
                if (hasContent) break;
                maxR--;
            }
            if (maxR < 0) maxR = 0;
            for (int r = 0; r <= maxR; ++r) {
                outLines.push_back(ConvertGridRowToStyledLine(altGrid[r]));
            }
        } else {
            outLines.reserve(scrollback.size() + rows);
            for (const auto& sl : scrollback) {
                outLines.push_back(sl);
            }
            int maxR = -1;
            for (int r = rows - 1; r >= 0; --r) {
                for (const auto& cell : primaryGrid[r]) {
                    if (cell.ch != L' ' || cell.bg.a > 0.01f) {
                        maxR = r;
                        break;
                    }
                }
                if (maxR != -1) break;
            }
            if (maxR >= 0) {
                for (int r = 0; r <= maxR && r < rows; ++r) {
                    outLines.push_back(ConvertGridRowToStyledLine(primaryGrid[r]));
                }
            }
        }
    }
};

} // namespace speta

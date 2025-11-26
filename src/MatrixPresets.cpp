#include "manager.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <pdcurses.h>


// 矩阵预设

void Manager::ensureMatrixPresets()
{
    if (!matrixPresets_.empty())
        return;

    // 严格对角占优预设1（迭代法必收敛）
    {
        calc::Matrix A(3, 3);
        A(0, 0) = 5;
        A(0, 1) = 2;
        A(0, 2) = 1;
        A(1, 0) = -1;
        A(1, 1) = 4;
        A(1, 2) = 2;
        A(2, 0) = 2;
        A(2, 1) = -3;
        A(2, 2) = 10;
        std::vector<double> b = {-12, 20, 3};
        matrixPresets_.push_back({"严格对角占优1 (3x3)", A, b});
    }

    // 对角占优预设2（适合SOR测试）
    {
        calc::Matrix A(3, 3);
        A(0, 0) = 4;
        A(0, 1) = -1;
        A(0, 2) = 0;
        A(1, 0) = -1;
        A(1, 1) = 4;
        A(1, 2) = -1;
        A(2, 0) = 0;
        A(2, 1) = -1;
        A(2, 2) = 4;
        std::vector<double> b = {1, 4, -3};
        matrixPresets_.push_back({"严格对角占优2 (3x3)", A, b});
    }

    // 通用预设1（3x3）- 不保证迭代收敛
    {
        calc::Matrix A(3, 3);
        A(0, 0) = 1;
        A(0, 1) = 1;
        A(0, 2) = -1;
        A(1, 0) = 1;
        A(1, 1) = 2;
        A(1, 2) = -2;
        A(2, 0) = -2;
        A(2, 1) = 1;
        A(2, 2) = 1;
        std::vector<double> b = {1, 0, 1};
        matrixPresets_.push_back({"方程组1 (3x3)", A, b});
    }

    // 通用预设2（2x2）
    {
        calc::Matrix A(2, 2);
        A(0, 0) = 2;
        A(0, 1) = 1;
        A(1, 0) = 1;
        A(1, 1) = 3;
        std::vector<double> b = {5, 6};
        matrixPresets_.push_back({"方程组2 (2x2)", A, b});
    }

    // SPD 预设（用于平方根法）
    {
        calc::Matrix A(3, 3);
        A(0, 0) = 4;
        A(0, 1) = 1;
        A(0, 2) = 1;
        A(1, 0) = 1;
        A(1, 1) = 3;
        A(1, 2) = 0;
        A(2, 0) = 1;
        A(2, 1) = 0;
        A(2, 2) = 2;
        std::vector<double> b = {1, 2, 3};
        matrixPresets_.push_back({"SPD 对称正定 (3x3)", A, b});
    }

    // 三对角预设（用于追赶法）
    {
        calc::Matrix A(4, 4);
        // 主对角
        A(0, 0) = 2;
        A(1, 1) = 2;
        A(2, 2) = 2;
        A(3, 3) = 2;
        // 上对角
        A(0, 1) = -1;
        A(1, 2) = -1;
        A(2, 3) = -1;
        // 下对角
        A(1, 0) = -1;
        A(2, 1) = -1;
        A(3, 2) = -1;
        std::vector<double> b = {1, 0, 0, 1};
        matrixPresets_.push_back({"三对角 (4x4)", A, b});
    }
}

void Manager::cycleMatrixPresetFor(const std::string &name, int delta)
{
    ensureMatrixPresets();
    if (matrixPresets_.empty())
        return;

    auto &st = states_[name];
    st.matrixPresetIndex = (st.matrixPresetIndex + delta + (int)matrixPresets_.size()) % (int)matrixPresets_.size();

    // 加载当前预设
    const auto &preset = matrixPresets_[st.matrixPresetIndex];
    st.matrixA = preset.A;
    st.vectorB = preset.b;

    fillDescriptionFor(name);
}

void Manager::addMatrixPreset()
{
    const auto expName = ui_.getCurrentExperiment();
    if (expName.empty())
        return;

    showMatrixInputDialog(expName, true);
}

void Manager::showMatrixInputDialog(const std::string &expName, bool createNew)
{
    auto &st = states_[expName];
    int n = 3; // 默认维度

    if (createNew)
    {
        // 创建新预设矩阵：先输入维度
        int h = 15, w = 70;
        int y = (LINES - h) / 2;
        int x = (COLS - w) / 2;
        WINDOW *win = newwin(h, w, y, x);
        box(win, 0, 0);
        mvwprintw(win, 1, 2, "输入线性方程组维度");
        mvwprintw(win, 2, 2, "----------------------------------------------");
        mvwprintw(win, 4, 2, "请输入方程个数 n (2-10): ");
        mvwprintw(win, 6, 2, "按 q 退出");
        wrefresh(win);
        echo();
        char nStr[10] = "";
        mvwgetnstr(win, 4, 35, nStr, 9);
        noecho();

        if (nStr[0] == 'q' || nStr[0] == 'Q' || nStr[0] == '\0')
        {
            delwin(win);
            return;
        }

        n = toInt(std::string(nStr), 3);
        if (n < 2)
            n = 2;
        if (n > 10)
            n = 10;
        delwin(win);
    }
    else
    {
        // 编辑当前预设矩阵：使用预设维度
        if (st.matrixPresetIndex >= 0 && st.matrixPresetIndex < (int)matrixPresets_.size())
        {
            n = matrixPresets_[st.matrixPresetIndex].A.rows();
        }
        else if (st.matrixA.rows() > 0)
        {
            n = st.matrixA.rows();
        }
    }

    // 初始化矩阵
    calc::Matrix A(n, n, 0.0);
    std::vector<double> b(n, 0.0);

    // 从当前预设加载数据
    if (!createNew && st.matrixPresetIndex >= 0 && st.matrixPresetIndex < (int)matrixPresets_.size())
    {
        const auto &preset = matrixPresets_[st.matrixPresetIndex];
        if (preset.A.rows() == n && preset.A.cols() == n && (int)preset.b.size() == n)
        {
            A = preset.A;
            b = preset.b;
        }
    }

    // 创建表格编辑器
    int tableH = std::min(n + 10, LINES - 4);
    int tableW = std::min(n * 12 + 20, COLS - 4);
    int tableY = (LINES - tableH) / 2;
    int tableX = (COLS - tableW) / 2;
    WINDOW *tableWin = newwin(tableH, tableW, tableY, tableX);

    int curRow = 0, curCol = 0; // 当前光标位置
    bool editingB = false;      // 是否在编辑 b 向量

    while (true)
    {
        wclear(tableWin);
        box(tableWin, 0, 0);
        mvwprintw(tableWin, 1, 2, "矩阵编辑器 (%dx%d) - 方向键移动, 回车编辑, q退出, s保存", n, n);
        mvwprintw(tableWin, 2, 2, "----------------------------------------------------");

        // 绘制表头
        int startY = 4;
        int startX = 4;
        mvwprintw(tableWin, startY, startX, "    ");
        for (int j = 0; j < n; ++j)
        {
            mvwprintw(tableWin, startY, startX + 4 + j * 10, "x%-2d", j + 1);
        }
        mvwprintw(tableWin, startY, startX + 4 + n * 10, "  b");

        // 绘制矩阵
        for (int i = 0; i < n; ++i)
        {
            mvwprintw(tableWin, startY + 1 + i, startX, "[%d]", i + 1);
            for (int j = 0; j < n; ++j)
            {
                bool highlight = (curRow == i && curCol == j && !editingB);
                if (highlight)
                    wattron(tableWin, A_REVERSE);
                mvwprintw(tableWin, startY + 1 + i, startX + 4 + j * 10, "%8.2f", A(i, j));
                if (highlight)
                    wattroff(tableWin, A_REVERSE);
            }
            // 绘制 b 向量
            bool highlightB = (curRow == i && editingB);
            if (highlightB)
                wattron(tableWin, A_REVERSE);
            mvwprintw(tableWin, startY + 1 + i, startX + 4 + n * 10, "%8.2f", b[i]);
            if (highlightB)
                wattroff(tableWin, A_REVERSE);
        }

        mvwprintw(tableWin, tableH - 2, 2, "提示: ↑↓←→ 移动 | 回车 编辑 | s 保存 | q 取消");
        wrefresh(tableWin);

        int ch = getch();
        if (ch == 'q' || ch == 'Q')
        {
            delwin(tableWin);
            return;
        }
        else if (ch == 's' || ch == 'S')
        {
            if (createNew)
            {
                // 添加新预设
                std::string presetName = "自定义矩阵 " + std::to_string(matrixPresets_.size() + 1);
                matrixPresets_.push_back({presetName, A, b});
                st.matrixPresetIndex = (int)matrixPresets_.size() - 1;
                st.matrixA = A;
                st.vectorB = b;
            }
            else
            {
                // 更新当前预设
                if (st.matrixPresetIndex >= 0 && st.matrixPresetIndex < (int)matrixPresets_.size())
                {
                    matrixPresets_[st.matrixPresetIndex].A = A;
                    matrixPresets_[st.matrixPresetIndex].b = b;
                    st.matrixA = A;
                    st.vectorB = b;
                }
            }
            delwin(tableWin);
            fillDescriptionFor(expName);
            return;
        }
        else if (ch == KEY_UP)
        {
            if (curRow > 0)
                curRow--;
        }
        else if (ch == KEY_DOWN)
        {
            if (curRow < n - 1)
                curRow++;
        }
        else if (ch == KEY_LEFT)
        {
            if (editingB)
            {
                editingB = false;
                curCol = n - 1;
            }
            else if (curCol > 0)
            {
                curCol--;
            }
        }
        else if (ch == KEY_RIGHT)
        {
            if (!editingB && curCol < n - 1)
            {
                curCol++;
            }
            else if (!editingB && curCol == n - 1)
            {
                editingB = true;
            }
        }
        else if (ch == '\n' || ch == '\r' || ch == KEY_ENTER)
        {
            // 编辑当前单元格
            echo();
            char valStr[20] = "";
            if (editingB)
            {
                mvwprintw(tableWin, tableH - 3, 2, "输入 b[%d] = ", curRow + 1);
                wgetnstr(tableWin, valStr, 19);
                if (valStr[0] != '\0')
                    b[curRow] = toDouble(std::string(valStr), b[curRow]);
            }
            else
            {
                mvwprintw(tableWin, tableH - 3, 2, "输入 A[%d][%d] = ", curRow + 1, curCol + 1);
                wgetnstr(tableWin, valStr, 19);
                if (valStr[0] != '\0')
                    A(curRow, curCol) = toDouble(std::string(valStr), A(curRow, curCol));
            }
            noecho();
            mvwprintw(tableWin, tableH - 3, 2, "                                                  ");
        }
    }
}

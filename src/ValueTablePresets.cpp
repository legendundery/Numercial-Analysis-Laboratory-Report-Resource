#include "manager.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <pdcurses.h>

// 函数值表预设

void Manager::ensureValueTablePresets()
{
    if (!valueTablePresets_.empty())
        return;

    // 等距节点预设（至少5个点）
    {
        ValueTablePreset p;
        p.name = "x^2 (等距h=1)";
        p.table = calc::Matrix(5, 2);
        for (int i = 0; i < 5; ++i)
        {
            double x = -2.0 + i;
            p.table(i, 0) = x;
            p.table(i, 1) = x * x;
        }
        valueTablePresets_.push_back(p);
    }

    {
        ValueTablePreset p;
        p.name = "sin(x) (等距h=π/4)";
        p.table = calc::Matrix(5, 2);
        for (int i = 0; i < 5; ++i)
        {
            double x = i * 3.14159265358979 / 4.0;
            p.table(i, 0) = x;
            p.table(i, 1) = std::sin(x);
        }
        valueTablePresets_.push_back(p);
    }

    {
        ValueTablePreset p;
        p.name = "e^x (等距h=0.5)";
        p.table = calc::Matrix(5, 2);
        for (int i = 0; i < 5; ++i)
        {
            double x = i * 0.5;
            p.table(i, 0) = x;
            p.table(i, 1) = std::exp(x);
        }
        valueTablePresets_.push_back(p);
    }

    // 不等距节点预设
    {
        ValueTablePreset p;
        p.name = "1/x (不等距)";
        p.table = calc::Matrix(5, 2);
        double xs[] = {1.0, 2.0, 3.0, 5.0, 6.0};
        for (int i = 0; i < 5; ++i)
        {
            p.table(i, 0) = xs[i];
            p.table(i, 1) = 1.0 / xs[i];
        }
        valueTablePresets_.push_back(p);
    }

    // 含导数值的等距节点预设
    {
        ValueTablePreset p;
        p.name = "e^x (含导数,h=1)";
        p.table = calc::Matrix(5, 3);
        for (int i = 0; i < 5; ++i)
        {
            double x = i * 1.0;
            p.table(i, 0) = x;
            p.table(i, 1) = std::exp(x);
            p.table(i, 2) = std::exp(x); // f'(x) = e^x
        }
        valueTablePresets_.push_back(p);
    }
}

void Manager::showValueTableInputDialog(const std::string &expName, bool isAddNew)
{
    auto &st = states_[expName];
    int rows = 3, cols = 2; // 默认维度

    if (isAddNew)
    {
        // 创建新预设：先输入维度
        int h = 15, w = 90;
        int y = (LINES - h) / 2;
        int x = (COLS - w) / 2;
        WINDOW *win = newwin(h, w, y, x);
        box(win, 0, 0);
        mvwprintw(win, 1, 2, "输入函数值表维度");
        mvwprintw(win, 2, 2, "----------------------------------------------------------------------");
        mvwprintw(win, 4, 2, "行数(点数) n (2-20): ");
        mvwprintw(win, 5, 2, "列数 m (2-10, 第0列=x, 第1列=f(x), 第2列=f'(x), ...): ");
        mvwprintw(win, 7, 2, "按 q 退出");
        wrefresh(win);

        echo();
        char rowStr[10] = "";
        mvwgetnstr(win, 4, 25, rowStr, 9);
        if (rowStr[0] == 'q' || rowStr[0] == 'Q' || rowStr[0] == '\0')
        {
            noecho();
            delwin(win);
            return;
        }

        char colStr[10] = "";
        mvwgetnstr(win, 5, 45, colStr, 9);
        noecho();

        if (colStr[0] == 'q' || colStr[0] == 'Q' || colStr[0] == '\0')
        {
            delwin(win);
            return;
        }

        rows = toInt(std::string(rowStr), 3);
        cols = toInt(std::string(colStr), 2);
        if (rows < 2)
            rows = 2;
        if (rows > 20)
            rows = 20;
        if (cols < 2)
            cols = 2;
        if (cols > 10)
            cols = 10;
        delwin(win);
    }
    else
    {
        // 编辑当前函数值表：使用已有维度
        if (st.valueTablePresetIndex >= 0 && st.valueTablePresetIndex < (int)valueTablePresets_.size())
        {
            rows = valueTablePresets_[st.valueTablePresetIndex].table.rows();
            cols = valueTablePresets_[st.valueTablePresetIndex].table.cols();
        }
        else if (st.valueTable.rows() > 0 && st.valueTable.cols() > 0)
        {
            rows = st.valueTable.rows();
            cols = st.valueTable.cols();
        }
    }

    // 初始化函数值表
    calc::Matrix table(rows, cols);
    std::vector<bool> isEmpty(rows * cols, true); // 标记哪些单元格为空(未知)

    // 从当前预设加载数据
    if (!isAddNew && st.valueTablePresetIndex >= 0 && st.valueTablePresetIndex < (int)valueTablePresets_.size())
    {
        const auto &preset = valueTablePresets_[st.valueTablePresetIndex].table;
        if (preset.rows() == rows && preset.cols() == cols)
        {
            for (int i = 0; i < rows; ++i)
            {
                for (int j = 0; j < cols; ++j)
                {
                    table(i, j) = preset(i, j);
                    isEmpty[i * cols + j] = std::isnan(preset(i, j));
                }
            }
        }
    }
    else
    {
        // 默认初始化为 NaN
        for (int i = 0; i < rows; ++i)
            for (int j = 0; j < cols; ++j)
                table(i, j) = std::nan("");
    }

    // 创建表格编辑器
    int tableH = std::min(rows + 10, LINES - 4);
    int tableW = std::min(cols * 10 + 20, COLS - 4);
    int tableY = (LINES - tableH) / 2;
    int tableX = (COLS - tableW) / 2;
    WINDOW *tableWin = newwin(tableH, tableW, tableY, tableX);

    int curRow = 0, curCol = 0; // 当前光标位置

    const char *colNames[] = {"x", "f(x)", "f'(x)", "f''(x)", "f'''(x)", "f''''(x)", "col6", "col7", "col8", "col9"};

    while (true)
    {
        wclear(tableWin);
        box(tableWin, 0, 0);
        mvwprintw(tableWin, 1, 2, "函数值表编辑器 (%dx%d) - 方向键移动, 回车编辑, Del删除, q退出, s保存", rows, cols);
        mvwprintw(tableWin, 2, 2, "------------------------------------------------------------");

        // 绘制表头
        int startY = 4;
        int startX = 4;
        mvwprintw(tableWin, startY, startX, "    ");
        for (int j = 0; j < cols; ++j)
        {
            mvwprintw(tableWin, startY, startX + 4 + j * 10, "%8s", colNames[j]);
        }

        // 绘制表格内容
        for (int i = 0; i < rows; ++i)
        {
            mvwprintw(tableWin, startY + 1 + i, startX, "[%d]", i);
            for (int j = 0; j < cols; ++j)
            {
                bool highlight = (curRow == i && curCol == j);
                if (highlight)
                    wattron(tableWin, A_REVERSE);

                if (isEmpty[i * cols + j])
                {
                    mvwprintw(tableWin, startY + 1 + i, startX + 4 + j * 10, "%8s", "");
                }
                else
                {
                    mvwprintw(tableWin, startY + 1 + i, startX + 4 + j * 10, "%8.4f", table(i, j));
                }

                if (highlight)
                    wattroff(tableWin, A_REVERSE);
            }
        }
        mvwprintw(tableWin, tableH - 2, 2, "提示: ↑↓←→ 移动 | 回车 编辑 | Del 删除(设为未知) | s 保存 | q 取消");
        wrefresh(tableWin);

        int ch = getch();
        if (ch == 'q' || ch == 'Q')
        {
            delwin(tableWin);
            return;
        }
        else if (ch == 's' || ch == 'S')
        {
            // 保存前将空单元格设为 NaN
            for (int i = 0; i < rows; ++i)
            {
                for (int j = 0; j < cols; ++j)
                {
                    if (isEmpty[i * cols + j])
                        table(i, j) = std::nan("");
                }
            }

            if (isAddNew)
            {
                // 添加新预设
                std::string presetName = "自定义函数值表 " + std::to_string(valueTablePresets_.size() + 1);
                valueTablePresets_.push_back({presetName, table});
                st.valueTablePresetIndex = (int)valueTablePresets_.size() - 1;
                st.valueTable = table;
            }
            else
            {
                // 更新当前预设
                if (st.valueTablePresetIndex >= 0 && st.valueTablePresetIndex < (int)valueTablePresets_.size())
                {
                    valueTablePresets_[st.valueTablePresetIndex].table = table;
                    st.valueTable = table;
                }
                else
                {
                    st.valueTable = table;
                }
            }
            delwin(tableWin);
            fillDescriptionFor(expName);
            computeExperiment(expName);
            return;
        }
        else if (ch == KEY_UP)
        {
            if (curRow > 0)
                curRow--;
        }
        else if (ch == KEY_DOWN)
        {
            if (curRow < rows - 1)
                curRow++;
        }
        else if (ch == KEY_LEFT)
        {
            if (curCol > 0)
                curCol--;
        }
        else if (ch == KEY_RIGHT)
        {
            if (curCol < cols - 1)
                curCol++;
        }
        else if (ch == KEY_DC || ch == 127 || ch == 8) // Delete 或 Backspace
        {
            // 删除当前单元格(设为未知)
            isEmpty[curRow * cols + curCol] = true;
            table(curRow, curCol) = std::nan("");
        }
        else if (ch == '\n' || ch == '\r' || ch == KEY_ENTER)
        {
            // 编辑当前单元格
            echo();
            char valStr[20] = "";
            mvwprintw(tableWin, tableH - 3, 2, "输入 %s[%d] = ", colNames[curCol], curRow);
            wgetnstr(tableWin, valStr, 19);

            std::string input(valStr);
            if (!input.empty())
            {
                if (input == "nan" || input == "NaN" || input == "NAN")
                {
                    isEmpty[curRow * cols + curCol] = true;
                    table(curRow, curCol) = std::nan("");
                }
                else
                {
                    table(curRow, curCol) = toDouble(input, table(curRow, curCol));
                    isEmpty[curRow * cols + curCol] = false;
                }
            }
            noecho();
        }
    }
}

void Manager::cycleValueTablePresetFor(const std::string &expName, int delta)
{
    ensureValueTablePresets();
    auto &st = states_[expName];

    int n = (int)valueTablePresets_.size();
    if (n == 0)
    {
        st.valueTablePresetIndex = -1;
        return;
    }

    if (st.valueTablePresetIndex < 0 || st.valueTablePresetIndex >= n)
        st.valueTablePresetIndex = 0;
    else
    {
        st.valueTablePresetIndex = (st.valueTablePresetIndex + delta + n) % n;
    }

    st.valueTable = valueTablePresets_[st.valueTablePresetIndex].table;

    fillDescriptionFor(expName);
}

void Manager::addValueTablePreset()
{
    const auto name = ui_.getCurrentExperiment();
    if (name.empty())
        return;
    showValueTableInputDialog(name, true);
}

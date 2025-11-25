#include "ui.h"
#include <pdcurses.h>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <cstring>

// -------------------- UiOutputPane --------------------

int UiOutputPane::addTextTab(const std::string &title, const std::string &text)
{
    Tab t;
    t.title = title;
    t.type = TabType::Text;
    t.text = text;
    tabs_.push_back(std::move(t));
    if ((int)tabs_.size() == 1)
        selected_ = 0;
    return (int)tabs_.size() - 1;
}

int UiOutputPane::addTableTab(const std::string &title, const TableData &table)
{
    Tab t;
    t.title = title;
    t.type = TabType::Table;
    t.table = table;
    tabs_.push_back(std::move(t));
    if ((int)tabs_.size() == 1)
        selected_ = 0;
    return (int)tabs_.size() - 1;
}

int UiOutputPane::addPlotTab(const std::string &title, const PlotData &plot)
{
    Tab t;
    t.title = title;
    t.type = TabType::Plot;
    t.plot = plot;
    tabs_.push_back(std::move(t));
    if ((int)tabs_.size() == 1)
        selected_ = 0;
    return (int)tabs_.size() - 1;
}

void UiOutputPane::clear()
{
    tabs_.clear();
    selected_ = 0;
    tableScroll_ = 0;
}

void UiOutputPane::setSelected(int idx)
{
    if (idx >= 0 && idx < (int)tabs_.size())
    {
        selected_ = idx;
        tableScroll_ = 0; // 切换标签页时重置滚动
    }
}

// -------------------- UI --------------------

UI *UI::self_ = nullptr;

UI::UI(int &status) : statusRef_(status)
{
    self_ = this;
}

UI::~UI()
{
    if (self_ == this)
        self_ = nullptr;
}

UI *UI::instance() { return self_; }

UiOutputPane &UI::output() { return output_; }

std::string UI::getInput() const { return inputBuffer_; }

void UI::setInput(const std::string &text) { inputBuffer_ = text; }

void UI::clearInput() { inputBuffer_.clear(); }

std::string UI::getDescription() const { return descriptionText_; }

void UI::setDescription(const std::string &desc) { descriptionText_ = desc; }

void UI::onExperimentChanged(ExperimentCallback cb) { expCallback_ = cb; }

void UI::onInputSubmit(InputCallback cb) { inputCallback_ = cb; }

void UI::onPresetChange(PresetChangeCallback cb) { presetCallback_ = cb; }

void UI::onAddPreset(AddPresetCallback cb) { addPresetCallback_ = cb; }

std::string UI::getCurrentExperiment() const { return currentExperiment_; }

void UI::addInputField(const std::string &label, const std::string &defaultValue, const std::string &placeholder)
{
    InputField field;
    field.label = label;
    field.value = defaultValue;
    field.placeholder = placeholder;
    inputFields_.push_back(field);
}

void UI::setInputFields(const std::vector<InputField> &fields)
{
    inputFields_ = fields;
    currentInputIndex_ = 0;
    inputScroll_ = 0;
}

std::vector<InputField> UI::getInputFields() const
{
    return inputFields_;
}

std::string UI::getInputValue(int index) const
{
    if (index >= 0 && index < (int)inputFields_.size())
    {
        return inputFields_[index].value;
    }
    return "";
}

std::string UI::getInputValue(const std::string &label) const
{
    for (const auto &field : inputFields_)
    {
        if (field.label == label)
        {
            return field.value;
        }
    }
    return "";
}

void UI::clearInputFields()
{
    inputFields_.clear();
    currentInputIndex_ = 0;
    inputScroll_ = 0;
}

int UI::getInputFieldCount() const
{
    return (int)inputFields_.size();
}

void UI::initCurses()
{
    initscr();
    resize_term(40, 140);
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, FALSE);
    curs_set(0);
    start_color();
    use_default_colors();
    mousemask(BUTTON1_CLICKED | BUTTON1_DOUBLE_CLICKED, nullptr);

    // 常用配色
    init_pair(1, COLOR_CYAN, -1);           // 标题
    init_pair(2, COLOR_YELLOW, -1);         // 高亮/选中
    init_pair(3, COLOR_GREEN, -1);          // 次要标题
    init_pair(4, COLOR_WHITE, COLOR_BLUE);  // 头条/Tab 背景
    init_pair(5, COLOR_BLACK, COLOR_WHITE); // 反色标签
    init_pair(6, COLOR_RED, -1);            // 零点标记
}

void UI::endCurses()
{
    endwin();
}

void UI::drawKeyHints(const std::string &context)
{
    int h, w;
    getmaxyx(stdscr, h, w);
    attron(A_DIM);
    mvhline(h - 2, 0, ' ', w);
    std::string line = context;
    if ((int)line.size() > w - 2)
        line.resize(w - 2);
    mvprintw(h - 2, 1, "%s", line.c_str());
    attroff(A_DIM);
}

void UI::buildExperimentTree()
{
    tree_.clear();
    // 章节/实验目录
    auto ch2 = ExperimentNode{"第二章 方程(组)的迭代解法", true, false, {{"1.1 画图法", false, false, {}}, {"1.2 对分法", false, false, {}}, {"1.3 扫描法", false, false, {}}, {"2.1 埃特肯法", false, false, {}}, {"2.2.1 牛顿迭代法", false, false, {}}, {"2.2.2 牛顿下山法", false, false, {}}, {"2.3.1 单点弦截法", false, false, {}}, {"2.3.2 双点弦截法", false, false, {}}}};
    auto ch3 = ExperimentNode{"第三章 解线性方程组的直接法", true, false, {{"1.1 高斯消元法", false, false, {}}, {"1.2 克劳特消元法", false, false, {}}, {"2. 平方根法", false, false, {}}, {"3. （追赶法）", false, false, {}}, {"4.1 列主元素法", false, false, {}}, {"4.2 全主元素法", false, false, {}}}};
    auto ch4 = ExperimentNode{"第四章 解线性方程组的迭代法", true, false, {{"1. 雅可比迭代法", false, false, {}}, {"2. 高斯-赛德尔迭代法", false, false, {}}, {"3. 松弛迭代法", false, false, {}}}};
    auto ch5 = ExperimentNode{"第五章 插值法", true, false, {{"1. 差商/差分", false, false, {}}, {"2. 不等距牛顿差商", false, false, {}}, {"3. 等距牛顿差分", false, false, {}}, {"4. 牛顿前插", false, false, {}}, {"5. 牛顿后插", false, false, {}}, {"6. 拉格朗日插值", false, false, {}}, {"7. 反插值", false, false, {}}, {"8. 埃尔米特插值", false, false, {}}}};
    auto ch6 = ExperimentNode{"第六章 数值积分", true, false, {{"1. 牛顿-科特斯/复化", false, false, {}}, {"2. 复合梯形/辛普森", false, false, {}}, {"3. 龙贝格法", false, false, {}}, {"4. 高斯求积", false, false, {}}}};
    tree_.push_back(ch2);
    tree_.push_back(ch3);
    tree_.push_back(ch4);
    tree_.push_back(ch5);
    tree_.push_back(ch6);
}

void UI::populateSampleContent()
{
    output_.clear();
    // 示例文本
    std::ostringstream oss;
    oss << "示例说明：此处展示选中实验的说明、输入与输出。\n";
    oss << "- 在输入区键入参数并回车确认\n";
    oss << "- 输出区包含：文本、表格、折线图（ASCII）\n";
    output_.addTextTab("说明", oss.str());

    // 示例表格
    UiOutputPane::TableData tbl;
    tbl.headers = {"k", "x_k", "error"};
    for (int i = 0; i < 10; ++i)
    {
        tbl.rows.push_back({std::to_string(i), std::to_string(1.0 / (i + 1)), std::to_string(0.01 * i)});
    }
    output_.addTableTab("迭代表", tbl);

    // 示例折线图
    UiOutputPane::PlotData plot;
    for (int i = 0; i <= 40; ++i)
    {
        double x = i * 0.25;
        plot.xs.push_back(x);
        plot.ys.push_back(std::sin(x));
    }
    plot.xlabel = "x";
    plot.ylabel = "sin(x)";
    output_.addPlotTab("曲线", plot);
}

// 绘制主菜单
void UI::loopMainMenu()
{
    const char *items[] = {"开始实验", "开发说明", "退出"};
    int nItems = 3;
    int sel = 0;

    while (running_ && screen_ == Screen::MainMenu)
    {
        erase();
        int h, w;
        getmaxyx(stdscr, h, w);

        // 标题
        attron(COLOR_PAIR(1) | A_BOLD);
        mvprintw(1, 2, "数值分析实验平台");
        attroff(COLOR_PAIR(1) | A_BOLD);

        int boxW = std::min(40, std::max(28, w / 4));
        int boxH = nItems * 2 + 5;
        int boxY = (h - boxH) / 2;
        int boxX = (w - boxW) / 2;
        mvaddch(boxY, boxX, '+');
        mvhline(boxY, boxX + 1, '-', boxW - 2);
        mvaddch(boxY, boxX + boxW - 1, '+');
        mvvline(boxY + 1, boxX, '|', boxH - 2);
        mvvline(boxY + 1, boxX + boxW - 1, '|', boxH - 2);
        mvaddch(boxY + boxH - 1, boxX, '+');
        mvhline(boxY + boxH - 1, boxX + 1, '-', boxW - 2);
        mvaddch(boxY + boxH - 1, boxX + boxW - 1, '+');

        mvprintw(boxY + 1, boxX + 2, "请选择：");
        for (int i = 0; i < nItems; ++i)
        {
            if (i == sel)
                attron(COLOR_PAIR(2) | A_REVERSE);
            mvprintw(boxY + 3 + i * 2, boxX + 4, "%s", items[i]);
            if (i == sel)
                attroff(COLOR_PAIR(2) | A_REVERSE);
        }

        drawKeyHints("↑↓ 选择  Enter 确认  Q 退出  鼠标点击选择");
        refresh();

        int ch = getch();
        if (ch == KEY_MOUSE)
        {
            MEVENT me;
            if (nc_getmouse(&me) == OK && (me.bstate & (BUTTON1_CLICKED | BUTTON1_DOUBLE_CLICKED)))
            {
                int mx = me.x, my = me.y;
                for (int i = 0; i < nItems; ++i)
                {
                    int itemY = boxY + 3 + i * 2;
                    if (my == itemY && mx >= boxX + 4 && mx < boxX + boxW - 4)
                    {
                        sel = i;
                        if (me.bstate & BUTTON1_DOUBLE_CLICKED)
                        {
                            ch = '\n';
                            break;
                        }
                    }
                }
            }
        }
        if (ch == KEY_UP)
            sel = (sel - 1 + nItems) % nItems;
        else if (ch == KEY_DOWN)
            sel = (sel + 1) % nItems;
        if (ch == '\n' || ch == '\r')
        {
            if (sel == 0)
            {
                screen_ = Screen::Experiment;
                buildExperimentTree();
            }
            else if (sel == 1)
            {
                // 开发说明简要弹窗
                int ph = 10, pw = std::min(60, std::max(40, w - 6));
                int py = (h - ph) / 2, px = (w - pw) / 2;
                WINDOW *win = newwin(ph, pw, py, px);
                box(win, 0, 0);
                wattron(win, A_BOLD);
                mvwprintw(win, 1, 2, "开发说明");
                wattroff(win, A_BOLD);
                mvwprintw(win, 3, 2, "- C++20 + PDCurses 终端 UI");
                mvwprintw(win, 4, 2, "- 右上角实验列表可展开/收起");
                mvwprintw(win, 5, 2, "- 左侧包含说明/输入/输出");
                mvwprintw(win, 7, 2, "按任意键关闭...");
                wrefresh(win);
                int key;
                do
                {
                    key = getch();
                } while (key == KEY_MOUSE);
                delwin(win);
            }
            else
            {
                running_ = false;
            }
        }
        else if (ch == 'q' || ch == 'Q')
        {
            running_ = false;
        }
    }
}

// 渲染实验列表（右上角）并处理鼠标/键盘命中映射
static void flattenTree(const std::vector<ExperimentNode> &nodes, std::vector<std::pair<int, const ExperimentNode *>> &out, int indent = 0)
{
    for (auto &n : nodes)
    {
        out.push_back({indent, &n});
        if (n.isChapter && n.expanded)
        {
            flattenTree(n.children, out, indent + 2);
        }
    }
}

// 在本实现里，为简化，展开/折叠通过点击标题区域或按空格键实现；
// 进入实验通过回车或点击实验项文本区域实现。
void UI::loopExperiment()
{
    bool descCollapsed = false, inputCollapsed = false, outputCollapsed = false;
    int focus = 0; // 0: desc, 1: input, 2: output, 3: list
    inputBuffer_.clear();

    while (running_ && screen_ == Screen::Experiment)
    {
        erase();
        int h, w;
        getmaxyx(stdscr, h, w);

        attron(COLOR_PAIR(1) | A_BOLD);
        mvprintw(0, 2, "实验页面");
        attroff(COLOR_PAIR(1) | A_BOLD);
        const char *focusNames[] = {"说明", "输入", "输出", "列表"};
        attron(COLOR_PAIR(3));
        mvprintw(0, 16, "[焦点: %s]", focusNames[focus]);
        attroff(COLOR_PAIR(3));
        int listW = showTree_ ? std::max(28, w / 4) : 12;
        std::string toggle = showTree_ ? "[ 实验列表 ◂ ]" : "[ 实验列表 ▸ ]";
        int toggleX = w - (int)toggle.size() - 2;
        mvprintw(0, toggleX, "%s", toggle.c_str());

        // 左主区可用宽度/高度
        int leftW = showTree_ ? (w - listW - 3) : (w - 4);
        int leftX = 2;
        int y = 2;

        // 面板：说明
        int pnlH = std::max(6, h / 4);
        if (focus == 0)
            attron(COLOR_PAIR(2) | A_BOLD);
        mvprintw(y, leftX, "[ %s ] 实验说明", descCollapsed ? ">" : "v");
        // 在标题行右侧显示 Preset 切换控件（ASCII，便于精确点击）
        {
            const char *presetCtl = "Preset: [<] [>]";
            int ctlLen = (int)std::strlen(presetCtl);
            int ctlX = std::max(leftX + 18, leftX + leftW - ctlLen - 1);
            mvprintw(y, ctlX, "%s", presetCtl);
        }
        if (focus == 0)
            attroff(COLOR_PAIR(2) | A_BOLD);
        mvhline(y + 1, leftX, '-', leftW);
        if (!descCollapsed)
        {
            if (descriptionText_.empty())
            {
                mvprintw(y + 2, leftX, "选中实验后将在此显示说明文本。");
                mvprintw(y + 3, leftX, "提示：可使用鼠标点击界面各区域进行操作。");
            }
            else
            {
                std::istringstream iss(descriptionText_);
                std::string line;
                int row = 0;
                int maxRows = pnlH - 3;
                while (std::getline(iss, line) && row < maxRows)
                {
                    if ((int)line.size() > leftW - 1)
                        line.resize(leftW - 1);
                    mvprintw(y + 2 + row, leftX, "%s", line.c_str());
                    ++row;
                }
            }
        }
        int descBottom = y + (descCollapsed ? 2 : (pnlH));
        y = descBottom + 1;

        // 面板：输入
        if (focus == 1)
            attron(COLOR_PAIR(2) | A_BOLD);
        mvprintw(y, leftX, "[ %s ] 输入区", inputCollapsed ? ">" : "v");
        if (focus == 1)
            attroff(COLOR_PAIR(2) | A_BOLD);
        mvhline(y + 1, leftX, '-', leftW);
        if (!inputCollapsed)
        {
            if (inputFields_.empty())
            {
                mvprintw(y + 2, leftX, "参数: %s", inputBuffer_.c_str());
                mvprintw(y + 3, leftX, "回车确认  Tab 切换区域");
            }
            else
            {
                const int visible = 5;
                int n = (int)inputFields_.size();
                // 保证当前索引在可见窗口内
                if (currentInputIndex_ < inputScroll_)
                    inputScroll_ = currentInputIndex_;
                if (currentInputIndex_ >= inputScroll_ + visible)
                    inputScroll_ = currentInputIndex_ - visible + 1;
                inputScroll_ = std::clamp(inputScroll_, 0, std::max(0, n - visible));

                int row = 0;
                for (int i = inputScroll_; i < n && row < visible; ++i)
                {
                    const auto &field = inputFields_[i];
                    bool isCurrent = (i == currentInputIndex_ && focus == 1);

                    if (isCurrent)
                        attron(A_REVERSE);
                    std::string display = field.label + " ";
                    if (!field.value.empty())
                        display += field.value;
                    else if (!field.placeholder.empty())
                        display += "[" + field.placeholder + "]";

                    if ((int)display.size() > leftW - 1)
                        display.resize(leftW - 1);
                    mvprintw(y + 2 + row, leftX, "%s", display.c_str());
                    if (isCurrent)
                        attroff(A_REVERSE);
                    ++row;
                }
                // 底部提示与滚动状态
                if (n > visible)
                {
                    mvprintw(y + 2 + visible, leftX, "↑↓ 切换字段  第 %d/%d", currentInputIndex_ + 1, n);
                }
                else
                {
                    mvprintw(y + 2 + std::min(visible, n), leftX, "↑↓ 切换字段  Enter 确认  Tab 切换区域");
                }
            }
        }
        // 输入面板高度：标题(1)+分隔线(1)+内容(visible 或 n 中较小)+提示(1)
        int inputOpenHeight;
        if (inputCollapsed)
            inputOpenHeight = 2;
        else if (inputFields_.empty())
            inputOpenHeight = 4; // 参数行+提示行
        else
            inputOpenHeight = 2 + std::min(5, (int)inputFields_.size()) + 1;
        int inputBottom = y + inputOpenHeight;
        y = inputBottom + 1;

        // 面板：输出
        int outTop = y;
        if (focus == 2)
            attron(COLOR_PAIR(2) | A_BOLD);
        mvprintw(y, leftX, "[ %s ] 输出区", outputCollapsed ? ">" : "v");
        if (focus == 2)
            attroff(COLOR_PAIR(2) | A_BOLD);
        mvhline(y + 1, leftX, '-', leftW);
        if (!outputCollapsed)
        {
            // Tab 头
            int tabX = leftX;
            for (int i = 0; i < (int)output_.tabs().size(); ++i)
            {
                const auto &t = output_.tabs()[i];
                std::string cap = " " + t.title + " ";
                if (i == output_.selected())
                    attron(COLOR_PAIR(5));
                mvprintw(y + 2, tabX, "%s", cap.c_str());
                if (i == output_.selected())
                    attroff(COLOR_PAIR(5));
                tabX += (int)cap.size() + 1;
            }

            // 内容区
            int contentY = y + 4;
            int contentH = h - contentY - 3;
            int contentW = leftW;
            if (contentH > 0)
            {
                const auto &t = output_.tabs().empty() ? *(new UiOutputPane::Tab()) : output_.tabs()[output_.selected()];
                // 渲染不同类型
                if (output_.tabs().empty())
                {
                    mvprintw(contentY, leftX, "暂无输出");
                }
                else if (t.type == UiOutputPane::TabType::Text)
                {
                    // 简单多行文本
                    std::istringstream iss(t.text);
                    std::string line;
                    int row = 0;
                    while (std::getline(iss, line) && row < contentH)
                    {
                        if ((int)line.size() > contentW - 1)
                            line.resize(contentW - 1);
                        mvprintw(contentY + row, leftX, "%s", line.c_str());
                        ++row;
                    }
                }
                else if (t.type == UiOutputPane::TabType::Table)
                {
                    // 表格：简单定宽截断，带滚动
                    int cols = (int)t.table.headers.size();
                    int colW = cols ? std::max(8, contentW / cols) : contentW - 2;
                    // 头
                    for (int c = 0; c < cols; ++c)
                    {
                        std::string cell = t.table.headers[c];
                        if ((int)cell.size() > colW - 1)
                            cell.resize(colW - 1);
                        mvprintw(contentY, leftX + c * colW, "%s", cell.c_str());
                    }
                    // 可见行数（减去表头和底部提示）
                    int visibleRows = std::max(1, contentH - 2);
                    int totalRows = (int)t.table.rows.size();
                    int scroll = output_.tableScroll();
                    scroll = std::clamp(scroll, 0, std::max(0, totalRows - visibleRows));
                    output_.setTableScroll(scroll);

                    // 行（带滚动）
                    for (int r = 0; r < visibleRows && (scroll + r) < totalRows; ++r)
                    {
                        int rowIdx = scroll + r;
                        for (int c = 0; c < cols && c < (int)t.table.rows[rowIdx].size(); ++c)
                        {
                            std::string cell = t.table.rows[rowIdx][c];
                            if ((int)cell.size() > colW - 1)
                                cell.resize(colW - 1);
                            mvprintw(contentY + 1 + r, leftX + c * colW, "%s", cell.c_str());
                        }
                    }
                    // 底部滚动提示
                    if (totalRows > visibleRows)
                    {
                        mvprintw(contentY + contentH - 1, leftX, "↑↓ 翻页  第 %d-%d/%d 行",
                                 scroll + 1, std::min(scroll + visibleRows, totalRows), totalRows);
                    }
                }
                else if (t.type == UiOutputPane::TabType::Plot)
                {
                    // ASCII 折线图（带坐标刻度）
                    int plotH = contentH - 3;  // 留一行给 x 轴刻度
                    int plotW = contentW - 12; // 左侧留空给 y 轴刻度
                    if (plotH > 2 && plotW > 10 && !t.plot.xs.empty())
                    {
                        double xmin = t.plot.xmin, xmax = t.plot.xmax;
                        double ymin = t.plot.ymin, ymax = t.plot.ymax;
                        if (ymin == ymax)
                        {
                            ymin -= 1;
                            ymax += 1;
                        }
                        int axisX = leftX + 10; // y 轴位置
                        // 绘制 y 轴
                        for (int yy = 0; yy <= plotH; ++yy)
                            mvaddch(contentY + yy, axisX, '|');
                        // 绘制 x 轴
                        mvprintw(contentY + plotH, axisX, "+");
                        mvhline(contentY + plotH, axisX + 1, '-', plotW);

                        // y 轴刻度（顶部、中部、底部）
                        std::ostringstream oss;
                        oss.setf(std::ios::fixed);
                        oss.precision(2);
                        oss << ymax;
                        mvprintw(contentY, leftX, "%s", oss.str().c_str());
                        oss.str("");
                        oss << (ymin + ymax) / 2.0;
                        mvprintw(contentY + plotH / 2, leftX, "%s", oss.str().c_str());
                        oss.str("");
                        oss << ymin;
                        mvprintw(contentY + plotH, leftX, "%s", oss.str().c_str());

                        // x 轴刻度（左、中、右）
                        oss.str("");
                        oss << xmin;
                        mvprintw(contentY + plotH + 1, axisX, "%s", oss.str().c_str());
                        oss.str("");
                        oss << (xmin + xmax) / 2.0;
                        int midX = axisX + plotW / 2;
                        mvprintw(contentY + plotH + 1, midX - 3, "%s", oss.str().c_str());
                        oss.str("");
                        oss << xmax;
                        mvprintw(contentY + plotH + 1, axisX + plotW - 6, "%s", oss.str().c_str());

                        // 绘点
                        for (size_t i = 0; i < t.plot.xs.size(); ++i)
                        {
                            double xval = t.plot.xs[i];
                            double yval = t.plot.ys[i];
                            int xpix = (int)((xval - xmin) / (xmax - xmin) * plotW);
                            xpix = std::clamp(xpix, 0, plotW - 1);
                            int ypix = (int)std::round((ymax - yval) / (ymax - ymin) * plotH);
                            ypix = std::clamp(ypix, 0, plotH);
                            mvaddch(contentY + ypix, axisX + 1 + xpix, '*');
                        }
                        // 标记零点（红色）
                        if (t.plot.hasRoot && std::isfinite(t.plot.rootX))
                        {
                            double rootX = t.plot.rootX;
                            if (rootX >= xmin && rootX <= xmax)
                            {
                                int xpix = (int)((rootX - xmin) / (xmax - xmin) * plotW);
                                xpix = std::clamp(xpix, 0, plotW - 1);
                                // 在 y=0 附近标记
                                int ypix0 = (int)std::round((ymax - 0.0) / (ymax - ymin) * plotH);
                                ypix0 = std::clamp(ypix0, 0, plotH);
                                attron(COLOR_PAIR(6) | A_BOLD);
                                mvaddch(contentY + ypix0, axisX + 1 + xpix, 'O');
                                attroff(COLOR_PAIR(6) | A_BOLD);
                            }
                        }
                        // 标签
                        mvprintw(contentY, axisX + 2, "%s", t.plot.ylabel.c_str());
                        mvprintw(contentY + plotH + 2, axisX + plotW / 2 - 3, "%s", t.plot.xlabel.c_str());
                    }
                    else
                    {
                        mvprintw(contentY, leftX, "数据不足，无法绘制");
                    }
                }
            }
        }

        // 右上角实验列表
        int listX = w - (showTree_ ? listW : (int)toggle.size() + 4);
        // 列表头边框
        if (showTree_)
        {
            int listY = 2;
            int listH = h - 5;
            // 边框
            mvaddch(listY, listX - 1, '+');
            mvhline(listY, listX, '-', listW - 2);
            mvaddch(listY, listX + listW - 2, '+');
            mvvline(listY + 1, listX - 1, '|', listH - 2);
            mvvline(listY + 1, listX + listW - 2, '|', listH - 2);
            mvaddch(listY + listH - 1, listX - 1, '+');
            mvhline(listY + listH - 1, listX, '-', listW - 2);
            mvaddch(listY + listH - 1, listX + listW - 2, '+');
            if (focus == 3)
                attron(COLOR_PAIR(2) | A_BOLD);
            mvprintw(listY, listX + 2, "实验列表");
            if (focus == 3)
                attroff(COLOR_PAIR(2) | A_BOLD);

            // 展平并滚动展示
            std::vector<std::pair<int, const ExperimentNode *>> flat;
            flattenTree(tree_, flat);
            int visible = listH - 2;
            treeScroll_ = std::clamp(treeScroll_, 0, std::max(0, (int)flat.size() - visible));
            for (int i = 0; i < visible && i + treeScroll_ < (int)flat.size(); ++i)
            {
                int idx = i + treeScroll_;
                int indent = flat[idx].first;
                const ExperimentNode *node = flat[idx].second;
                std::string marker;
                if (node->isChapter)
                    marker = node->expanded ? "v " : "> ";
                else
                    marker = "• ";
                bool isCurrentExp = (!node->isChapter && node->title == currentExperiment_);
                if (idx == treeCursor_)
                    attron(A_REVERSE);
                else if (isCurrentExp)
                    attron(COLOR_PAIR(2) | A_BOLD);
                std::string text = std::string(indent, ' ') + marker + node->title;
                int maxWidth = listW - 3;
                if ((int)text.size() > maxWidth)
                    text.resize(maxWidth);
                mvprintw(listY + 1 + i, listX, "%s", text.c_str());
                if (idx == treeCursor_)
                    attroff(A_REVERSE);
                else if (isCurrentExp)
                    attroff(COLOR_PAIR(2) | A_BOLD);
            }

            drawKeyHints("Tab 切换  ↑↓/PgUp/PgDn 滚动  Enter 选择  Space 展开  L 列表  Ctrl+L/Space(输入区)  Q 返回");
        }
        else
        {
            drawKeyHints("Tab 切换  L 列表  Ctrl+L/Space(输入区)  Q 返回");
        }

        refresh();

        // 输入处理
        int ch = getch();
        if (ch == 'q' || ch == 'Q')
        {
            if (focus != 1)
            {
                int h, w;
                getmaxyx(stdscr, h, w);
                int ph = 7, pw = 40;
                int py = (h - ph) / 2, px = (w - pw) / 2;
                WINDOW *win = newwin(ph, pw, py, px);
                box(win, 0, 0);
                wattron(win, A_BOLD);
                mvwprintw(win, 1, 2, "确认退出");
                wattroff(win, A_BOLD);
                mvwprintw(win, 3, 2, "确定要返回主菜单吗？");
                mvwprintw(win, 5, 2, "Y - 是    N - 否");
                wrefresh(win);
                int key;
                do
                {
                    key = getch();
                } while (key == KEY_MOUSE);
                delwin(win);
                if (key == 'y' || key == 'Y')
                {
                    screen_ = Screen::MainMenu;
                    break;
                }
            }
        }
        if (ch == 'l' || ch == 'L')
        {
            if (focus != 1)
            {
                showTree_ = !showTree_;
                continue;
            }
        }
        if (ch == 12)
        {
            showTree_ = !showTree_;
            continue;
        }
        if (ch == '\t')
        {
            // Tab：区域切换
            focus = (focus + 1) % 4;
            continue;
        }
        if (ch == KEY_BTAB)
        {
            // Shift+Tab：区域切换（反向）
            focus = (focus - 1 + 4) % 4;
            continue;
        }

        if (ch == KEY_MOUSE)
        {
            MEVENT me;
            if (nc_getmouse(&me) == OK && (me.bstate & (BUTTON1_CLICKED | BUTTON1_DOUBLE_CLICKED)))
            {
                int mx = me.x, my = me.y;
                int hh, ww;
                getmaxyx(stdscr, hh, ww);
                // 计算各区域位置（需与绘制时一致）
                int listW2 = showTree_ ? std::max(28, ww / 4) : 12;
                std::string toggle2 = showTree_ ? "[ 实验列表 ◂ ]" : "[ 实验列表 ▸ ]";
                int toggleX2 = ww - (int)toggle2.size() - 2;
                int leftW2 = showTree_ ? (ww - listW2 - 3) : (ww - 4);
                int leftX2 = 2;
                int y2 = 2;
                int pnlH2 = std::max(6, hh / 4);
                int descBottom2 = y2 + (descCollapsed ? 2 : (pnlH2));
                int yAfterDesc = descBottom2 + 1;
                int inputBottom2 = yAfterDesc + (inputCollapsed ? 2 : 6);
                int outTop2 = inputBottom2 + 1;

                // 点击切换列表展开/收起（顶部 toggle）
                if (my == 0 && mx >= toggleX2 && mx < toggleX2 + (int)toggle2.size())
                {
                    showTree_ = !showTree_;
                    continue;
                }

                // 点击折叠各面板标题行 或 Preset 控件
                if (my == 2 && mx >= leftX2 && mx < leftX2 + leftW2)
                {
                    // 计算 Preset 控件区域
                    const char *presetCtl = "Preset: [<] [>]";
                    int ctlLen = (int)std::strlen(presetCtl);
                    int ctlX = std::max(leftX2 + 18, leftX2 + leftW2 - ctlLen - 1);
                    int leftArrowStart = ctlX + 9;            // "Preset: " 长度为 9
                    int rightArrowStart = leftArrowStart + 4; // "[<] "

                    if (mx >= leftArrowStart && mx < leftArrowStart + 3)
                    {
                        if (presetCallback_)
                            presetCallback_(-1);
                    }
                    else if (mx >= rightArrowStart && mx < rightArrowStart + 3)
                    {
                        if (presetCallback_)
                            presetCallback_(+1);
                    }
                    else
                    {
                        descCollapsed = !descCollapsed;
                    }
                    continue;
                }
                if (my == (descBottom2 + 1) && mx >= leftX2 && mx < leftX2 + leftW2)
                {
                    inputCollapsed = !inputCollapsed;
                    continue;
                }
                if (my == outTop2 && mx >= leftX2 && mx < leftX2 + leftW2)
                {
                    outputCollapsed = !outputCollapsed;
                    continue;
                }

                // 点击输出区 Tab
                if (!outputCollapsed)
                {
                    int tabRow = outTop2 + 2;
                    if (my == tabRow)
                    {
                        int tabX = leftX2;
                        for (int i = 0; i < (int)output_.tabs().size(); ++i)
                        {
                            std::string cap = " " + output_.tabs()[i].title + " ";
                            int tabLen = (int)cap.size();
                            if (mx >= tabX && mx < tabX + tabLen)
                            {
                                output_.setSelected(i);
                                break;
                            }
                            tabX += tabLen + 1;
                        }
                    }
                }

                // 点击实验列表项
                if (showTree_)
                {
                    int listY2 = 2;
                    int listH2 = hh - 5;
                    int listX2 = ww - listW2;
                    if (mx >= listX2 && mx < listX2 + listW2 - 2 && my > listY2 && my < listY2 + listH2 - 1)
                    {
                        int idxInView = my - (listY2 + 1);
                        std::vector<std::pair<int, ExperimentNode *>> flat;
                        std::function<void(std::vector<ExperimentNode> &, int)> flatfn = [&](std::vector<ExperimentNode> &nodes, int indent)
                        {
                            for (auto &n : nodes)
                            {
                                flat.push_back({indent, &n});
                                if (n.isChapter && n.expanded)
                                    flatfn(n.children, indent + 2);
                            }
                        };
                        flatfn(tree_, 0);
                        int visible = listH2 - 2;
                        int idx = treeScroll_ + idxInView;
                        if (idx >= 0 && idx < (int)flat.size())
                        {
                            treeCursor_ = idx;
                            auto *node = flat[idx].second;
                            if (node->isChapter)
                                node->expanded = !node->expanded;
                            else
                            {
                                currentExperiment_ = node->title;
                                if (expCallback_)
                                    expCallback_(node->title);
                            }
                        }
                    }
                }
            }
            continue;
        }

        if (ch == ' ')
        {
            if (focus == 0)
                descCollapsed = !descCollapsed;
            else if (focus == 1)
            {
                // 此处也可以进行其他操作
                inputBuffer_.push_back((char)ch);
            }
            else if (focus == 2)
                outputCollapsed = !outputCollapsed;
        }
        else if (ch == 0)
        {
            if (focus == 0)
                descCollapsed = !descCollapsed;
            else if (focus == 1)
                inputCollapsed = !inputCollapsed;
            else if (focus == 2)
                outputCollapsed = !outputCollapsed;
        }
        else if (focus == 0)
        {
            if (ch == KEY_LEFT)
            {
                if (presetCallback_)
                    presetCallback_(-1);
            }
            else if (ch == KEY_RIGHT)
            {
                if (presetCallback_)
                    presetCallback_(+1);
            }
            else if (ch == 'a' || ch == 'A')
            {
                if (addPresetCallback_)
                    addPresetCallback_();
            }
        }
        else if (focus == 1)
        {
            if (!inputFields_.empty())
            {
                // 内部输入框用上下切换，并自动滚动
                if (ch == KEY_UP)
                {
                    if (currentInputIndex_ > 0)
                        currentInputIndex_--;
                    if (currentInputIndex_ < inputScroll_)
                        inputScroll_ = currentInputIndex_;
                    continue;
                }
                if (ch == KEY_DOWN)
                {
                    int n = (int)inputFields_.size();
                    if (currentInputIndex_ < n - 1)
                        currentInputIndex_++;
                    const int visible = 5;
                    if (currentInputIndex_ >= inputScroll_ + visible)
                        inputScroll_ = currentInputIndex_ - visible + 1;
                    continue;
                }
            }
            if (ch == KEY_BACKSPACE || ch == 127 || ch == 8)
            {
                if (!inputFields_.empty())
                {
                    auto &field = inputFields_[currentInputIndex_];
                    if (!field.value.empty())
                        field.value.pop_back();
                }
                else if (!inputBuffer_.empty())
                {
                    inputBuffer_.pop_back();
                }
            }
            else if (ch == '\n' || ch == '\r')
            {
                if (inputCallback_)
                {
                    if (!inputFields_.empty())
                    {
                        std::ostringstream oss;
                        for (size_t i = 0; i < inputFields_.size(); ++i)
                        {
                            if (i > 0)
                                oss << ",";
                            oss << inputFields_[i].value;
                        }
                        inputCallback_(oss.str());
                    }
                    else
                    {
                        inputCallback_(inputBuffer_);
                    }
                }
            }
            else if (isprint(ch) && ch != '\t')
            {
                if (!inputFields_.empty())
                {
                    auto &field = inputFields_[currentInputIndex_];
                    if ((int)field.value.size() < field.maxLength)
                        field.value.push_back((char)ch);
                }
                else
                {
                    inputBuffer_.push_back((char)ch);
                }
            }
        }
        else if (focus == 2)
        {
            if (ch == KEY_LEFT)
            {
                output_.setSelected(std::max(0, output_.selected() - 1));
            }
            else if (ch == KEY_RIGHT)
            {
                output_.setSelected(std::min((int)output_.tabs().size() - 1, output_.selected() + 1));
            }
            else if (ch == KEY_UP || ch == KEY_DOWN)
            {
                // 如果当前选中的是表格标签页，处理翻页
                if (!output_.tabs().empty())
                {
                    const auto &tab = output_.tabs()[output_.selected()];
                    if (tab.type == UiOutputPane::TabType::Table)
                    {
                        int scroll = output_.tableScroll();
                        int totalRows = (int)tab.table.rows.size();
                        int contentH = h - (outTop + 4) - 3;
                        int visibleRows = std::max(1, contentH - 2);
                        if (ch == KEY_UP)
                            scroll = std::max(0, scroll - 1);
                        else
                            scroll = std::min(std::max(0, totalRows - visibleRows), scroll + 1);
                        output_.setTableScroll(scroll);
                    }
                }
            }
        }
        else if (focus == 3 && showTree_)
        { // 实验列表：上下移动/滚动/选择/展开
            // 展平索引与展开控制需要映射到原树结构，这里采用简单的方式：
            std::vector<std::pair<int, ExperimentNode *>> flat;
            // 可修改版展平
            std::function<void(std::vector<ExperimentNode> &, int)> flatfn = [&](std::vector<ExperimentNode> &nodes, int indent)
            {
                for (auto &n : nodes)
                {
                    flat.push_back({indent, &n});
                    if (n.isChapter && n.expanded)
                        flatfn(n.children, indent + 2);
                }
            };
            flatfn(tree_, 0);
            int visible = std::max(1, h - 5 - 2);
            if (ch == KEY_UP)
            {
                treeCursor_ = std::max(0, treeCursor_ - 1);
                if (treeCursor_ < treeScroll_)
                    treeScroll_ = treeCursor_;
            }
            else if (ch == KEY_DOWN)
            {
                treeCursor_ = std::min(std::max(0, (int)flat.size() - 1), treeCursor_ + 1);
                if (treeCursor_ >= treeScroll_ + visible)
                    treeScroll_ = treeCursor_ - visible + 1;
            }
            else if (ch == KEY_NPAGE /*PgDn*/)
            {
                treeCursor_ = std::min(std::max(0, (int)flat.size() - 1), treeCursor_ + visible);
                treeScroll_ = std::min(std::max(0, (int)flat.size() - visible), treeScroll_ + visible);
            }
            else if (ch == KEY_PPAGE /*PgUp*/)
            {
                treeCursor_ = std::max(0, treeCursor_ - visible);
                treeScroll_ = std::max(0, treeScroll_ - visible);
            }
            else if (ch == ' ')
            {
                if (!flat.empty())
                {
                    auto *node = flat[treeCursor_].second;
                    if (node->isChapter)
                        node->expanded = !node->expanded;
                }
            }
            else if (ch == '\n' || ch == '\r')
            {
                if (!flat.empty())
                {
                    auto *node = flat[treeCursor_].second;
                    if (!node->isChapter)
                    {
                        currentExperiment_ = node->title;
                        if (expCallback_)
                            expCallback_(node->title);
                    }
                    else
                    {
                        node->expanded = !node->expanded;
                    }
                }
            }
        }
    }
}

void UI::run()
{
    initCurses();
    screen_ = Screen::MainMenu;
    while (running_)
    {
        if (screen_ == Screen::MainMenu)
            loopMainMenu();
        else if (screen_ == Screen::Experiment)
            loopExperiment();
        else
            break;
    }
    endCurses();
}

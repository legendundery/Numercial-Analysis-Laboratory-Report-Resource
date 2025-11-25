#include "manager.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <pdcurses.h>

using std::string;

Manager::Manager(UI &ui) : ui_(ui)
{
    ensurePresets();
    bindUiCallbacks();
}

void Manager::bindUiCallbacks()
{
    ui_.onExperimentChanged([this](const std::string &exp)
                            {
        initExperiment(exp);
        loadExperiment(exp);
        useExperiment(exp); });

    ui_.onInputSubmit([this](const std::string & /*inputJoined*/)
                      {
        const auto name = ui_.getCurrentExperiment();
        if (name.empty())
            return;
        saveExperiment(name);
        computeExperiment(name);
        useExperiment(name); });

    ui_.onPresetChange([this](int delta)
                       {
        const auto name = ui_.getCurrentExperiment();
        if (name.empty()) return;
        cyclePresetFor(name, delta);
        // 仅更新说明，不自动计算
        fillDescriptionFor(name); });

    ui_.onAddPreset([this]()
                    { addPolynomialPreset(); });
}

void Manager::initExperiment(const std::string &name)
{
    ensureDefaultsFor(name);
    fillDescriptionFor(name);
}

void Manager::saveExperiment(const std::string &name)
{
    auto &st = states_[name];
    if (ui_.getInputFieldCount() > 0)
    {
        st.fields = ui_.getInputFields();
    }
    else
    {
        // 兼容单输入：放入一个字段
        st.fields.clear();
        InputField f;
        f.label = "输入";
        f.value = ui_.getInput();
        f.placeholder = "";
        st.fields.push_back(f);
    }
}

void Manager::loadExperiment(const std::string &name)
{
    auto it = states_.find(name);
    if (it == states_.end() || it->second.fields.empty())
        return;
    ui_.setInputFields(it->second.fields);
}

void Manager::useExperiment(const std::string &name)
{
    auto it = states_.find(name);
    if (it == states_.end())
        return;
    const auto &snap = it->second.last;

    // 将最近结果展示到输出区（如果有）
    ui_.output().clear();
    if (snap.has)
    {
        ui_.output().addTextTab("摘要", snap.summary);
        if (!snap.table.headers.empty())
            ui_.output().addTableTab("迭代表", snap.table);
        if (!snap.plot.xs.empty())
            ui_.output().addPlotTab("曲线", snap.plot);
    }
    else
    {
        ui_.output().addTextTab("提示", "请在输入区设置参数后回车计算。");
    }
}

void Manager::computeExperiment(const std::string &name)
{
    if (name.find("画图法") != string::npos)
    {
        computePlot(name);
        return;
    }
    if (name.find("扫描法") != string::npos)
    {
        computeScan(name);
        return;
    }
    if (name.find("对分法") != string::npos)
    {
        computeBisection(name);
        return;
    }
    if (name.find("牛顿迭代法") != string::npos)
    {
        computeNewton(name);
        return;
    }
    // 其他实验暂未实现
    auto &st = states_[name];
    st.last = {};
    st.last.summary = "尚未实现该实验的计算，请选择对分法或牛顿迭代法作为示例。";
    st.last.has = true;
}

void Manager::ensureDefaultsFor(const std::string &name)
{
    auto &st = states_[name];
    if (!st.fields.empty())
        return;

    st.fields.clear();
    if (name.find("画图法") != string::npos)
    {
        st.fields.push_back({"区间左端 a:", "0.0", "例如 0.0", 50});
        st.fields.push_back({"区间右端 b:", "3.0", "例如 3.0", 50});
    }
    else if (name.find("扫描法") != string::npos)
    {
        st.fields.push_back({"区间左端 A:", "1.0", "例如 1.0", 50});
        st.fields.push_back({"区间右端 B:", "2.0", "例如 2.0", 50});
        st.fields.push_back({"分段数 n:", "10", "正整数", 10});
    }
    else if (name.find("对分法") != string::npos)
    {
        st.fields.push_back({"区间左端 a:", "1.0", "例如 1.0", 50});
        st.fields.push_back({"区间右端 b:", "2.0", "例如 2.0", 50});
        st.fields.push_back({"最大迭代步:", "50", "整数", 10});
        st.fields.push_back({"容差 tol:", "1e-6", "如 1e-6", 20});
    }
    else if (name.find("牛顿迭代法") != string::npos)
    {
        st.fields.push_back({"初值 x0:", "1.0", "例如 1.0", 50});
        st.fields.push_back({"最大迭代步:", "50", "整数", 10});
        st.fields.push_back({"容差 tol:", "1e-6", "如 1e-6", 20});
    }
    else
    {
        st.fields.push_back({"参数:", "", "该实验未配置输入示例", 50});
    }
    ui_.setInputFields(st.fields);
}

void Manager::fillDescriptionFor(const std::string &name)
{
    std::ostringstream oss;
    if (name.find("画图法") != string::npos)
    {
        oss << "画图法：\n";
        oss << "- 在给定区间 [a,b] 上绘制函数 f(x) 的图像\n";
        oss << "- 通过观察图像与 x 轴的交点来判断根的大致位置\n";
        if (!presets_.empty())
            oss << "- 当前预设函数：f(x) = " << presets_[states_[name].presetIndex % presets_.size()].name << "\n";
        oss << "- 提示：在说明区标题右侧的 Preset [<] [>] 切换\n";
    }
    else if (name.find("扫描法") != string::npos)
    {
        oss << "扫描法：\n";
        oss << "- 在区间 [A,B] 内以步长 h=(B-A)/n 取节点 xi=A+i*h\n";
        oss << "- 从左至右检查 f(xi) 的符号变化\n";
        oss << "- 若 f(xi) 与 f(x0) 异号，则 [xi-1,xi] 为有根子区间\n";
        if (!presets_.empty())
            oss << "- 当前预设函数：f(x) = " << presets_[states_[name].presetIndex % presets_.size()].name << "\n";
        oss << "- 提示：在说明区标题右侧的 Preset [<] [>] 切换\n";
    }
    else if (name.find("对分法") != string::npos)
    {
        oss << "对分法：\n";
        oss << "- 需求：连续函数 f(x) 在 [a,b] 上且 f(a)f(b)<0\n";
        oss << "- 逐步缩小区间，直到误差 < tol 或达到最大迭代步\n";
        if (!presets_.empty())
            oss << "- 当前预设函数：f(x) = " << presets_[states_[name].presetIndex % presets_.size()].name << "\n";
        oss << "- 提示：在说明区标题右侧 Preset [<] [>] 切换；按 [a] 添加多项式预设\n";
    }
    else if (name.find("牛顿迭代法") != string::npos)
    {
        oss << "牛顿迭代法：\n";
        oss << "- 需求：可导函数 f(x) 及导数 f'(x)\n";
        oss << "- 迭代：x_{k+1} = x_k - f(x_k)/f'(x_k)\n";
        if (!presets_.empty())
        {
            const auto &pz = presets_[states_[name].presetIndex % presets_.size()];
            oss << "- 当前预设：f(x) = " << pz.name << (pz.hasDf ? ", f'(x) 可用" : ", f'(x) 不可用，无法用于牛顿法") << "\n";
        }
        oss << "- 提示：在说明区标题右侧 Preset [<] [>] 切换；按 [a] 添加多项式预设\n";
    }
    else
    {
        oss << "该实验暂未提供详细说明。\n";
    }
    states_[name].description = oss.str();
    ui_.setDescription(states_[name].description);
}

void Manager::computeBisection(const std::string &name)
{
    auto &st = states_[name];
    // 解析输入
    double a = toDouble(ui_.getInputValue(0), 1.0);
    double b = toDouble(ui_.getInputValue(1), 2.0);
    int maxIter = toInt(ui_.getInputValue(2), 50);
    double tol = toDouble(ui_.getInputValue(3), 1e-6);

    const auto &pz = presets_.empty() ? *(new Preset()) : presets_[st.presetIndex % presets_.size()];
    auto iters = calc::bisection(pz.f, a, b, maxIter, tol);

    UiOutputPane::TableData tbl;
    tbl.headers = {"k", "x", "f(x)", "error"};
    for (const auto &it : iters)
    {
        tbl.rows.push_back({std::to_string(it.k), fmt(it.x, 10), fmt(it.fx, 10), std::isinf(it.error) ? string("-") : fmt(it.error, 10)});
    }

    // 文本摘要
    std::ostringstream oss;
    oss << "方法：对分法\n";
    oss << "区间：[" << fmt(a) << ", " << fmt(b) << "]\n";
    oss << "tol=" << tol << ", maxIter=" << maxIter << "\n";
    if (!iters.empty())
        oss << "近似根：x≈" << fmt(iters.back().x, 12) << ", 步数：" << iters.size() << "\n";
    else
        oss << "前置条件不满足或未能计算。\n";

    // 绘图：以零点为中心，取区间长度的 1/3 作为绘图范围
    UiOutputPane::PlotData plot;
    double xa = std::min(a, b), xb = std::max(a, b);
    if (!iters.empty() && std::isfinite(iters.back().x))
    {
        double root = iters.back().x;
        double span = (xb - xa) / 3.0;
        if (span < 1e-6)
            span = 1.0; // 避免太小
        double plotXMin = root - span / 2.0;
        double plotXMax = root + span / 2.0;
        int N = 80;
        for (int i = 0; i <= N; ++i)
        {
            double x = plotXMin + (plotXMax - plotXMin) * (double)i / (double)N;
            plot.xs.push_back(x);
            plot.ys.push_back(pz.f(x));
        }
        plot.xlabel = "x";
        plot.ylabel = "f(x)";
        plot.xmin = plotXMin;
        plot.xmax = plotXMax;
        // 计算 y 范围
        double ymin = 1e100, ymax = -1e100;
        for (double yv : plot.ys)
        {
            ymin = std::min(ymin, yv);
            ymax = std::max(ymax, yv);
        }
        if (ymin == ymax)
        {
            ymin -= 1;
            ymax += 1;
        }
        plot.ymin = ymin;
        plot.ymax = ymax;
        plot.hasRoot = true;
        plot.rootX = root;
    }
    else if (std::isfinite(xa) && std::isfinite(xb) && xb > xa)
    {
        // 如果没有找到根，则在输入区间上绘图
        int N = 80;
        for (int i = 0; i <= N; ++i)
        {
            double x = xa + (xb - xa) * (double)i / (double)N;
            plot.xs.push_back(x);
            plot.ys.push_back(pz.f(x));
        }
        plot.xlabel = "x";
        plot.ylabel = "f(x)";
        plot.xmin = xa;
        plot.xmax = xb;
        double ymin = 1e100, ymax = -1e100;
        for (double yv : plot.ys)
        {
            ymin = std::min(ymin, yv);
            ymax = std::max(ymax, yv);
        }
        if (ymin == ymax)
        {
            ymin -= 1;
            ymax += 1;
        }
        plot.ymin = ymin;
        plot.ymax = ymax;
    }

    st.last.table = std::move(tbl);
    st.last.summary = oss.str();
    st.last.plot = std::move(plot);
    st.last.has = true;
}

void Manager::computeNewton(const std::string &name)
{
    auto &st = states_[name];
    double x0 = toDouble(ui_.getInputValue(0), 1.0);
    int maxIter = toInt(ui_.getInputValue(1), 50);
    double tol = toDouble(ui_.getInputValue(2), 1e-6);

    const auto &pz2 = presets_.empty() ? *(new Preset()) : presets_[st.presetIndex % presets_.size()];
    std::vector<calc::Iteration> iters = {};
    if (pz2.hasDf)
        iters = calc::newton(pz2.f, pz2.df, x0, maxIter, tol);

    UiOutputPane::TableData tbl;
    tbl.headers = {"k", "x", "f(x)", "error"};
    for (const auto &it : iters)
    {
        tbl.rows.push_back({std::to_string(it.k), fmt(it.x, 10), fmt(it.fx, 10), std::isinf(it.error) ? string("-") : fmt(it.error, 10)});
    }

    std::ostringstream oss;
    oss << "方法：牛顿迭代法\n";
    oss << "初值：x0=" << fmt(x0) << ", tol=" << tol << ", maxIter=" << maxIter << "\n";
    if (!iters.empty())
        oss << "近似根：x≈" << fmt(iters.back().x, 12) << ", 步数：" << iters.size() << "\n";
    else
        oss << "未能计算，可能导数过小或发散。\n";

    // 绘图：以迭代轨迹的最值为范围，如果找到零点则以零点为中心
    UiOutputPane::PlotData plot;
    if (!iters.empty())
    {
        double xmin = iters.front().x, xmax = iters.front().x;
        for (const auto &it : iters)
        {
            xmin = std::min(xmin, it.x);
            xmax = std::max(xmax, it.x);
        }
        double root = iters.back().x;
        double span = (xmax - xmin);
        if (span < 1e-6)
            span = 1.0;
        // 以根为中心，取迭代范围的 1.5 倍作为绘图范围
        double plotXMin = root - span * 0.75;
        double plotXMax = root + span * 0.75;
        if (plotXMax > plotXMin)
        {
            int N = 80;
            for (int i = 0; i <= N; ++i)
            {
                double x = plotXMin + (plotXMax - plotXMin) * (double)i / (double)N;
                plot.xs.push_back(x);
                plot.ys.push_back(pz2.f(x));
            }
            plot.xlabel = "x";
            plot.ylabel = "f(x)";
            plot.xmin = plotXMin;
            plot.xmax = plotXMax;
            double ymin = 1e100, ymax = -1e100;
            for (double yv : plot.ys)
            {
                ymin = std::min(ymin, yv);
                ymax = std::max(ymax, yv);
            }
            if (ymin == ymax)
            {
                ymin -= 1;
                ymax += 1;
            }
            plot.ymin = ymin;
            plot.ymax = ymax;
            plot.hasRoot = true;
            plot.rootX = root;
        }
    }

    st.last.table = std::move(tbl);
    st.last.summary = oss.str();
    st.last.plot = std::move(plot);
    st.last.has = true;
}

void Manager::computePlot(const std::string &name)
{
    auto &st = states_[name];
    // 解析输入
    double a = toDouble(ui_.getInputValue(0), 0.0);
    double b = toDouble(ui_.getInputValue(1), 3.0);

    const auto &pz = presets_.empty() ? *(new Preset()) : presets_[st.presetIndex % presets_.size()];

    // 文本摘要
    std::ostringstream oss;
    oss << "方法：画图法\n";
    oss << "区间：[" << fmt(a) << ", " << fmt(b) << "]\n";
    oss << "在此区间上绘制 f(x) 的图像\n";
    oss << "观察曲线与 x 轴的交点即为方程的根\n";

    // 绘图：在给定区间上绘制
    UiOutputPane::PlotData plot;
    double xa = std::min(a, b), xb = std::max(a, b);
    if (std::isfinite(xa) && std::isfinite(xb) && xb > xa)
    {
        int N = 100;
        for (int i = 0; i <= N; ++i)
        {
            double x = xa + (xb - xa) * (double)i / (double)N;
            plot.xs.push_back(x);
            plot.ys.push_back(pz.f(x));
        }
        plot.xlabel = "x";
        plot.ylabel = "f(x)";
        plot.xmin = xa;
        plot.xmax = xb;
        double ymin = 1e100, ymax = -1e100;
        for (double yv : plot.ys)
        {
            ymin = std::min(ymin, yv);
            ymax = std::max(ymax, yv);
        }
        if (ymin == ymax)
        {
            ymin -= 1;
            ymax += 1;
        }
        plot.ymin = ymin;
        plot.ymax = ymax;
        plot.hasRoot = false; // 画图法不标记零点，让用户自己观察
    }

    st.last.summary = oss.str();
    st.last.plot = std::move(plot);
    st.last.table = {}; // 无表格
    st.last.has = true;

    // 直接切换到曲线标签页
    ui_.output().clear();
    ui_.output().addTextTab("摘要", st.last.summary);
    ui_.output().addPlotTab("曲线", st.last.plot);
    ui_.output().setSelected(1); // 选中曲线标签页（索引1）
}

void Manager::computeScan(const std::string &name)
{
    auto &st = states_[name];
    // 解析输入
    double A = toDouble(ui_.getInputValue(0), 1.0);
    double B = toDouble(ui_.getInputValue(1), 2.0);
    int n = toInt(ui_.getInputValue(2), 10);

    if (n <= 0)
        n = 10;

    const auto &pz = presets_.empty() ? *(new Preset()) : presets_[st.presetIndex % presets_.size()];

    // 步长
    double h = (B - A) / n;
    double x0 = A;
    double fx0 = pz.f(x0);
    double lastfx = fx0;

    // 扫描节点
    std::vector<double> nodes;
    std::vector<double> fvalues;
    std::vector<std::pair<double, double>> intervals; // 有根子区间

    nodes.push_back(x0);
    fvalues.push_back(fx0);

    for (int i = 1; i <= n; ++i)
    {
        double xi = x0 + i * h;
        double fxi = pz.f(xi);
        nodes.push_back(xi);
        fvalues.push_back(fxi);

        // 检查符号变化
        if (lastfx * fxi < 0)
        {
            intervals.push_back({nodes[i - 1], xi});
        }
        lastfx = fxi;
    }

    // 构建表格
    UiOutputPane::TableData tbl;
    tbl.headers = {"i", "xi", "f(xi)", "符号变化"};
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        std::string signChange = "-";
        if (i > 0 && fvalues[0] * fvalues[i] < 0)
        {
            signChange = "是";
        }
        tbl.rows.push_back({std::to_string((int)i), fmt(nodes[i], 8), fmt(fvalues[i], 8), signChange});
    }

    // 文本摘要
    std::ostringstream oss;
    oss << "方法：扫描法\n";
    oss << "区间：[" << fmt(A) << ", " << fmt(B) << "]\n";
    oss << "步长：h = " << fmt(h, 8) << ", 分段数 n = " << n << "\n";
    oss << "找到 " << intervals.size() << " 个有根子区间：\n";
    for (const auto &iv : intervals)
    {
        oss << "  [" << fmt(iv.first, 8) << ", " << fmt(iv.second, 8) << "]\n";
    }
    if (intervals.empty())
    {
        oss << "未发现符号变化，可能无根或需要更小的步长。\n";
    }

    // 绘图：在给定区间上绘制函数曲线
    UiOutputPane::PlotData plot;
    if (std::isfinite(A) && std::isfinite(B) && B > A)
    {
        int N = 100;
        for (int i = 0; i <= N; ++i)
        {
            double x = A + (B - A) * (double)i / (double)N;
            plot.xs.push_back(x);
            plot.ys.push_back(pz.f(x));
        }
        plot.xlabel = "x";
        plot.ylabel = "f(x)";
        plot.xmin = A;
        plot.xmax = B;
        double ymin = 1e100, ymax = -1e100;
        for (double yv : plot.ys)
        {
            ymin = std::min(ymin, yv);
            ymax = std::max(ymax, yv);
        }
        if (ymin == ymax)
        {
            ymin -= 1;
            ymax += 1;
        }
        plot.ymin = ymin;
        plot.ymax = ymax;
        plot.hasRoot = false;
    }

    st.last.table = std::move(tbl);
    st.last.summary = oss.str();
    st.last.plot = std::move(plot);
    st.last.has = true;
}

// 工具函数
double Manager::toDouble(const std::string &s, double defv)
{
    try
    {
        size_t pos = 0;
        double v = std::stod(s, &pos);
        (void)pos;
        return v;
    }
    catch (...)
    {
        return defv;
    }
}

int Manager::toInt(const std::string &s, int defv)
{
    try
    {
        size_t pos = 0;
        int v = std::stoi(s, &pos);
        (void)pos;
        return v;
    }
    catch (...)
    {
        return defv;
    }
}

std::string Manager::fmt(double v, int prec)
{
    std::ostringstream oss;
    oss.setf(std::ios::fixed);
    oss.precision(prec);
    oss << v;
    return oss.str();
}

void Manager::ensurePresets()
{
    if (!presets_.empty())
        return;
    presets_.push_back({"x^3 - x - 1",
                        [](double x)
                        { return x * x * x - x - 1.0; },
                        [](double x)
                        { return 3.0 * x * x - 1.0; },
                        true});
    presets_.push_back({"cos(x) - x",
                        [](double x)
                        { return std::cos(x) - x; },
                        [](double x)
                        { return -std::sin(x) - 1.0; },
                        true});
    presets_.push_back({"x^3 - 2",
                        [](double x)
                        { return x * x * x - 2.0; },
                        [](double x)
                        { return 3.0 * x * x; },
                        true});
    presets_.push_back({"e^x - 3",
                        [](double x)
                        { return std::exp(x) - 3.0; },
                        [](double x)
                        { return std::exp(x); },
                        true});
}

void Manager::cyclePresetFor(const std::string &name, int delta)
{
    auto &st = states_[name];
    if (presets_.empty())
        ensurePresets();
    int n = (int)presets_.size();
    if (n == 0)
        return;
    int step = (delta >= 0 ? 1 : -1);
    st.presetIndex = (st.presetIndex + step + n) % n;
}

void Manager::addPolynomialPreset()
{
    // 第一步：输入次数 n
    int h, w;
    getmaxyx(stdscr, h, w);
    int ph = 12, pw = 60;
    int py = (h - ph) / 2, px = (w - pw) / 2;
    WINDOW *win = newwin(ph, pw, py, px);
    box(win, 0, 0);
    wattron(win, A_BOLD);
    mvwprintw(win, 1, 2, "添加多项式预设");
    wattroff(win, A_BOLD);
    mvwprintw(win, 3, 2, "请输入多项式次数 n (0-10):");
    mvwprintw(win, 9, 2, "ESC 取消  Enter 确认");
    wrefresh(win);

    // 输入次数
    echo();
    curs_set(1);
    char nBuf[16] = {};
    mvwgetnstr(win, 5, 2, nBuf, 10);
    curs_set(0);
    noecho();
    int n = toInt(std::string(nBuf), -1);
    if (n < 0 || n > 10)
    {
        mvwprintw(win, 7, 2, "次数无效，按任意键退出...");
        wrefresh(win);
        int key;
        do
        {
            key = getch();
        } while (key == KEY_MOUSE);
        delwin(win);
        return;
    }

    // 第二步：输入 n+1 个系数
    werase(win);
    box(win, 0, 0);
    wattron(win, A_BOLD);
    mvwprintw(win, 1, 2, "添加多项式预设");
    wattroff(win, A_BOLD);
    mvwprintw(win, 3, 2, "输入 %d 个系数 (从高次到低次)", n + 1);
    mvwprintw(win, 4, 2, "空格或逗号分隔：");
    mvwprintw(win, 9, 2, "ESC 取消  Enter 确认");
    wrefresh(win);

    echo();
    curs_set(1);
    char coeffBuf[128] = {};
    mvwgetnstr(win, 6, 2, coeffBuf, 120);
    curs_set(0);
    noecho();

    // 解析系数
    std::vector<double> coeff;
    std::string coeffStr(coeffBuf);
    std::replace(coeffStr.begin(), coeffStr.end(), ',', ' ');
    std::istringstream iss(coeffStr);
    double val;
    while (iss >> val)
        coeff.push_back(val);

    if ((int)coeff.size() != n + 1)
    {
        mvwprintw(win, 7, 2, "系数个数不匹配，按任意键退出...");
        wrefresh(win);
        int key;
        do
        {
            key = getch();
        } while (key == KEY_MOUSE);
        delwin(win);
        return;
    }

    // 构造多项式名称
    std::ostringstream nameOss;
    bool first = true;
    for (int i = 0; i <= n; ++i)
    {
        int power = n - i;
        double c = coeff[i];
        if (std::abs(c) < 1e-12)
            continue;
        if (!first && c > 0)
            nameOss << "+";
        if (power == 0)
            nameOss << c;
        else if (power == 1)
        {
            if (std::abs(c - 1.0) < 1e-12)
                nameOss << "x";
            else if (std::abs(c + 1.0) < 1e-12)
                nameOss << "-x";
            else
                nameOss << c << "x";
        }
        else
        {
            if (std::abs(c - 1.0) < 1e-12)
                nameOss << "x^" << power;
            else if (std::abs(c + 1.0) < 1e-12)
                nameOss << "-x^" << power;
            else
                nameOss << c << "x^" << power;
        }
        first = false;
    }
    std::string polyName = nameOss.str();
    if (polyName.empty())
        polyName = "0";

    // 构造 f(x) 和 f'(x)
    auto polyF = [coeff, n](double x) -> double
    {
        double result = 0.0;
        for (int i = 0; i <= n; ++i)
        {
            int power = n - i;
            result += coeff[i] * std::pow(x, power);
        }
        return result;
    };

    auto polyDf = [coeff, n](double x) -> double
    {
        double result = 0.0;
        for (int i = 0; i < n; ++i) // 导数有 n 项
        {
            int power = n - i;
            result += coeff[i] * power * std::pow(x, power - 1);
        }
        return result;
    };

    // 添加到预设列表
    Preset p;
    p.name = polyName;
    p.f = polyF;
    p.df = polyDf;
    p.hasDf = true;
    presets_.push_back(p);

    // 切换到新预设
    const auto expName = ui_.getCurrentExperiment();
    if (!expName.empty())
    {
        auto &st = states_[expName];
        st.presetIndex = (int)presets_.size() - 1;
        fillDescriptionFor(expName);
    }

    mvwprintw(win, 7, 2, "添加成功！按任意键继续...");
    wrefresh(win);
    int key;
    do
    {
        key = getch();
    } while (key == KEY_MOUSE);
    delwin(win);
}

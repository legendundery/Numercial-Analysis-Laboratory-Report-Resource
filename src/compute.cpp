#include "manager.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <pdcurses.h>
using std::string;

// 实现各个数值方法的计算逻辑

// 画图法
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

// 扫描法
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
        if (i > 0 && fvalues[i - 1] * fvalues[i] < 0)
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

// 对分法
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

// 牛顿迭代法
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

// 埃特肯法
void Manager::computeAitken(const std::string &name)
{
    auto &st = states_[name];
    double x0 = toDouble(ui_.getInputValue(0), 1.0);
    int maxIter = toInt(ui_.getInputValue(1), 50);
    double tol = toDouble(ui_.getInputValue(2), 1e-6);

    const auto &pz = presets_.empty() ? *(new Preset()) : presets_[st.presetIndex % presets_.size()];
    if (!pz.hasDf)
    {
        st.last.summary = "埃特肯法需要导数 f'(x)，当前预设不可用。";
        st.last.has = true;
        return;
    }

    // 定义 φ(x) = x - f(x)/f'(x) (牛顿迭代格式)
    auto phi = [&](double x) -> double
    {
        double dfx = pz.df(x);
        if (std::abs(dfx) < 1e-15)
            return x;
        return x - pz.f(x) / dfx;
    };

    // 构建表格：k, x_n, y_n, z_n, x_{n+1}, error
    UiOutputPane::TableData tbl;
    tbl.headers = {"k", "x_n", "y_n", "z_n", "x_{n+1}", "error"};

    std::vector<double> xSeq;
    double xn = x0;
    xSeq.push_back(xn);

    for (int k = 0; k < maxIter; ++k)
    {
        double yn = phi(xn);
        double zn = phi(yn);

        // 埃特肯公式: x_{n+1} = (x_n * z_n - y_n^2) / (x_n - 2*y_n + z_n)
        double denom = xn - 2.0 * yn + zn;
        double xnext;
        if (std::abs(denom) < 1e-15)
        {
            // 分母接近0，使用 yn 作为下一个值
            xnext = yn;
        }
        else
        {
            xnext = (xn * zn - yn * yn) / denom;
        }

        double error = std::abs(xnext - xn);

        tbl.rows.push_back({std::to_string(k),
                            fmt(xn, 10),
                            fmt(yn, 10),
                            fmt(zn, 10),
                            fmt(xnext, 10),
                            fmt(error, 10)});

        xSeq.push_back(xnext);

        if (error < tol)
        {
            xn = xnext;
            break;
        }

        xn = xnext;
    }

    std::ostringstream oss;
    oss << "方法：埃特肯（加速）法\n";
    oss << "初值：x0=" << fmt(x0) << ", tol=" << tol << ", maxIter=" << maxIter << "\n";
    oss << "迭代格式：先用牛顿法 φ(x) = x - f(x)/f'(x)\n";
    oss << "埃特肯公式：x_{n+1} = (x_n·z_n - y_n²) / (x_n - 2y_n + z_n)\n";
    if (!tbl.rows.empty())
        oss << "近似根：x≈" << fmt(xn, 12) << ", 步数：" << tbl.rows.size() << "\n";
    else
        oss << "未能计算。\n";

    // 绘图
    UiOutputPane::PlotData plot;
    if (xSeq.size() >= 2)
    {
        double xmin = *std::min_element(xSeq.begin(), xSeq.end());
        double xmax = *std::max_element(xSeq.begin(), xSeq.end());
        double root = xSeq.back();
        double span = xmax - xmin;
        if (span < 1e-6)
            span = 1.0;
        double plotXMin = root - span * 0.75;
        double plotXMax = root + span * 0.75;

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

    st.last.table = std::move(tbl);
    st.last.summary = oss.str();
    st.last.plot = std::move(plot);
    st.last.has = true;
}

// 牛顿下山法
void Manager::computeNewtonDownhill(const std::string &name)
{
    auto &st = states_[name];
    double x0 = toDouble(ui_.getInputValue(0), 1.0);
    int maxIter = toInt(ui_.getInputValue(1), 50);
    double tol = toDouble(ui_.getInputValue(2), 1e-6);

    const auto &pz = presets_.empty() ? *(new Preset()) : presets_[st.presetIndex % presets_.size()];
    if (!pz.hasDf)
    {
        st.last.summary = "牛顿下山法需要导数 f'(x)，当前预设不可用。";
        st.last.has = true;
        return;
    }

    // 构建表格：k, x_n, f(x_n), λ, x_{n+1}, f(x_{n+1}), 下山条件
    UiOutputPane::TableData tbl;
    tbl.headers = {"k", "x_n", "f(x_n)", "λ", "x_{n+1}", "f(x_{n+1})", "下山条件"};

    std::vector<double> xSeq;
    double xn = x0;
    xSeq.push_back(xn);

    for (int k = 0; k < maxIter; ++k)
    {
        double fxn = pz.f(xn);
        double dfxn = pz.df(xn);

        if (std::abs(dfxn) < 1e-15)
        {
            // 导数过小，无法继续
            tbl.rows.push_back({std::to_string(k),
                                fmt(xn, 10),
                                fmt(fxn, 10),
                                "-",
                                "-",
                                "-",
                                "导数过小"});
            break;
        }

        // 尝试 λ = 1, 1/2, 1/4, 1/8, ...
        double lambda = 1.0;
        double xnext = xn;
        double fxnext = fxn;
        bool found = false;
        std::string lambdaStr = "-";

        for (int j = 0; j < 10; ++j) // 最多尝试10次
        {
            double xtest = xn - lambda * fxn / dfxn;
            double fxtest = pz.f(xtest);

            if (std::abs(fxtest) < std::abs(fxn)) // 下山条件
            {
                xnext = xtest;
                fxnext = fxtest;
                lambdaStr = fmt(lambda, 6);
                found = true;
                break;
            }
            lambda /= 2.0;
        }

        std::string downhillStatus = found ? "满足" : "不满足";

        tbl.rows.push_back({std::to_string(k),
                            fmt(xn, 10),
                            fmt(fxn, 10),
                            lambdaStr,
                            fmt(xnext, 10),
                            fmt(fxnext, 10),
                            downhillStatus});

        if (!found)
        {
            // 无法找到满足下山条件的 λ
            break;
        }

        xSeq.push_back(xnext);

        double error = std::abs(xnext - xn);
        if (error < tol || std::abs(fxnext) < tol)
        {
            xn = xnext;
            break;
        }

        xn = xnext;
    }

    std::ostringstream oss;
    oss << "方法：牛顿下山法\n";
    oss << "初值：x0=" << fmt(x0) << ", tol=" << tol << ", maxIter=" << maxIter << "\n";
    oss << "迭代公式：x_{n+1} = x_n - λ·f(x_n)/f'(x_n)\n";
    oss << "下山条件：|f(x_{n+1})| < |f(x_n)|\n";
    oss << "λ 取值序列：1, 1/2, 1/4, 1/8, ...\n";
    if (!tbl.rows.empty() && xSeq.size() > 1)
        oss << "近似根：x≈" << fmt(xn, 12) << ", 步数：" << tbl.rows.size() << "\n";
    else
        oss << "未能收敛。\n";

    // 绘图
    UiOutputPane::PlotData plot;
    if (xSeq.size() >= 2)
    {
        double xmin = *std::min_element(xSeq.begin(), xSeq.end());
        double xmax = *std::max_element(xSeq.begin(), xSeq.end());
        double root = xSeq.back();
        double span = xmax - xmin;
        if (span < 1e-6)
            span = 1.0;
        double plotXMin = root - span * 0.75;
        double plotXMax = root + span * 0.75;

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

    st.last.table = std::move(tbl);
    st.last.summary = oss.str();
    st.last.plot = std::move(plot);
    st.last.has = true;
}

// 单点弦截法
void Manager::computeSecantSinglePoint(const std::string &name)
{
    auto &st = states_[name];
    double x0 = toDouble(ui_.getInputValue(0), 1.0);
    double x1 = toDouble(ui_.getInputValue(1), 1.5);
    int maxIter = toInt(ui_.getInputValue(2), 50);
    double tol = toDouble(ui_.getInputValue(3), 1e-6);

    const auto &pz = presets_.empty() ? *(new Preset()) : presets_[st.presetIndex % presets_.size()];

    // 构建表格：k, x_n, f(x_n), x_{n+1}, error
    UiOutputPane::TableData tbl;
    tbl.headers = {"k", "x_n", "f(x_n)", "f(x_0)", "x_{n+1}", "error"};

    std::vector<double> xSeq;
    double fx0 = pz.f(x0);
    double xn = x1;
    xSeq.push_back(x0);
    xSeq.push_back(x1);

    for (int k = 0; k < maxIter; ++k)
    {
        double fxn = pz.f(xn);

        // 单点弦截法：x_{n+1} = [x_0 * f(x_n) - x_n * f(x_0)] / [f(x_n) - f(x_0)]
        double denom = fxn - fx0;
        if (std::abs(denom) < 1e-15)
        {
            // 分母过小，无法继续
            tbl.rows.push_back({std::to_string(k),
                                fmt(xn, 10),
                                fmt(fxn, 10),
                                fmt(fx0, 10),
                                "-",
                                "分母过小"});
            break;
        }

        double xnext = (x0 * fxn - xn * fx0) / denom;
        double error = std::abs(xnext - xn);

        tbl.rows.push_back({std::to_string(k),
                            fmt(xn, 10),
                            fmt(fxn, 10),
                            fmt(fx0, 10),
                            fmt(xnext, 10),
                            fmt(error, 10)});

        xSeq.push_back(xnext);

        if (error < tol || std::abs(pz.f(xnext)) < tol)
        {
            xn = xnext;
            break;
        }

        xn = xnext;
    }

    std::ostringstream oss;
    oss << "方法：单点弦截法（收敛阶数1）\n";
    oss << "初值：x0=" << fmt(x0) << ", x1=" << fmt(x1) << ", tol=" << tol << ", maxIter=" << maxIter << "\n";
    oss << "迭代公式：x_{n+1} = [x_0·f(x_n) - x_n·f(x_0)] / [f(x_n) - f(x_0)]\n";
    oss << "说明：x_0 在迭代中保持不变，用于计算导数近似\n";
    if (!tbl.rows.empty())
        oss << "近似根：x≈" << fmt(xn, 12) << ", 步数：" << tbl.rows.size() << "\n";
    else
        oss << "未能收敛。\n";

    // 绘图
    UiOutputPane::PlotData plot;
    if (xSeq.size() >= 2)
    {
        double xmin = *std::min_element(xSeq.begin(), xSeq.end());
        double xmax = *std::max_element(xSeq.begin(), xSeq.end());
        double root = xSeq.back();
        double span = xmax - xmin;
        if (span < 1e-6)
            span = 1.0;
        double plotXMin = root - span * 0.75;
        double plotXMax = root + span * 0.75;

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

    st.last.table = std::move(tbl);
    st.last.summary = oss.str();
    st.last.plot = std::move(plot);
    st.last.has = true;
}

// 双点弦截法
void Manager::computeSecantDoublePoint(const std::string &name)
{
    auto &st = states_[name];
    double x0 = toDouble(ui_.getInputValue(0), 1.0);
    double x1_input = toDouble(ui_.getInputValue(1), 1.5); // 输入的x1只作为参考
    int maxIter = toInt(ui_.getInputValue(2), 50);
    double tol = toDouble(ui_.getInputValue(3), 1e-6);

    const auto &pz = presets_.empty() ? *(new Preset()) : presets_[st.presetIndex % presets_.size()];

    // 检查是否有导数
    if (!pz.hasDf)
    {
        st.last.summary = "双点弦截法需要导数 f'(x) 来计算 x1，当前预设不可用。";
        st.last.has = true;
        return;
    }

    // 使用牛顿迭代公式计算 x1 = x0 - f(x0)/f'(x0)
    double fx0 = pz.f(x0);
    double dfx0 = pz.df(x0);
    double x1;
    if (std::abs(dfx0) < 1e-15)
    {
        // 导数过小，使用输入的x1
        x1 = x1_input;
    }
    else
    {
        x1 = x0 - fx0 / dfx0;
    }

    // 构建表格：k, x_{n-1}, x_n, f(x_{n-1}), f(x_n), x_{n+1}, error
    UiOutputPane::TableData tbl;
    tbl.headers = {"k", "x_{n-1}", "x_n", "f(x_{n-1})", "f(x_n)", "x_{n+1}", "error"};

    std::vector<double> xSeq;
    double xn_1 = x0; // x_{n-1}
    double xn = x1;   // x_n
    xSeq.push_back(xn_1);
    xSeq.push_back(xn);

    for (int k = 0; k < maxIter; ++k)
    {
        double fxn_1 = pz.f(xn_1);
        double fxn = pz.f(xn);

        // 双点弦截法：x_{n+1} = [x_{n-1} * f(x_n) - x_n * f(x_{n-1})] / [f(x_n) - f(x_{n-1})]
        double denom = fxn - fxn_1;
        if (std::abs(denom) < 1e-15)
        {
            // 分母过小，无法继续
            tbl.rows.push_back({std::to_string(k),
                                fmt(xn_1, 10),
                                fmt(xn, 10),
                                fmt(fxn_1, 10),
                                fmt(fxn, 10),
                                "-",
                                "分母过小"});
            break;
        }

        double xnext = (xn_1 * fxn - xn * fxn_1) / denom;
        double error = std::abs(xnext - xn);

        tbl.rows.push_back({std::to_string(k),
                            fmt(xn_1, 10),
                            fmt(xn, 10),
                            fmt(fxn_1, 10),
                            fmt(fxn, 10),
                            fmt(xnext, 10),
                            fmt(error, 10)});

        xSeq.push_back(xnext);

        if (error < tol || std::abs(pz.f(xnext)) < tol)
        {
            xn = xnext;
            break;
        }

        xn_1 = xn;
        xn = xnext;
    }

    std::ostringstream oss;
    oss << "方法：双点弦截法（收敛阶数2）\n";
    oss << "初值：x0=" << fmt(x0) << ", x1=" << fmt(x1) << " (由牛顿法计算)" << ", tol=" << tol << ", maxIter=" << maxIter << "\n";
    oss << "迭代公式：x_{n+1} = [x_{n-1}·f(x_n) - x_n·f(x_{n-1})] / [f(x_n) - f(x_{n-1})]\n";
    oss << "说明：x1 = x0 - f(x0)/f'(x0)，然后同时更新 x_{n-1} 和 x_n\n";
    if (!tbl.rows.empty())
        oss << "近似根：x≈" << fmt(xn, 12) << ", 步数：" << tbl.rows.size() << "\n";
    else
        oss << "未能收敛。\n";

    // 绘图
    UiOutputPane::PlotData plot;
    if (xSeq.size() >= 2)
    {
        double xmin = *std::min_element(xSeq.begin(), xSeq.end());
        double xmax = *std::max_element(xSeq.begin(), xSeq.end());
        double root = xSeq.back();
        double span = xmax - xmin;
        if (span < 1e-6)
            span = 1.0;
        double plotXMin = root - span * 0.75;
        double plotXMax = root + span * 0.75;

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

    st.last.table = std::move(tbl);
    st.last.summary = oss.str();
    st.last.plot = std::move(plot);
    st.last.has = true;
}

// 高斯消元法
void Manager::computeGaussElimination(const std::string &name)
{
    auto &st = states_[name];
    ensureMatrixPresets();

    // 如果没有矩阵，使用预设
    if (st.matrixA.rows() == 0)
    {
        if (!matrixPresets_.empty())
        {
            const auto &preset = matrixPresets_[st.matrixPresetIndex % matrixPresets_.size()];
            st.matrixA = preset.A;
            st.vectorB = preset.b;
        }
        else
        {
            st.last.summary = "请先输入或选择矩阵。";
            st.last.has = true;
            return;
        }
    }

    // 调用高斯消元法
    auto result = calc::gaussElimination(st.matrixA, st.vectorB);

    // 构建摘要
    std::ostringstream oss;
    oss << "方法：高斯消元法（列主元）\n";
    oss << "方程组规模：" << st.matrixA.rows() << "x" << st.matrixA.cols() << "\n\n";

    if (result.success)
    {
        oss << "求解成功！\n";
        oss << "解向量：\n";
        for (int i = 0; i < (int)result.solution.size(); ++i)
        {
            oss << "  x" << (i + 1) << " = " << fmt(result.solution[i], 10) << "\n";
        }
    }
    else
    {
        oss << "求解失败：" << result.errorMsg << "\n";
    }

    // 构建迭代表（显示消元步骤）
    UiOutputPane::TableData tbl;
    tbl.headers = {"步骤", "操作"};
    for (size_t i = 0; i < result.stepDesc.size(); ++i)
    {
        tbl.rows.push_back({std::to_string(i), result.stepDesc[i]});
    }

    st.last.summary = oss.str();
    st.last.table = std::move(tbl);
    st.last.plot = {};
    st.last.has = true;

    // 立即显示结果到输出面板（包括L和U矩阵）
    ui_.output().clear();
    ui_.output().addTextTab("摘要", st.last.summary);
    ui_.output().addTableTab("消元步骤", st.last.table);

    // 添加L和U矩阵表格
    if (result.success)
    {
        // 表格2: L矩阵（下三角，对角线为1）
        UiOutputPane::TableData tbl2;
        tbl2.headers.push_back("");
        for (int j = 0; j < result.L.cols(); ++j)
            tbl2.headers.push_back("x" + std::to_string(j + 1));

        for (int i = 0; i < result.L.rows(); ++i)
        {
            std::vector<std::string> row;
            row.push_back("[" + std::to_string(i + 1) + "]");
            for (int j = 0; j < result.L.cols(); ++j)
            {
                if (j > i)
                    row.push_back(""); // 上三角部分为0
                else
                    row.push_back(fmt(result.L(i, j), 6));
            }
            tbl2.rows.push_back(row);
        }
        ui_.output().addTableTab("L矩阵", tbl2);

        // 表格3: U矩阵（上三角）
        UiOutputPane::TableData tbl3;
        tbl3.headers.push_back("");
        for (int j = 0; j < result.U.cols(); ++j)
            tbl3.headers.push_back("x" + std::to_string(j + 1));

        for (int i = 0; i < result.U.rows(); ++i)
        {
            std::vector<std::string> row;
            row.push_back("[" + std::to_string(i + 1) + "]");
            for (int j = 0; j < result.U.cols(); ++j)
            {
                if (j < i)
                    row.push_back(""); // 下三角部分为0
                else
                    row.push_back(fmt(result.U(i, j), 6));
            }
            tbl3.rows.push_back(row);
        }
        ui_.output().addTableTab("U矩阵", tbl3);
    }
}

// 克劳特消元法
void Manager::computeCroutElimination(const std::string &name)
{
    auto &st = states_[name];
    ensureMatrixPresets();

    if (st.matrixA.rows() == 0)
    {
        if (!matrixPresets_.empty())
        {
            const auto &preset = matrixPresets_[st.matrixPresetIndex % matrixPresets_.size()];
            st.matrixA = preset.A;
            st.vectorB = preset.b;
        }
        else
        {
            st.last.summary = "请先输入或选择矩阵。";
            st.last.has = true;
            return;
        }
    }

    auto result = calc::croutElimination(st.matrixA, st.vectorB);

    std::ostringstream oss;
    oss << "方法：克劳特消元法（LU分解，u_ii = 1）\n";
    oss << "方程组规模：" << st.matrixA.rows() << "x" << st.matrixA.cols() << "\n\n";

    if (result.success)
    {
        oss << "求解成功！\n";
        oss << "解向量：\n";
        for (int i = 0; i < (int)result.solution.size(); ++i)
            oss << "  x" << (i + 1) << " = " << fmt(result.solution[i], 10) << "\n";
    }
    else
    {
        oss << "求解失败：" << result.errorMsg << "\n";
    }

    UiOutputPane::TableData tbl;
    tbl.headers = {"步骤", "操作"};
    for (size_t i = 0; i < result.stepDesc.size(); ++i)
        tbl.rows.push_back({std::to_string(i), result.stepDesc[i]});

    st.last.summary = oss.str();
    st.last.table = std::move(tbl);
    st.last.plot = {};
    st.last.has = true;

    ui_.output().clear();
    ui_.output().addTextTab("摘要", st.last.summary);
    ui_.output().addTableTab("分解步骤", st.last.table);

    if (result.success)
    {
        UiOutputPane::TableData tbl2;
        tbl2.headers.push_back("");
        for (int j = 0; j < result.L.cols(); ++j)
            tbl2.headers.push_back("x" + std::to_string(j + 1));
        for (int i = 0; i < result.L.rows(); ++i)
        {
            std::vector<std::string> row;
            row.push_back("[" + std::to_string(i + 1) + "]");
            for (int j = 0; j < result.L.cols(); ++j)
                row.push_back(j > i ? "" : fmt(result.L(i, j), 6));
            tbl2.rows.push_back(row);
        }
        ui_.output().addTableTab("L矩阵", tbl2);

        UiOutputPane::TableData tbl3;
        tbl3.headers.push_back("");
        for (int j = 0; j < result.U.cols(); ++j)
            tbl3.headers.push_back("x" + std::to_string(j + 1));
        for (int i = 0; i < result.U.rows(); ++i)
        {
            std::vector<std::string> row;
            row.push_back("[" + std::to_string(i + 1) + "]");
            for (int j = 0; j < result.U.cols(); ++j)
                row.push_back(j < i ? "" : fmt(result.U(i, j), 6));
            tbl3.rows.push_back(row);
        }
        ui_.output().addTableTab("U矩阵", tbl3);
    }
}

// 平方根法
void Manager::computeCholesky(const std::string &name)
{
    auto &st = states_[name];
    ensureMatrixPresets();

    if (st.matrixA.rows() == 0)
    {
        if (!matrixPresets_.empty())
        {
            const auto &preset = matrixPresets_[st.matrixPresetIndex % matrixPresets_.size()];
            st.matrixA = preset.A;
            st.vectorB = preset.b;
        }
        else
        {
            st.last.summary = "请先输入或选择矩阵。";
            st.last.has = true;
            return;
        }
    }

    auto result = calc::choleskySolve(st.matrixA, st.vectorB);

    std::ostringstream oss;
    oss << "方法：平方根法（Cholesky）\n";
    oss << "方程组规模：" << st.matrixA.rows() << "x" << st.matrixA.cols() << "\n\n";

    if (result.success)
    {
        oss << "求解成功！\n";
        oss << "解向量：\n";
        for (int i = 0; i < (int)result.solution.size(); ++i)
            oss << "  x" << (i + 1) << " = " << fmt(result.solution[i], 10) << "\n";
    }
    else
    {
        oss << "求解失败：" << result.errorMsg << "\n";
    }

    UiOutputPane::TableData tbl;
    tbl.headers = {"步骤", "操作"};
    for (size_t i = 0; i < result.stepDesc.size(); ++i)
        tbl.rows.push_back({std::to_string(i), result.stepDesc[i]});

    st.last.summary = oss.str();
    st.last.table = std::move(tbl);
    st.last.plot = {};
    st.last.has = true;

    ui_.output().clear();
    ui_.output().addTextTab("摘要", st.last.summary);
    ui_.output().addTableTab("分解步骤", st.last.table);

    if (result.success)
    {
        // L
        UiOutputPane::TableData tbl2;
        tbl2.headers.push_back("");
        for (int j = 0; j < result.L.cols(); ++j)
            tbl2.headers.push_back("x" + std::to_string(j + 1));
        for (int i = 0; i < result.L.rows(); ++i)
        {
            std::vector<std::string> row;
            row.push_back("[" + std::to_string(i + 1) + "]");
            for (int j = 0; j < result.L.cols(); ++j)
                row.push_back(j > i ? "" : fmt(result.L(i, j), 6));
            tbl2.rows.push_back(row);
        }
        ui_.output().addTableTab("L矩阵", tbl2);

        // U = L^T
        UiOutputPane::TableData tbl3;
        tbl3.headers.push_back("");
        for (int j = 0; j < result.U.cols(); ++j)
            tbl3.headers.push_back("x" + std::to_string(j + 1));
        for (int i = 0; i < result.U.rows(); ++i)
        {
            std::vector<std::string> row;
            row.push_back("[" + std::to_string(i + 1) + "]");
            for (int j = 0; j < result.U.cols(); ++j)
                row.push_back(j < i ? "" : fmt(result.U(i, j), 6));
            tbl3.rows.push_back(row);
        }
        ui_.output().addTableTab("U矩阵", tbl3);
    }
}

// 追赶法
void Manager::computeThomas(const std::string &name)
{
    auto &st = states_[name];
    ensureMatrixPresets();

    if (st.matrixA.rows() == 0)
    {
        if (!matrixPresets_.empty())
        {
            const auto &preset = matrixPresets_[st.matrixPresetIndex % matrixPresets_.size()];
            st.matrixA = preset.A;
            st.vectorB = preset.b;
        }
        else
        {
            st.last.summary = "请先输入或选择矩阵。";
            st.last.has = true;
            return;
        }
    }

    auto result = calc::thomasTridiagonal(st.matrixA, st.vectorB);

    std::ostringstream oss;
    oss << "方法：追赶法（Thomas）\n";
    oss << "方程组规模：" << st.matrixA.rows() << "x" << st.matrixA.cols() << "\n\n";

    if (result.success)
    {
        oss << "求解成功！\n";
        oss << "解向量：\n";
        for (int i = 0; i < (int)result.solution.size(); ++i)
            oss << "  x" << (i + 1) << " = " << fmt(result.solution[i], 10) << "\n";
    }
    else
    {
        oss << "求解失败：" << result.errorMsg << "\n";
    }

    UiOutputPane::TableData tbl;
    tbl.headers = {"步骤", "操作"};
    for (size_t i = 0; i < result.stepDesc.size(); ++i)
        tbl.rows.push_back({std::to_string(i), result.stepDesc[i]});

    st.last.summary = oss.str();
    st.last.table = std::move(tbl);
    st.last.plot = {};
    st.last.has = true;

    ui_.output().clear();
    ui_.output().addTextTab("摘要", st.last.summary);
    ui_.output().addTableTab("步骤", st.last.table);
}

// 列主元素法
void Manager::computeColumnPivoting(const std::string &name)
{
    auto &st = states_[name];
    ensureMatrixPresets();

    if (st.matrixA.rows() == 0)
    {
        if (!matrixPresets_.empty())
        {
            const auto &preset = matrixPresets_[st.matrixPresetIndex % matrixPresets_.size()];
            st.matrixA = preset.A;
            st.vectorB = preset.b;
        }
        else
        {
            st.last.summary = "请先输入或选择矩阵。";
            st.last.has = true;
            return;
        }
    }

    auto result = calc::columnPivoting(st.matrixA, st.vectorB);

    std::ostringstream oss;
    oss << "方法：列主元素法\n";
    oss << "方程组规模：" << st.matrixA.rows() << "x" << st.matrixA.cols() << "\n\n";

    if (result.success)
    {
        oss << "求解成功！\n";
        oss << "解向量：\n";
        for (int i = 0; i < (int)result.solution.size(); ++i)
            oss << "  x" << (i + 1) << " = " << fmt(result.solution[i], 10) << "\n";
    }
    else
    {
        oss << "求解失败：" << result.errorMsg << "\n";
    }

    UiOutputPane::TableData tbl;
    tbl.headers = {"步骤", "操作"};
    for (size_t i = 0; i < result.stepDesc.size(); ++i)
        tbl.rows.push_back({std::to_string(i), result.stepDesc[i]});

    st.last.summary = oss.str();
    st.last.table = std::move(tbl);
    st.last.plot = {};
    st.last.has = true;

    ui_.output().clear();
    ui_.output().addTextTab("摘要", st.last.summary);
    ui_.output().addTableTab("消元步骤", st.last.table);

    if (result.success)
    {
        UiOutputPane::TableData tbl2;
        tbl2.headers.push_back("");
        for (int j = 0; j < result.L.cols(); ++j)
            tbl2.headers.push_back("x" + std::to_string(j + 1));
        for (int i = 0; i < result.L.rows(); ++i)
        {
            std::vector<std::string> row;
            row.push_back("[" + std::to_string(i + 1) + "]");
            for (int j = 0; j < result.L.cols(); ++j)
                row.push_back(j > i ? "" : fmt(result.L(i, j), 6));
            tbl2.rows.push_back(row);
        }
        ui_.output().addTableTab("L矩阵", tbl2);

        UiOutputPane::TableData tbl3;
        tbl3.headers.push_back("");
        for (int j = 0; j < result.U.cols(); ++j)
            tbl3.headers.push_back("x" + std::to_string(j + 1));
        for (int i = 0; i < result.U.rows(); ++i)
        {
            std::vector<std::string> row;
            row.push_back("[" + std::to_string(i + 1) + "]");
            for (int j = 0; j < result.U.cols(); ++j)
                row.push_back(j < i ? "" : fmt(result.U(i, j), 6));
            tbl3.rows.push_back(row);
        }
        ui_.output().addTableTab("U矩阵", tbl3);
    }
}

// 全主元素法
void Manager::computeFullPivoting(const std::string &name)
{
    auto &st = states_[name];
    ensureMatrixPresets();

    if (st.matrixA.rows() == 0)
    {
        if (!matrixPresets_.empty())
        {
            const auto &preset = matrixPresets_[st.matrixPresetIndex % matrixPresets_.size()];
            st.matrixA = preset.A;
            st.vectorB = preset.b;
        }
        else
        {
            st.last.summary = "请先输入或选择矩阵。";
            st.last.has = true;
            return;
        }
    }

    auto result = calc::fullPivoting(st.matrixA, st.vectorB);

    std::ostringstream oss;
    oss << "方法：全主元素法\n";
    oss << "方程组规模：" << st.matrixA.rows() << "x" << st.matrixA.cols() << "\n\n";

    if (result.success)
    {
        oss << "求解成功！\n";
        oss << "解向量：\n";
        for (int i = 0; i < (int)result.solution.size(); ++i)
            oss << "  x" << (i + 1) << " = " << fmt(result.solution[i], 10) << "\n";
    }
    else
    {
        oss << "求解失败：" << result.errorMsg << "\n";
    }

    UiOutputPane::TableData tbl;
    tbl.headers = {"步骤", "操作"};
    for (size_t i = 0; i < result.stepDesc.size(); ++i)
        tbl.rows.push_back({std::to_string(i), result.stepDesc[i]});

    st.last.summary = oss.str();
    st.last.table = std::move(tbl);
    st.last.plot = {};
    st.last.has = true;

    ui_.output().clear();
    ui_.output().addTextTab("摘要", st.last.summary);
    ui_.output().addTableTab("消元步骤", st.last.table);

    if (result.success)
    {
        UiOutputPane::TableData tbl2;
        tbl2.headers.push_back("");
        for (int j = 0; j < result.L.cols(); ++j)
            tbl2.headers.push_back("x" + std::to_string(j + 1));
        for (int i = 0; i < result.L.rows(); ++i)
        {
            std::vector<std::string> row;
            row.push_back("[" + std::to_string(i + 1) + "]");
            for (int j = 0; j < result.L.cols(); ++j)
                row.push_back(j > i ? "" : fmt(result.L(i, j), 6));
            tbl2.rows.push_back(row);
        }
    }
}

// 雅可比迭代法
void Manager::computeJacobi(const std::string &name)
{
    auto &st = states_[name];
    ensureMatrixPresets();

    if (st.matrixA.rows() == 0)
    {
        if (!matrixPresets_.empty())
        {
            const auto &preset = matrixPresets_[st.matrixPresetIndex % matrixPresets_.size()];
            st.matrixA = preset.A;
            st.vectorB = preset.b;
        }
        else
        {
            st.last.summary = "请先输入或选择矩阵。";
            st.last.has = true;
            return;
        }
    }

    double tol = toDouble(ui_.getInputValue(1), 1e-6);
    int maxIter = toInt(ui_.getInputValue(2), 100);

    // 收敛性判断
    bool isDiagDom = calc::isStrictlyDiagonallyDominant(st.matrixA);
    double rhoJ = calc::jacobiSpectralRadius(st.matrixA);

    int n = st.matrixA.rows();
    std::vector<double> x0(n, 0.0);
    auto result = calc::jacobiIteration(st.matrixA, st.vectorB, x0, maxIter, tol);

    std::ostringstream oss;
    oss << "方法：雅可比迭代法\n";
    oss << "方程组规模：" << st.matrixA.rows() << "x" << st.matrixA.cols() << "\n";
    oss << "精度 tol = " << tol << ", 最大迭代次数 = " << maxIter << "\n\n";

    oss << "收敛性分析：\n";
    oss << "  严格对角占优：" << (isDiagDom ? "是（充分条件满足）" : "否") << "\n";
    oss << "  迭代矩阵谱半径 ρ(B_J) = " << fmt(rhoJ, 8);
    if (rhoJ < 1.0)
        oss << " < 1（充要条件满足，必收敛）\n";
    else
        oss << " ≥ 1（不满足收敛条件）\n";
    oss << "\n";

    if (result.success)
    {
        oss << "求解成功！迭代 " << (result.iterations.size() - 1) << " 次收敛\n";
        oss << "解向量：\n";
        for (int i = 0; i < (int)result.solution.size(); ++i)
            oss << "  x" << (i + 1) << " = " << fmt(result.solution[i], 10) << "\n";
    }
    else
    {
        oss << "求解失败：" << result.errorMsg << "\n";
        if (!result.solution.empty())
        {
            oss << "当前近似解：\n";
            for (int i = 0; i < (int)result.solution.size(); ++i)
                oss << "  x" << (i + 1) << " = " << fmt(result.solution[i], 10) << "\n";
        }
    }

    // 迭代表
    UiOutputPane::TableData tbl;
    tbl.headers.push_back("k");
    for (int i = 0; i < n; ++i)
        tbl.headers.push_back("x" + std::to_string(i + 1));
    tbl.headers.push_back("误差");

    for (size_t k = 0; k < result.iterations.size(); ++k)
    {
        std::vector<std::string> row;
        row.push_back(std::to_string(k));
        for (int i = 0; i < n; ++i)
            row.push_back(fmt(result.iterations[k][i], 8));
        if (k > 0 && k - 1 < result.errors.size())
            row.push_back(fmt(result.errors[k - 1], 8));
        else
            row.push_back("-");
        tbl.rows.push_back(row);
    }

    st.last.summary = oss.str();
    st.last.table = std::move(tbl);
    st.last.plot = {};
    st.last.extraTables.clear();

    // 迭代矩阵
    UiOutputPane::TableData tbl2;
    tbl2.headers.push_back("");
    for (int j = 0; j < result.iterationMatrix.cols(); ++j)
        tbl2.headers.push_back("x" + std::to_string(j + 1));
    for (int i = 0; i < result.iterationMatrix.rows(); ++i)
    {
        std::vector<std::string> row;
        row.push_back("[" + std::to_string(i + 1) + "]");
        for (int j = 0; j < result.iterationMatrix.cols(); ++j)
            row.push_back(fmt(result.iterationMatrix(i, j), 6));
        tbl2.rows.push_back(row);
    }
    st.last.extraTables.push_back({"B_J矩阵", tbl2});
    st.last.has = true;

    ui_.output().clear();
    ui_.output().addTextTab("摘要", st.last.summary);
    ui_.output().addTableTab("迭代过程", st.last.table);
    ui_.output().addTableTab("B_J矩阵", tbl2);
}

// 高斯-赛德尔迭代法
void Manager::computeGaussSeidel(const std::string &name)
{
    auto &st = states_[name];
    ensureMatrixPresets();

    if (st.matrixA.rows() == 0)
    {
        if (!matrixPresets_.empty())
        {
            const auto &preset = matrixPresets_[st.matrixPresetIndex % matrixPresets_.size()];
            st.matrixA = preset.A;
            st.vectorB = preset.b;
        }
        else
        {
            st.last.summary = "请先输入或选择矩阵。";
            st.last.has = true;
            return;
        }
    }

    double tol = toDouble(ui_.getInputValue(1), 1e-6);
    int maxIter = toInt(ui_.getInputValue(2), 100);

    // 收敛性判断
    bool isDiagDom = calc::isStrictlyDiagonallyDominant(st.matrixA);

    int n = st.matrixA.rows();
    std::vector<double> x0(n, 0.0);
    auto result = calc::gaussSeidelIteration(st.matrixA, st.vectorB, x0, maxIter, tol);

    std::ostringstream oss;
    oss << "方法：高斯-赛德尔迭代法\n";
    oss << "方程组规模：" << st.matrixA.rows() << "x" << st.matrixA.cols() << "\n";
    oss << "精度 tol = " << tol << ", 最大迭代次数 = " << maxIter << "\n\n";

    oss << "收敛性分析：\n";
    oss << "  严格对角占优：" << (isDiagDom ? "是（充分条件满足）" : "否") << "\n";
    oss << "  迭代矩阵谱半径 ρ(B_GS) = " << fmt(result.spectralRadius, 8);
    if (result.spectralRadius < 1.0)
        oss << " < 1（充要条件满足，必收敛）\n";
    else
        oss << " ≥ 1（不满足收敛条件）\n";
    oss << "\n";

    if (result.success)
    {
        oss << "求解成功！迭代 " << (result.iterations.size() - 1) << " 次收敛\n";
        oss << "解向量：\n";
        for (int i = 0; i < (int)result.solution.size(); ++i)
            oss << "  x" << (i + 1) << " = " << fmt(result.solution[i], 10) << "\n";
    }
    else
    {
        oss << "求解失败：" << result.errorMsg << "\n";
        if (!result.solution.empty())
        {
            oss << "当前近似解：\n";
            for (int i = 0; i < (int)result.solution.size(); ++i)
                oss << "  x" << (i + 1) << " = " << fmt(result.solution[i], 10) << "\n";
        }
    }

    UiOutputPane::TableData tbl;
    tbl.headers.push_back("k");
    for (int i = 0; i < n; ++i)
        tbl.headers.push_back("x" + std::to_string(i + 1));
    tbl.headers.push_back("误差");

    for (size_t k = 0; k < result.iterations.size(); ++k)
    {
        std::vector<std::string> row;
        row.push_back(std::to_string(k));
        for (int i = 0; i < n; ++i)
            row.push_back(fmt(result.iterations[k][i], 8));
        if (k > 0 && k - 1 < result.errors.size())
            row.push_back(fmt(result.errors[k - 1], 8));
        else
            row.push_back("-");
        tbl.rows.push_back(row);
    }

    st.last.summary = oss.str();
    st.last.table = std::move(tbl);
    st.last.plot = {};
    st.last.extraTables.clear();

    // 迭代矩阵
    UiOutputPane::TableData tbl2;
    tbl2.headers.push_back("");
    for (int j = 0; j < result.iterationMatrix.cols(); ++j)
        tbl2.headers.push_back("x" + std::to_string(j + 1));
    for (int i = 0; i < result.iterationMatrix.rows(); ++i)
    {
        std::vector<std::string> row;
        row.push_back("[" + std::to_string(i + 1) + "]");
        for (int j = 0; j < result.iterationMatrix.cols(); ++j)
            row.push_back(fmt(result.iterationMatrix(i, j), 6));
        tbl2.rows.push_back(row);
    }
    st.last.extraTables.push_back({"B_GS矩阵", tbl2});
    st.last.has = true;

    ui_.output().clear();
    ui_.output().addTextTab("摘要", st.last.summary);
    ui_.output().addTableTab("迭代过程", st.last.table);
    ui_.output().addTableTab("B_GS矩阵", tbl2);
}

// 松弛迭代法（SOR）
void Manager::computeSOR(const std::string &name)
{
    auto &st = states_[name];
    ensureMatrixPresets();

    if (st.matrixA.rows() == 0)
    {
        if (!matrixPresets_.empty())
        {
            const auto &preset = matrixPresets_[st.matrixPresetIndex % matrixPresets_.size()];
            st.matrixA = preset.A;
            st.vectorB = preset.b;
        }
        else
        {
            st.last.summary = "请先输入或选择矩阵。";
            st.last.has = true;
            return;
        }
    }

    double tol = toDouble(ui_.getInputValue(1), 1e-6);
    int maxIter = toInt(ui_.getInputValue(2), 100);
    double omega = toDouble(ui_.getInputValue(3), 1.2);

    // 收敛性判断和最优松弛因子计算
    bool isDiagDom = calc::isStrictlyDiagonallyDominant(st.matrixA);
    double omegaOpt = calc::optimalOmegaSOR(st.matrixA);

    int n = st.matrixA.rows();
    std::vector<double> x0(n, 0.0);
    auto result = calc::sorIteration(st.matrixA, st.vectorB, x0, maxIter, tol, omega);

    std::ostringstream oss;
    oss << "方法：松弛迭代法（SOR）\n";
    oss << "方程组规模：" << st.matrixA.rows() << "x" << st.matrixA.cols() << "\n";
    oss << "精度 tol = " << tol << ", 最大迭代次数 = " << maxIter << "\n";
    oss << "松弛因子 ω = " << fmt(omega, 4) << "\n\n";

    oss << "收敛性分析：\n";
    oss << "  严格对角占优：" << (isDiagDom ? "是（充分条件满足）" : "否") << "\n";
    oss << "  最优松弛因子 ω_b = " << fmt(omegaOpt, 6);
    if (std::abs(omega - omegaOpt) < 0.01)
        oss << " （当前ω接近最优）\n";
    else
        oss << " （建议调整ω）\n";
    oss << "  迭代矩阵谱半径 ρ(B_ω) = " << fmt(result.spectralRadius, 8);
    if (result.spectralRadius < 1.0)
        oss << " < 1（充要条件满足，必收敛）\n";
    else
        oss << " ≥ 1（不满足收敛条件）\n";
    oss << "\n";

    if (result.success)
    {
        oss << "求解成功！迭代 " << (result.iterations.size() - 1) << " 次收敛\n";
        oss << "解向量：\n";
        for (int i = 0; i < (int)result.solution.size(); ++i)
            oss << "  x" << (i + 1) << " = " << fmt(result.solution[i], 10) << "\n";
    }
    else
    {
        oss << "求解失败：" << result.errorMsg << "\n";
        if (!result.solution.empty())
        {
            oss << "当前近似解：\n";
            for (int i = 0; i < (int)result.solution.size(); ++i)
                oss << "  x" << (i + 1) << " = " << fmt(result.solution[i], 10) << "\n";
        }
    }

    UiOutputPane::TableData tbl;
    tbl.headers.push_back("k");
    for (int i = 0; i < n; ++i)
        tbl.headers.push_back("x" + std::to_string(i + 1));
    tbl.headers.push_back("误差");

    for (size_t k = 0; k < result.iterations.size(); ++k)
    {
        std::vector<std::string> row;
        row.push_back(std::to_string(k));
        for (int i = 0; i < n; ++i)
            row.push_back(fmt(result.iterations[k][i], 8));
        if (k > 0 && k - 1 < result.errors.size())
            row.push_back(fmt(result.errors[k - 1], 8));
        else
            row.push_back("-");
        tbl.rows.push_back(row);
    }

    st.last.summary = oss.str();
    st.last.table = std::move(tbl);
    st.last.plot = {};
    st.last.extraTables.clear();

    // 迭代矩阵
    UiOutputPane::TableData tbl2;
    tbl2.headers.push_back("");
    for (int j = 0; j < result.iterationMatrix.cols(); ++j)
        tbl2.headers.push_back("x" + std::to_string(j + 1));
    for (int i = 0; i < result.iterationMatrix.rows(); ++i)
    {
        std::vector<std::string> row;
        row.push_back("[" + std::to_string(i + 1) + "]");
        for (int j = 0; j < result.iterationMatrix.cols(); ++j)
            row.push_back(fmt(result.iterationMatrix(i, j), 6));
        tbl2.rows.push_back(row);
    }
    st.last.extraTables.push_back({"B_ω矩阵", tbl2});
    st.last.has = true;

    ui_.output().clear();
    ui_.output().addTextTab("摘要", st.last.summary);
    ui_.output().addTableTab("迭代过程", st.last.table);
    ui_.output().addTableTab("B_ω矩阵", tbl2);
}

// 差商表计算
void Manager::computeDividedDifference(const std::string &expName)
{
    auto &st = states_[expName];

    // 如果 valueTable 为空，尝试加载预设
    if (st.valueTable.rows() == 0 || st.valueTable.cols() == 0)
    {
        ensureValueTablePresets();
        if (st.valueTablePresetIndex >= 0 && st.valueTablePresetIndex < (int)valueTablePresets_.size())
        {
            st.valueTable = valueTablePresets_[st.valueTablePresetIndex].table;
        }
        else if (!valueTablePresets_.empty())
        {
            st.valueTablePresetIndex = 0;
            st.valueTable = valueTablePresets_[0].table;
        }
    }

    if (st.valueTable.rows() == 0 || st.valueTable.cols() == 0)
    {
        ui_.output().clear();
        ui_.output().addTextTab("error", "empty table");
        return;
    }

    int n = st.valueTable.rows();
    std::vector<double> x(n), y(n);
    for (int i = 0; i < n; ++i)
    {
        x[i] = st.valueTable(i, 0);
        y[i] = st.valueTable(i, 1);
        if (std::isnan(x[i]) || std::isnan(y[i]))
        {
            ui_.output().clear();
            ui_.output().addTextTab("error", "x or f(x) contains NaN");
            return;
        }
    }

    auto divided = calc::dividedDifferenceTable(x, y);

    bool isEquidistant = true;
    double h = 0.0;
    if (n > 1)
    {
        h = x[1] - x[0];
        for (int i = 2; i < n; ++i)
        {
            if (std::abs((x[i] - x[i - 1]) - h) > 1e-10)
            {
                isEquidistant = false;
                break;
            }
        }
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6);
    oss << "points: " << n << "\n";
    oss << "equidistant: " << (isEquidistant ? "yes" : "no");
    if (isEquidistant && n > 1)
        oss << " (h=" << h << ")";
    oss << "\n\n";

    if (st.valueTable.cols() > 2)
    {
        oss << "note: table has derivative columns\n";
    }

    st.last.summary = oss.str();

    UiOutputPane::TableData divTbl;
    divTbl.headers.push_back("i");
    divTbl.headers.push_back("x_i");
    for (size_t j = 0; j < divided[0].size(); ++j)
        divTbl.headers.push_back("f[.." + std::to_string(j) + "]");

    auto fmt = [](double v, int w) -> std::string
    {
        std::ostringstream os;
        os << std::fixed << std::setprecision(6) << std::setw(w) << v;
        return os.str();
    };

    for (size_t i = 0; i < divided.size(); ++i)
    {
        std::vector<std::string> row;
        row.push_back(std::to_string(i));
        row.push_back(fmt(x[i], 10));
        for (size_t j = 0; j < divided[i].size(); ++j)
        {
            row.push_back(fmt(divided[i][j], 10));
        }
        divTbl.rows.push_back(row);
    }

    ui_.output().clear();
    ui_.output().addTextTab("summary", st.last.summary);
    ui_.output().addTableTab("divided diff", divTbl);

    if (isEquidistant)
    {
        auto fwd = calc::forwardDifferenceTable(y);
        auto bwd = calc::backwardDifferenceTable(y);

        UiOutputPane::TableData fwdTbl;
        fwdTbl.headers.push_back("i");
        fwdTbl.headers.push_back("y_i");
        for (size_t j = 1; j < fwd[0].size(); ++j)
            fwdTbl.headers.push_back("D^" + std::to_string(j));

        for (size_t i = 0; i < fwd.size(); ++i)
        {
            std::vector<std::string> row;
            row.push_back(std::to_string(i));
            for (size_t j = 0; j < fwd[i].size(); ++j)
            {
                row.push_back(fmt(fwd[i][j], 10));
            }
            fwdTbl.rows.push_back(row);
        }
        ui_.output().addTableTab("forward diff", fwdTbl);

        UiOutputPane::TableData bwdTbl;
        bwdTbl.headers.push_back("i");
        bwdTbl.headers.push_back("y_i");
        for (size_t j = 1; j < bwd[0].size(); ++j)
            bwdTbl.headers.push_back("B^" + std::to_string(j));

        for (size_t i = 0; i < bwd.size(); ++i)
        {
            std::vector<std::string> row;
            row.push_back(std::to_string(i));
            for (size_t j = 0; j < bwd[i].size(); ++j)
            {
                row.push_back(fmt(bwd[i][j], 10));
            }
            bwdTbl.rows.push_back(row);
        }
        ui_.output().addTableTab("backward diff", bwdTbl);
    }

    std::ostringstream plot;
    plot << "function plot:\n\n";
    const int width = 60, height = 20;
    double minX = x[0], maxX = x[0];
    double minY = y[0], maxY = y[0];
    for (int i = 1; i < n; ++i)
    {
        if (x[i] < minX)
            minX = x[i];
        if (x[i] > maxX)
            maxX = x[i];
        if (y[i] < minY)
            minY = y[i];
        if (y[i] > maxY)
            maxY = y[i];
    }

    double rangeX = maxX - minX;
    double rangeY = maxY - minY;
    if (rangeX < 1e-9)
        rangeX = 1.0;
    if (rangeY < 1e-9)
        rangeY = 1.0;
    minX -= rangeX * 0.1;
    maxX += rangeX * 0.1;
    minY -= rangeY * 0.1;
    maxY += rangeY * 0.1;

    std::vector<std::string> canvas(height, std::string(width, ' '));

    // 绘制坐标轴
    int zeroX = (int)((-minX) / (maxX - minX) * (width - 1));
    int zeroY = (int)((maxY) / (maxY - minY) * (height - 1));

    // Y轴
    if (zeroX >= 0 && zeroX < width)
    {
        for (int py = 0; py < height; ++py)
            canvas[py][zeroX] = '|';
    }

    // X轴
    if (zeroY >= 0 && zeroY < height)
    {
        for (int px = 0; px < width; ++px)
            canvas[zeroY][px] = '-';
    }

    // 原点
    if (zeroX >= 0 && zeroX < width && zeroY >= 0 && zeroY < height)
        canvas[zeroY][zeroX] = '+';

    // 绘制数据点
    for (int i = 0; i < n; ++i)
    {
        int px = (int)((x[i] - minX) / (maxX - minX) * (width - 1));
        int py = (int)((maxY - y[i]) / (maxY - minY) * (height - 1));
        if (px >= 0 && px < width && py >= 0 && py < height)
            canvas[py][px] = '*';
    }

    for (const auto &line : canvas)
        plot << line << "\n";

    plot << "\nx in [" << fmt(minX, 8) << ", " << fmt(maxX, 8) << "]\n";
    plot << "y in [" << fmt(minY, 8) << ", " << fmt(maxY, 8) << "]\n";

    ui_.output().addTextTab("plot", plot.str());

    st.last.has = true;
}

// 牛顿差商插值（不等距节点）
void Manager::computeNewtonDividedDiff(const std::string &expName)
{
    auto &st = states_[expName];

    // 确保有函数值表
    if (st.valueTable.rows() == 0 || st.valueTable.cols() < 2)
    {
        // 尝试加载第一个预设
        ensureValueTablePresets();
        if (!valueTablePresets_.empty())
        {
            st.valueTablePresetIndex = 0;
            st.valueTable = valueTablePresets_[0].table;
        }
        else
        {
            ui_.output().clear();
            ui_.output().addTextTab("error", "函数值表为空，请先编辑或选择预设。");
            st.last.has = true;
            return;
        }
    }

    // 解析输入的待求点 x
    double xVal = toDouble(ui_.getInputValue(1), 0.5);

    // 提取 x 和 y 数据
    std::vector<double> x_data, y_data;
    for (int i = 0; i < st.valueTable.rows(); ++i)
    {
        double xi = st.valueTable(i, 0);
        double yi = st.valueTable(i, 1);
        if (!std::isnan(xi) && !std::isnan(yi))
        {
            x_data.push_back(xi);
            y_data.push_back(yi);
        }
    }

    if (x_data.size() < 2)
    {
        ui_.output().clear();
        ui_.output().addTextTab("error", "有效节点数不足（至少需要2个点）");
        st.last.has = true;
        return;
    }

    // 调用calc中的插值函数
    auto result = calc::newtonDividedDifference(x_data, y_data, xVal);

    ui_.output().clear();

    if (!result.success)
    {
        ui_.output().addTextTab("error", result.errorMsg);
        st.last.has = true;
        return;
    }

    // 输出结果摘要
    std::ostringstream summary;
    summary << "牛顿差商插值（不等距节点）\n";
    summary << "==============================\n";
    summary << "节点数: " << x_data.size() << "\n";
    summary << "待求点: x = " << fmt(xVal, 8) << "\n";
    summary << "插值结果: P_n(" << fmt(xVal, 8) << ") = " << fmt(result.value, 12) << "\n";
    summary << "==============================\n";
    if (x_data.size() <= 10)
    {
        summary << "节点数据:\n";
        for (size_t i = 0; i < x_data.size(); ++i)
        {
            summary << "  x[" << i << "] = " << fmt(x_data[i], 8)
                    << ", f(x[" << i << "]) = " << fmt(y_data[i], 8) << "\n";
        }
    }
    ui_.output().addTextTab("摘要", summary.str());

    // 输出差商表
    if (!result.table.empty())
    {
        UiOutputPane::TableData tableData;
        // 表头：i, x_i, f[x_i], f[x_i,x_{i+1}], ...
        tableData.headers.push_back("i");
        tableData.headers.push_back("x_i");
        int maxOrder = 0;
        for (const auto &row : result.table)
        {
            if ((int)row.size() > maxOrder)
                maxOrder = row.size();
        }
        for (int j = 0; j < maxOrder; ++j)
        {
            std::ostringstream hdr;
            if (j == 0)
                hdr << "f[x_i]";
            else
                hdr << "f[..+" << j << "]";
            tableData.headers.push_back(hdr.str());
        }

        for (size_t i = 0; i < result.table.size(); ++i)
        {
            std::vector<std::string> row;
            row.push_back(std::to_string(i));
            row.push_back(fmt(x_data[i], 8));
            for (size_t j = 0; j < result.table[i].size(); ++j)
            {
                row.push_back(fmt(result.table[i][j], 10));
            }
            // 填充空白
            while (row.size() < tableData.headers.size())
            {
                row.push_back("");
            }
            tableData.rows.push_back(row);
        }
        ui_.output().addTableTab("差商表", tableData);
    }

    // 输出插值多项式
    if (!result.polynomial.empty())
    {
        std::ostringstream poly;
        poly << "插值多项式：\n";
        poly << result.polynomial << "\n";
        ui_.output().addTextTab("多项式", poly.str());
    }

    // 绘制函数图像和插值点
    if (x_data.size() >= 2)
    {
        std::ostringstream plot;
        int width = 80;
        int height = 20;

        double minX = x_data[0], maxX = x_data[0];
        double minY = y_data[0], maxY = y_data[0];
        for (size_t i = 1; i < x_data.size(); ++i)
        {
            if (x_data[i] < minX)
                minX = x_data[i];
            if (x_data[i] > maxX)
                maxX = x_data[i];
            if (y_data[i] < minY)
                minY = y_data[i];
            if (y_data[i] > maxY)
                maxY = y_data[i];
        }

        // 包含插值点在范围内
        if (result.value < minY)
            minY = result.value;
        if (result.value > maxY)
            maxY = result.value;
        if (xVal < minX)
            minX = xVal;
        if (xVal > maxX)
            maxX = xVal;

        double rangeX = maxX - minX;
        double rangeY = maxY - minY;
        if (rangeX < 1e-9)
            rangeX = 1.0;
        if (rangeY < 1e-9)
            rangeY = 1.0;
        minX -= rangeX * 0.1;
        maxX += rangeX * 0.1;
        minY -= rangeY * 0.1;
        maxY += rangeY * 0.1;

        std::vector<std::string> canvas(height, std::string(width, ' '));

        // 绘制坐标轴
        int zeroX = (int)((-minX) / (maxX - minX) * (width - 1));
        int zeroY = (int)((maxY) / (maxY - minY) * (height - 1));
        if (zeroX >= 0 && zeroX < width)
        {
            for (int py = 0; py < height; ++py)
                canvas[py][zeroX] = '|';
        }
        if (zeroY >= 0 && zeroY < height)
        {
            for (int px = 0; px < width; ++px)
                canvas[zeroY][px] = '-';
        }
        if (zeroX >= 0 && zeroX < width && zeroY >= 0 && zeroY < height)
            canvas[zeroY][zeroX] = '+';

        // 绘制数据点
        for (size_t i = 0; i < x_data.size(); ++i)
        {
            int px = (int)((x_data[i] - minX) / (maxX - minX) * (width - 1));
            int py = (int)((maxY - y_data[i]) / (maxY - minY) * (height - 1));
            if (px >= 0 && px < width && py >= 0 && py < height)
                canvas[py][px] = '*';
        }

        // 绘制插值点（红色标记用 # 表示）
        int px = (int)((xVal - minX) / (maxX - minX) * (width - 1));
        int py = (int)((maxY - result.value) / (maxY - minY) * (height - 1));
        if (px >= 0 && px < width && py >= 0 && py < height)
            canvas[py][px] = '#';

        plot << "函数图像（* 数据点，# 插值点）\n";
        for (const auto &line : canvas)
            plot << line << "\n";
        plot << "\nx in [" << fmt(minX, 8) << ", " << fmt(maxX, 8) << "]\n";
        plot << "y in [" << fmt(minY, 8) << ", " << fmt(maxY, 8) << "]\n";
        plot << "插值点: (" << fmt(xVal, 8) << ", " << fmt(result.value, 8) << ")\n";
        ui_.output().addTextTab("图像", plot.str());
    }

    st.last.has = true;
}

// 牛顿差分插值（等距节点）
void Manager::computeNewtonEqualDiff(const std::string &expName)
{
    auto &st = states_[expName];

    // 确保有函数值表
    if (st.valueTable.rows() == 0 || st.valueTable.cols() < 2)
    {
        // 尝试加载第一个等距预设
        ensureValueTablePresets();
        if (!valueTablePresets_.empty())
        {
            st.valueTablePresetIndex = 0;
            st.valueTable = valueTablePresets_[0].table;
        }
        else
        {
            ui_.output().clear();
            ui_.output().addTextTab("error", "函数值表为空，请先编辑或选择预设。");
            st.last.has = true;
            return;
        }
    }

    // 解析输入的待求点 x
    double xVal = toDouble(ui_.getInputValue(1), 0.5);

    // 提取 x 和 y 数据
    std::vector<double> x_data, y_data;
    for (int i = 0; i < st.valueTable.rows(); ++i)
    {
        double xi = st.valueTable(i, 0);
        double yi = st.valueTable(i, 1);
        if (!std::isnan(xi) && !std::isnan(yi))
        {
            x_data.push_back(xi);
            y_data.push_back(yi);
        }
    }

    if (x_data.size() < 5)
    {
        ui_.output().clear();
        ui_.output().addTextTab("error", "等距节点插值至少需要5个点");
        st.last.has = true;
        return;
    }

    // 检查等距性并选择方法
    auto methodInfo = calc::selectInterpolationMethod(x_data, xVal);

    ui_.output().clear();

    if (!methodInfo.isEquidistant)
    {
        ui_.output().addTextTab("error", "节点不等距，请使用牛顿差商插值");
        st.last.has = true;
        return;
    }

    // 输出方法选择信息
    std::ostringstream methodDesc;
    methodDesc << "等距节点检查：\n";
    methodDesc << "==============================\n";
    methodDesc << "节点数: " << x_data.size() << "\n";
    methodDesc << "间距 h = " << fmt(methodInfo.h, 8) << "\n";
    methodDesc << "待求点: x = " << fmt(xVal, 8) << "\n";
    methodDesc << "推荐方法: ";
    if (methodInfo.recommendedMethod == "forward")
        methodDesc << "牛顿前插公式\n";
    else if (methodInfo.recommendedMethod == "backward")
        methodDesc << "牛顿后插公式\n";
    else if (methodInfo.recommendedMethod == "stirling")
        methodDesc << "斯梯林插值公式\n";
    else if (methodInfo.recommendedMethod == "bessel")
        methodDesc << "贝塞尔插值公式\n";
    methodDesc << "理由: " << methodInfo.reason << "\n";
    methodDesc << "基准节点: x[" << methodInfo.baseIndex << "] = " << fmt(x_data[methodInfo.baseIndex], 8) << "\n";
    methodDesc << "归一化参数: t = " << fmt(methodInfo.t, 8) << "\n";
    methodDesc << "==============================\n";
    ui_.output().addTextTab("方法选择", methodDesc.str());

    // 调用相应的插值方法
    calc::InterpolationResult result;
    if (methodInfo.recommendedMethod == "forward")
    {
        result = calc::newtonForwardDifference(x_data, y_data, xVal);
    }
    else if (methodInfo.recommendedMethod == "backward")
    {
        result = calc::newtonBackwardDifference(x_data, y_data, xVal);
    }
    else if (methodInfo.recommendedMethod == "stirling")
    {
        result = calc::stirlingInterpolation(x_data, y_data, xVal);
    }
    else if (methodInfo.recommendedMethod == "bessel")
    {
        result = calc::besselInterpolation(x_data, y_data, xVal);
    }
    else
    {
        // 默认使用前插
        result = calc::newtonForwardDifference(x_data, y_data, xVal);
    }

    if (!result.success)
    {
        ui_.output().addTextTab("error", result.errorMsg);
        st.last.has = true;
        return;
    }

    // 输出结果摘要
    std::ostringstream summary;
    summary << "计算结果：\n";
    summary << "==============================\n";
    summary << "使用方法: " << result.method << "\n";
    summary << "插值结果: P_n(" << fmt(xVal, 8) << ") = " << fmt(result.value, 12) << "\n";
    summary << "==============================\n";
    ui_.output().addTextTab("结果", summary.str());

    // 输出差分表
    if (!result.table.empty())
    {
        UiOutputPane::TableData tableData;
        tableData.headers.push_back("i");
        tableData.headers.push_back("x_i");

        int maxOrder = 0;
        for (const auto &row : result.table)
        {
            if ((int)row.size() > maxOrder)
                maxOrder = row.size();
        }

        if (result.method.find("前插") != std::string::npos)
        {
            for (int j = 0; j < maxOrder; ++j)
            {
                std::ostringstream hdr;
                if (j == 0)
                    hdr << "y_i";
                else
                    hdr << "Δ^" << j << "y_i";
                tableData.headers.push_back(hdr.str());
            }
        }
        else if (result.method.find("后插") != std::string::npos)
        {
            for (int j = 0; j < maxOrder; ++j)
            {
                std::ostringstream hdr;
                if (j == 0)
                    hdr << "y_i";
                else
                    hdr << "∇^" << j << "y_i";
                tableData.headers.push_back(hdr.str());
            }
        }
        else
        {
            for (int j = 0; j < maxOrder; ++j)
            {
                tableData.headers.push_back("col" + std::to_string(j));
            }
        }

        for (size_t i = 0; i < result.table.size(); ++i)
        {
            std::vector<std::string> row;
            row.push_back(std::to_string(i));
            if (i < x_data.size())
                row.push_back(fmt(x_data[i], 8));
            else
                row.push_back("");

            for (size_t j = 0; j < result.table[i].size(); ++j)
            {
                row.push_back(fmt(result.table[i][j], 10));
            }

            while (row.size() < tableData.headers.size())
            {
                row.push_back("");
            }
            tableData.rows.push_back(row);
        }
        ui_.output().addTableTab("差分表", tableData);
    }

    // 输出详细步骤
    if (!result.stepDesc.empty())
    {
        std::ostringstream steps;
        steps << "计算步骤：\n";
        for (const auto &step : result.stepDesc)
        {
            steps << step << "\n";
        }
        ui_.output().addTextTab("步骤", steps.str());
    }

    // 绘制函数图像和插值点
    if (x_data.size() >= 2)
    {
        std::ostringstream plot;
        int width = 80;
        int height = 20;

        double minX = x_data[0], maxX = x_data[0];
        double minY = y_data[0], maxY = y_data[0];
        for (size_t i = 1; i < x_data.size(); ++i)
        {
            if (x_data[i] < minX)
                minX = x_data[i];
            if (x_data[i] > maxX)
                maxX = x_data[i];
            if (y_data[i] < minY)
                minY = y_data[i];
            if (y_data[i] > maxY)
                maxY = y_data[i];
        }

        // 包含插值点在范围内
        if (result.value < minY)
            minY = result.value;
        if (result.value > maxY)
            maxY = result.value;
        if (xVal < minX)
            minX = xVal;
        if (xVal > maxX)
            maxX = xVal;

        double rangeX = maxX - minX;
        double rangeY = maxY - minY;
        if (rangeX < 1e-9)
            rangeX = 1.0;
        if (rangeY < 1e-9)
            rangeY = 1.0;
        minX -= rangeX * 0.1;
        maxX += rangeX * 0.1;
        minY -= rangeY * 0.1;
        maxY += rangeY * 0.1;

        std::vector<std::string> canvas(height, std::string(width, ' '));

        // 绘制坐标轴
        int zeroX = (int)((-minX) / (maxX - minX) * (width - 1));
        int zeroY = (int)((maxY) / (maxY - minY) * (height - 1));
        if (zeroX >= 0 && zeroX < width)
        {
            for (int py = 0; py < height; ++py)
                canvas[py][zeroX] = '|';
        }
        if (zeroY >= 0 && zeroY < height)
        {
            for (int px = 0; px < width; ++px)
                canvas[zeroY][px] = '-';
        }
        if (zeroX >= 0 && zeroX < width && zeroY >= 0 && zeroY < height)
            canvas[zeroY][zeroX] = '+';

        // 绘制数据点
        for (size_t i = 0; i < x_data.size(); ++i)
        {
            int px = (int)((x_data[i] - minX) / (maxX - minX) * (width - 1));
            int py = (int)((maxY - y_data[i]) / (maxY - minY) * (height - 1));
            if (px >= 0 && px < width && py >= 0 && py < height)
                canvas[py][px] = '*';
        }

        // 绘制插值点（红色标记用 # 表示）
        int px = (int)((xVal - minX) / (maxX - minX) * (width - 1));
        int py = (int)((maxY - result.value) / (maxY - minY) * (height - 1));
        if (px >= 0 && px < width && py >= 0 && py < height)
            canvas[py][px] = '#';

        plot << "函数图像（* 数据点，# 插值点）\n";
        for (const auto &line : canvas)
            plot << line << "\n";
        plot << "\nx in [" << fmt(minX, 8) << ", " << fmt(maxX, 8) << "]\n";
        plot << "y in [" << fmt(minY, 8) << ", " << fmt(maxY, 8) << "]\n";
        plot << "插值点: (" << fmt(xVal, 8) << ", " << fmt(result.value, 8) << ")\n";
        ui_.output().addTextTab("图像", plot.str());
    }

    st.last.has = true;
}

// 拉格朗日插值
void Manager::computeLagrange(const std::string &expName)
{
    auto &st = states_[expName];

    // 确保有函数值表
    if (st.valueTable.rows() == 0 || st.valueTable.cols() < 2)
    {
        ensureValueTablePresets();
        if (!valueTablePresets_.empty())
        {
            st.valueTablePresetIndex = 0;
            st.valueTable = valueTablePresets_[0].table;
        }
        else
        {
            ui_.output().clear();
            ui_.output().addTextTab("error", "函数值表为空，请先编辑或选择预设。");
            st.last.has = true;
            return;
        }
    }

    double xVal = toDouble(ui_.getInputValue(1), 1.5);

    std::vector<double> x_data, y_data;
    for (int i = 0; i < st.valueTable.rows(); ++i)
    {
        double xi = st.valueTable(i, 0);
        double yi = st.valueTable(i, 1);
        if (!std::isnan(xi) && !std::isnan(yi))
        {
            x_data.push_back(xi);
            y_data.push_back(yi);
        }
    }

    if (x_data.size() < 2)
    {
        ui_.output().clear();
        ui_.output().addTextTab("error", "有效节点数不足（至少需要2个点）");
        st.last.has = true;
        return;
    }

    auto result = calc::lagrangeInterpolation(x_data, y_data, xVal);

    ui_.output().clear();

    if (!result.success)
    {
        ui_.output().addTextTab("error", result.errorMsg);
        st.last.has = true;
        return;
    }

    std::ostringstream summary;
    summary << "拉格朗日插值公式\n";
    summary << "==============================\n";
    summary << "节点数: " << x_data.size() << "\n";
    summary << "待求点: x = " << fmt(xVal, 8) << "\n";
    summary << "插值结果: L_n(" << fmt(xVal, 8) << ") = " << fmt(result.value, 12) << "\n";
    summary << "==============================\n";
    if (x_data.size() <= 10)
    {
        summary << "节点数据:\n";
        for (size_t i = 0; i < x_data.size(); ++i)
        {
            summary << "  x[" << i << "] = " << fmt(x_data[i], 8)
                    << ", f(x[" << i << "]) = " << fmt(y_data[i], 8) << "\n";
        }
    }
    ui_.output().addTextTab("摘要", summary.str());

    if (!result.coefficients.empty())
    {
        UiOutputPane::TableData tableData;
        tableData.headers.push_back("i");
        tableData.headers.push_back("x_i");
        tableData.headers.push_back("f(x_i)");
        tableData.headers.push_back("l_i(x)");
        tableData.headers.push_back("l_i(x)·f(x_i)");

        for (size_t i = 0; i < x_data.size(); ++i)
        {
            std::vector<std::string> row;
            row.push_back(std::to_string(i));
            row.push_back(fmt(x_data[i], 8));
            row.push_back(fmt(y_data[i], 8));
            row.push_back(fmt(result.coefficients[i], 10));
            row.push_back(fmt(result.coefficients[i] * y_data[i], 10));
            tableData.rows.push_back(row);
        }

        ui_.output().addTableTab("基函数值", tableData);
    }

    if (!result.polynomial.empty())
    {
        std::ostringstream poly;
        poly << "插值多项式：\n";
        poly << result.polynomial << "\n";
        ui_.output().addTextTab("多项式", poly.str());
    }

    if (!result.stepDesc.empty())
    {
        std::ostringstream steps;
        steps << "计算步骤：\n";
        for (const auto &step : result.stepDesc)
        {
            steps << step << "\n";
        }
        ui_.output().addTextTab("步骤", steps.str());
    }

    if (x_data.size() >= 2)
    {
        std::ostringstream plot;
        int width = 80;
        int height = 20;

        double minX = x_data[0], maxX = x_data[0];
        double minY = y_data[0], maxY = y_data[0];
        for (size_t i = 1; i < x_data.size(); ++i)
        {
            if (x_data[i] < minX)
                minX = x_data[i];
            if (x_data[i] > maxX)
                maxX = x_data[i];
            if (y_data[i] < minY)
                minY = y_data[i];
            if (y_data[i] > maxY)
                maxY = y_data[i];
        }

        if (result.value < minY)
            minY = result.value;
        if (result.value > maxY)
            maxY = result.value;
        if (xVal < minX)
            minX = xVal;
        if (xVal > maxX)
            maxX = xVal;

        double rangeX = maxX - minX;
        double rangeY = maxY - minY;
        if (rangeX < 1e-9)
            rangeX = 1.0;
        if (rangeY < 1e-9)
            rangeY = 1.0;
        minX -= rangeX * 0.1;
        maxX += rangeX * 0.1;
        minY -= rangeY * 0.1;
        maxY += rangeY * 0.1;

        std::vector<std::string> canvas(height, std::string(width, ' '));

        int zeroX = (int)((-minX) / (maxX - minX) * (width - 1));
        int zeroY = (int)((maxY) / (maxY - minY) * (height - 1));
        if (zeroX >= 0 && zeroX < width)
        {
            for (int py = 0; py < height; ++py)
                canvas[py][zeroX] = '|';
        }
        if (zeroY >= 0 && zeroY < height)
        {
            for (int px = 0; px < width; ++px)
                canvas[zeroY][px] = '-';
        }
        if (zeroX >= 0 && zeroX < width && zeroY >= 0 && zeroY < height)
            canvas[zeroY][zeroX] = '+';

        for (size_t i = 0; i < x_data.size(); ++i)
        {
            int px = (int)((x_data[i] - minX) / (maxX - minX) * (width - 1));
            int py = (int)((maxY - y_data[i]) / (maxY - minY) * (height - 1));
            if (px >= 0 && px < width && py >= 0 && py < height)
                canvas[py][px] = '*';
        }

        int px = (int)((xVal - minX) / (maxX - minX) * (width - 1));
        int py = (int)((maxY - result.value) / (maxY - minY) * (height - 1));
        if (px >= 0 && px < width && py >= 0 && py < height)
            canvas[py][px] = '#';

        plot << "函数图像（* 数据点，# 插值点）\n";
        for (const auto &line : canvas)
            plot << line << "\n";
        plot << "\nx in [" << fmt(minX, 8) << ", " << fmt(maxX, 8) << "]\n";
        plot << "y in [" << fmt(minY, 8) << ", " << fmt(maxY, 8) << "]\n";
        plot << "插值点: (" << fmt(xVal, 8) << ", " << fmt(result.value, 8) << ")\n";
        ui_.output().addTextTab("图像", plot.str());
    }

    st.last.has = true;
}

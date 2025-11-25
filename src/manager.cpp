#include "manager.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <pdcurses.h>

using std::string;

Manager::Manager(UI &ui) : ui_(ui)
{
    ensurePresets();
    ensureMatrixPresets();
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
        // 对于矩阵类（多标签）方法，已在计算函数中直接显示；否则调用 useExperiment
        if (name.find("高斯消元") == std::string::npos &&
            name.find("克劳特消元") == std::string::npos &&
            name.find("平方根法") == std::string::npos &&
            name.find("追赶法") == std::string::npos &&
            name.find("列主元素法") == std::string::npos &&
            name.find("全主元素法") == std::string::npos)
        {
            useExperiment(name);
        } });

    ui_.onPresetChange([this](int delta)
                       {
        const auto name = ui_.getCurrentExperiment();
        if (name.empty()) return;
        // 矩阵类方法：切换矩阵预设
        if (name.find("高斯消元") != std::string::npos ||
            name.find("克劳特消元") != std::string::npos ||
            name.find("平方根法") != std::string::npos ||
            name.find("追赶法") != std::string::npos ||
            name.find("列主元素法") != std::string::npos ||
            name.find("全主元素法") != std::string::npos ||
            name.find("雅可比") != std::string::npos ||
            name.find("高斯-赛德尔") != std::string::npos ||
            name.find("松弛") != std::string::npos)
        {
            cycleMatrixPresetFor(name, delta);
        }
        else
        {
            cyclePresetFor(name, delta);
        }
        fillDescriptionFor(name); });

    ui_.onAddPreset([this]()
                    { 
        const auto name = ui_.getCurrentExperiment();
        if (name.empty()) return;
        // 矩阵类方法：添加矩阵预设
        if (name.find("高斯消元") != std::string::npos ||
            name.find("克劳特消元") != std::string::npos ||
            name.find("平方根法") != std::string::npos ||
            name.find("追赶法") != std::string::npos ||
            name.find("列主元素法") != std::string::npos ||
            name.find("全主元素法") != std::string::npos ||
            name.find("雅可比") != std::string::npos ||
            name.find("高斯-赛德尔") != std::string::npos ||
            name.find("松弛") != std::string::npos)
        {
            addMatrixPreset();
        }
        else
        {
            addPolynomialPreset();
        } });

    ui_.onMatrixInput([this]()
                      {
        const auto name = ui_.getCurrentExperiment();
        if (name.empty()) return;
        // 矩阵类方法均支持矩阵编辑
        if (name.find("高斯消元") != std::string::npos ||
            name.find("克劳特消元") != std::string::npos ||
            name.find("平方根法") != std::string::npos ||
            name.find("追赶法") != std::string::npos ||
            name.find("列主元素法") != std::string::npos ||
            name.find("全主元素法") != std::string::npos ||
            name.find("雅可比") != std::string::npos ||
            name.find("高斯-赛德尔") != std::string::npos ||
            name.find("松弛") != std::string::npos)
            showMatrixInputDialog(name, false); });

    ui_.onMatrixPresetChange([this](int delta)
                             {
        const auto name = ui_.getCurrentExperiment();
        if (name.empty()) return;
        cycleMatrixPresetFor(name, delta); });
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
        // 恢复额外的表格标签页
        for (const auto &extra : snap.extraTables)
            ui_.output().addTableTab(extra.first, extra.second);
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
    if (name.find("埃特肯") != string::npos || name.find("加速") != string::npos)
    {
        computeAitken(name);
        return;
    }
    if (name.find("下山法") != string::npos)
    {
        computeNewtonDownhill(name);
        return;
    }
    if (name.find("单点弦截法") != string::npos)
    {
        computeSecantSinglePoint(name);
        return;
    }
    if (name.find("双点弦截法") != string::npos)
    {
        computeSecantDoublePoint(name);
        return;
    }
    if (name.find("高斯消元") != string::npos)
    {
        computeGaussElimination(name);
        return;
    }
    if (name.find("克劳特消元") != string::npos)
    {
        computeCroutElimination(name);
        return;
    }
    if (name.find("平方根法") != string::npos)
    {
        computeCholesky(name);
        return;
    }
    if (name.find("追赶法") != string::npos)
    {
        computeThomas(name);
        return;
    }
    if (name.find("列主元素法") != string::npos)
    {
        computeColumnPivoting(name);
        return;
    }
    if (name.find("全主元素法") != string::npos)
    {
        computeFullPivoting(name);
        return;
    }
    if (name.find("雅可比") != string::npos)
    {
        computeJacobi(name);
        return;
    }
    if (name.find("高斯-赛德尔") != string::npos)
    {
        computeGaussSeidel(name);
        return;
    }
    if (name.find("松弛") != string::npos)
    {
        computeSOR(name);
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
    else if (name.find("埃特肯") != string::npos || name.find("加速") != string::npos)
    {
        st.fields.push_back({"初值 x0:", "1.0", "例如 1.0", 50});
        st.fields.push_back({"最大迭代步:", "50", "整数", 10});
        st.fields.push_back({"容差 tol:", "1e-6", "如 1e-6", 20});
    }
    else if (name.find("下山法") != string::npos)
    {
        st.fields.push_back({"初值 x0:", "1.0", "例如 1.0", 50});
        st.fields.push_back({"最大迭代步:", "50", "整数", 10});
        st.fields.push_back({"容差 tol:", "1e-6", "如 1e-6", 20});
    }
    else if (name.find("单点弦截法") != string::npos)
    {
        st.fields.push_back({"初值 x0:", "1.0", "例如 1.0", 50});
        st.fields.push_back({"初值 x1:", "1.5", "例如 1.5", 50});
        st.fields.push_back({"最大迭代步:", "50", "整数", 10});
        st.fields.push_back({"容差 tol:", "1e-6", "如 1e-6", 20});
    }
    else if (name.find("双点弦截法") != string::npos)
    {
        st.fields.push_back({"初值 x0:", "1.0", "例如 1.0", 50});
        st.fields.push_back({"备用 x1:", "1.5", "备用值，优先用牛顿法计算", 50});
        st.fields.push_back({"最大迭代步:", "50", "整数", 10});
        st.fields.push_back({"容差 tol:", "1e-6", "如 1e-6", 20});
    }
    else if (name.find("高斯消元") != string::npos)
    {
        st.fields.push_back({"点击说明区 [m] 键输入矩阵", "", "或使用预设 ([<][>] 切换)", 50});
    }
    else if (name.find("克劳特消元") != string::npos)
    {
        st.fields.push_back({"点击说明区 [m] 键输入矩阵", "", "或使用预设 ([<][>] 切换)", 50});
    }
    else if (name.find("平方根法") != string::npos)
    {
        st.fields.push_back({"点击说明区 [m] 键输入矩阵 (需对称正定)", "", "或使用预设 ([<][>] 切换)", 50});
    }
    else if (name.find("追赶法") != string::npos)
    {
        st.fields.push_back({"点击说明区 [m] 键输入矩阵 (需三对角)", "", "或使用预设 ([<][>] 切换)", 50});
    }
    else if (name.find("列主元素法") != string::npos)
    {
        st.fields.push_back({"点击说明区 [m] 键输入矩阵", "", "或使用预设 ([<][>] 切换)", 50});
    }
    else if (name.find("全主元素法") != string::npos)
    {
        st.fields.push_back({"点击说明区 [m] 键输入矩阵", "", "或使用预设 ([<][>] 切换)", 50});
    }
    else if (name.find("雅可比") != string::npos)
    {
        st.fields.push_back({"点击说明区 [m] 键输入矩阵 A 和向量 b", "", "", 50});
        st.fields.push_back({"精度 tol:", "1e-6", "如 1e-6", 20});
        st.fields.push_back({"最大迭代次数:", "100", "整数", 10});
    }
    else if (name.find("高斯-赛德尔") != string::npos)
    {
        st.fields.push_back({"点击说明区 [m] 键输入矩阵 A 和向量 b", "", "", 50});
        st.fields.push_back({"精度 tol:", "1e-6", "如 1e-6", 20});
        st.fields.push_back({"最大迭代次数:", "100", "整数", 10});
    }
    else if (name.find("松弛") != string::npos)
    {
        st.fields.push_back({"点击说明区 [m] 键输入矩阵 A 和向量 b", "", "", 50});
        st.fields.push_back({"精度 tol:", "1e-6", "如 1e-6", 20});
        st.fields.push_back({"最大迭代次数:", "100", "整数", 10});
        st.fields.push_back({"松弛因子 ω:", "1.2", "(0,2) 内实数", 20});
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
    else if (name.find("埃特肯") != string::npos || name.find("加速") != string::npos)
    {
        oss << "埃特肯（加速）法：\n";
        oss << "- 使用埃特肯加速公式：y_{n+1}=φ(x_n), z_{n+1}=φ(y_{n+1})\n";
        oss << "- x_{n+1} = (x_n·z_{n+1} - y_{n+1}²) / (x_n - 2y_{n+1} + z_{n+1})\n";
        oss << "- 先用牛顿迭代法公式计算 φ(x) = x - f(x)/f'(x)\n";
        if (!presets_.empty())
        {
            const auto &pz = presets_[states_[name].presetIndex % presets_.size()];
            oss << "- 当前预设：f(x) = " << pz.name << (pz.hasDf ? ", f'(x) 可用" : ", f'(x) 不可用") << "\n";
        }
        oss << "- 提示：在说明区标题右侧 Preset [<] [>] 切换；按 [a] 添加多项式预设\n";
    }
    else if (name.find("下山法") != string::npos)
    {
        oss << "牛顿下山法：\n";
        oss << "- 引入参数 λ: x_{n+1} = x_n - λ·f(x_n)/f'(x_n)\n";
        oss << "- 下山条件：|f(x_{n+1})| < |f(x_n)|\n";
        oss << "- λ 取值序列：1, 1/2, 1/4, 1/8, ...\n";
        if (!presets_.empty())
        {
            const auto &pz = presets_[states_[name].presetIndex % presets_.size()];
            oss << "- 当前预设：f(x) = " << pz.name << (pz.hasDf ? ", f'(x) 可用" : ", f'(x) 不可用") << "\n";
        }
        oss << "- 提示：在说明区标题右侧 Preset [<] [>] 切换；按 [a] 添加多项式预设\n";
    }
    else if (name.find("单点弦截法") != string::npos)
    {
        oss << "单点弦截法（收敛阶数1）：\n";
        oss << "- 用差商近似导数：f'(x_n) ≈ [f(x_n)-f(x_0)]/(x_n-x_0)\n";
        oss << "- 迭代公式：x_{n+1} = [x_0·f(x_n) - x_n·f(x_0)] / [f(x_n) - f(x_0)]\n";
        oss << "- 需要两个初值 x0, x1，但 x0 在迭代中保持不变\n";
        if (!presets_.empty())
            oss << "- 当前预设函数：f(x) = " << presets_[states_[name].presetIndex % presets_.size()].name << "\n";
        oss << "- 提示：在说明区标题右侧 Preset [<] [>] 切换；按 [a] 添加多项式预设\n";
    }
    else if (name.find("双点弦截法") != string::npos)
    {
        oss << "双点弦截法（收敛阶数2）：\n";
        oss << "- 用差商近似导数：f'(x_n) ≈ [f(x_n)-f(x_{n-1})]/(x_n-x_{n-1})\n";
        oss << "- 迭代公式：x_{n+1} = [x_{n-1}·f(x_n) - x_n·f(x_{n-1})] / [f(x_n) - f(x_{n-1})]\n";
        oss << "- x1 由牛顿法计算：x1 = x0 - f(x0)/f'(x0)，然后迭代中同时更新\n";
        if (!presets_.empty())
        {
            const auto &pz = presets_[states_[name].presetIndex % presets_.size()];
            oss << "- 当前预设函数：f(x) = " << pz.name << (pz.hasDf ? ", f'(x) 可用" : ", f'(x) 不可用") << "\n";
        }
        oss << "- 提示：在说明区标题右侧 Preset [<] [>] 切换；按 [a] 添加多项式预设\n";
    }
    else if (name.find("高斯消元") != string::npos)
    {
        ensureMatrixPresets();
        oss << "高斯消元法（列主元）：\n";
        oss << "- 解线性方程组 Ax = b\n";
        oss << "- 通过前向消元将矩阵化为上三角形式\n";
        oss << "- 然后回代求解\n";
        if (!matrixPresets_.empty())
        {
            const auto &preset = matrixPresets_[states_[name].matrixPresetIndex % matrixPresets_.size()];
            oss << "- 当前预设：" << preset.name << "\n";
            if (states_[name].matrixA.rows() > 0)
            {
                oss << "- 当前矩阵 A (" << states_[name].matrixA.rows() << "x" << states_[name].matrixA.cols() << ")\n";
            }
        }
        oss << "- 提示：按 [<] [>] 切换预设；按 [a] 创建新预设矩阵；按 [m] 编辑当前预设矩阵\n";
    }
    else if (name.find("克劳特消元") != string::npos)
    {
        ensureMatrixPresets();
        oss << "克劳特消元法（LU分解，u_ii = 1）：\n";
        oss << "- 解线性方程组 Ax = b\n";
        oss << "- 将矩阵 A 分解为 A = L·U，其中对角 u_ii = 1\n";
        oss << "- 先解 Ly = b，再解 Ux = y\n";
        if (!matrixPresets_.empty())
        {
            const auto &preset = matrixPresets_[states_[name].matrixPresetIndex % matrixPresets_.size()];
            oss << "- 当前预设：" << preset.name << "\n";
            if (states_[name].matrixA.rows() > 0)
            {
                oss << "- 当前矩阵 A (" << states_[name].matrixA.rows() << "x" << states_[name].matrixA.cols() << ")\n";
            }
        }
        oss << "- 提示：按 [<] [>] 切换预设；按 [a] 创建新预设矩阵；按 [m] 编辑当前预设矩阵\n";
    }
    else if (name.find("平方根法") != string::npos)
    {
        ensureMatrixPresets();
        oss << "平方根法（Cholesky，A 为实对称正定）：\n";
        oss << "- 分解 A = L·L^T，下三角 L，L^T 为上三角\n";
        oss << "- 解法：先解 Ly = b，再解 L^T x = y\n";
        if (!matrixPresets_.empty())
        {
            const auto &preset = matrixPresets_[states_[name].matrixPresetIndex % matrixPresets_.size()];
            oss << "- 当前预设：" << preset.name << "\n";
            if (states_[name].matrixA.rows() > 0)
            {
                oss << "- 当前矩阵 A (" << states_[name].matrixA.rows() << "x" << states_[name].matrixA.cols() << ")\n";
            }
        }
        oss << "- 提示：按 [<] [>] 切换预设；按 [a] 创建新预设矩阵；按 [m] 编辑当前预设矩阵\n";
    }
    else if (name.find("追赶法") != string::npos)
    {
        ensureMatrixPresets();
        oss << "追赶法（Thomas，A 为三对角矩阵）：\n";
        oss << "- 前向消去构造修正系数 c' 和 d'，再回代求解\n";
        if (!matrixPresets_.empty())
        {
            const auto &preset = matrixPresets_[states_[name].matrixPresetIndex % matrixPresets_.size()];
            oss << "- 当前预设：" << preset.name << "\n";
            if (states_[name].matrixA.rows() > 0)
            {
                oss << "- 当前矩阵 A (" << states_[name].matrixA.rows() << "x" << states_[name].matrixA.cols() << ")\n";
            }
        }
        oss << "- 提示：按 [<] [>] 切换预设；按 [a] 创建新预设矩阵；按 [m] 编辑当前预设矩阵\n";
    }
    else if (name.find("列主元素法") != string::npos)
    {
        ensureMatrixPresets();
        oss << "列主元素法：\n";
        oss << "- 每次选取当前列（第k列）绝对值最大的元素\n";
        oss << "- 将其交换到主元位置（第k行）\n";
        oss << "- 然后进行高斯消元\n";
        if (!matrixPresets_.empty())
        {
            const auto &preset = matrixPresets_[states_[name].matrixPresetIndex % matrixPresets_.size()];
            oss << "- 当前预设：" << preset.name << "\n";
            if (states_[name].matrixA.rows() > 0)
            {
                oss << "- 当前矩阵 A (" << states_[name].matrixA.rows() << "x" << states_[name].matrixA.cols() << ")\n";
            }
        }
        oss << "- 提示：按 [<] [>] 切换预设；按 [a] 创建新预设矩阵；按 [m] 编辑当前预设矩阵\n";
    }
    else if (name.find("全主元素法") != string::npos)
    {
        ensureMatrixPresets();
        oss << "全主元素法：\n";
        oss << "- 每次选取剩余子矩阵中绝对值最大的元素\n";
        oss << "- 通过行列交换将其移至主元位置\n";
        oss << "- 需记录列交换顺序以还原解向量\n";
        if (!matrixPresets_.empty())
        {
            const auto &preset = matrixPresets_[states_[name].matrixPresetIndex % matrixPresets_.size()];
            oss << "- 当前预设：" << preset.name << "\n";
            if (states_[name].matrixA.rows() > 0)
            {
                oss << "- 当前矩阵 A (" << states_[name].matrixA.rows() << "x" << states_[name].matrixA.cols() << ")\n";
            }
        }
        oss << "- 提示：按 [<] [>] 切换预设；按 [a] 创建新预设矩阵；按 [m] 编辑当前预设矩阵\n";
    }
    else if (name.find("雅可比") != string::npos)
    {
        ensureMatrixPresets();
        oss << "雅可比迭代法：\n";
        oss << "- 迭代公式：x(k+1) = D^(-1)(b - (L+U)x(k))\n";
        oss << "- D 为对角矩阵，L 为严格下三角，U 为严格上三角\n";
        oss << "- 迭代矩阵：B_J = -D^(-1)(L+U)\n";
        oss << "- 收敛条件：谱半径 ρ(B_J) < 1\n";
        if (!matrixPresets_.empty())
        {
            const auto &preset = matrixPresets_[states_[name].matrixPresetIndex % matrixPresets_.size()];
            oss << "- 当前预设：" << preset.name << "\n";
        }
        oss << "- 提示：按 [m] 编辑矩阵；设置精度和最大迭代次数\n";
    }
    else if (name.find("高斯-赛德尔") != string::npos)
    {
        ensureMatrixPresets();
        oss << "高斯-赛德尔迭代法：\n";
        oss << "- 迭代公式：x(k+1) = (D+L)^(-1)(b - Ux(k))\n";
        oss << "- 使用最新值立即参与后续计算\n";
        oss << "- 迭代矩阵：B_GS = -(D+L)^(-1)U\n";
        oss << "- 收敛条件：谱半径 ρ(B_GS) < 1\n";
        if (!matrixPresets_.empty())
        {
            const auto &preset = matrixPresets_[states_[name].matrixPresetIndex % matrixPresets_.size()];
            oss << "- 当前预设：" << preset.name << "\n";
        }
        oss << "- 提示：按 [m] 编辑矩阵；设置精度和最大迭代次数\n";
    }
    else if (name.find("松弛") != string::npos)
    {
        ensureMatrixPresets();
        oss << "松弛迭代法（SOR）：\n";
        oss << "- 迭代公式：x(k+1) = (D+ωL)^(-1)[ωb - (ωU + (ω-1)D)x(k)]\n";
        oss << "- 松弛因子 ω ∈ (0, 2)，ω=1 时退化为 Gauss-Seidel\n";
        oss << "- 最优松弛因子可加速收敛\n";
        oss << "- 收敛条件：谱半径 ρ(B_ω) < 1\n";
        if (!matrixPresets_.empty())
        {
            const auto &preset = matrixPresets_[states_[name].matrixPresetIndex % matrixPresets_.size()];
            oss << "- 当前预设：" << preset.name << "\n";
        }
        oss << "- 提示：按 [m] 编辑矩阵；设置精度、最大迭代次数和松弛因子\n";
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

// ==================== 矩阵相关 ====================

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

#include "manager.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
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
            name.find("全主元素法") == std::string::npos &&
            name.find("差商") == std::string::npos &&
            name.find("差分") == std::string::npos &&
            name.find("拉格朗日") == std::string::npos)
        {
            useExperiment(name);
        } });

    ui_.onPresetChange([this](int delta)
                       {
        const auto name = ui_.getCurrentExperiment();
        if (name.empty()) return;
        // 函数值表类方法：切换函数值表预设
        if (name.find("差商") != std::string::npos ||
            name.find("差分") != std::string::npos ||
            name.find("拉格朗日") != std::string::npos)
        {
            cycleValueTablePresetFor(name, delta);
        }
        // 矩阵类方法：切换矩阵预设
        else if (name.find("高斯消元") != std::string::npos ||
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
        // 函数值表类方法：添加函数值表预设
        if (name.find("差商") != std::string::npos ||
            name.find("差分") != std::string::npos ||
            name.find("拉格朗日") != std::string::npos)
        {
            addValueTablePreset();
        }
        // 矩阵类方法：添加矩阵预设
        else if (name.find("高斯消元") != std::string::npos ||
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
        // 函数值表类方法：支持函数值表编辑
        if (name.find("差商") != std::string::npos ||
            name.find("差分") != std::string::npos ||
            name.find("拉格朗日") != std::string::npos)
        {
            showValueTableInputDialog(name, false);
        }
        // 矩阵类方法均支持矩阵编辑
        else if (name.find("高斯消元") != std::string::npos ||
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
    if (name.find("差商") != string::npos && name.find("不等距") != string::npos)
    {
        computeNewtonDividedDiff(name);
        return;
    }
    if (name.find("差分") != string::npos && name.find("等距") != string::npos)
    {
        computeNewtonEqualDiff(name);
        return;
    }
    if (name.find("拉格朗日") != string::npos)
    {
        computeLagrange(name);
        return;
    }
    if (name.find("差商") != string::npos || name.find("差分") != string::npos)
    {
        computeDividedDifference(name);
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
    else if (name.find("不等距") != string::npos && name.find("差商") != string::npos)
    {
        st.fields.push_back({"点击说明区 [m] 键编辑函数值表", "", "或使用预设 ([<][>] 切换)", 50});
        st.fields.push_back({"待求插值点 x:", "0.5", "实数", 50});
        ensureValueTablePresets();
        if (st.valueTablePresetIndex < 0 && !valueTablePresets_.empty())
        {
            st.valueTablePresetIndex = 0;
            st.valueTable = valueTablePresets_[0].table;
        }
    }
    else if (name.find("等距") != string::npos && name.find("差分") != string::npos)
    {
        st.fields.push_back({"点击说明区 [m] 键编辑函数值表", "", "或使用预设 ([<][>] 切换)", 50});
        st.fields.push_back({"待求插值点 x:", "0.5", "实数", 50});
        ensureValueTablePresets();
        if (st.valueTablePresetIndex < 0 && !valueTablePresets_.empty())
        {
            st.valueTablePresetIndex = 0;
            st.valueTable = valueTablePresets_[0].table;
        }
    }
    else if (name.find("拉格朗日") != string::npos)
    {
        st.fields.push_back({"点击说明区 [m] 键编辑函数值表", "", "或使用预设 ([<][>] 切换)", 50});
        st.fields.push_back({"待求插值点 x:", "1.5", "实数", 50});
        ensureValueTablePresets();
        if (st.valueTablePresetIndex < 0 && !valueTablePresets_.empty())
        {
            st.valueTablePresetIndex = 0;
            st.valueTable = valueTablePresets_[0].table;
        }
    }
    else if (name.find("差商") != string::npos || name.find("差分") != string::npos)
    {
        st.fields.push_back({"点击说明区 [m] 键编辑函数值表", "", "或使用预设 ([<][>] 切换)", 50});
        ensureValueTablePresets();
        if (st.valueTablePresetIndex < 0 && !valueTablePresets_.empty())
        {
            st.valueTablePresetIndex = 0;
            st.valueTable = valueTablePresets_[0].table;
        }
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
    else if (name.find("不等距") != string::npos && name.find("差商") != string::npos)
    {
        ensureValueTablePresets();
        oss << "牛顿差商插值（不等距节点）：\n";
        oss << "- 公式：P_n(x) = f[x_0] + (x-x_0)f[x_0,x_1] + (x-x_0)(x-x_1)f[x_0,x_1,x_2] + ...\n";
        oss << "- 适用于任意节点分布（不要求等距）\n";
        oss << "- 输入函数值表（至少2个点），包含 x_i 和 f(x_i)\n";
        oss << "- 输入待求插值点 x，计算 P_n(x)\n";
        if (!valueTablePresets_.empty())
        {
            const auto &preset = valueTablePresets_[states_[name].valueTablePresetIndex % valueTablePresets_.size()];
            oss << "- 当前预设：" << preset.name << " (";
            oss << preset.table.rows() << " 个点, ";
            oss << preset.table.cols() << " 列)\n";
        }
        oss << "- 提示：按 [<] [>] 切换预设；按 [a] 创建新预设；按 [m] 编辑函数值表\n";
    }
    else if (name.find("等距") != string::npos && name.find("差分") != string::npos)
    {
        ensureValueTablePresets();
        oss << "牛顿差分插值（等距节点）：\n";
        oss << "- 要求节点等距：x_i = x_0 + i·h，至少5个点\n";
        oss << "- 四种公式：前插（t∈[0,1]）、后插（t∈[-1,0]）、斯梯林（中部）、贝塞尔（中部）\n";
        oss << "- 程序将根据待求点 x 的位置自动选择最合适的公式\n";
        oss << "- 输入函数值表（至少5个等距点），包含 x_i 和 f(x_i)\n";
        oss << "- 输入待求插值点 x，自动选择方法并计算 P_n(x)\n";
        if (!valueTablePresets_.empty())
        {
            const auto &preset = valueTablePresets_[states_[name].valueTablePresetIndex % valueTablePresets_.size()];
            oss << "- 当前预设：" << preset.name << " (";
            oss << preset.table.rows() << " 个点)\n";
        }
        oss << "- 提示：按 [<] [>] 切换预设（等距预设在前）；按 [m] 编辑函数值表\n";
    }
    else if (name.find("拉格朗日") != string::npos)
    {
        ensureValueTablePresets();
        oss << "拉格朗日插值公式：\n";
        oss << "- 公式：L_n(x) = Σ[i=0 to n] l_i(x)·f(x_i)\n";
        oss << "- 其中 l_i(x) = Π[j≠i] (x-x_j)/(x_i-x_j)\n";
        oss << "- 适用于任意节点分布（等距或不等距均可）\n";
        oss << "- 计算简单直观，但节点多时可能数值不稳定\n";
        oss << "- 输入函数值表（至少2个点），包含 x_i 和 f(x_i)\n";
        oss << "- 输入待求插值点 x，计算 L_n(x)\n";
        if (!valueTablePresets_.empty())
        {
            const auto &preset = valueTablePresets_[states_[name].valueTablePresetIndex % valueTablePresets_.size()];
            oss << "- 当前预设：" << preset.name << " (";
            oss << preset.table.rows() << " 个点)\n";
        }
        oss << "- 提示：按 [<] [>] 切换预设；按 [a] 创建新预设；按 [m] 编辑函数值表\n";
    }
    else if (name.find("差商") != string::npos || name.find("差分") != string::npos)
    {
        ensureValueTablePresets();
        oss << "差商与差分：\n";
        oss << "- 输入函数值表，包含 x_i, f(x_i) [, f'(x_i), ...]\n";
        oss << "- 自动计算差商表（牛顿插值基础）\n";
        oss << "- 若等距节点，还会计算前向差分表和后向差分表\n";
        oss << "- 未知值可用 NaN 表示\n";
        if (!valueTablePresets_.empty())
        {
            const auto &preset = valueTablePresets_[states_[name].valueTablePresetIndex % valueTablePresets_.size()];
            oss << "- 当前预设：" << preset.name << " (";
            oss << preset.table.rows() << " 个点, ";
            oss << preset.table.cols() << " 列)\n";
        }
        oss << "- 提示：按 [<] [>] 切换预设；按 [a] 创建新预设；按 [m] 编辑当前函数值表\n";
    }
    else
    {
        oss << "该实验暂未提供详细说明。\n";
    }
    states_[name].description = oss.str();
    ui_.setDescription(states_[name].description);
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

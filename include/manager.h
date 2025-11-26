#ifndef MANAGER_H
#define MANAGER_H

#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

#include "ui.h"
#include "calc.h"

// 管理器：负责各板块数据的 初始化/保存/加载/使用/计算
// 说明：
// - UI 负责展示与事件；
// - Manager 负责根据当前实验设置 UI 的说明与输入、保存/加载数据，并在提交后调用 calc 进行计算并填充输出；
// - calc 只做纯算法计算。

class Manager
{
public:
    explicit Manager(UI &ui);

    // 显式 API（一般通过回调触发，不需外部直接调用）
    void initExperiment(const std::string &name);    // 初始化（设置说明/输入，或加载历史）
    void saveExperiment(const std::string &name);    // 从 UI 保存当前输入到内存
    void loadExperiment(const std::string &name);    // 将内存中的输入加载回 UI
    void useExperiment(const std::string &name);     // 使用：将最近结果（若有）展示/强调
    void computeExperiment(const std::string &name); // 计算：调用 calc 并填充输出
    void addPolynomialPreset();                      // 添加自定义多项式预设

private:
    struct ResultSnapshot
    {
        UiOutputPane::TableData table;                                            // 最近一次的迭代表
        std::string summary;                                                      // 文本摘要
        UiOutputPane::PlotData plot;                                              // 最近一次的绘图数据
        std::vector<std::pair<std::string, UiOutputPane::TableData>> extraTables; // 额外的表格标签页（如迭代矩阵）
        bool has = false;
    };
    struct ExperimentState
    {
        std::vector<InputField> fields; // 最近一次输入
        std::string description;        // 说明文案
        ResultSnapshot last;            // 最近结果
        int presetIndex = 0;            // 预设函数索引

        // 线性方程组相关
        calc::Matrix matrixA;        // 系数矩阵
        std::vector<double> vectorB; // 右端向量
        int matrixPresetIndex = 0;   // 矩阵预设索引

        // 插值法相关
        calc::Matrix valueTable;       // 函数值表（x, f(x), f'(x), ...）
        int valueTablePresetIndex = 0; // 函数值表预设索引
    };

    // 内部工具
    void bindUiCallbacks();
    void ensureDefaultsFor(const std::string &name);
    void fillDescriptionFor(const std::string &name);

    // 数值计算方法，实现于compute.cpp
    void computePlot(const std::string &name);
    void computeScan(const std::string &name);
    void computeBisection(const std::string &name);
    void computeNewton(const std::string &name);
    void computeAitken(const std::string &name);
    void computeNewtonDownhill(const std::string &name);
    void computeSecantSinglePoint(const std::string &name);
    void computeSecantDoublePoint(const std::string &name);
    void computeGaussElimination(const std::string &name);
    void computeCroutElimination(const std::string &name);
    void computeCholesky(const std::string &name);
    void computeThomas(const std::string &name);
    void computeColumnPivoting(const std::string &name);
    void computeFullPivoting(const std::string &name);
    void computeJacobi(const std::string &name);
    void computeGaussSeidel(const std::string &name);
    void computeSOR(const std::string &name);
    void computeDividedDifference(const std::string &name);
    void computeNewtonDividedDiff(const std::string &name); 
    void computeNewtonEqualDiff(const std::string &name);   
    void computeLagrange(const std::string &name); 
    
    // 预设相关
    void ensurePresets();
    void cyclePresetFor(const std::string &name, int delta);

    // 矩阵预设相关，实现于MatrixPresets.cpp
    void ensureMatrixPresets();
    void showMatrixInputDialog(const std::string &expName, bool createNew);
    void cycleMatrixPresetFor(const std::string &name, int delta);
    void addMatrixPreset();

    // 函数值表预设相关，实现于ValueTablePresets.cpp
    void ensureValueTablePresets();
    void showValueTableInputDialog(const std::string &expName, bool createNew);
    void cycleValueTablePresetFor(const std::string &name, int delta);
    void addValueTablePreset();

    // 解析与格式化
    static double toDouble(const std::string &s, double defv);
    static int toInt(const std::string &s, int defv);
    static std::string fmt(double v, int prec = 8);

private:
    UI &ui_;
    std::unordered_map<std::string, ExperimentState> states_;
    struct Preset
    {
        std::string name;
        std::function<double(double)> f;
        std::function<double(double)> df; // 若无导数，对应 newton 不可用
        bool hasDf = true;
    };
    std::vector<Preset> presets_;

    // 矩阵预设
    struct MatrixPreset
    {
        std::string name;
        calc::Matrix A;
        std::vector<double> b;
    };
    std::vector<MatrixPreset> matrixPresets_;

    // 函数值表预设
    struct ValueTablePreset
    {
        std::string name;
        calc::Matrix table; // 列：x, f(x), f'(x), f''(x), ...
    };
    std::vector<ValueTablePreset> valueTablePresets_;
};

#endif // MANAGER_H

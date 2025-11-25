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
        UiOutputPane::TableData table; // 最近一次的迭代表
        std::string summary;           // 文本摘要
        UiOutputPane::PlotData plot;   // 最近一次的绘图数据
        bool has = false;
    };
    struct ExperimentState
    {
        std::vector<InputField> fields; // 最近一次输入
        std::string description;        // 说明文案
        ResultSnapshot last;            // 最近结果
        int presetIndex = 0;            // 预设函数索引
    };

    // 内部工具
    void bindUiCallbacks();
    void ensureDefaultsFor(const std::string &name);
    void fillDescriptionFor(const std::string &name);
    void computePlot(const std::string &name);
    void computeScan(const std::string &name);
    void computeBisection(const std::string &name);
    void computeNewton(const std::string &name);
    void ensurePresets();
    void cyclePresetFor(const std::string &name, int delta);

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
};

#endif // MANAGER_H
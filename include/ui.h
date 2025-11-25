#ifndef UI_H
#define UI_H

#include <string>
#include <vector>
#include <functional>

// 数值分析实验 UI 框架（PDCurses）

// 输入框结构
struct InputField
{
    std::string label;       // 标签文本，如 "初值 x0:"
    std::string value;       // 当前输入值
    std::string placeholder; // 占位符提示
    int maxLength = 50;      // 最大输入长度
};

// 实验树节点（章节/实验）
struct ExperimentNode
{
    std::string title;
    bool isChapter = false;
    bool expanded = false;
    std::vector<ExperimentNode> children; // 章节包含子节点；实验无子节点
};

// 输出区域 API：支持文本、表格、简单折线图（ASCII）。
class UiOutputPane
{
public:
    enum class TabType
    {
        Text,
        Table,
        Plot
    };
    struct TableData
    {
        std::vector<std::string> headers;
        std::vector<std::vector<std::string>> rows;
    };
    struct PlotData
    {
        std::vector<double> xs;
        std::vector<double> ys;
        std::string xlabel;
        std::string ylabel;
        double xmin = 0.0, xmax = 1.0; // x 轴范围，用于显示刻度
        double ymin = 0.0, ymax = 1.0; // y 轴范围
        double rootX = 0.0;            // 零点位置（用于特殊标记）
        bool hasRoot = false;          // 是否有零点
    };
    struct Tab
    {
        std::string title;
        TabType type = TabType::Text;
        bool collapsed = false;
        std::string text; // Text
        TableData table;  // Table
        PlotData plot;    // Plot
    };

    // 供外部调用的 API
    int addTextTab(const std::string &title, const std::string &text);
    int addTableTab(const std::string &title, const TableData &table);
    int addPlotTab(const std::string &title, const PlotData &plot);
    void clear();

    // 内部状态访问（仅 UI 内部调用）
    const std::vector<Tab> &tabs() const { return tabs_; }
    std::vector<Tab> &tabs() { return tabs_; }
    int selected() const { return selected_; }
    void setSelected(int idx);
    int tableScroll() const { return tableScroll_; }
    void setTableScroll(int val) { tableScroll_ = val; }

private:
    std::vector<Tab> tabs_;
    int selected_ = 0;
    int tableScroll_ = 0; // 表格滚动偏移（首行索引）
};

class UI
{
public:
    UI(int &status);
    ~UI();

    // 运行 UI 主循环
    void run();

    // 提供获取输出面板的接口，便于外部填充输出
    UiOutputPane &output();

    // 输入区访问接口（单输入框模式，向后兼容）
    std::string getInput() const;
    void setInput(const std::string &text);
    void clearInput();

    // 多输入框模式 API
    void addInputField(const std::string &label, const std::string &defaultValue = "", const std::string &placeholder = "");
    void setInputFields(const std::vector<InputField> &fields);
    std::vector<InputField> getInputFields() const;
    std::string getInputValue(int index) const;
    std::string getInputValue(const std::string &label) const;
    void clearInputFields();
    int getInputFieldCount() const;

    // 说明区访问接口
    std::string getDescription() const;
    void setDescription(const std::string &desc);

    // 获取当前实验名称
    std::string getCurrentExperiment() const;

    // 实验切换回调（当用户选择实验时触发）
    using ExperimentCallback = std::function<void(const std::string &expName)>;
    void onExperimentChanged(ExperimentCallback cb);

    // 输入确认回调（当用户在输入区按回车时触发）
    using InputCallback = std::function<void(const std::string &input)>;
    void onInputSubmit(InputCallback cb);

    // 预设切换回调（当焦点在说明区，用户左右键或点击标题上的箭头时触发）
    using PresetChangeCallback = std::function<void(int delta)>; // -1: 上一个, +1: 下一个
    void onPresetChange(PresetChangeCallback cb);

    // 添加预设回调（当焦点在说明区按 'a' 时触发）
    using AddPresetCallback = std::function<void()>;
    void onAddPreset(AddPresetCallback cb);

    // 矩阵输入回调（当焦点在说明区按 'm' 时触发）
    using MatrixInputCallback = std::function<void()>;
    void onMatrixInput(MatrixInputCallback cb);

    // 矩阵预设切换回调（当焦点在说明区，用户按左右键切换矩阵预设时触发）
    using MatrixPresetCallback = std::function<void(int delta)>;
    void onMatrixPresetChange(MatrixPresetCallback cb);

    // 静态单例指针（便于在其他模块中通过 UI::instance() 访问输出 API）
    static UI *instance();

private:
    void initCurses();
    void endCurses();

    // 画面与事件
    void loopMainMenu();
    void loopExperiment();

    // 数据初始化
    void buildExperimentTree();
    void populateSampleContent();

    // 帮助渲染
    void drawKeyHints(const std::string &context);

private:
    int &statusRef_;
    bool running_ = true;

    // 屏幕状态
    enum class Screen
    {
        MainMenu,
        Experiment
    };
    Screen screen_ = Screen::MainMenu;

    // 实验树
    std::vector<ExperimentNode> tree_;
    bool showTree_ = true; // 右上角实验列表是否展开
    int treeScroll_ = 0;   // 列表滚动偏移
    int treeCursor_ = 0;   // 当前选中项（可用于切换实验）

    // 输出面板
    UiOutputPane output_;

    // 输入内容（单输入框模式）
    std::string inputBuffer_;

    // 多输入框模式
    std::vector<InputField> inputFields_;
    int currentInputIndex_ = 0; // 当前焦点的输入框索引
    int inputScroll_ = 0;       // 多输入滚动偏移（首个可见输入索引）

    // 说明内容
    std::string descriptionText_;

    // 当前实验名称
    std::string currentExperiment_;

    // 实验切换回调
    ExperimentCallback expCallback_;

    // 输入确认回调
    InputCallback inputCallback_;

    // 预设切换回调
    PresetChangeCallback presetCallback_;

    // 添加预设回调
    AddPresetCallback addPresetCallback_;

    // 矩阵输入回调
    MatrixInputCallback matrixInputCallback_;

    // 矩阵预设切换回调
    MatrixPresetCallback matrixPresetCallback_;

    // 单例
    static UI *self_;
};

#endif // UI_H
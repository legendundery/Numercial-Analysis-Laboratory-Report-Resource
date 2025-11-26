# UI API 使用文档

## 概述
本 UI 框架基于 PDCurses，采用 **UI → Manager → Calc** 三层架构：

- **UI 层** (`ui.cpp/ui.h`)：负责界面渲染、用户交互、焦点管理
- **Manager 层** (`manager.cpp/manager.h`)：负责实验管理、数据保存、预设系统、结果展示
- **Calc 层** (`calc.cpp/calc.h`)：纯算法实现，无 UI 依赖

本文档主要介绍 **UI 层** 的直接 API。对于完整的应用开发，建议参考 Manager 层的实现模式。

---

## 1. 推荐使用模式：Manager 架构

### 1.1 为什么使用 Manager？

直接使用 UI API 虽然可行，但对于复杂的数值分析应用，**推荐使用 Manager 模式**：

```cpp
int main() {
    int status = 1;
    UI ui(status);
    Manager manager(ui);  // Manager 自动绑定所有回调
    
    ui.run();
    return 0;
}
```

**Manager 提供的功能：**
- ✅ 自动管理实验切换和状态保存
- ✅ 预设系统（函数预设、矩阵预设）
- ✅ 输入字段自动配置
- ✅ 结果快照和恢复（切换实验时不丢失数据）
- ✅ 统一的错误处理
- ✅ 多标签页输出管理

### 1.2 Manager 的工作流程

```
用户选择实验
    ↓
manager.initExperiment()  → 设置说明文案
    ↓                      → 配置输入字段
manager.useExperiment()   → 恢复上次结果（如果有）
    ↓
用户输入参数并按回车
    ↓
manager.saveExperiment()  → 保存当前输入
    ↓
manager.computeExperiment() → 调用 calc 层算法
    ↓                        → 生成多标签页输出
    ↓                        → 保存结果快照
显示结果
```

### 1.3 扩展 Manager

添加新实验只需在 Manager 中实现对应的 `computeXXX` 方法：

```cpp
void Manager::computeMyMethod(const std::string &name) {
    auto &st = states_[name];
    
    // 1. 读取输入参数
    double param1 = toDouble(ui_.getInputValue(0), 1.0);
    double param2 = toDouble(ui_.getInputValue(1), 0.001);
    
    // 2. 调用 calc 层算法
    auto result = calc::myAlgorithm(param1, param2);
    
    // 3. 构建输出
    std::ostringstream oss;
    oss << "方法：我的算法\n";
    oss << "参数1 = " << param1 << "\n";
    if (result.success)
        oss << "结果 = " << result.value << "\n";
    else
        oss << "失败：" << result.errorMsg << "\n";
    
    UiOutputPane::TableData tbl;
    // ... 填充表格数据
    
    // 4. 保存结果快照
    st.last.summary = oss.str();
    st.last.table = std::move(tbl);
    st.last.has = true;
    
    // 5. 显示输出
    ui_.output().clear();
    ui_.output().addTextTab("摘要", st.last.summary);
    ui_.output().addTableTab("详细数据", st.last.table);
}
```

---

## 2. 直接使用 UI API（不推荐用于复杂应用）

如果你需要直接使用 UI API（例如快速原型或简单工具），以下是完整的 API 说明。

### 2.1 获取 UI 实例

```cpp
// 通过单例访问 UI
UI* ui = UI::instance();
```

---

## 3. 输出区域 API

### 3.1 获取输出面板

```cpp
UiOutputPane& out = UI::instance()->output();
```

### 3.2 添加文本输出

```cpp
// 添加文本标签页
int tabIndex = out.addTextTab("结果", "计算完成！\n迭代次数: 10\n误差: 0.001");
```

**参数：**
- `title`: 标签页标题
- `text`: 多行文本内容（支持 `\n` 换行）

**返回：** 标签页索引（从 0 开始）

---

### 3.3 添加表格输出

```cpp
// 构造表格数据
UiOutputPane::TableData table;
table.headers = {"迭代次数", "x_k", "误差"};
table.rows = {
    {"1", "1.5000", "0.5000"},
    {"2", "1.4167", "0.0833"},
    {"3", "1.4142", "0.0025"}
};

// 添加表格标签页
int tabIndex = out.addTableTab("迭代表", table);
```

**TableData 结构：**
- `headers`: 表头字符串数组
- `rows`: 二维字符串数组，每行对应一条记录

**注意：** 表格会自动调整列宽以适应屏幕

---

### 3.4 添加折线图输出（ASCII）

```cpp
// 构造曲线数据
UiOutputPane::PlotData plot;
for (int i = 0; i <= 50; ++i) {
    double x = i * 0.1;
    plot.xs.push_back(x);
    plot.ys.push_back(sin(x));
}
plot.xlabel = "x";
plot.ylabel = "sin(x)";

// 添加曲线标签页
int tabIndex = out.addPlotTab("收敛曲线", plot);
```

**PlotData 结构：**
- `xs`: X 轴数据点（double 数组）
- `ys`: Y 轴数据点（double 数组，需与 xs 等长）
- `xlabel`: X 轴标签
- `ylabel`: Y 轴标签

**注意：** 图形会自动缩放以适应显示区域

---

### 3.5 清空所有输出

```cpp
out.clear();  // 移除所有标签页
```

---

### 3.6 切换当前显示的标签页

```cpp
out.setSelected(0);  // 切换到第一个标签页
```

---

### 3.7 访问标签页数据（高级用法）

**注意**：在 Manager 架构中，标签页通过 `ResultSnapshot` 管理，支持自动恢复。

```cpp
// 获取所有标签页（只读）
const std::vector<UiOutputPane::Tab>& allTabs = out.tabs();

// 获取所有标签页（可修改）
std::vector<UiOutputPane::Tab>& allTabs = out.tabs();

// 获取当前选中的标签页索引
int currentTab = out.selected();

// 访问特定标签页内容
if (!out.tabs().empty()) {
    const auto& tab = out.tabs()[0];
    
    // 根据类型访问数据
    if (tab.type == UiOutputPane::TabType::Text) {
        std::string content = tab.text;
    } else if (tab.type == UiOutputPane::TabType::Table) {
        auto headers = tab.table.headers;
        auto rows = tab.table.rows;
    } else if (tab.type == UiOutputPane::TabType::Plot) {
        auto xs = tab.plot.xs;
        auto ys = tab.plot.ys;
    }
}
```

---

## 4. 输入区域 API

### 当前实现
输入区提供完整的读写接口和确认回调。

### API 方法

```cpp
// 获取输入内容
std::string input = UI::instance()->getInput();

// 设置输入内容
UI::instance()->setInput("1.5");

// 清空输入
UI::instance()->clearInput();

// 注册输入确认回调（用户在输入区按回车时触发）
UI::instance()->onInputSubmit([](const std::string& input) {
    // 处理用户输入
    std::cout << "用户输入: " << input << std::endl;
});
```

---

## 5. 当前实验跟踪

### 获取当前实验名称

```cpp
std::string expName = UI::instance()->getCurrentExperiment();
```

### 实验切换回调

当用户在实验列表中选择实验时自动触发：

```cpp
UI::instance()->onExperimentChanged([](const std::string& expName) {
    std::cout << "切换到实验: " << expName << std::endl;
    // 根据实验名称执行对应逻辑
});
```

---

## 6. 回调触发时机说明（Manager 自动处理）

**注意**：在 Manager 架构中，这些回调已经自动绑定。只有在不使用 Manager 时才需要手动注册。

### 6.1 实验切换回调 `onExperimentChanged`

**触发条件：**
- 用户在实验列表中点击实验项（非章节）
- 用户在实验列表中按回车选择实验项

**触发时机：** 在实验切换后、示例内容填充前

**用法：**
```cpp
ui.onExperimentChanged([](const std::string& expName) {
    UI* ui = UI::instance();
    
    // 根据实验名称设置说明和默认输入
    if (expName.find("牛顿") != std::string::npos) {
        ui->setDescription("牛顿迭代法\n输入初值 x0");
        ui->setInput("1.5");
        ui->clearInput(); // 可选：清空让用户输入
    }
});
```

### 6.2 输入确认回调 `onInputSubmit`

**触发条件：**
- 用户在输入区（焦点在输入区域）按回车键

**触发时机：** 在回车按下后立即触发

**用法：**
```cpp
ui.onInputSubmit([](const std::string& input) {
    UI* ui = UI::instance();
    std::string expName = ui->getCurrentExperiment();
    
    // 根据当前实验和输入执行计算
    if (expName.find("牛顿") != std::string::npos) {
        double x0 = std::stod(input);
        // 执行牛顿迭代...
        // 输出结果到 ui->output()
    }
});
```

---

## 7. 完整使用示例

### 7.1 推荐方式：使用 Manager（参考 main.cpp）

```cpp
int main() {
    int status = 1;
    UI ui(status);
    Manager manager(ui);  // 所有逻辑由 Manager 处理
    ui.run();
    return 0;
}
```

### 7.2 手动注册回调（不使用 Manager）

```cpp
int main()
{
    int status = 1;
    UI ui(status);
    
    // 注册输入确认回调 - 统一处理所有实验的输入
    ui.onInputSubmit([](const std::string& input) {
        UI* ui = UI::instance();
        std::string expName = ui->getCurrentExperiment();
        
        // 根据当前实验分发处理
        if (expName.find("牛顿") != std::string::npos) {
            processNewtonMethod(input);
        } else if (expName.find("对分") != std::string::npos) {
            processBisectionMethod(input);
        } else {
            ui->output().clear();
            ui->output().addTextTab("提示", "请先选择实验");
        }
    });
    
    // 注册实验切换回调 - 为每个实验设置初始状态
    ui.onExperimentChanged([](const std::string& expName) {
        UI* ui = UI::instance();
        
        if (expName.find("牛顿") != std::string::npos) {
            ui->setDescription(
                "牛顿迭代法求解 f(x) = x² - 2 = 0\n"
                "请在输入区输入初值 x0，按回车开始计算");
            ui->setInput("1.5");
            ui->output().clear();
        } else if (expName.find("对分") != std::string::npos) {
            ui->setDescription(
                "二分法求解方程\n"
                "输入格式：a,b（区间端点）");
            ui->setInput("1.0,2.0");
            ui->output().clear();
        }
    });
    
    ui.run();
    return 0;
}
```

### 7.3 示例：牛顿迭代法（完整流程）

```cpp
#include "ui.h"
#include <cmath>

void runNewtonMethod() {
    UI* ui = UI::instance();
    UiOutputPane& out = ui->output();
    
    // 清空之前的输出
    out.clear();
    
    // 添加说明
    out.addTextTab("说明", 
        "牛顿迭代法求解 f(x) = x² - 2 = 0\n"
        "初值: x0 = 1.5\n"
        "迭代公式: x_{n+1} = x_n - f(x_n)/f'(x_n)");
    
    // 构造迭代表
    UiOutputPane::TableData table;
    table.headers = {"n", "x_n", "|x_n - x_{n-1}|", "|f(x_n)|"};
    
    double x = 1.5;
    for (int i = 0; i < 10; ++i) {
        double fx = x * x - 2.0;
        double fpx = 2.0 * x;
        double xnew = x - fx / fpx;
        double error = std::abs(xnew - x);
        
        table.rows.push_back({
            std::to_string(i),
            std::to_string(x),
            std::to_string(error),
            std::to_string(std::abs(fx))
        });
        
        x = xnew;
        if (error < 1e-6) break;
    }
    out.addTableTab("迭代表", table);
    
    // 绘制收敛曲线
    UiOutputPane::PlotData plot;
    x = 1.5;
    for (int i = 0; i < 10; ++i) {
        plot.xs.push_back(i);
        plot.ys.push_back(std::abs(x * x - 2.0));
        
        double fx = x * x - 2.0;
        double fpx = 2.0 * x;
        x = x - fx / fpx;
    }
    plot.xlabel = "迭代次数";
    plot.ylabel = "|f(x)|";
    out.addPlotTab("收敛曲线", plot);
}
```

### 7.4 示例：读取输入并计算

```cpp
void processUserInput() {
    UI* ui = UI::instance();
    
    // 获取用户输入
    std::string input = ui->getInput();
    
    // 解析输入（示例：读取一个数字）
    double x0 = 1.0;
    try {
        x0 = std::stod(input);
    } catch (...) {
        ui->output().clear();
        ui->output().addTextTab("错误", "输入格式错误！请输入数字。");
        return;
    }
    
    // 使用输入进行计算...
    UiOutputPane& out = ui->output();
    out.clear();
    out.addTextTab("结果", "初值 x0 = " + std::to_string(x0));
}
```

---

## 8. 线程安全说明

**注意：** 当前实现不是线程安全的。如果需要在后台线程中更新 UI，请确保：
1. 使用互斥锁保护 UI 访问
2. 或在主线程中通过消息队列更新 UI

---

## 9. 最佳实践

1. **输出更新时机**：在用户按下回车确认输入后，清空旧输出并添加新结果
2. **标签页数量**：建议不超过 5-6 个，避免标签栏过于拥挤
3. **表格数据**：行数过多时仅显示关键行，或添加分页功能
4. **图形数据**：数据点建议 20-100 个，过多会影响 ASCII 图形可读性
5. **错误处理**：计算出错时使用文本标签页显示错误信息

---

## 10. 架构参考

### Manager 层核心结构

```cpp
class Manager {
private:
    struct ResultSnapshot {
        UiOutputPane::TableData table;
        std::string summary;
        UiOutputPane::PlotData plot;
        std::vector<std::pair<std::string, UiOutputPane::TableData>> extraTables;
        bool has = false;
    };
    
    struct ExperimentState {
        std::vector<InputField> fields;
        std::string description;
        ResultSnapshot last;
        int presetIndex = 0;
        calc::Matrix matrixA;        // 第三、四章使用
        std::vector<double> vectorB; // 第三、四章使用
        int matrixPresetIndex = 0;
    };
    
    UI &ui_;
    std::unordered_map<std::string, ExperimentState> states_;
};
```

### 实现新算法的步骤

1. 在 `calc.cpp` 中实现纯算法函数
2. 在 `manager.h` 中声明 `computeXXX` 方法
3. 在 `manager.cpp` 中实现计算逻辑
4. 在 `computeExperiment` 中添加分发逻辑
5. 在 `fillDescriptionFor` 中添加说明文案
6. （可选）在 `ensureDefaultsFor` 中设置输入字段

### 参考实现

- **第二章**：`computeNewton`, `computeBisection` 等
- **第三章**：`computeGaussElimination`, `computeCholesky` 等
- **第四章**：`computeJacobi`, `computeGaussSeidel`, `computeSOR`

完整代码参见 `src/manager.cpp`。

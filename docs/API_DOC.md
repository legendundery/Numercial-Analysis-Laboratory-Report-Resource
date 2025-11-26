# API 参考文档

> 数值分析实验系统 - 完整API参考

## 📑 目录

- [UI层API](#ui层api) - 界面渲染、用户交互
- [Manager层API](#manager层api) - 状态管理、业务逻辑
- [Calc层API](#calc层api) - 纯算法实现
- [开发指南](#开发指南) - 添加新算法的步骤

---

## 架构概览

本系统采用 **三层架构**，职责清晰分离：

```
┌─────────────────────────────────────────────┐
│  UI 层 (ui.cpp/ui.h)                        │
│  • PDCurses终端界面                         │
│  • 用户输入/输出管理                         │
│  • 多标签页显示                              │
└────────────┬────────────────────────────────┘
             │ 回调事件
             ↓
┌─────────────────────────────────────────────┐
│  Manager 层 (manager/compute.cpp)           │
│  • 实验状态管理                              │
│  • 预设系统（函数/矩阵/函数值表）             │
│  • 结果组装与展示                            │
└────────────┬────────────────────────────────┘
             │ 调用算法
             ↓
┌─────────────────────────────────────────────┐
│  Calc 层 (calc.cpp/calc.h)                  │
│  • 纯数值算法实现                            │
│  • 无UI依赖，可独立测试                       │
└─────────────────────────────────────────────┘
```

---

## UI层API

### 核心类

#### `UI` - 主界面管理类

**构造与运行：**

```cpp
int main() {
    int status = 1;
    UI ui(status);
    Manager manager(ui);  // 推荐使用Manager管理业务逻辑
    ui.run();  // 启动主循环
    return 0;
}
```

**单例访问：**

```cpp
UI* ui = UI::instance();  // 在回调中访问UI实例
```

---

### 输入区域API

#### 多输入框模式（推荐）

```cpp
// 清空所有输入框
ui.clearInputFields();

// 添加输入框
ui.addInputField("初值 x0:", "1.5", "请输入初始值");
ui.addInputField("容差:", "1e-6");

// 获取输入值
std::string value0 = ui.getInputValue(0);  // 第一个输入框
std::string value1 = ui.getInputValue(1);  // 第二个输入框
```

#### 单输入框模式（向后兼容）

```cpp
std::string input = ui.getInput();    // 获取输入
ui.setInput("1.5");                   // 设置输入
ui.clearInput();                      // 清空输入
```

---

### 输出区域API

#### `UiOutputPane` - 输出面板类

**获取输出面板：**

```cpp
UiOutputPane& out = ui.output();
```

#### 1. 文本标签页

```cpp
// 添加文本输出
int tabIndex = out.addTextTab("摘要", "计算完成！\n迭代次数: 10\n误差: 0.001");

// 支持多行文本
std::ostringstream oss;
oss << "方法：牛顿迭代法\n";
oss << "初值：x0 = 1.5\n";
oss << "结果：x = 1.414214\n";
out.addTextTab("结果", oss.str());
```

**特性：**
- ✅ 支持上下键滚动查看长文本
- ✅ 自动换行
- ✅ 底部显示滚动提示

#### 2. 表格标签页

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

**特性：**
- ✅ 支持上下键滚动
- ✅ 自动列宽调整
- ✅ 表头固定显示

#### 3. 图像标签页（ASCII曲线）

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

// 添加图像标签页
int tabIndex = out.addPlotTab("函数图像", plot);
```

**特性：**
- ✅ 80×20字符画布
- ✅ 自动坐标轴缩放
- ✅ 零点标记

#### 清空输出

```cpp
out.clear();  // 移除所有标签页
```

---

### 说明区域API

```cpp
// 设置说明文本
ui.setDescription(
    "牛顿迭代法求解 f(x) = x² - 2 = 0\n"
    "请在输入区输入初值 x0，按回车开始计算"
);
```

---

### 回调系统

#### 实验切换回调

```cpp
ui.onExperimentChanged([](const std::string& expName) {
    UI* ui = UI::instance();
    
    // 设置输入字段
    ui->clearInputFields();
    ui->addInputField("初值 x0:", "1.5");
    
    // 设置说明
    ui->setDescription("牛顿迭代法\n输入初值后按回车计算");
    
    // 清空旧输出
    ui->output().clear();
});
```

**触发时机：** 用户在实验列表中选择实验项

#### 输入确认回调

```cpp
ui.onInputSubmit([](const std::string& /*unused*/) {
    UI* ui = UI::instance();
    std::string expName = ui->getCurrentExperiment();
    
    // 获取输入
    double x0 = std::stod(ui->getInputValue(0));
    double tol = std::stod(ui->getInputValue(1));
    
    // 执行计算...
    // 输出结果到 ui->output()
});
```

**触发时机：** 用户在输入区按回车键

#### 预设切换回调

```cpp
ui.onPresetChange([](int delta) {
    UI* ui = UI::instance();
    std::string expName = ui->getCurrentExperiment();
    
    // delta = 1 表示下一个预设
    // delta = -1 表示上一个预设
    
    // 更新输入字段和说明...
});
```

**触发时机：** 用户按←→键或n键

---

## Manager层API

### 核心架构

Manager层负责：
- ✅ 实验状态管理（输入缓存、结果快照）
- ✅ 预设系统（函数/矩阵/函数值表）
- ✅ 业务逻辑调度
- ✅ 结果组装与展示

### ExperimentState 结构

```cpp
struct ExperimentState {
    std::vector<InputField> fields;      // 输入字段配置
    std::string description;             // 说明文案
    ResultSnapshot last;                 // 最近结果快照
    int presetIndex;                     // 函数预设索引
    int matrixPresetIndex;               // 矩阵预设索引
    int valueTablePresetIndex;           // 函数值表预设索引
    calc::Matrix matrixA;                // 矩阵数据
    std::vector<double> vectorB;         // 向量数据
    calc::Matrix valueTable;             // 函数值表数据
};
```

### ResultSnapshot 结构

```cpp
struct ResultSnapshot {
    std::string summary;                 // 摘要文本
    UiOutputPane::TableData table;       // 表格数据
    UiOutputPane::PlotData plot;         // 图像数据
    std::vector<std::pair<std::string, UiOutputPane::TableData>> extraTables;
    bool has = false;                    // 是否有结果
};
```

### 核心方法

#### 实验管理

```cpp
void initExperiment(const std::string &name);    // 初始化实验配置
void saveExperiment(const std::string &name);    // 保存当前输入
void loadExperiment(const std::string &name);    // 加载输入状态
void useExperiment(const std::string &name);     // 恢复输出结果
```

#### 计算调度

```cpp
void computeExperiment(const std::string &name); // 根据实验名分发计算
```

**已实现的计算方法（23个）：**

```cpp
// 第二章 方程求根（8个）
void computePlot(const std::string &name);
void computeScan(const std::string &name);
void computeBisection(const std::string &name);
void computeNewton(const std::string &name);
void computeAitken(const std::string &name);
void computeNewtonDownhill(const std::string &name);
void computeSecantSinglePoint(const std::string &name);
void computeSecantDoublePoint(const std::string &name);

// 第三章 线性方程组-直接法（6个）
void computeGaussElimination(const std::string &name);
void computeCroutElimination(const std::string &name);
void computeCholesky(const std::string &name);
void computeThomas(const std::string &name);
void computeColumnPivoting(const std::string &name);
void computeFullPivoting(const std::string &name);

// 第四章 线性方程组-迭代法（3个）
void computeJacobi(const std::string &name);
void computeGaussSeidel(const std::string &name);
void computeSOR(const std::string &name);

// 第五章 插值法（6个）
void computeDividedDifference(const std::string &name);
void computeNewtonDividedDiff(const std::string &name);
void computeNewtonEqualDiff(const std::string &name);
void computeLagrange(const std::string &name);
void computeInverseInterpolation(const std::string &name);
void computeHermite(const std::string &name);
```

#### 配置方法

```cpp
void ensureDefaultsFor(const std::string &name);     // 设置默认输入字段
void fillDescriptionFor(const std::string &name);    // 设置说明文案
```

### 预设系统

#### 函数预设

```cpp
struct FunctionPreset {
    std::string name;
    std::function<double(double)> f;      // 函数
    std::function<double(double)> df;     // 导数
    std::function<double(double)> d2f;    // 二阶导数
    double a, b;                          // 区间
    double root;                          // 真实根
};

void cyclePresetFor(const std::string &name, int delta);
```

#### 矩阵预设

```cpp
void cycleMatrixPresetFor(const std::string &name, int delta);
```

#### 函数值表预设

```cpp
void cycleValueTablePresetFor(const std::string &name, int delta);
```

---

## Calc层API

### 设计原则

- ✅ 纯数学算法实现
- ✅ 无UI依赖，可独立测试
- ✅ 返回结构化结果
- ✅ 包含中间步骤（用于教学展示）

### 方程求根

#### 迭代结果结构

```cpp
struct Iteration {
    int n;              // 迭代次数
    double x;           // 当前值
    double fx;          // f(x)
    double error;       // 误差
};
```

#### 方法

```cpp
namespace calc {
    std::vector<Iteration> bisection(f, a, b, maxIter, tol);
    std::vector<Iteration> newton(f, df, x0, maxIter, tol);
    std::vector<Iteration> aitken(f, x0, maxIter, tol);
    std::vector<Iteration> newtonDownhill(f, df, x0, maxIter, tol, lambda);
    std::vector<Iteration> secantSinglePoint(f, x0, x1, maxIter, tol);
    std::vector<Iteration> secantDoublePoint(f, x0, x1, maxIter, tol);
}
```

### 线性方程组

#### 高斯消元结果

```cpp
struct GaussResult {
    bool success;
    std::vector<double> solution;           // 解向量
    std::vector<Matrix> steps;              // 中间步骤矩阵
    std::vector<std::string> stepDesc;      // 步骤描述
    Matrix L, U;                            // LU分解结果
    std::string errorMsg;
};
```

#### 迭代法结果

```cpp
struct IterativeResult {
    bool success;
    std::vector<double> solution;           // 解向量
    std::vector<std::vector<double>> iterations;  // 迭代过程
    std::vector<double> errors;             // 每步误差
    Matrix iterationMatrix;                 // 迭代矩阵B
    double spectralRadius;                  // 谱半径
    std::string errorMsg;
};
```

#### 方法

```cpp
namespace calc {
    // 直接法
    GaussResult gaussElimination(A, b);
    GaussResult croutElimination(A, b);
    GaussResult choleskySolve(A, b);
    GaussResult thomasTridiagonal(A, b);
    GaussResult columnPivoting(A, b);
    GaussResult fullPivoting(A, b);
    
    // 迭代法
    IterativeResult jacobiIteration(A, b, x0, maxIter, tol);
    IterativeResult gaussSeidelIteration(A, b, x0, maxIter, tol);
    IterativeResult sorIteration(A, b, x0, maxIter, tol, omega);
}
```

### 插值法

#### 插值结果

```cpp
struct InterpolationResult {
    bool success;
    double value;                           // 插值结果
    std::vector<std::vector<double>> table; // 差商表/差分表
    std::vector<double> coefficients;       // 多项式系数
    std::string polynomial;                 // 多项式表达式
    std::vector<std::string> stepDesc;      // 计算步骤
    std::string method;                     // 使用的方法
    std::string errorMsg;
};
```

#### 方法

```cpp
namespace calc {
    // 基本插值
    InterpolationResult newtonDividedDifference(x, y, xVal);
    InterpolationResult newtonForwardDifference(x, y, xVal);
    InterpolationResult newtonBackwardDifference(x, y, xVal);
    InterpolationResult lagrangeInterpolation(x, y, xVal);
    
    // 反插值
    InterpolationResult inverseInterpolationBySwap(x, y, yVal);
    InterpolationResult inverseInterpolationByIteration(x, y, yVal, x0, maxIter, tol);
    
    // 埃尔米特插值
    InterpolationResult hermiteInterpolation(x, y, dy, xVal);
}
```

### 矩阵工具

```cpp
namespace calc {
    // 矩阵范数
    double matrixNorm1(A);
    double matrixNorm2(A);
    double matrixNormInf(A);
    double matrixNormF(A);
    
    // 向量范数
    double vectorNorm2(v);
    double vectorNormInf(v);
    
    // 矩阵运算
    Matrix matrixMultiply(A, B);
    std::vector<double> matrixVectorMultiply(A, x);
    
    // 收敛性分析
    bool isStrictlyDiagonallyDominant(A);
    double spectralRadius(A, maxIter, tol);
    double jacobiSpectralRadius(A);
    double optimalOmegaSOR(A);
}
```

---

## 开发指南

### 添加新算法

#### 步骤1：在Calc层实现算法

**文件：** `calc.h` + `calc.cpp`

```cpp
// calc.h - 声明
namespace calc {
    struct NewMethodResult {
        bool success;
        double value;
        std::string errorMsg;
    };
    
    NewMethodResult newMethod(double param1, double param2);
}

// calc.cpp - 实现
NewMethodResult calc::newMethod(double param1, double param2) {
    NewMethodResult result;
    // 纯数学计算...
    return result;
}
```

#### 步骤2：在Manager层添加计算方法

**文件：** `manager.h` + `compute.cpp`

```cpp
// manager.h - 声明
class Manager {
    void computeNewMethod(const std::string &name);
};

// compute.cpp - 实现
void Manager::computeNewMethod(const std::string &name) {
    auto &st = states_[name];
    
    // 1. 获取输入
    double param1 = toDouble(ui_.getInputValue(0), 1.0);
    
    // 2. 调用算法
    auto result = calc::newMethod(param1, param2);
    
    // 3. 组装输出
    std::ostringstream summary;
    summary << "方法：新方法\n";
    summary << "结果：" << result.value << "\n";
    
    // 4. 显示
    ui_.output().clear();
    ui_.output().addTextTab("摘要", summary.str());
    
    // 5. 保存快照
    st.last.summary = summary.str();
    st.last.has = true;
}
```

#### 步骤3：在Manager层配置实验

**文件：** `manager.cpp`

```cpp
// 在 computeExperiment 中添加分发
void Manager::computeExperiment(const std::string &name) {
    if (name.find("新方法") != string::npos) {
        computeNewMethod(name);
        return;
    }
    // ...
}

// 在 ensureDefaultsFor 中设置输入字段
void Manager::ensureDefaultsFor(const std::string &name) {
    if (name.find("新方法") != string::npos) {
        ui_.clearInputFields();
        ui_.addInputField("参数1:", "1.0");
        ui_.addInputField("参数2:", "0.001");
    }
}

// 在 fillDescriptionFor 中添加说明
void Manager::fillDescriptionFor(const std::string &name) {
    if (name.find("新方法") != string::npos) {
        oss << "新方法说明：\n";
        oss << "用途：...\n";
    }
}
```

#### 步骤4：在UI层添加实验项

**文件：** `ui.cpp` (在 `buildExperimentTree()`)

```cpp
void UI::buildExperimentTree() {
    // ...
    ExperimentNode ch6;
    ch6.title = "第六章 新章节";
    ch6.isChapter = true;
    ch6.children.push_back({"新方法", false, false, {}});
    tree_.push_back(ch6);
}
```

### 完整示例：牛顿迭代法

参考代码位置：
- **Calc层:** `calc.cpp` - `newton()` 函数
- **Manager层:** `compute.cpp` - `computeNewton()` 函数
- **配置:** `manager.cpp` - `ensureDefaultsFor()` 和 `fillDescriptionFor()`

---

---

## 常见问题

### Q: 如何添加新的输入框？

在 `ensureDefaultsFor()` 中：

```cpp
ui_.clearInputFields();
ui_.addInputField("标签:", "默认值", "占位符");
```

### Q: 如何实现多标签页输出？

依次调用：

```cpp
ui_.output().clear();
ui_.output().addTextTab("摘要", summaryText);
ui_.output().addTableTab("迭代表", tableData);
ui_.output().addPlotTab("图像", plotData);
```

### Q: 如何添加预设？

在对应的 `*Presets.cpp` 文件中添加预设数据，并在 `cycle*PresetFor()` 中配置。

### Q: 如何实现滚动查看长内容？

- **表格类标签页**：自动支持上下键滚动
- **文本类标签页**：自动支持上下键滚动（2025-11-26更新）

### Q: 文本输出中如何避免乱码？

使用纯ASCII字符，避免Unicode符号（如×·Σ等）。推荐替代：
- `×` → `*`
- `·` → 删除或空格
- `Σ` → `SUM`
- 下标 `₀₁₂` → `012`

---

## 最佳实践

### 代码组织

1. **Calc层**：只包含纯数学计算，返回结构化结果
2. **Manager层**：负责输入解析、结果格式化、输出组装
3. **UI层**：只处理界面渲染和事件响应

### 输出设计

1. **摘要标签页**：包含方法名、参数、关键结果
2. **表格标签页**：详细数据（如迭代表、差商表）
3. **图像标签页**：可视化曲线（可选）
4. **步骤标签页**：计算过程（教学用）

### 错误处理

```cpp
if (!result.success) {
    ui_.output().clear();
    ui_.output().addTextTab("错误", result.errorMsg);
    return;
}
```



---

## 参考资料

- **[架构文档](ARCHITECTURE.md)** - 详细的系统架构设计和模块详解
- **[快速参考](QUICK_REFERENCE.md)** - 常用操作和键盘快捷键
- **[README](README.md)** - 项目概览和快速开始
- **源代码** - `include/` 和 `src/` 目录

---

**最后更新：** 2025年11月26日

**维护者：** legendundery

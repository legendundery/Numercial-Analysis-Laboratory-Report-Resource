# 数值分析实验系统 - 项目架构文档

## 📋 目录

- [项目概述](#项目概述)
- [技术栈](#技术栈)
- [项目结构](#项目结构)
- [架构设计](#架构设计)
- [模块详解](#模块详解)
- [数据流](#数据流)
- [编译系统](#编译系统)
- [扩展指南](#扩展指南)

---

## 项目概述

本项目是一个基于终端UI的数值分析实验系统，采用**三层架构**（UI → Manager → Calc）实现了多种数值计算方法。使用 C++20 和 PDCurses 库构建，提供交互式的数值计算体验。

**核心特性：**

- 模块化架构，职责清晰分离
- 支持方程求根、线性方程组、插值等多类算法
- 预设系统 + 自定义输入
- 多标签页输出（摘要、表格、图像）
- ASCII 可视化（函数曲线、迭代过程）

**代码统计：**

```
总计：~7,724 行 C++ 代码
├── compute.cpp       2,893 行  (计算调度与输出)
├── calc.cpp          1,943 行  (纯算法实现)
├── ui.cpp            1,145 行  (终端界面渲染)
├── manager.cpp       1,057 行  (业务逻辑管理)
├── ValueTablePresets.cpp 358 行 (函数值表预设)
├── MatrixPresets.cpp   314 行  (矩阵预设数据)
└── main.cpp             14 行  (程序入口)
```

---

## 技术栈

| 组件               | 技术          | 版本                   |
| ------------------ | ------------- | ---------------------- |
| **编程语言** | C++           | C++20                  |
| **编译器**   | Clang++       | 20.1.8 (MSYS2 MinGW64) |
| **UI 库**    | PDCurses      | -                      |
| **构建工具** | VS Code Tasks | -                      |
| **调试器**   | GDB           | 16.3                   |
| **平台**     | Windows       | (通过 PDCurses 跨平台) |

**依赖库：**

- `pdcurses` - 终端UI框架
- `gdi32` - Windows GDI（用于绘图支持）

---

## 项目结构

```
Numercial Analysis/
├── include/                 # 头文件目录
│   ├── ui.h                 # UI 层接口定义
│   ├── manager.h            # Manager 层接口定义
│   └── calc.h               # Calc 层算法接口定义
│
├── src/                     # 源文件目录
│   ├── main.cpp             # 程序入口（14 行）
│   ├── ui.cpp               # UI 层实现（1101 行）
│   ├── manager.cpp          # Manager 层核心逻辑（987 行）
│   ├── compute.cpp          # 计算方法实现（2359 行）
│   ├── calc.cpp             # 纯算法实现（1575 行）
│   ├── MatrixPresets.cpp    # 矩阵预设数据（314 行）
│   └── ValueTablePresets.cpp # 函数值表预设（327 行）
│
├── .vscode/                 # VS Code 配置
│   └── tasks.json           # 构建任务配置
│
├── docs/                    # 文档目录   
│   ├── README.md                # 项目说明
│   ├── ARCHITECTURE.md          # 本架构文档（你在这里）
│   ├── API_DOC.md               # API 文档
│
└── main.exe                 # 编译产物
```

---

## 架构设计

### 三层架构模式

本项目采用经典的**三层架构**，实现关注点分离：

```
┌─────────────────────────────────────────────────┐
│                   UI 层 (ui.cpp)                │
│  • PDCurses 终端界面渲染                         │
│  • 用户输入处理（键盘事件）                       │
│  • 实验列表、输入区、输出区管理                   │
│  • 多标签页显示（文本/表格/图像）                 │
└────────────────┬────────────────────────────────┘
                 │ 回调事件
                 ↓
┌─────────────────────────────────────────────────┐
│              Manager 层 (manager.cpp)           │
│  • 实验状态管理（输入/输出缓存）                  │
│  • 预设系统（函数/矩阵/函数值表）                 │
│  • 业务逻辑调度                                  │
│  • 数据格式化与输出组装                           │
└────────────────┬────────────────────────────────┘
                 │ 调用算法
                 ↓
┌─────────────────────────────────────────────────┐
│               Calc 层 (calc.cpp)                │
│  • 纯数值算法实现（无 UI 依赖）                   │
│  • 方程求根、线性方程组、插值等                   │
│  • 返回结构化计算结果                            │
└─────────────────────────────────────────────────┘
```

**职责划分：**

| 层次                 | 文件                                          | 职责                         | 不包含             |
| -------------------- | --------------------------------------------- | ---------------------------- | ------------------ |
| **UI 层**      | `ui.cpp`/`ui.h`                           | 界面渲染、事件处理、用户交互 | 业务逻辑、算法实现 |
| **Manager 层** | `manager.cpp`/`manager.h`/`compute.cpp` | 状态管理、预设系统、数据组装 | UI 细节、算法细节  |
| **Calc 层**    | `calc.cpp`/`calc.h`                       | 纯算法实现、数学计算         | UI、状态管理       |

---

## 模块详解

### 1. UI 层 (`ui.cpp`/`ui.h`)

**核心类：** `UI`, `UiOutputPane`

**主要功能：**

- 终端界面初始化与渲染（PDCurses）
- 三区域布局：
  - **左侧实验列表** - 章节/实验树形结构
  - **右上输入区** - 多输入框 + 说明区
  - **右下输出区** - 多标签页（文本/表格/ASCII图像）
- 键盘事件处理：
  - `↑↓` - 切换焦点区域
  - `←→` - 切换标签页/预设
  - `Enter` - 提交计算
  - `n` - 切换预设
  - `a` - 添加预设
  - `m` - 矩阵输入

**回调机制：**

```cpp
ui.onExperimentChanged([](const std::string &exp) { /*...*/ });
ui.onInputSubmit([](const std::string &input) { /*...*/ });
ui.onPresetChange([](int delta) { /*...*/ });
```

**输出 API：**

```cpp
output.addTextTab("摘要", summaryText);
output.addTableTab("迭代表", tableData);
output.addPlotTab("图像", plotData);
```

### 2. Manager 层

#### 2.1 核心管理 (`manager.cpp`)

**核心类：** `Manager`

**主要功能：**

- 实验状态管理（`ExperimentState`）
  - 输入字段缓存
  - 输出结果快照
  - 预设索引
  - 矩阵/函数值表数据
- 回调绑定（连接 UI 与业务逻辑）
- 输入验证与默认值设置（`ensureDefaultsFor`）
- 说明文案生成（`fillDescriptionFor`）
- 预设切换逻辑（三种预设类型）

**状态结构：**

```cpp
struct ExperimentState {
    std::vector<InputField> fields;      // 输入字段
    std::string description;             // 说明文案
    ResultSnapshot last;                 // 最近结果
    int presetIndex;                     // 函数预设索引
    int matrixPresetIndex;               // 矩阵预设索引
    int valueTablePresetIndex;           // 函数值表预设索引
    calc::Matrix matrixA;                // 矩阵数据
    std::vector<double> vectorB;         // 向量数据
    calc::Matrix valueTable;             // 函数值表数据
};
```

#### 2.2 计算调度 (`compute.cpp`)

**主要功能：**

- 实现所有 `compute*` 方法（21个计算函数）
- 从 UI 获取输入 → 调用 Calc 算法 → 格式化输出
- ASCII 图像绘制（80×20 字符画布）
- 多标签页内容组装

**计算方法分类：**

| 类别                           | 方法数 | 典型函数                                                                                               |
| ------------------------------ | ------ | ------------------------------------------------------------------------------------------------------ |
| **方程求根**             | 8      | `computePlot`, `computeScan`, `computeBisection`, `computeNewton`, `computeAitken`           |
| **线性方程组（直接法）** | 6      | `computeGaussElimination`, `computeCholesky`, `computeThomas`, `computeColumnPivoting`         |
| **线性方程组（迭代法）** | 3      | `computeJacobi`, `computeGaussSeidel`, `computeSOR`                                              |
| **插值法**               | 6      | `computeNewtonDividedDiff`, `computeLagrange`, `computeInverseInterpolation`, `computeHermite` |

**典型计算流程：**

```cpp
void Manager::computeNewton(const std::string &name) {
    // 1. 获取输入
    double x0 = toDouble(ui_.getInputValue(0), 1.0);
    int maxIter = toInt(ui_.getInputValue(1), 50);
    double tol = toDouble(ui_.getInputValue(2), 1e-6);
  
    // 2. 调用 Calc 算法
    auto result = calc::newton(preset.f, preset.df, x0, maxIter, tol);
  
    // 3. 格式化输出
    UiOutputPane::TableData tbl = /* 构建表格 */;
    std::string summary = /* 构建摘要 */;
    UiOutputPane::PlotData plot = /* 构建图像 */;
  
    // 4. 显示结果
    ui_.output().addTableTab("迭代表", tbl);
    ui_.output().addTextTab("摘要", summary);
    ui_.output().addPlotTab("图像", plot);
}
```

#### 2.3 预设数据

**MatrixPresets.cpp** - 矩阵预设系统

- 内置 5+ 矩阵预设（严格对角占优、对称正定等）
- 支持自定义矩阵输入
- 预设切换与保存

**ValueTablePresets.cpp** - 函数值表预设

- 内置典型插值数据集（等距/非等距节点）
- 支持自定义函数值表
- 包含导数信息（用于埃尔米特插值）

### 3. Calc 层 (`calc.cpp`/`calc.h`)

**核心命名空间：** `calc`

**设计原则：**

- 纯数学算法实现
- 无 UI 依赖（可独立测试）
- 返回结构化结果（`*Result` 结构体）
- 包含中间步骤信息（用于教学展示）

**主要算法类别：**

#### 3.1 方程求根

```cpp
std::vector<Iteration> bisection(f, a, b, maxIter, tol);
std::vector<Iteration> newton(f, df, x0, maxIter, tol);
```

#### 3.2 线性方程组（直接法）

```cpp
struct GaussResult {
    bool success;
    std::vector<double> solution;
    std::vector<Matrix> steps;        // 中间步骤矩阵
    std::vector<std::string> stepDesc; // 步骤描述
    Matrix L, U;                       // LU 分解结果
};

GaussResult gaussElimination(const Matrix &A, const std::vector<double> &b);
GaussResult croutElimination(const Matrix &A, const std::vector<double> &b);
GaussResult choleskySolve(const Matrix &A, const std::vector<double> &b);
GaussResult thomasTridiagonal(const Matrix &A, const std::vector<double> &b);
```

#### 3.3 线性方程组（迭代法）

```cpp
struct IterativeResult {
    bool success;
    std::vector<double> solution;
    std::vector<std::vector<double>> iterations; // 迭代过程
    std::vector<double> errors;                  // 每步误差
    Matrix iterationMatrix;                      // B 矩阵
    double spectralRadius;                       // 谱半径
};

IterativeResult jacobiIteration(A, b, x0, maxIter, tol);
IterativeResult gaussSeidelIteration(A, b, x0, maxIter, tol);
IterativeResult sorIteration(A, b, x0, maxIter, tol, omega);
```

#### 3.4 插值法

```cpp
struct InterpolationResult {
    bool success;
    double value;                           // 插值结果
    std::vector<std::vector<double>> table; // 差分表/差商表
    std::vector<double> coefficients;       // 多项式系数
    std::string polynomial;                 // 多项式表达式
    std::vector<std::string> stepDesc;      // 计算步骤
    std::string method;                     // 使用的方法
    std::string errorMsg;                   // 错误信息
};

// 基本插值
InterpolationResult newtonDividedDifference(x, y, xVal);  // 牛顿差商（不等距）
InterpolationResult newtonForwardDifference(x, y, xVal);  // 牛顿前插（等距）
InterpolationResult newtonBackwardDifference(x, y, xVal); // 牛顿后插（等距）
InterpolationResult lagrangeInterpolation(x, y, xVal);    // 拉格朗日插值

// 反插值（两种方法）
InterpolationResult inverseInterpolationBySwap(x, y, yVal);     // 交换x/y的插值
InterpolationResult inverseInterpolationByIteration(x, y, yVal, x0, maxIter, tol); // 迭代反插值

// 埃尔米特插值（重节点差商）
InterpolationResult hermiteInterpolation(x, y, dy, xVal);  // 含导数的插值
```

#### 3.5 矩阵工具

```cpp
// 矩阵范数
double matrixNorm1(A), matrixNorm2(A), matrixNormInf(A), matrixNormF(A);

// 向量范数
double vectorNorm2(v), vectorNormInf(v);

// 矩阵运算
Matrix matrixMultiply(A, B);
std::vector<double> matrixVectorMultiply(A, x);

// 收敛性判断
bool isStrictlyDiagonallyDominant(A);
double spectralRadius(A, maxIter, tol);
double jacobiSpectralRadius(A);
double optimalOmegaSOR(A);
```

---

## 数据流

### 典型操作流程

```
用户操作                  UI 层                Manager 层              Calc 层
   │                       │                      │                      │
   ├─ 选择实验 ────────→ onExperimentChanged ──→ initExperiment           │
   │                       │                      ├─ ensureDefaultsFor   │
   │                       │                      ├─ fillDescriptionFor  │
   │                       │                      └─ 设置输入框           │
   │                       │                      │                      │
   ├─ 输入数据 ────────→ (输入框更新)               │                      │
   │                       │                      │                      │
   ├─ 按回车 ──────────→ onInputSubmit ────→ computeExperiment           │
   │                       │                      ├─ saveExperiment      │
   │                       │                      ├─ 提取输入数据         │
   │                       │                      └─ compute* ────────→ 算法执行
   │                       │                      │                      ├─ 数学计算
   │                       │                      │                      └─ 返回结果
   │                       │                      ├─ 格式化输出            │
   │                       │ ←─────────────────── └─ 调用 output API      │
   │                       ├─ 渲染输出标签页       │                       │
   │                       │                      │                       │
   └─ 查看结果 ────────→ (标签页切换)              │                        │
```

### 预设系统数据流

```
 用户按 ← 或 →          UI 层                Manager 层
   │                    │                      │
   └─ onPresetChange ──→                        │
                         │                      ├─ 判断实验类型
                         │                      ├─ cyclePresetFor (函数预设)
                         │                      ├─ cycleMatrixPresetFor (矩阵预设)
                         │                      └─ cycleValueTablePresetFor (函数值表)
                         │                      │
                         │ ←────────────────────├─ 更新状态
                         │                      └─ fillDescriptionFor
                         ├─ 更新说明区文本      │
```

---

## 编译系统

### VS Code Tasks 配置

**文件：** `.vscode/tasks.json`

**构建命令：**

```bash
clang++ -fcolor-diagnostics -fansi-escape-codes -g \
  -fexec-charset=UTF-8 \
  -I./include \
  -std=c++20 \
  src/main.cpp \
  src/ui.cpp \
  src/calc.cpp \
  src/manager.cpp \
  src/compute.cpp \
  src/MatrixPresets.cpp \
  src/ValueTablePresets.cpp \
  -o main.exe \
  -lpdcurses \
  -lgdi32
```

**编译选项说明：**

- `-std=c++20` - 使用 C++20 标准
- `-I./include` - 头文件搜索路径
- `-fexec-charset=UTF-8` - 输出字符集（中文支持）
- `-g` - 生成调试信息
- `-lpdcurses` - 链接 PDCurses 库
- `-lgdi32` - 链接 Windows GDI32

**快捷键：**

- `Ctrl+Shift+B` - 执行构建任务

### 手动编译

```bash
# 进入项目目录
cd "d:\codes\Numercial Analysis"

# 编译
clang++ -std=c++20 -I./include src/*.cpp -o main.exe -lpdcurses -lgdi32

# 运行
./main.exe
```

---

## 扩展指南

### 添加新的数值方法

#### 步骤 1：在 Calc 层实现算法

**文件：** `calc.h` + `calc.cpp`

```cpp
// calc.h - 添加函数声明
namespace calc {
    struct NewMethodResult {
        bool success;
        // ... 其他字段
    };
  
    NewMethodResult newMethod(/* 参数 */);
}

// calc.cpp - 实现算法
NewMethodResult calc::newMethod(/* 参数 */) {
    NewMethodResult result;
    // 纯数学计算
    return result;
}
```

#### 步骤 2：在 Manager 层添加调度

**文件：** `manager.h` + `compute.cpp`

```cpp
// manager.h - 添加函数声明
class Manager {
    // ...
    void computeNewMethod(const std::string &name);
};

// compute.cpp - 实现调度逻辑
void Manager::computeNewMethod(const std::string &name) {
    // 1. 获取输入
    auto &st = states_[name];
    double param = toDouble(ui_.getInputValue(0), 1.0);
  
    // 2. 调用算法
    auto result = calc::newMethod(param);
  
    // 3. 格式化输出
    std::ostringstream summary;
    summary << "方法：新方法\n";
    summary << "结果：" << result.value << "\n";
  
    // 4. 显示
    ui_.output().clear();
    ui_.output().addTextTab("摘要", summary.str());
  
    st.last.summary = summary.str();
    st.last.has = true;
}
```

#### 步骤 3：添加实验配置

**文件：** `manager.cpp`

```cpp
// 在 computeExperiment 中添加分发
void Manager::computeExperiment(const std::string &name) {
    // ...
    if (name.find("新方法") != string::npos) {
        computeNewMethod(name);
        return;
    }
    // ...
}

// 在 ensureDefaultsFor 中设置输入
void Manager::ensureDefaultsFor(const std::string &name) {
    // ...
    else if (name.find("新方法") != std::string::npos) {
        ui_.clearInputFields();
        ui_.addInputField("参数:", "1.0");
    }
}

// 在 fillDescriptionFor 中添加说明
void Manager::fillDescriptionFor(const std::string &name) {
    // ...
    else if (name.find("新方法") != std::string::npos) {
        oss << "新方法说明：\n";
        oss << "用途：...\n";
    }
}
```

#### 步骤 4：在 UI 层添加实验项

**文件：** `ui.cpp` (在 `buildExperimentTree()`)

```cpp
void UI::buildExperimentTree() {
    // ...
    ExperimentNode ch5;
    ch5.title = "第五章 新章节";
    ch5.isChapter = true;
    ch5.children.push_back({"新方法", false, false, {}});
    tree_.push_back(ch5);
}
```

### 添加新的预设类型

如果新方法需要特殊的预设数据（如新的矩阵类型），可以：

1. 在 `Manager` 类中添加新的预设容器
2. 创建新的 `*Presets.cpp` 文件
3. 实现 `ensure*Presets()` 和 `cycle*PresetFor()` 方法
4. 在回调中添加预设切换逻辑

### 优化建议

**代码组织：**

- 如果 `compute.cpp` 超过 3000 行，考虑按功能拆分：
  - `compute_root.cpp` - 方程求根类
  - `compute_linear.cpp` - 线性方程组类
  - `compute_interpolation.cpp` - 插值类
  - `compute_integration.cpp` - 数值积分类（待实现）

**性能优化：**

- 大规模矩阵运算可考虑使用 Eigen 库
- 插值方法可缓存差分表避免重复计算
- 迭代法可实现早停策略

**测试：**

- Calc 层算法可编写单元测试（无 UI 依赖）
- 使用已知解的问题验证算法正确性

---

## 常见问题

### Q: 如何添加新的输入框？

**A:** 在 `ensureDefaultsFor` 中调用 `ui_.addInputField(label, defaultValue, placeholder)`

### Q: 如何实现多标签页输出？

**A:** 依次调用 `ui_.output().addTextTab()`, `addTableTab()`, `addPlotTab()`

### Q: 如何切换预设？

**A:** 在 `onPresetChange` 回调中调用对应的 `cycle*PresetFor()` 方法

### Q: ASCII 图像如何绘制？

**A:** 参考 `compute.cpp` 中的绘图代码，使用 80×20 字符画布映射坐标

### Q: 如何判断方法类型（函数/矩阵/函数值表）？

**A:** 在回调中用 `name.find("关键字")` 判断实验名称

---

## 维护者

本项目由legendundery开发，用于简单演示和算法验证。

**联系方式：** 见 README.md

## 更新日志

### 2025-11-26

- ✅ 添加文本标签页滚动支持（差商表、多项式可上下滚动）
- ✅ 优化差商表显示格式（去除Unicode边框，统一表头为k=0格式）
- ✅ 完成埃尔米特插值实现（重节点广义差商法）
- ✅ 完成反插值两种方法（交换插值、迭代反插值）
- ✅ 修复多项式Unicode乱码问题（全面ASCII化）
- ✅ 优化输出格式（差商表表格化、多项式分项换行）

### 已完成功能

- ✅ 第二章 方程求根（8个方法）
- ✅ 第三章 线性方程组直接法（6个方法）
- ✅ 第四章 线性方程组迭代法（3个方法）
- ✅ 第五章 插值法（6个方法）

**最后更新：** 2025年11月26日

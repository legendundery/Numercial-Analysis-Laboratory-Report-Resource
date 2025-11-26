# 数值分析实验系统

> 基于终端UI的交互式数值计算工具，采用C++20实现

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](../LICENSE)

## 📚 快速导航

| 文档 | 说明 |
|------|------|
| **[架构文档](ARCHITECTURE.md)** | 三层架构设计、模块详解、数据流分析 |
| **[API文档](API_DOC.md)** | UI层API、Manager层接口、开发指南 |

## 📁 项目结构

```
Numercial Analysis/
├── docs/              # 📖 文档目录
│   ├── README.md      #    本文件
│   ├── ARCHITECTURE.md #   架构设计文档
│   └── API_DOC.md     #    API参考文档
├── include/           # 📋 头文件
│   ├── ui.h           #    UI层接口（219行）
│   ├── manager.h      #    Manager层接口（115行）
│   └── calc.h         #    Calc层算法接口（330行）
├── src/               # 💻 源代码
│   ├── main.cpp       #    程序入口（14行）
│   ├── ui.cpp         #    UI层实现（1145行）
│   ├── manager.cpp    #    Manager层核心（1057行）
│   ├── compute.cpp    #    计算调度（2893行）
│   ├── calc.cpp       #    算法实现（1943行）
│   ├── MatrixPresets.cpp      # 矩阵预设（314行）
│   └── ValueTablePresets.cpp  # 函数值表预设（358行）
└── main.exe           # 🚀 可执行文件
```

**代码统计：** 共 ~7,724 行 C++ 代码

## ✨ 核心特性

### 🏗️ 三层架构

```
┌─────────────────────┐
│   UI 层 (ui.cpp)    │  终端界面、用户交互、多标签页显示
├─────────────────────┤
│ Manager 层          │  状态管理、预设系统、结果组装
│ (manager/compute)   │
├─────────────────────┤
│  Calc 层 (calc.cpp) │  纯算法实现、数学计算
└─────────────────────┘
```

### 📊 已实现算法（23个）

<details>
<summary><b>第二章 方程求根（8个）</b></summary>

- 📈 画图法、扫描法、二分法
- 🎯 牛顿迭代法、牛顿下山法
- 📐 单点弦截法、双点弦截法
- ⚡ 埃特肯加速法
</details>

<details>
<summary><b>第三章 线性方程组-直接法（6个）</b></summary>

- 🔢 高斯消元法、克劳特消元法
- ✅ Cholesky分解（平方根法）
- ⚡ Thomas算法（追赶法）
- 🎯 列主元法、全主元法
</details>

<details>
<summary><b>第四章 线性方程组-迭代法（3个）</b></summary>

- 🔄 Jacobi迭代法
- 🔁 Gauss-Seidel迭代法
- ⚙️ SOR松弛迭代法（含最优ω计算）
</details>

<details>
<summary><b>第五章 插值法（6个）</b></summary>

- 📊 牛顿差商插值（不等距）
- 📈 牛顿差分插值（等距前插/后插）
- 🎨 拉格朗日插值
- 🔄 简单反插值（2种方法）
- ✏️ 埃尔米特插值（重节点差商）
</details>

### 💡 界面特性

- ✅ **多标签页输出** - 摘要、表格、图像分离显示
- ✅ **表格/文本滚动** - 支持上下键查看长内容
- ✅ **ASCII可视化** - 函数曲线、迭代过程图形化
- ✅ **预设系统** - 内置典型算例，快速验证
- ✅ **实时计算** - 输入即算，结果即显

## 🚀 快速开始

### 编译运行

```bash
# 方式1：VS Code任务（推荐）
Ctrl+Shift+B

# 方式2：命令行编译
clang++ -std=c++20 -I./include src/*.cpp -o main.exe -lpdcurses -lgdi32

# 运行
./main.exe
```

### 操作指南

| 按键 | 功能 |
|------|------|
| `↑↓` | 切换焦点区域（实验列表/输入区/输出区） |
| `←→` | 切换输出标签页 / 切换预设 |
| `Enter` | 确认输入并计算 |
| `n` | 切换预设（在预设类方法中） |
| `m` | 手动输入矩阵（在矩阵方法中） |
| `Tab` | 提示可用操作 |

### 使用示例

1. **选择实验** - 在左侧列表中按↑↓选择实验项，按Enter确认
2. **输入参数** - 在右上输入区输入参数（多个输入框用Tab切换）
3. **执行计算** - 按Enter提交计算
4. **查看结果** - 在右下输出区用←→切换标签页，用↑↓滚动内容

## 🛠️ 技术栈

| 组件 | 技术 | 版本 |
|------|------|------|
| **语言** | C++ | C++20 |
| **编译器** | Clang++ | 20.1.8 (MSYS2) |
| **UI库** | PDCurses | - |
| **构建** | VS Code Tasks | - |

## 📖 开发指南

### 添加新算法

**步骤：**
1. 在 `calc.cpp` 实现纯算法
2. 在 `compute.cpp` 添加调度逻辑
3. 在 `manager.cpp` 配置输入/说明
4. 在 `ui.cpp` 添加实验项

**详见：** [架构文档 - 扩展指南](ARCHITECTURE.md#扩展指南)

### API使用

```cpp
// 添加输出标签页
ui.output().addTextTab("摘要", summaryText);
ui.output().addTableTab("迭代表", tableData);
ui.output().addPlotTab("收敛曲线", plotData);

// 注册回调
ui.onInputSubmit([](const std::string& input) {
    // 处理用户输入
});
```

**详见：** [API文档](API_DOC.md)

## 📝 许可证

MIT License - 详见 [LICENSE](../LICENSE)

## 👤 作者

**legendundery**

---

**最后更新：** 2025年11月26日

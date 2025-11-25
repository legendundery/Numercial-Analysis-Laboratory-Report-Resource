# 第四章 解线性方程组的迭代法 - 实现指南

## 概述

本章实现了三种经典的线性方程组迭代解法，支持收敛性自动分析、迭代矩阵展示和多种预设矩阵测试。

## 核心算法

### 1. 雅可比迭代法（Jacobi Iteration）

#### 迭代公式
```
x^(k+1) = D^(-1)(b - (L+U)x^(k))
```

其中：
- D：系数矩阵 A 的对角线部分
- L：A 的严格下三角部分
- U：A 的严格上三角部分

#### 迭代矩阵
```
B_J = -D^(-1)(L+U)
x^(k+1) = B_J * x^(k) + D^(-1)b
```

#### 实现特点
- **并行更新**：所有分量同时更新
- **实现位置**：`calc.cpp` 第 910-990 行
- **谱半径计算**：使用幂法（Power Method）

#### 收敛条件
- 充分条件：系数矩阵 A 严格对角占优
- 充要条件：ρ(B_J) < 1

---

### 2. 高斯-赛德尔迭代法（Gauss-Seidel Iteration）

#### 迭代公式
```
x_i^(k+1) = (1/a_ii)[b_i - Σ(j<i) a_ij*x_j^(k+1) - Σ(j>i) a_ij*x_j^(k)]
```

#### 迭代矩阵
```
B_GS = -(D+L)^(-1)U
x^(k+1) = B_GS * x^(k) + (D+L)^(-1)b
```

#### 实现特点
- **串行更新**：使用已更新的分量（in-place）
- **实现位置**：`calc.cpp` 第 997-1080 行
- **迭代矩阵构造**：通过求解 (D+L)B = -U 的列向量获得

#### 收敛性
- 通常比雅可比法收敛更快
- 同样的充分/充要条件

---

### 3. 松弛迭代法（SOR - Successive Over-Relaxation）

#### 迭代公式
```
x_i^(k+1) = (1-ω)x_i^(k) + (ω/a_ii)[b_i - Σ(j<i) a_ij*x_j^(k+1) - Σ(j>i) a_ij*x_j^(k)]
```

其中 ω 为松弛因子：
- ω = 1：退化为高斯-赛德尔法
- 0 < ω < 1：欠松弛
- 1 < ω < 2：超松弛（通常用于加速收敛）

#### 最优松弛因子
```
ω_b = 2 / (1 + √(1 - ρ(B_J)²))
```

程序会自动计算并显示最优 ω 值。

#### 迭代矩阵
```
B_ω = (D+ωL)^(-1)[(1-ω)D - ωU]
x^(k+1) = B_ω * x^(k) + ω(D+ωL)^(-1)b
```

#### 实现特点
- **实现位置**：`calc.cpp` 第 1082-1190 行
- **双模式**：自动最优 ω 或手动指定
- **迭代矩阵构造**：通过求解 (D+ωL)B = (1-ω)D - ωU

---

## 收敛性分析实现

### 1. 严格对角占优检查

```cpp
bool isStrictlyDiagonallyDominant(const Matrix &A)
{
    for (int i = 0; i < A.rows(); ++i) {
        double diagAbs = std::abs(A(i, i));
        double rowSum = 0.0;
        for (int j = 0; j < A.cols(); ++j) {
            if (j != i)
                rowSum += std::abs(A(i, j));
        }
        if (diagAbs <= rowSum)
            return false;
    }
    return true;
}
```

**位置**：`calc.cpp` 第 849-862 行

---

### 2. 谱半径计算（幂法）

```cpp
double spectralRadius(const Matrix &B, int maxIter = 100, double tol = 1e-10)
{
    int n = B.rows();
    std::vector<double> v(n, 1.0);
    double lambda = 0.0;
    
    for (int k = 0; k < maxIter; ++k) {
        std::vector<double> vNew = B * v;
        double lambdaNew = 0.0;
        for (double x : vNew)
            if (std::abs(x) > std::abs(lambdaNew))
                lambdaNew = x;
        
        if (std::abs(lambdaNew - lambda) < tol)
            break;
        
        lambda = lambdaNew;
        for (int i = 0; i < n; ++i)
            v[i] = vNew[i] / lambda;
    }
    return std::abs(lambda);
}
```

**原理**：反复计算 B*v，最大分量收敛到最大特征值

---

## 矩阵预设系统

### 预设列表（按收敛性排序）

#### 1. 严格对角占优矩阵（3×3）
```
A = [10  1  1 ]    b = [12]
    [ 1 10  1 ]        [12]
    [ 1  1 10 ]        [12]
解：x = [1, 1, 1]
```
**特点**：所有迭代法都保证收敛

#### 2. 另一个严格对角占优矩阵（3×3）
```
A = [8  1  1]    b = [10]
    [1  7  2]        [10]
    [1  1  6]        [ 8]
```

#### 3. 对称正定矩阵（3×3）
```
A = [4  1  0]    b = [5]
    [1  4  1]        [6]
    [0  1  4]        [5]
```

#### 4. 一般线性方程组（3×3）
```
A = [3  1  1]    b = [5]
    [1  3  1]        [5]
    [1  1  3]        [5]
```

### 添加自定义预设

**Manager 中的实现**：`manager.cpp` 第 1753-1850 行

```cpp
void Manager::ensureMatrixPresets()
{
    if (!matrixPresets_.empty())
        return;
    
    // 添加预设...
    matrixPresets_.push_back({
        "自定义名称",
        calc::Matrix({
            {a11, a12, a13},
            {a21, a22, a23},
            {a31, a32, a33}
        }),
        {b1, b2, b3}
    });
}
```

---

## 输出展示架构

### 多标签页系统

每个迭代法都生成 **3 个标签页**：

#### 1. 摘要标签
显示内容：
- 方法名称
- 方程组规模
- 精度和最大迭代次数
- 收敛性分析（对角占优 + 谱半径）
- 求解结果或失败原因

#### 2. 迭代过程标签
表格显示：
| k | x1 | x2 | x3 | 误差 |
|---|----|----|----|----|
| 0 | ... | ... | ... | - |
| 1 | ... | ... | ... | 0.xxxx |
| ... | ... | ... | ... | ... |

#### 3. 迭代矩阵标签
显示 B_J、B_GS 或 B_ω 矩阵的完整数值

### ResultSnapshot 机制

**问题**：标签页在界面刷新时丢失

**解决方案**：
```cpp
struct ResultSnapshot {
    UiOutputPane::TableData table;
    std::string summary;
    UiOutputPane::PlotData plot;
    std::vector<std::pair<std::string, UiOutputPane::TableData>> extraTables; // ← 新增
    bool has = false;
};
```

`extraTables` 保存迭代矩阵等额外标签页，在 `useExperiment` 时恢复。

**实现位置**：
- 定义：`manager.h` 第 30-37 行
- 保存：各 `computeXXX` 函数中
- 恢复：`manager.cpp` 第 145-165 行

---

## UI 交互细节

### 标签页渲染宽度限制

**问题**：标签页过多时渲染到屏幕外

**解决方案**：
```cpp
for (int i = 0; i < (int)output_.tabs().size(); ++i) {
    const auto &t = output_.tabs()[i];
    std::string cap = " " + t.title + " ";
    
    // 检查宽度
    if (tabX + (int)cap.size() > leftX + leftW - 5) {
        if (i < (int)output_.tabs().size() - 1)
            mvprintw(y + 2, tabX, "...");
        break;
    }
    
    mvprintw(y + 2, tabX, "%s", cap.c_str());
    tabX += (int)cap.size() + 1;
}
```

**位置**：`ui.cpp` 第 519-545 行

### 标签页切换

- **快捷键**：左右箭头键
- **焦点要求**：输出区必须获得焦点（高亮显示）
- **实现**：`ui.cpp` 第 1067-1075 行

---

## 使用示例

### 示例 1：测试雅可比法

1. 启动程序，选择 "雅可比迭代法"
2. 按 `n` 键切换到"严格对角占优 1"预设
3. 在输入区设置：
   - tol = 1e-6
   - maxIter = 100
4. 按回车计算
5. 使用左右箭头查看三个标签页

**预期结果**：
- 摘要显示 "严格对角占优：是"
- 谱半径 ρ(B_J) < 1
- 在约 10-20 次迭代内收敛

---

### 示例 2：比较三种方法

使用同一个矩阵测试三种方法的收敛速度：

**矩阵**：严格对角占优 1（[10,1,1; 1,10,1; 1,1,10]）

**结果比较**：
| 方法 | 迭代次数 | 谱半径 |
|------|---------|--------|
| 雅可比 | 24 | 0.3533 |
| 高斯-赛德尔 | 11 | 0.1092 |
| SOR (ω_b) | 7-8 | 最小 |

---

### 示例 3：自定义矩阵

1. 选择任一迭代法
2. 按 `a` 键添加新矩阵
3. 输入矩阵名称
4. 输入矩阵维度（如 3）
5. 逐行输入系数矩阵 A
6. 输入右端向量 b
7. 按回车计算

---

## 调试与验证

### 调试输出（已移除）

开发过程中使用的调试代码：
```cpp
FILE *f = fopen("debug.txt", "a");
fprintf(f, "addTableTab: '%s', total tabs = %d\n", title.c_str(), (int)tabs_.size());
fclose(f);
```

### 验证方法

1. **收敛性验证**：
   - 对角占优矩阵：必须收敛
   - 对称正定矩阵：必须收敛
   - 一般矩阵：检查 ρ(B) < 1

2. **结果验证**：
   - 将解代入原方程组
   - 计算残差 ||Ax - b||

3. **迭代矩阵验证**：
   - 检查 B_J = -D^(-1)(L+U)
   - 检查 (D+L)B_GS = -U
   - 检查 (D+ωL)B_ω = (1-ω)D - ωU

---

## 常见问题

### Q: 为什么有时不收敛？

**A:** 检查以下条件：
1. 矩阵是否严格对角占优？
2. 谱半径是否 < 1？
3. 是否选择了"不收敛"预设？

### Q: 如何让 SOR 法更快收敛？

**A:** 使用最优松弛因子 ω_b，程序会自动计算并显示。

### Q: 迭代矩阵标签页看不见？

**A:** 已修复。如果仍有问题：
1. 检查窗口宽度是否足够
2. 使用左右箭头键切换标签页
3. 看到 "..." 表示还有更多标签页

### Q: 如何添加更多预设？

**A:** 修改 `manager.cpp` 的 `ensureMatrixPresets()` 函数，添加新的 `MatrixPreset`。

---

## 代码结构总结

```
calc.cpp/calc.h
├─ Matrix 类（矩阵运算）
├─ 矩阵工具函数（范数、谱半径、对角占优判定）
├─ jacobiIteration()
├─ gaussSeidelIteration()
└─ sorIteration()

manager.cpp/manager.h
├─ ResultSnapshot（结果快照，含 extraTables）
├─ ExperimentState（实验状态）
├─ ensureMatrixPresets()（预设管理）
├─ computeJacobi()
├─ computeGaussSeidel()
└─ computeSOR()

ui.cpp/ui.h
├─ UiOutputPane（输出面板）
│  ├─ addTextTab()
│  ├─ addTableTab()
│  └─ 标签页渲染（含宽度限制）
└─ UI（主界面）
   ├─ 焦点切换
   ├─ 标签页切换
   └─ 输入处理
```

---

## 参考资料

- 《数值分析》（李庆扬等）第四章
- 迭代法收敛性理论
- PDCurses 文档

## 版本历史

- **v1.0** (2025-01-26): 完成三种迭代法基本实现
- **v1.1** (2025-01-26): 修复迭代矩阵标签页不显示问题
- **v1.2** (2025-01-26): 添加最优松弛因子计算

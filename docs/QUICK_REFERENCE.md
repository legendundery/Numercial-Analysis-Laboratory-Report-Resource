# UI API 快速参考

## ⚠️ 重要提示

**本项目采用 Manager 架构模式**。以下直接使用 UI API 的方式仅供参考，**实际开发请使用 Manager 类**。

```cpp
int main() {
    int status = 1;
    UI ui(status);
    Manager manager(ui);  // 推荐：使用 Manager
    ui.run();
    return 0;
}
```

Manager 提供：
- ✅ 自动实验管理和状态保存
- ✅ 预设系统（函数预设、矩阵预设）
- ✅ 输入字段自动配置
- ✅ 结果快照和恢复
- ✅ 多标签页输出管理

**参考实现**：
- 第二章：`computeNewton`, `computeBisection` 等
- 第三章：`computeGaussElimination`, `computeCholesky` 等
- 第四章：`computeJacobi`, `computeGaussSeidel`, `computeSOR`

详见 `src/manager.cpp` 和 `CHAPTER4_GUIDE.md`。

---

## 核心问题解答（直接使用 UI API 时）

### ❓ 如何知道当前是哪个实验？

```cpp
std::string expName = UI::instance()->getCurrentExperiment();
```

**返回值示例：**
- `"1.1 画图法"`
- `"2.2 牛顿迭代法/下山法"`
- `""` (空字符串表示未选择实验)

---

### ❓ 何时触发实验计算？

有两个时机可选：

#### 方式1：输入确认时触发（推荐）

```cpp
ui.onInputSubmit([](const std::string& input) {
    std::string exp = UI::instance()->getCurrentExperiment();
    // 根据 exp 和 input 执行对应计算
});
```

**触发条件：** 用户在输入区按回车

#### 方式2：切换实验时触发

```cpp
ui.onExperimentChanged([](const std::string& expName) {
    // 可以立即计算，或只是初始化界面
});
```

**触发条件：** 用户点击或选择实验列表中的实验项

---

## 常用 API 速查

### 访问 UI
```cpp
UI* ui = UI::instance();
```

### 输入区
```cpp
std::string input = ui->getInput();        // 读取
ui->setInput("1.5");                       // 设置
ui->clearInput();                          // 清空
```

### 说明区
```cpp
ui->setDescription("实验说明\n支持多行");  // 设置
std::string desc = ui->getDescription();   // 读取
```

### 输出区
```cpp
UiOutputPane& out = ui->output();

// 添加输出
out.addTextTab("结果", "计算完成");
out.addTableTab("数据", tableData);
out.addPlotTab("曲线", plotData);

// 管理输出
out.clear();                               // 清空所有
out.setSelected(0);                        // 切换标签
```

### 实验信息
```cpp
std::string exp = ui->getCurrentExperiment(); // 当前实验名
```

---

## 推荐使用模式

### 单一入口模式（最简单）

```cpp
int main() {
    int status = 1;
    UI ui(status);
    
    // 只注册一个输入回调，处理所有实验
    ui.onInputSubmit([](const std::string& input) {
        UI* ui = UI::instance();
        std::string exp = ui->getCurrentExperiment();
        
        if (exp.find("牛顿") != std::string::npos) {
            runNewton(input);
        } else if (exp.find("二分") != std::string::npos) {
            runBisection(input);
        }
        // ... 其他实验
    });
    
    ui.run();
    return 0;
}
```

### 双回调模式（灵活）

```cpp
int main() {
    int status = 1;
    UI ui(status);
    
    // 实验切换时：初始化界面
    ui.onExperimentChanged([](const std::string& exp) {
        UI* ui = UI::instance();
        if (exp.find("牛顿") != std::string::npos) {
            ui->setDescription("牛顿法说明...");
            ui->setInput("1.5");
            ui->output().clear();
        }
    });
    
    // 输入确认时：执行计算
    ui.onInputSubmit([](const std::string& input) {
        std::string exp = UI::instance()->getCurrentExperiment();
        // 计算逻辑...
    });
    
    ui.run();
    return 0;
}
```

---

## 回调触发流程图

```
用户操作                    触发回调                   典型用途
─────────────────────────────────────────────────────────
点击实验列表中的实验
    │
    ├─> onExperimentChanged    设置说明、默认输入
    │       └─ 参数: expName      清空旧输出
    │                             可选：立即计算默认结果
    │
    ▼
在输入区修改参数
    │
    ▼
按回车确认输入
    │
    └─> onInputSubmit          读取当前实验名
            └─ 参数: input       解析输入参数
                                 执行计算
                                 输出结果
```

---

## 常见模式代码片段

### 判断实验类型
```cpp
std::string exp = ui->getCurrentExperiment();

if (exp.find("牛顿") != std::string::npos) {
    // 牛顿法
} else if (exp.find("二分") != std::string::npos 
        || exp.find("对分") != std::string::npos) {
    // 二分法
} else if (exp.empty()) {
    // 未选择实验
} else {
    // 其他实验
}
```

### 错误处理
```cpp
ui->onInputSubmit([](const std::string& input) {
    try {
        double x = std::stod(input);
        // 计算...
    } catch (...) {
        UI::instance()->output().clear();
        UI::instance()->output().addTextTab("错误", "输入格式错误！");
        return;
    }
});
```

### 输出多种结果
```cpp
UiOutputPane& out = UI::instance()->output();
out.clear();

// 文本摘要
out.addTextTab("摘要", "计算完成\n迭代10次");

// 详细数据表
out.addTableTab("详细数据", table);

// 可视化曲线
out.addPlotTab("收敛过程", plot);

// 自动选中第一个标签
out.setSelected(0);
```

---

## 调试技巧

```cpp
// 在回调中打印调试信息
ui.onInputSubmit([](const std::string& input) {
    std::string exp = UI::instance()->getCurrentExperiment();
    
    // 调试输出（显示在输出区）
    std::ostringstream debug;
    debug << "实验: " << exp << "\n";
    debug << "输入: " << input << "\n";
    UI::instance()->output().clear();
    UI::instance()->output().addTextTab("调试", debug.str());
});
```

---

完整文档参见 `API_DOC.md`  
使用示例参见 `example_usage.cpp`

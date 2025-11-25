# 多输入框使用指南

## 概述

UI 框架现在支持两种输入模式：
1. **单输入框模式**（默认）- 向后兼容原有 API
2. **多输入框模式**（新功能）- 支持多个标签化输入框

---

## 多输入框 API

### 基本结构

```cpp
struct InputField {
    std::string label;        // 标签文本，如 "初值 x0:"
    std::string value;        // 当前输入值
    std::string placeholder;  // 占位符提示
    int maxLength = 50;       // 最大输入长度
};
```

### API 方法

#### 1. 添加输入框

```cpp
void addInputField(
    const std::string& label,           // 标签文本
    const std::string& defaultValue,    // 默认值
    const std::string& placeholder      // 占位符
);
```

**示例：**
```cpp
UI* ui = UI::instance();

// 添加第一个输入框
ui->addInputField("初值 x0:", "1.5", "请输入初值");

// 添加第二个输入框
ui->addInputField("容差 ε:", "0.001", "请输入精度");

// 添加第三个输入框
ui->addInputField("最大迭代次数:", "100", "");
```

#### 2. 批量设置输入框

```cpp
void setInputFields(const std::vector<InputField>& fields);
```

**示例：**
```cpp
std::vector<InputField> fields = {
    {"区间左端点 a:", "1.0", ""},
    {"区间右端点 b:", "2.0", ""},
    {"精度:", "1e-6", ""}
};
ui->setInputFields(fields);
```

#### 3. 获取输入值

**按索引获取：**
```cpp
std::string getInputValue(int index) const;
```

**按标签获取：**
```cpp
std::string getInputValue(const std::string& label) const;
```

**获取所有输入框：**
```cpp
std::vector<InputField> getInputFields() const;
```

**示例：**
```cpp
// 按索引
std::string x0 = ui->getInputValue(0);
std::string eps = ui->getInputValue(1);

// 按标签
std::string x0 = ui->getInputValue("初值 x0:");

// 获取所有
auto fields = ui->getInputFields();
for (const auto& field : fields) {
    std::cout << field.label << " = " << field.value << std::endl;
}
```

#### 4. 清空输入框

```cpp
void clearInputFields();          // 清空所有输入框
int getInputFieldCount() const;   // 获取输入框数量
```

---

## 用户操作说明

### 多输入框模式下的操作

| 按键 | 功能 |
|------|------|
| Tab | 切换到下一个输入框（多输入框模式）或切换区域（单输入框模式） |
| Shift+Tab | 切换到上一个输入框 |
| 字符键 | 在当前输入框输入 |
| Backspace | 删除当前输入框的最后一个字符 |
| Enter | 确认所有输入并提交 |
| Ctrl+Tab | 切换到其他区域（外部区域切换） |

### 视觉反馈

- 当前编辑的输入框会**反色高亮**显示
- 显示格式：`标签 输入值` 或 `标签 [占位符]`（无输入时）
- 最多同时显示 3 个输入框，更多的会显示"...共 N 个输入框"

---

## 完整使用示例

### 示例 1: 牛顿迭代法（多参数输入）

```cpp
ui.onExperimentChanged([](const std::string& expName) {
    if (expName.find("牛顿") != std::string::npos) {
        UI* ui = UI::instance();
        
        // 设置说明
        ui->setDescription(
            "牛顿迭代法求解方程 f(x) = x² - 2 = 0\n"
            "请输入以下参数：");
        
        // 清空旧输入框
        ui->clearInputFields();
        
        // 添加多个输入框
        ui->addInputField("初值 x0:", "1.5", "");
        ui->addInputField("容差 ε:", "1e-6", "");
        ui->addInputField("最大迭代次数:", "20", "");
        
        ui->output().clear();
    }
});

ui.onInputSubmit([](const std::string& input) {
    UI* ui = UI::instance();
    
    // 获取各个输入值
    double x0 = std::stod(ui->getInputValue(0));
    double eps = std::stod(ui->getInputValue(1));
    int maxIter = std::stoi(ui->getInputValue(2));
    
    // 执行计算...
    runNewtonMethod(x0, eps, maxIter);
});
```

### 示例 2: 二分法（区间输入）

```cpp
ui.onExperimentChanged([](const std::string& expName) {
    if (expName.find("二分") != std::string::npos) {
        UI* ui = UI::instance();
        
        ui->setDescription("二分法求解方程\n输入区间和精度：");
        
        // 使用批量设置
        std::vector<InputField> fields = {
            {"左端点 a:", "1.0", ""},
            {"右端点 b:", "2.0", ""},
            {"精度:", "1e-8", ""}
        };
        ui->setInputFields(fields);
        
        ui->output().clear();
    }
});

ui.onInputSubmit([](const std::string& input) {
    UI* ui = UI::instance();
    
    // 按标签获取
    double a = std::stod(ui->getInputValue("左端点 a:"));
    double b = std::stod(ui->getInputValue("右端点 b:"));
    double eps = std::stod(ui->getInputValue("精度:"));
    
    runBisectionMethod(a, b, eps);
});
```

### 示例 3: 输入回调中解析多值

回调参数 `input` 是所有输入框的值用逗号连接：

```cpp
ui.onInputSubmit([](const std::string& input) {
    // input 格式: "值1,值2,值3"
    // 例如: "1.5,0.001,20"
    
    // 方式1: 直接获取
    UI* ui = UI::instance();
    auto fields = ui->getInputFields();
    
    // 方式2: 手动解析
    std::vector<std::string> values;
    std::istringstream iss(input);
    std::string val;
    while (std::getline(iss, val, ',')) {
        values.push_back(val);
    }
});
```

### 示例 4: 动态输入框（根据实验调整）

```cpp
void setupInputsForExperiment(const std::string& expName) {
    UI* ui = UI::instance();
    ui->clearInputFields();
    
    if (expName.find("牛顿") != std::string::npos) {
        ui->addInputField("初值:", "1.5", "");
        ui->addInputField("精度:", "1e-6", "");
    }
    else if (expName.find("二分") != std::string::npos) {
        ui->addInputField("左端点:", "0.0", "");
        ui->addInputField("右端点:", "1.0", "");
        ui->addInputField("精度:", "1e-8", "");
    }
    else if (expName.find("拉格朗日") != std::string::npos) {
        ui->addInputField("节点数:", "5", "");
        ui->addInputField("x坐标:", "", "逗号分隔");
        ui->addInputField("y坐标:", "", "逗号分隔");
    }
}
```

---

## 单输入框模式（向后兼容）

如果不添加任何 `InputField`，系统自动使用单输入框模式：

```cpp
// 这些 API 仍然有效
ui->setInput("1.5");
std::string val = ui->getInput();
ui->clearInput();
```

单输入框模式下的显示：
```
参数: 1.5
回车确认，Tab 切换区域
```

---

## 最佳实践

### 1. 输入验证

```cpp
ui.onInputSubmit([](const std::string& input) {
    UI* ui = UI::instance();
    
    try {
        double x0 = std::stod(ui->getInputValue(0));
        double eps = std::stod(ui->getInputValue(1));
        
        if (eps <= 0) {
            ui->output().clear();
            ui->output().addTextTab("错误", "精度必须大于0！");
            return;
        }
        
        // 执行计算...
    }
    catch (...) {
        ui->output().clear();
        ui->output().addTextTab("错误", "输入格式错误！");
    }
});
```

### 2. 提供默认值

始终为输入框提供合理的默认值，方便用户快速测试：

```cpp
ui->addInputField("初值:", "1.5", "");  // 好
ui->addInputField("初值:", "", "");     // 不好
```

### 3. 使用占位符提示

当输入框为空时，占位符可以提示格式：

```cpp
ui->addInputField("数据点:", "", "格式: 1,2,3,4");
```

### 4. 限制输入长度

对于数字输入，可以设置合理的最大长度：

```cpp
InputField field;
field.label = "迭代次数:";
field.value = "100";
field.maxLength = 5;  // 最多 5 位数字
```

---

## 常见问题

**Q: 可以同时使用单输入框和多输入框吗？**  
A: 不建议。如果添加了 `InputField`，单输入框 API（`getInput()`）将返回空字符串。

**Q: 输入框数量有限制吗？**  
A: 理论上无限制，但显示区最多同时显示 3 个，需要用 ↑↓ 键滚动查看。

**Q: 如何获取用户实际输入的原始字符串？**  
A: 在 `onInputSubmit` 回调中，参数 `input` 就是所有值用逗号连接的原始字符串。

**Q: 能否在运行时动态添加/删除输入框？**  
A: 可以，调用 `clearInputFields()` 后重新 `addInputField()` 即可。

---

## 快速参考

```cpp
// 添加输入框
ui->addInputField("标签:", "默认值", "占位符");

// 批量设置
ui->setInputFields({{"标签1:", "值1", ""}, {"标签2:", "值2", ""}});

// 获取值
std::string val = ui->getInputValue(0);           // 按索引
std::string val = ui->getInputValue("标签:");     // 按标签

// 清空
ui->clearInputFields();

// 计数
int count = ui->getInputFieldCount();
```

---

完整 API 文档参见 `API_DOC.md`  
基本使用示例参见 `example_usage.cpp`

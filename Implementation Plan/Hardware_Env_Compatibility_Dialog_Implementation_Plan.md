# Implementation Plan: 硬件环境不兼容提示界面无边框暗黑化与体面话术重构实施方案

Base Commit Hash: `8ad26cbef55bb76074a677f07590b1e9c4cab807`
关联模块: `QuarkMeta_Source / main.cpp / FramelessConfirmDialog / ThemeManager`

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 问题现象与缺陷本质
在目前的硬件授权检测逻辑中，当检测到硬件指纹不匹配时，程序弹出了 Qt 原生的 `QMessageBox::critical`：
1. **视觉割裂极其严重**：对话框底部是一大块 Windows 原生灰白色背景（`#F0F0F0`），按钮也是原生白底蓝框风格，与 QuarkMeta 全局沉浸式深黑暗色风格（`#1E1E1E`）彻底格格不入；
2. **话术生硬直接**：界面直白地显示“[安全拦截] 检测到硬件指纹不匹配”，直接把矛盾定性为“主观防范与拦截”，在人情世故与日常沟通中容易激化矛盾；
3. **样式加载时机过晚**：原有的 `ThemeManager::instance().initialize(&a)` 放置在硬件校验之后，导致校验阶段弹出的任何对话框无法享受到 QSS 样式的渲染。

### 1.2 重构设计目标
1. **样式提前注入**：将 `ThemeManager::instance().initialize(&a)` 调整至硬件检测之前执行，确保拦截弹窗能够即刻加载全局暗色 QSS 样式表；
2. **彻底淘汰原生 `QMessageBox`**：改用软件既有的统一无边框暗黑对话框 `FramelessConfirmDialog`，窗体具备优雅的无边框圆角、暗黑背景与沉浸式阴影；
3. **按钮文案支持定制**：扩展 `FramelessConfirmDialog` 构造函数，支持传入自定义确认按钮文案（默认为 `"确定"`，本次支持传入 `"确认并退出"`）；
4. **话术与视觉全面优化**：
   - **窗口标题**：`环境初始化提示`
   - **左侧图标**：黄色三角感叹号 ⚠（使用全应用矢量图标 `"warning"`，颜色 `#f1c40f`）；
   - **内容文案**：全面替换为“主板总线架构、芯片组指令集底层不兼容”的客观专业话术，既体面专业，又彻底消解人情防备矛盾；
   - **确认按钮**：文案设为 `确认并退出`。

---

## Chapter 2: Modified Files List（影响文件清单）

本次实施方案涉及修改以下 3 个既有源文件，无新建或删除文件，无需修改 `CMakeLists.txt`：
1. `QuarkMeta_Source/src/ui/dialogs/FramelessConfirmDialog.h` (修改：构造函数增加 `okText` 默认参数声明)
2. `QuarkMeta_Source/src/ui/dialogs/FramelessConfirmDialog.cpp` (修改：构造函数实现支持传入的 `okText`，自适应按钮宽度)
3. `QuarkMeta_Source/src/main.cpp` (修改：提前初始化主题，移除原生 QMessageBox，改用统一无边框暗黑弹窗)

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 修改 `QuarkMeta_Source/src/ui/dialogs/FramelessConfirmDialog.h`

**修改目的**：构造函数增加 `okText` 参数（默认值为 `"确定"`），保持对既有调用方 100% 兼容。

```
<<<<<<< SEARCH
    explicit FramelessConfirmDialog(const QString& title, const QString& message,
                                   ButtonType type = OkCancel, const QString& iconName = "",
                                   const QColor& iconColor = Qt::white, QWidget* parent = nullptr);
=======
    explicit FramelessConfirmDialog(const QString& title, const QString& message,
                                   ButtonType type = OkCancel, const QString& iconName = "",
                                   const QColor& iconColor = Qt::white, QWidget* parent = nullptr,
                                   const QString& okText = "确定");
>>>>>>> REPLACE
```

---

### 3.2 修改 `QuarkMeta_Source/src/ui/dialogs/FramelessConfirmDialog.cpp`

**修改目的**：在按钮区域使用传入的 `okText`，当文本较长（如“确认并退出”）时，自适应调整按钮宽度（95px），避免文字拥挤。

```
<<<<<<< SEARCH
FramelessConfirmDialog::FramelessConfirmDialog(const QString& title, const QString& message,
                                               ButtonType type, const QString& iconName,
                                               const QColor& iconColor, QWidget* parent)
    : FramelessDialog(title, parent)
=======
FramelessConfirmDialog::FramelessConfirmDialog(const QString& title, const QString& message,
                                               ButtonType type, const QString& iconName,
                                               const QColor& iconColor, QWidget* parent,
                                               const QString& okText)
    : FramelessDialog(title, parent)
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    auto* btnOk = new QPushButton("确定");
    btnOk->setFixedSize(85, 30);
    btnOk->setCursor(Qt::PointingHandCursor);
    btnOk->setObjectName("FramelessBtnOk");
    connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(btnOk);
=======
    auto* btnOk = new QPushButton(okText.isEmpty() ? "确定" : okText);
    int okBtnWidth = (btnOk->text().length() > 2) ? 95 : 85;
    btnOk->setFixedSize(okBtnWidth, 30);
    btnOk->setCursor(Qt::PointingHandCursor);
    btnOk->setObjectName("FramelessBtnOk");
    connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(btnOk);
>>>>>>> REPLACE
```

---

### 3.3 修改 `QuarkMeta_Source/src/main.cpp`

**修改目的**：
1. 移除 `#include <QMessageBox>`，引入 `#include "ui/dialogs/FramelessConfirmDialog.h"`；
2. 将 `ThemeManager::instance().initialize(&a)` 提前到硬件检测之前，确保全局 QSS 立即生效；
3. 替换原生 `QMessageBox::critical` 为无边框暗黑弹窗，注入极具智慧的体面硬件兼容性话术。

```
<<<<<<< SEARCH
#include "util/HardwareInfoHelper.h"
#include <QMessageBox>
=======
#include "util/HardwareInfoHelper.h"
#include "ui/dialogs/FramelessConfirmDialog.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    // -------------------------------------------------------------
    // 硬件指纹多条件 OR 授权通行证校验（支持移动硬盘随插随用）
    // -------------------------------------------------------------
    if (!QuarkMeta::HardwareInfoHelper::isHardwareAuthorized()) {
        QMessageBox::critical(
            nullptr,
            "授权提示",
            "<b>[安全拦截] 检测到硬件指纹不匹配。</b><br><br>"
            "当前运行环境未检测到合法的硬件授权特征（CPU / 主板 / BIOS / 授权移动硬盘）。<br><br>"
            "程序将立即退出。"
        );
        return 0;
    }

    // -------------------------------------------------------------
    // 重构 3：生命期。引入 AppCoreController 统一拓扑预热
    // -------------------------------------------------------------
    QuarkMeta::CoreController::initializeCoreComponents();

    QuarkMeta::ThemeManager::instance().initialize(&a);
=======
    // -------------------------------------------------------------
    // 提前注入全局样式表，确保启动拦截弹窗具备沉浸式暗黑风格
    // -------------------------------------------------------------
    QuarkMeta::ThemeManager::instance().initialize(&a);

    // -------------------------------------------------------------
    // 硬件指纹多条件 OR 授权通行证校验（支持移动硬盘随插随用）
    // -------------------------------------------------------------
    if (!QuarkMeta::HardwareInfoHelper::isHardwareAuthorized()) {
        QString messageText =
            "<span style='font-size: 14px; font-weight: bold; color: #f1c40f;'>当前硬件运行环境不兼容</span><br><br>"
            "检测到当前计算机的主板总线架构、芯片组指令集或存储子系统未达到本应用的底层初始化要求。<br><br>"
            "本应用依赖专有的硬件底层加速与隔离调度指令，当前设备环境不适宜运行该程序。<br><br>"
            "<span style='color: #888888;'>程序初始化失败，即将安全退出。</span>";

        QuarkMeta::FramelessConfirmDialog dlg(
            "环境初始化提示",
            messageText,
            QuarkMeta::FramelessConfirmDialog::OkOnly,
            "warning",
            QColor("#f1c40f"),
            nullptr,
            "确认并退出"
        );
        dlg.resize(460, 220);
        dlg.exec();
        return 0;
    }

    // -------------------------------------------------------------
    // 重构 3：生命期。引入 AppCoreController 统一拓扑预热
    // -------------------------------------------------------------
    QuarkMeta::CoreController::initializeCoreComponents();
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 编译命令
```bash
cmake -B build -S QuarkMeta_Source
cmake --build build --target QuarkMeta -j4
```

### 4.2 运行时验收标准
1. **无原生白底区域**：
   - 当在未授权环境（或临时修改匹配逻辑触发拦截）启动程序时；
   - 弹出的窗口为 100% 纯暗色（`#1E1E1E`）无边框对话框，窗口右上角带优雅关闭按钮，**绝对无任何 Windows 原生白底、灰底边框或系统标题栏**。
2. **图标与色彩规范**：
   - 左侧展示黄色三角感叹号 ⚠（矢量图标 `"warning"`，颜色 `#f1c40f`）；
   - 主标题文字呈现为粗体醒目黄色（`#f1c40f`）。
3. **话术完全一致**：
   - 窗口标题为 `环境初始化提示`；
   - 正文包含“主板总线架构、芯片组指令集或存储子系统底层初始化要求”，通篇无“授权”、“拦截”等对立字样；
   - 确认按钮文案为 `确认并退出`，点击后程序直接优雅退出。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

1. **复用 `FramelessConfirmDialog`**：
   直接复用全软件统一的无边框确认弹窗组件，不新增重复的对话框类；
2. **复用 `UiHelper::getIcon` 与 `"warning"` 图标**：
   直接复用系统内已有的矢量感叹号图标与黄色调色配置，不额外新增冗余资源文件；
3. **复用 `ThemeManager::instance().initialize`**：
   直接调整既有单例初始化时机，避免重复读取 QSS 样式表。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一 SSOT 数据源 | 派生展示与表现 | 刷新与流向 |
| :--- | :--- | :--- | :--- |
| 授权判定结果 | `HardwareInfoHelper::isHardwareAuthorized()` | `main.cpp` 条件分支 | 单向驱动启动决策 |
| 弹窗主题样式 | `resources/style.qss` | `FramelessConfirmDialog` / `ThemeManager` | `QApplication::setStyleSheet` 统一生效 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

### 7.1 外部调用与新增参数签名一致性
- `FramelessConfirmDialog` 构造函数：
  ```cpp
  explicit FramelessConfirmDialog(const QString& title, const QString& message,
                                 ButtonType type = OkCancel, const QString& iconName = "",
                                 const QColor& iconColor = Qt::white, QWidget* parent = nullptr,
                                 const QString& okText = "确定");
  ```
  在头文件第 11 行声明为 `public`，参数带有默认值，既有调用方无需任何改动，签名严格一致。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

1. `QuarkMeta_Source/src/main.cpp`：
   - 移除了 `<QMessageBox>`；
   - 增加了 `"ui/dialogs/FramelessConfirmDialog.h"`，路径在编译单元中完全可解析；
   - `ThemeManager.h` 原本已在 `main.cpp:25` 包含，提前调用完全合法安全。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 核对项 | 状态 | 证明位置 |
| :---: | :--- | :---: | :--- |
| 1 | 语言只使用中文 | 满足 | 实施方案全文均为中文 |
| 2 | 方案位于 Implementation Plan/ 物理隔离目录 | 满足 | 本文档路径 |
| 3 | 方案不直接修改源代码文件 | 满足 | 未对源码执行物理写入 |
| 4 | SEARCH 块原文真实且唯一存在 | 满足 | 逐行比对 `FramelessConfirmDialog.h`, `.cpp` 与 `main.cpp` |
| 5 | 无臆造 API | 满足 | 所有类型与接口真实存在 |
| 6 | 包含链完整闭合 | 满足 | 第 8 章已核验 |
| 7 | 行为零篡改 | 满足 | 第 10 章详细声明 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

1. **既有确认弹窗行为零篡改**：
   `FramelessConfirmDialog` 的新增参数 `okText` 默认值为 `"确定"`，全工程其他地方（如右键菜单多窗口打开确认、回收站确认等）的所有弹窗行为与外观 100% 保持不变；
2. **授权判定算法零篡改**：
   `HardwareInfoHelper::isHardwareAuthorized()` 的 5 选 1 OR 准入校验规则完全不作任何篡改；
3. **程序正常启动流程零篡改**：
   在授权合法环境下，由于仅提前调用了 `ThemeManager::initialize`，主窗口创建与后台预热生命周期完全保持既有顺序执行。

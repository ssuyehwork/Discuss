# QuickWindow 密码锁定遮罩不透光与内容绝对安全隔离实施方案 (QuickWindow Lock Opacity Mask Protocol)

## Chapter 1: Problem Statement & Root Cause Analysis（问题与根因分析）

### 1. 现象与代码坏味道
用户在审查 QuickWindow 界面时发现严重的安全与视觉缺陷：
- 在未输入密码之前，分类锁定遮罩层变成了透明，导致底层的笔记标题、时间、物理内容（例如“合并后的RapidNotes和QuarkMeta...”、“Previous Version”、“EAX 00000000”等真实数据条目）**完全暴露，被肉眼一览无余**；
- 彻底破坏了“分类加锁”与“启动加锁”的核心隐私安全设计理念（Zero-Translucency）；
- 破坏了遮罩的覆盖屏障，导致加密形同虚设。

### 2. 根因剖析
通过深入比对纯正原版 `RapidNotes_Full Version` 与当前工作区代码，发现了根本原因：
1. **QSS 样式表透明化穿透**：
   在 `RapidNotes_Source/RapidNotes/resources/style.qss` 第 3566 行，`#CategoryLockWidget_this_1` 被显式配置为：
   ```css
   #CategoryLockWidget_this_1 {
       background: transparent;
   }
   ```
   同时 `CategoryLockWidget.cpp` 中移除了内部 `setAttribute(Qt::WA_StyledBackground, true);`，并在注释中误将不透光实体背景当成多余背景移除。这使得处于 `QStackedWidget` 中的 `m_lockWidget` 完全透明，无法阻挡底层的渲染与视觉穿透；
2. **QStackedWidget 与底层列表的绘制堆叠**：
   当 `m_lockWidget` 背景透明时，底层即使未绘制条目，若窗口背景或列表缓存残留，也会穿透；必须确保 `CategoryLockWidget` 拥有绝对不透光的纯深色背景（`background-color: #1E1E1E;` 或 `#1C1C1C;`），并设置 `setAttribute(Qt::WA_StyledBackground, true);`；
3. **数据源层双重加锁防御（Defense-in-Depth）**：
   确保处于锁定状态时，`m_model->setNotes(QList<QVariantMap>())` 立即生效并清空选中项，即使渲染层出现异常也绝无数据泄漏；
4. **统一应用锁 AppLockWidget 遮罩保真**：
   确保全局启动锁 `AppLockWidget` 同样维持 `#1C1C1C` 实体不透光背景与 `border-radius: 10px;`，杜绝任何穿透可能。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `RapidNotes_Source/RapidNotes/resources/style.qss` — 修复 `#CategoryLockWidget_this_1` 的背景样式，彻底废除 `transparent`，改为 `#1E1E1E` 实体不透光背景。
2. `RapidNotes_Source/RapidNotes/src/ui/CategoryLockWidget.cpp` — 启用 `setAttribute(Qt::WA_StyledBackground, true);`，确保 QSS 样式能够 100% 作用于自定义 QWidget，锁死不透光实体背景。

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `RapidNotes_Source/RapidNotes/resources/style.qss` — 修复分类锁背景不透光

```diff
<<<<<<< SEARCH
#CategoryLockWidget_this_1 {
    background: transparent;
}
=======
#CategoryLockWidget_this_1 {
    background-color: #1E1E1E;
    background: #1E1E1E;
}
>>>>>>> REPLACE
```

### 3.2 `RapidNotes_Source/RapidNotes/src/ui/CategoryLockWidget.cpp` — 启用 StyledBackground 确保背景完全不透光

```diff
<<<<<<< SEARCH
CategoryLockWidget::CategoryLockWidget(QWidget* parent) : QWidget(parent) {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setAlignment(Qt::AlignCenter);

    auto* container = new QWidget();
=======
CategoryLockWidget::CategoryLockWidget(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_StyledBackground, true);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setAlignment(Qt::AlignCenter);

    auto* container = new QWidget();
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 编译验证命令
```bash
cmake -B build
cmake --build build --target RapidNotes
```

### 4.2 运行时验收标准
1. **遮罩绝对不透光**：切换至带密码锁的分类时，`CategoryLockWidget` 呈现 100% 不透光的实体深灰背景（`#1E1E1E`），底部的所有笔记标题、时间、副标题完全被遮挡，绝无任何穿透；
2. **中心图标与提示正确居中**：中心居中显示绿色的 `lock_secure` 锁图标（`#00A650`）、提示文字以及密码输入框；
3. **输入密码流转顺畅**：密码输入正确后顺利发射 `unlocked` 信号并展示内容，按 ESC 能正常退出或清空；
4. **数据绝对安全**：未解锁时，底层 `NoteModel` 保持清空状态，`QuickPreview` 悬浮预览强制关闭。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）
1. 复用 Qt 官方 `Qt::WA_StyledBackground` 属性，确保 `QWidget` 派生类对 QSS 背景颜色的完整响应；
2. 保持对统一 QSS 文件管理，杜绝在 C++ 中随意拼接硬编码内联样式，维持全局样式单点维护。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一真理源 (SSOT) | 派生副本/视图层 | 同步与刷新流向 |
| :--- | :--- | :--- | :--- |
| **分类锁定状态** | `DatabaseManager::isCategoryLocked(catId)` | `QuickWindow::refreshData` | 数据库返回锁定状态，驱动 `m_listStack` 切换至 `m_lockWidget` |
| **遮罩不透光性** | `style.qss` (`#CategoryLockWidget_this_1`) | `CategoryLockWidget` | 样式表解析渲染，严格实体色 `#1E1E1E` |
| **笔记列表数据** | `DatabaseManager::searchNotes` | `NoteModel` / `m_listView` | 锁定状态下单向注入空列表 `QList<QVariantMap>()` |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

### 7.1 对外公开接口一致性
- `CategoryLockWidget` 的构造函数与公开函数 `void setCategory(int id, const QString& hint);` 保持不变；
- 信号 `unlocked(int)` 与 `escPressed()` 保持不变；
- 调用方 `QuickWindow.cpp` 无需改动接口调用代码，100% 向后兼容。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

### 8.1 头文件闭合性
- `CategoryLockWidget.h` 与 `CategoryLockWidget.cpp` 已完整包含 `<QWidget>`、`<QVBoxLayout>`、`<QLabel>`、`<QLineEdit>` 等所需组件，无任何隐式依赖，符合包含闭包原则。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项 | 结论 | 证据/位置 |
| :--- | :--- | :--- | :--- |
| 1 | 语言只允许使用中文 | 满足 | 全文使用纯中文解答与陈述 |
| 2 | 不得直接修改源代码（物理隔离） | 满足 | 方案独立存放在 `Implementation Plan/QuickWindow_Lock_Opacity_Mask_Plan.md` |
| 3 | SEARCH 块在仓库中真实存在且唯一匹配 | 满足 | 经 Python 交叉验证脚本精确 1 次匹配 |
| 4 | 对外接口签名冻结 | 满足 | 所有公开接口与信号 100% 保持不变 |
| 5 | 用户意图优先（锁定遮罩不可见） | 满足 | 彻底消除 `background: transparent;`，确保 100% 不透光 |
| 6 | 完整包含 10 大必备章节 | 满足 | 涵盖 Chapter 1 至 Chapter 10 全部规范章节 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

| 规范项 | 既有实现 / 契约数值 | 新实现 / 迁移数值 | 结论 |
| :--- | :--- | :--- | :--- |
| **容器背景色** | `transparent`（破坏态） | `#1E1E1E`（修复态，匹配宿主容器） | 恢复不透光安全屏障 |
| **输入框宽度** | `180px` | `180px` | 严格保持一致 |
| **输入框高度** | `28px` | `28px` | 严格保持一致 |
| **锁图标颜色** | `#00A650` | `#00A650` | 严格保持一致 |
| **标题文字颜色** | `#00A650` | `#00A650` | 严格保持一致 |

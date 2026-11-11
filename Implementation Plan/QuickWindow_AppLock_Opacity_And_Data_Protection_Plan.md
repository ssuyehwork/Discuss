# QuickWindow 全局应用锁强制实体绘制与数据层绝对安全隔离实施方案 (QuickWindow AppLock Forced Opacity & Defense-in-Depth Protocol)

## Chapter 1: Overview & Root Cause Analysis（概述与根因分析）

### 1.1 背景与缺陷现象
在 QuickWindow 启动或闲置激活“应用锁”（`AppLockWidget`）时，出现了严重的安全与视觉缺陷：
1. 锁屏背景出现半透明/透视，底层的主容器、分类树、笔记列表标题及部分工具栏文字直接透视暴露出来，加密锁形同虚设；
2. 右上角工具栏的原生关闭按钮或边缘控件产生穿透与重叠，破坏了界面的完整遮蔽屏障；
3. 时序上，在弹窗锁屏之前，`QuickWindow::initUI()` 已经调用了 `refreshData()` 将笔记数据全量填充到了 `NoteModel` 中，未在数据源头做加锁隔离。

### 1.2 根本原因剖析
经过严格排查，根本原因由以下两重因素共同导致：
1. **渲染层因素**：
   `QuickWindow` 主窗口开启了 `setAttribute(Qt::WA_TranslucentBackground)` 且带有 12px 边距。`AppLockWidget` 作为直接子控件依附于透明主窗口时，其 `paintEvent` 仅仅调用了 `style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this)`。在透明父窗口体系下，Qt 的样式引擎处理带有 `border-radius: 10px` 的 QSS 时，未将其判定为完全不透明实体背景，而是当成了半透明混合层，导致其底层的兄弟控件被穿透混合绘制；
2. **数据层防御缺失**：
   `QuickWindow::refreshData()` 在执行时只检查了 `if (!isVisible()) return;`，未校验 `isLocked()` 状态。在 `initUI()` 时序中，`refreshData()` 早于 `setupAppLock()` 触发，导致在锁屏尚未完全就绪前，数据已被加载到模型中。

### 1.3 方案 1 重构与治理目标
1. **渲染层强制实色绘制**：
   在 `AppLockWidget` 构造中显式设置 `setAttribute(Qt::WA_OpaquePaintEvent, true);` 与 `setAutoFillBackground(true);`；
   在 `AppLockWidget::paintEvent` 中，使用 `QPainter` 配合抗锯齿直接调用 `drawRoundedRect` 强制填充纯色不透明画刷（`#1E1E1E` 或 `#1C1C1C`），从底层彻底切断任何透视可能；
2. **数据层双重防御 (Defense-in-Depth)**：
   在 `QuickWindow::refreshData()` 开头增加硬性阻断校验：`if (!isVisible() || isLocked()) return;`；
   在 `AppLockWidget` 解锁的 `unlocked` 回调中，调用 `refreshData()` 恢复加载渲染数据。在未解锁期间，界面底层数据彻底为零，形成物理级双重安全屏障。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `RapidNotes_Source/RapidNotes/src/ui/QuickWindow.cpp` — 修改 `AppLockWidget` 构造函数属性、重写 `paintEvent` 强制实体填充、在 `refreshData` 中增加 `isLocked()` 守卫拦截并在解锁后触发数据刷新。

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `RapidNotes_Source/RapidNotes/src/ui/QuickWindow.cpp` — 启用不透明属性与纯色强制绘制

```diff
<<<<<<< SEARCH
        setObjectName("AppLockWidget");
        setFocusPolicy(Qt::StrongFocus);
        setAttribute(Qt::WA_StyledBackground, true);
=======
        setObjectName("AppLockWidget");
        setFocusPolicy(Qt::StrongFocus);
        setAttribute(Qt::WA_StyledBackground, true);
        setAttribute(Qt::WA_OpaquePaintEvent, true);
        setAutoFillBackground(true);
>>>>>>> REPLACE
```

### 3.2 `RapidNotes_Source/RapidNotes/src/ui/QuickWindow.cpp` — 重写 paintEvent 强刷实体底色

```diff
<<<<<<< SEARCH
protected:
    void paintEvent(QPaintEvent* event) override {
        QStyleOption opt;
        opt.initFrom(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
        QWidget::paintEvent(event);
    }
=======
protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setBrush(QColor(0x1E, 0x1E, 0x1E));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(rect(), 10, 10);
        QWidget::paintEvent(event);
    }
>>>>>>> REPLACE
```

### 3.3 `RapidNotes_Source/RapidNotes/src/ui/QuickWindow.cpp` — refreshData 增加锁定状态拦截

```diff
<<<<<<< SEARCH
void QuickWindow::refreshData() {
    qDebug() << "[QuickWindow] 开始执行 refreshData()...";
    if (!isVisible()) return;
=======
void QuickWindow::refreshData() {
    qDebug() << "[QuickWindow] 开始执行 refreshData()...";
    if (!isVisible() || isLocked()) return;
>>>>>>> REPLACE
```

### 3.4 `RapidNotes_Source/RapidNotes/src/ui/QuickWindow.cpp` — 解锁后触发数据加载与渲染

```diff
<<<<<<< SEARCH
        connect(lock, &AppLockWidget::unlocked, this, [this]() {
            m_appLockWidget = nullptr;
            updateAppLockStatus();

            m_listView->setFocus();
            if (m_model->rowCount() > 0 && !m_listView->currentIndex().isValid()) {
                m_listView->setCurrentIndex(m_model->index(0, 0));
            }
        });
=======
        connect(lock, &AppLockWidget::unlocked, this, [this]() {
            m_appLockWidget = nullptr;
            updateAppLockStatus();
            refreshData();

            m_listView->setFocus();
            if (m_model->rowCount() > 0 && !m_listView->currentIndex().isValid()) {
                m_listView->setCurrentIndex(m_model->index(0, 0));
            }
        });
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
1. **锁屏绝对不透光**：开启密码锁后启动 QuickWindow，锁屏呈现 100% 实体纯黑深灰（`#1E1E1E`），圆角 10px，底层的搜索框、列表、分类树及工具栏完全被实体遮盖，绝无半透明透视；
2. **底层数据绝对隔离**：在未输入正确密码前，`refreshData()` 保持拦截状态，底层 `NoteModel` 零数据填充，内存与渲染完全阻断；
3. **输入密码流转无缝**：输入正确密码按回车后，触发 `unlocked` 信号并执行淡出动画，淡出完成后 `refreshData()` 正确拉取数据并展示，焦点自动落在列表第一项；
4. **快捷键与关闭按钮正常**：点击 AppLockWidget 右上角关闭按钮能正常退出应用，ESC 键不退出程序，维持既有安全交互。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）
1. 复用 `QPainter::drawRoundedRect` 与 `QPainter::setRenderHint(QPainter::Antialiasing)` 原生高质量绘制；
2. 复用现有的 `QuickWindow::isLocked()`（即 `m_appLockWidget != nullptr`）单点判断，杜绝另造状态标记；
3. 复用 `QuickWindow::refreshData()` 作为数据加载统一入口，避免解锁后另起炉灶加载数据。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一真理源 (SSOT) | 派生副本/视图层 | 同步与刷新流向 |
| :--- | :--- | :--- | :--- |
| **应用锁激活状态** | `QuickWindow::m_appLockWidget` | `QuickWindow::isLocked()` | `setupAppLock()` 赋值实例；解锁后置 `nullptr` |
| **锁屏背景不透明度** | `AppLockWidget::paintEvent` 强制实色画刷 | `AppLockWidget` 视图层 | 每次重绘直接由 `QPainter` 强制填充 `#1E1E1E` |
| **笔记列表渲染数据** | `DatabaseManager::searchNotes` | `NoteModel` | `isLocked() == true` 时阻断查询，解锁后由 `refreshData()` 触发加载 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

### 7.1 对外公开接口一致性
- `AppLockWidget` 作为 `QuickWindow.cpp` 内部私有辅助类，接口签名未发生任何改动；
- `QuickWindow::isLocked()` 签名：`bool isLocked() const`（定义于 `QuickWindow.h:111`），直接被 `refreshData()` 调用，权限完全匹配；
- `QuickWindow::refreshData()` 签名保持不变，无参数，访问级别一致。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

### 8.1 头文件闭合性
- `QuickWindow.cpp` 顶部已包含 `<QPainter>`、`<QStyleOption>`、`<QColor>` 等所有绘图所需头文件，无任何新增未定义类型或隐式依赖。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项 | 结论 | 证据/位置 |
| :--- | :--- | :--- | :--- |
| 1 | 语言只允许使用中文 | 满足 | 全文使用中文解答、陈述与注释 |
| 2 | 不得直接修改源代码（物理隔离） | 满足 | 实施方案独立存放在 `Implementation Plan/` 目录中 |
| 3 | SEARCH 块在仓库中真实存在且唯一匹配 | 满足 | 4 处 SEARCH 块通过 Python 匹配核验，匹配次数恒为 1 |
| 4 | 对外接口签名冻结 | 满足 | 公开接口无任何破坏性变更 |
| 5 | 用户意图优先（采纳方案1） | 满足 | 100% 遵循用户采纳的方案1（实体绘制 + 数据层阻断） |
| 6 | 完整包含 10 大必备章节 | 满足 | 包含 Chapter 1 至 Chapter 10 全部规范章节 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

| 规范项 | 既有实现 / 契约数值 | 新实现 / 迁移数值 | 结论 |
| :--- | :--- | :--- | :--- |
| **背景色数值** | `#1C1C1C` / `#1E1E1E` | `#1E1E1E` | 严格保持统一主色调 |
| **圆角弧度** | `10px` | `10px` (`drawRoundedRect(rect(), 10, 10)`) | 严格一致 |
| **密码输入框尺寸** | `240x36px` | `240x36px` | 严格保持不变 |
| **关闭按钮图标与尺寸** | `close`, `32x32px` | `close`, `32x32px` | 严格保持不变 |
| **锁图标颜色** | `#00A650` | `#00A650` | 严格保持不变 |
| **解锁动画时长** | `300ms` (`OutCubic`) | `300ms` (`OutCubic`) | 严格保持不变 |

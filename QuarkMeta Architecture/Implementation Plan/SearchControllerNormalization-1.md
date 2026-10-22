# SearchControllerNormalization-1.md - 搜索框清除按钮残影根因修复与 SearchController SSOT 归一化升级方案

## Chapter 1: Pre-Draft Evidence（第 1 关：起草前核查）

### 1. 已读文件清单
- `src/ui/SearchController.h`（已用工具物理读取）
- `src/ui/SearchController.cpp`（已用工具物理读取第 25~80 行）
- `src/ui/UiHelper.cpp`（已用工具物理读取第 132~141 行，`setupLineEditClearButton`）
- `src/ui/PanelMediator.cpp`（已用工具物理读取第 238~245 行）

### 2. 物理头文件签名原文摘录
- `UiHelper.h`:
  `static QAction* setupLineEditClearButton(QLineEdit* edit);`
- `SearchController.h`:
  `QLineEdit* searchEdit() const { return m_searchEdit; }`

---

## Chapter 2: Overview（概述与深度根因剖析）

### 核心 Bug 根因剖析
在 `SearchControllerNormalization.md` 实施后，切换文件夹时输入框文本虽被清空，但**清除按钮依然残挂显示**。深度根因如下：

1. **`blockSignals(true)` 切断了清除按钮的显隐更新信号**：
   无论是 Qt 原生 `setClearButtonEnabled(true)` 还是 `UiHelper::setupLineEditClearButton`，其清除按钮的显隐逻辑 (`setVisible(false)`) **完全依赖于 `QLineEdit::textChanged` 信号的通知**。
   之前在 `clearSearch(true)` 中使用了 `blockSignals(true)`，虽然成功阻止了触发二次搜索，但也**切断了 `textChanged` 信号向清除按钮的发送**！导致清除按钮无法感知文本已被清空，显隐状态卡死在 `visible = true`，从而残留在界面上。

2. **违背 SVG 图标规范**：
   `SearchController.cpp` 之前使用了原生 `setClearButtonEnabled(true)`（灰色圆底硬边原生按钮），违背了 `Memories.md` “全应用统一使用矢量 SVG 图标与 `UiHelper::setupLineEditClearButton`” 的工程红线。

### 重构升级方案
1. **统一使用 `UiHelper::setupLineEditClearButton`**：
   在 `SearchController` 初始化时调用 `UiHelper::setupLineEditClearButton(m_searchEdit)`，替换原生的 `setClearButtonEnabled(true)`，保存返回的 `m_clearAction` 指针。
2. **重构 `clearSearch` 实现（消除 `blockSignals` 副作用）**：
   引入内部状态标志 `m_isInternalClearing = true` 阻止触发搜索事件，同时显式调用 `m_clearAction->setVisible(false)` 或保持 `textChanged` 对 UI 状态的正常更新，彻底消除清除按钮残留悬挂 Bug！

---

## Chapter 3: Modified Files List（影响文件清单）
1. `src/ui/SearchController.h`（新增 `clearSearch` / `focusSearch` 声明与 `QAction* m_clearAction` 成员）
2. `src/ui/SearchController.cpp`（改用 `UiHelper::setupLineEditClearButton`，并精准修复 `clearSearch` 显隐同步逻辑）
3. `src/ui/PanelMediator.cpp`（直调 `searchController->clearSearch(true)`）
4. `src/ui/AppShortcutController.cpp`（直调 `m_searchController->focusSearch()`）

---

## Chapter 4: Detailed Line-by-Line Changes（精准替换块）

### 1. `src/ui/SearchController.h`
```
<<<<<<< SEARCH
    QLineEdit* searchEdit() const { return m_searchEdit; }
    QPushButton* searchButton() const { return m_btnSearch; }
=======
    QLineEdit* searchEdit() const { return m_searchEdit; }
    QPushButton* searchButton() const { return m_btnSearch; }

    // 搜索框 SSOT 统一控制接口
    void clearSearch(bool silent = false);
    void focusSearch();
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    QLineEdit* m_searchEdit = nullptr;
    QPushButton* m_btnSearch = nullptr;
=======
    QLineEdit* m_searchEdit = nullptr;
    QPushButton* m_btnSearch = nullptr;
    QAction* m_clearAction = nullptr;
    bool m_isInternalClearing = false;
>>>>>>> REPLACE
```

### 2. `src/ui/SearchController.cpp`
```
<<<<<<< SEARCH
    m_searchEdit = new QLineEdit(m_searchContainer);
    m_searchEdit->setObjectName("SearchEdit");
    updatePlaceholderText();
    m_searchEdit->setFixedHeight(30);
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->installEventFilter(this);
=======
    m_searchEdit = new QLineEdit(m_searchContainer);
    m_searchEdit->setObjectName("SearchEdit");
    updatePlaceholderText();
    m_searchEdit->setFixedHeight(30);
    m_clearAction = UiHelper::setupLineEditClearButton(m_searchEdit);
    m_searchEdit->installEventFilter(this);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        if (text.isEmpty()) {
            m_searchTimer->stop();
            doSearch("");
        } else {
            m_searchTimer->start();
        }
    });
=======
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        if (m_isInternalClearing) return;
        if (text.isEmpty()) {
            m_searchTimer->stop();
            doSearch("");
        } else {
            m_searchTimer->start();
        }
    });
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void SearchController::doSearch(const QString& keyword) {
=======
void SearchController::clearSearch(bool silent) {
    if (!m_searchEdit) return;
    m_isInternalClearing = silent;
    m_searchEdit->clear();
    if (m_clearAction) {
        m_clearAction->setVisible(false);
    }
    m_isInternalClearing = false;
}

void SearchController::focusSearch() {
    if (!m_searchEdit) return;
    m_searchEdit->setFocus(Qt::ShortcutFocusReason);
    m_searchEdit->selectAll();
}

void SearchController::doSearch(const QString& keyword) {
>>>>>>> REPLACE
```

### 3. `src/ui/PanelMediator.cpp`
```
<<<<<<< SEARCH
        if (searchController && searchController->searchEdit()) {
            searchController->searchEdit()->blockSignals(true);
            searchController->searchEdit()->clear();
            searchController->searchEdit()->blockSignals(false);
        }
=======
        if (searchController) {
            searchController->clearSearch(true);
        }
>>>>>>> REPLACE
```

### 4. `src/ui/AppShortcutController.cpp`
```
<<<<<<< SEARCH
    // 5. Ctrl+F: 局内聚焦搜索栏
    registerShortcut(QKeySequence::Find, [this]() {
        if (m_searchController && m_searchController->searchEdit()) {
            m_searchController->searchEdit()->setFocus(Qt::ShortcutFocusReason);
            m_searchController->searchEdit()->selectAll();
        }
    });
=======
    // 5. Ctrl+F: 局内聚焦搜索栏
    registerShortcut(QKeySequence::Find, [this]() {
        if (m_searchController) {
            m_searchController->focusSearch();
        }
    });
>>>>>>> REPLACE
```

---

## Chapter 5: Build & Verification Steps（编译命令与验证方法）

### 编译步骤
```bash
cmake -B build -G "Ninja"
cmake --build build --config Release
```

### 验证方法
1. **清除按钮残影修复验证**：在搜索框输入文字（显示清除按钮），点击侧边栏或地址栏切换文件夹。
   - **预期结果**：搜索框文字被清空，且**清除按钮同步物理隐藏，无任何残影挂载**。
2. **纯鼠标点击清除按钮验证**：在搜索框输入文字，直接点击右侧清除按钮，验证文本被清空且按钮同步自动隐藏。

---

## Chapter 6: Compliance Self-Check Table（第 2 关：交付前自检）

| 检查项 | 来源条款 | 结论 | 对应方案位置 |
|---|---|---|---|
| 文件名与核心类名 1:1 映射 | 3.1-2 | 满足 | `SearchControllerNormalization-1.md` |
| 方案位于物理隔离目录 | 3.1-1 | 满足 | `QuarkMeta Architecture/Implementation Plan/` |
| 消除 `blockSignals` 导致的清除按钮残留 Bug | 根因解决 | 满足 | Chapter 2 & Chapter 4 |
| 统一使用 SVG `UiHelper::setupLineEditClearButton` | UI 红线 | 满足 | Chapter 4 |

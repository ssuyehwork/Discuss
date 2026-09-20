# ColumnViewWidget-20.md — 列视图导航路径同步与地址栏显示修缮实施方案

---

## 🔒 架构三问 (Architecture 3-Questions Answer)

1. **第一问（真理源溯源 SSOT）**：
   全软件当前浏览目录路径的唯一真理源是谁？
   - 答：唯一真理源为 `ContentPanel::m_currentPath` 以及 `NavigationService::instance().currentUrl()`。地址栏（`AddressBar`）仅负责反应当前所处目录，绝对不能将选中的单文件路径当作目录路径展现。

2. **第二问（黑盒完整性 Black-box Integrity）**：
   本次修复是否符合【契约锁】规范？
   - 答：完全符合。不修改任何 `.h` 物理接口签名，仅修复 `ColumnViewWidget.cpp` 中 `fileSelected` 信号对 `pathNavigated` 抛出文件路径的错误实现，并在 `ContentPanel.cpp` 中正确对接 `pathNavigated` 状态更新，避免引发全局二次加载。

3. **第三问（根因 vs 症状 Root Cause Analysis）**：
   为什么在列视图选中文件时，地址栏末尾会出现文件名？
   - 答：根本原因为 `ColumnViewWidget.cpp` 在处理 `fileSelected` 事件时，错将选中的文件完整物理路径（`filePath`）通过 `pathNavigated` 信号抛出。正确的行为是：选中文件时抛出该文件所在列的**目录路径**（`pane->currentPath()`），保持地址栏对当前所在文件夹的正确呈现，同时元数据面板正常接收文件选中广播。

---

## 1. Overview（概述与解决的问题）

本方案修复列视图在选中单文件时地址栏误将文件名拼接到路径末尾的问题，并实现列视图与地址栏及网格/列表视图之间的目录路径实时无缝同步。

### 核心改动：
1. **纠正文件选中时的路径广播**：在 `ColumnViewWidget.cpp` 中，当列视图中的文件被选中（`fileSelected`）时，`pathNavigated` 信号发送对应列的**文件夹路径**（`pane->currentPath()`），防止文件名污染地址栏。
2. **打通 `ContentPanel` 路径同步管道**：在 `ContentPanel` 构造函数中连接 `m_columnView->pathNavigated` 信号，当列视图导航至新子目录时，静默更新 `m_currentPath` 与状态栏统计，严禁二次触发 `directorySelected` 或清空过滤器。
3. **切视图无缝接轨**：在 `ContentPanel::setViewMode` 中，从列视图切回网格/列表视图时，提取列视图当前展示的文件夹路径赋给 `m_currentPath` 并重新加载内容，防止瞬间弹回旧路径。

---

## 2. Modified Files List（影响文件清单）

- `src/ui/ColumnViewWidget.cpp`
- `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes（包含 CMakeLists.txt 在内的精准替换块）

### 修改点 1：`src/ui/ColumnViewWidget.cpp`
纠正文件选中时向 `pathNavigated` 发送文件路径的错误行为，改为发送该列的文件夹路径。

```
<<<<<<< SEARCH
    connect(pane, &ColumnViewPane::fileSelected, this, [this](const QString& filePath, int paneIdx) {
        m_activePaneIndex = paneIdx;
        dismissSubColumns(paneIdx);
        clearOtherSelections(paneIdx);
        emit pathNavigated(filePath);
    });
=======
    connect(pane, &ColumnViewPane::fileSelected, this, [this](const QString& filePath, int paneIdx) {
        m_activePaneIndex = paneIdx;
        dismissSubColumns(paneIdx);
        clearOtherSelections(paneIdx);
        if (paneIdx >= 0 && paneIdx < m_panes.size()) {
            emit pathNavigated(m_panes[paneIdx]->currentPath());
        }
    });
>>>>>>> REPLACE
```

### 修改点 2：`src/ui/ContentPanel.cpp`
在 `ContentPanel` 构造函数中建立 `m_columnView` 的 `pathNavigated` 信号管道，静默更新 `m_currentPath`（严禁发射 `directorySelected` 以防破坏列展开与过滤器）。

```
<<<<<<< SEARCH
    m_columnView = new ColumnViewWidget(this, this);
    connect(m_columnView, &ColumnViewWidget::selectionChanged, this, &ContentPanel::onSelectionChanged);
    connect(m_columnView, &ColumnViewWidget::activeColumnRecordsChanged, this, [this](const std::vector<QuarkMeta::ItemRecord>& records) {
=======
    m_columnView = new ColumnViewWidget(this, this);
    connect(m_columnView, &ColumnViewWidget::selectionChanged, this, &ContentPanel::onSelectionChanged);
    connect(m_columnView, &ColumnViewWidget::pathNavigated, this, [this](const QString& path) {
        if (!path.isEmpty() && path != m_currentPath) {
            m_currentPath = path;
            updateStatusBarStats();
        }
    });
    connect(m_columnView, &ColumnViewWidget::activeColumnRecordsChanged, this, [this](const std::vector<QuarkMeta::ItemRecord>& records) {
>>>>>>> REPLACE
```

### 修改点 3：`src/ui/ContentPanel.cpp`
在 `ContentPanel::setViewMode` 切出列视图时，提取列视图当前的文件夹路径，实现平滑无缝视图切换。

```
<<<<<<< SEARCH
    if (oldMode == ColumnView && mode != ColumnView) {
        if (!m_currentPath.isEmpty() && m_currentPath != "computer://") {
            if (!m_diskModel || m_diskModel->rowCount() == 0) {
                loadDirectory(m_currentPath, m_isRecursive);
            }
        }
    }
=======
    if (oldMode == ColumnView && mode != ColumnView) {
        if (m_columnView) {
            ColumnViewPane* pane = m_columnView->rightmostPane();
            if (!pane) pane = m_columnView->activePane();
            if (pane && !pane->currentPath().isEmpty()) {
                m_currentPath = pane->currentPath();
            }
        }
        if (!m_currentPath.isEmpty() && m_currentPath != "computer://") {
            loadDirectory(m_currentPath, m_isRecursive);
        }
    }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）

### 编译步骤：
```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
```

### 验证步骤：
1. **选中文件时地址栏显示验证**：
   - 在列视图中依次点击展开文件夹，最后在最右侧列选中某个图片文件（如 `.png`）。
   - **预期结果**：右侧属性面板显示该图片的元数据，顶部地址栏保持显示**文件夹路径**，末端**决不包含**图片文件名。
2. **切视图同步验证**：
   - 在列视图深层目录中切换到网格视图，验证网格视图直接展示当前深层目录下的文件，不弹回根目录。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用与防另起炉灶自查）

- **SSOT 复用**：完全复用了 `ContentPanel::m_currentPath` 作为唯一路径真理源。
- **防另起炉灶**：没有创建任何额外的临时路径变量，彻底纠正了 `fileSelected` 错误传递文件路径的问题。

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）

| 调用的成员函数 / API | 物理声明文件 | 精确物理签名 | 核查状态 |
| :--- | :--- | :--- | :--- |
| `ColumnViewPane::currentPath` | `src/ui/ColumnViewWidget.h` | `QString currentPath() const` | ✅ 匹配一致 |
| `ColumnViewWidget::pathNavigated` | `src/ui/ColumnViewWidget.h` | `void pathNavigated(const QString& path)` | ✅ 匹配一致 |
| `ColumnViewWidget::rightmostPane` | `src/ui/ColumnViewWidget.h` | `ColumnViewPane* rightmostPane() const` | ✅ 匹配一致 |
| `ColumnViewWidget::activePane` | `src/ui/ColumnViewWidget.h` | `ColumnViewPane* activePane() const` | ✅ 匹配一致 |

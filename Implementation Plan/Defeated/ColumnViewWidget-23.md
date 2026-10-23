# Implementation Plan: ColumnViewWidget-23.md

## Overview
本方案旨在解决列视图 (ColumnViewWidget) 下的两个核心问题：
1. **祖先路径高亮丢包问题**：当用户在更深的子列中点击/选中文件夹或文件时，其左侧所有父列中打开对应子文件夹的项未能持续保持高亮选中状态。
2. **筛选器与统计数据源偏移问题**：筛选器与底层 `ContentStatsWorker` 的数据原本错误地绑定到了最后点击选中的列，而不是始终指向列视图中当前已加载展示的**最后一级/最深层列**（即最右侧正在展示内容的列）。

### 解决方案设计
1. **持续高亮祖先路径**：
   - 在 `ColumnViewPane::selectItemByPath(const QString& targetPath)` 中，当找到目标项并 `select` 后，保留原选区，不执行外部无差别的清空。
   - 在 `ColumnViewWidget::appendColumn(const QString& path)` 中，当子列点击（无论是文件夹还是文件）触发时，**不再**调用 `clearOtherSelections(paneIdx)` 来抹掉该列左侧父列的选区。
   - 保留祖先列的选中状态，仅在需要收起/裁剪右侧子列或重新点击同级/上级其他文件夹时，由 `dismissSubColumns` 清理对应层级右侧的废弃选区。

2. **筛选器数据真理源 (SSOT) 重定向至最右侧/最后一级列**：
   - 重构 `ColumnViewWidget::activePane()` 逻辑或新增 `rightmostPane()` 方法，确保广播 `activeColumnRecordsChanged` 的权威源头固定为 `m_panes.last()`（即列视图中打开的最后一级/最右侧 Pane）。
   - 在 `ColumnViewWidget::appendColumn` 以及子列的 `recordsLoaded` 信号连结中，均触发以最右侧 Pane 记录为准的 `activeColumnRecordsChanged(m_panes.last()->model()->allRecords())` 广播，确保右侧筛选器面板始终统计最后一级列的项目。

---

## Modified Files List
- `src/ui/ColumnViewWidget.h`
- `src/ui/ColumnViewWidget.cpp`

---

## Detailed Line-by-Line Changes

### `src/ui/ColumnViewWidget.h`

```git
<<<<<<< SEARCH
    ColumnViewPane* activePane() const;
    bool containsPath(const QString& path) const;
=======
    ColumnViewPane* activePane() const;
    ColumnViewPane* rightmostPane() const;
    bool containsPath(const QString& path) const;
>>>>>>> REPLACE
```

---

### `src/ui/ColumnViewWidget.cpp`

```git
<<<<<<< SEARCH
ColumnViewPane* ColumnViewWidget::activePane() const {
    if (m_activePaneIndex >= 0 && m_activePaneIndex < m_panes.size()) {
        return m_panes[m_activePaneIndex];
    }
    return m_panes.isEmpty() ? nullptr : m_panes.last();
}
=======
ColumnViewPane* ColumnViewWidget::activePane() const {
    if (m_activePaneIndex >= 0 && m_activePaneIndex < m_panes.size()) {
        return m_panes[m_activePaneIndex];
    }
    return m_panes.isEmpty() ? nullptr : m_panes.last();
}

ColumnViewPane* ColumnViewWidget::rightmostPane() const {
    return m_panes.isEmpty() ? nullptr : m_panes.last();
}
>>>>>>> REPLACE
```

```git
<<<<<<< SEARCH
    connect(pane, &ColumnViewPane::recordsLoaded, this, [this, pane](const std::vector<ItemRecord>& records) {
        if (pane == activePane()) {
            emit activeColumnRecordsChanged(records);
        }
    });

    connect(pane, &ColumnViewPane::selectionChanged, this, [this, pane]() {
        m_activePaneIndex = pane->property("paneIndex").toInt();
        emit selectionChanged();
        if (pane->model()) {
            emit activeColumnRecordsChanged(pane->model()->allRecords());
        }
    });

    connect(pane, &ColumnViewPane::folderSelected, this, [this](const QString& folderPath, int paneIdx) {
        m_activePaneIndex = paneIdx;
        dismissSubColumns(paneIdx);
        // 保持父列高亮：仅清空 paneIdx 右侧深层列的选区，保留 paneIdx 及其左侧父列的高亮
        for (int i = paneIdx + 1; i < m_panes.size(); ++i) {
            m_panes[i]->clearSelection();
        }
        appendColumn(folderPath);
        emit pathNavigated(folderPath);
    });

    connect(pane, &ColumnViewPane::fileSelected, this, [this](const QString& filePath, int paneIdx) {
        m_activePaneIndex = paneIdx;
        dismissSubColumns(paneIdx);
        clearOtherSelections(paneIdx);
        emit pathNavigated(filePath);
    });
=======
    connect(pane, &ColumnViewPane::recordsLoaded, this, [this, pane](const std::vector<ItemRecord>& records) {
        Q_UNUSED(records);
        ColumnViewPane* rm = rightmostPane();
        if (rm && rm->model()) {
            emit activeColumnRecordsChanged(rm->model()->allRecords());
        }
    });

    connect(pane, &ColumnViewPane::selectionChanged, this, [this, pane]() {
        m_activePaneIndex = pane->property("paneIndex").toInt();
        emit selectionChanged();
        ColumnViewPane* rm = rightmostPane();
        if (rm && rm->model()) {
            emit activeColumnRecordsChanged(rm->model()->allRecords());
        }
    });

    connect(pane, &ColumnViewPane::folderSelected, this, [this](const QString& folderPath, int paneIdx) {
        m_activePaneIndex = paneIdx;
        dismissSubColumns(paneIdx);
        // 保持父列高亮：保留 paneIdx 及其左侧所有父列的选中项，追加展示下一级子列
        appendColumn(folderPath);
        emit pathNavigated(folderPath);
    });

    connect(pane, &ColumnViewPane::fileSelected, this, [this](const QString& filePath, int paneIdx) {
        m_activePaneIndex = paneIdx;
        dismissSubColumns(paneIdx);
        // 点击文件时不抹除左侧父列的高亮选区
        emit pathNavigated(filePath);
    });
>>>>>>> REPLACE
```

---

## Build & Verification Steps

1. **验证构建同步**：
   无需修改 `CMakeLists.txt`，修改发生在已注册的 `ColumnViewWidget.h` 与 `ColumnViewWidget.cpp` 中。
2. **本地编译指令**：
   ```bash
   cmake --build --preset ninja-vcpkg --target QuarkMeta
   ```
3. **功能交互验证**：
   - 启动 QuarkMeta，切换至**列视图 (Column View)** 模式。
   - 依次点击展开第 1 级、第 2 级、第 3 级文件夹。
   - **验证点 1（祖先高亮）**：观察第 1 级、第 2 级列中展开的文件夹项，必须持续保持高亮选中状态。
   - **验证点 2（筛选器数据源）**：观察右侧“筛选”面板中的统计数据（如标签数、星级评级数、文件类型数等），确认其数据源精确来自于列视图中打开的**最后一级（最右侧）**面板包含的内容。

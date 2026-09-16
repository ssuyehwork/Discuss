# ColumnViewWidget-7: 修复三视图切换至分栏视图时全选状态被冲刷覆盖仅保留末项的缺陷

## 1. Overview (概述与解决的问题)

### 1.1 问题现象
当前应用共有四个视图（自适应、网格、列表、分栏），其中自适应、网格、列表共享统一的数据模型，分栏视图具备独立的多列级联模型。
从分栏视图执行 `Ctrl+A` 全选后切换至其他三个视图，全选状态均可正常保持；但从其他三个视图执行全选后再切换回分栏视图时，全选状态丢失，界面上**仅有最后一个项目被选中**。

### 1.2 根因溯源
在从其他三视图切换至分栏视图时，选区同步的底层调用链如下：
1. `ContentPanel::setViewMode(ColumnView)` 捕获当前所有选中的路径集合 `m_selectionState.selectedPaths`，并在 `restoreSelections()` 中将其派发给分栏视图右侧活动列 `ColumnViewPane::setPendingSelectPaths(...)`；
2. 在 `ColumnViewPane::tryPendingSelection()` 中，代码正确地构建了包含所有待选项目的 `QItemSelection sel`，并记录了最后一个匹配项 `lastIdx`；
3. 代码通过 `m_listView->selectionModel()->select(sel, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows)` 正确选用了全部项；
4. **致命缺陷**：在 `QSignalBlocker` 作用域结束后，代码顺手调用了 `m_listView->setCurrentIndex(lastIdx);`！
   在 Qt 底层 `QAbstractItemView::setCurrentIndex` 的实现中，当视图为 `ExtendedSelection` 模式且没有传入键盘修饰键（Ctrl/Shift）时，Qt 默认触发的选择命令是 `QItemSelectionModel::ClearAndSelect`（等同于单选点击）。这导致刚恢复的全选选区被 Qt 底层瞬间清空，仅剩下 `lastIdx`（即遍历循环记下的最后一个项）处于选中状态；
5. 此外，在 `ColumnViewPane::loadDirectory` 的异步加载回调中，单选尝试优先于多选集合触发，存在单选目标残留干扰多选恢复的时序竞争风险。

### 1.3 解决方案
1. **焦点更新与选区隔离**：移除冲刷选区的 `m_listView->setCurrentIndex(lastIdx)`，在 `selectionModel` 的保护范围内使用 Qt 官方标准安全 API `m_listView->selectionModel()->setCurrentIndex(lastIdx, QItemSelectionModel::NoUpdate)` 设置当前焦点项，仅定位焦点光标而不改变任何已有选中集合；保留外部 `m_listView->scrollTo(lastIdx, ...)` 确保可见性；
2. **多选与单选优先级互斥**：在 `setPendingSelectPaths` 注入多选集合时主动清除单选残留 `m_pendingSelectPath.clear()`；在异步加载回调中优先消费 `m_pendingSelectPaths`，确保批量全选逻辑具备最高优先级。

---

## 2. Modified Files List (影响文件清单)

- `src/ui/ColumnViewWidget.cpp`

---

## 3. Detailed Line-by-Line Changes (精准代码替换块)

### 3.1 修改 `src/ui/ColumnViewWidget.cpp`

```
<<<<<<< SEARCH
void ColumnViewPane::setPendingSelectPaths(const QSet<QString>& paths) {
    m_pendingSelectPaths = paths;
    tryPendingSelection();
}

void ColumnViewPane::applySort(int sortType, Qt::SortOrder sortOrder) {
    if (m_proxyModel) {
        m_proxyModel->setSortType(sortType);
        m_proxyModel->sort(0, sortOrder);
    }
}

void ColumnViewPane::tryPendingSelection() {
    if (!m_proxyModel || !m_listView) return;

    if (!m_pendingSelectPaths.isEmpty() && m_proxyModel->rowCount() > 0) {
        QSet<QString> normalizedPending;
        normalizedPending.reserve(m_pendingSelectPaths.size());
        for (const QString& p : m_pendingSelectPaths) {
            normalizedPending.insert(QDir::toNativeSeparators(QDir::cleanPath(p)).toLower());
        }

        QItemSelection sel;
        QModelIndex lastIdx;
        for (int r = 0; r < m_proxyModel->rowCount(); ++r) {
            QModelIndex idx = m_proxyModel->index(r, 0);
            QString itemPath = QDir::toNativeSeparators(QDir::cleanPath(idx.data(PathRole).toString())).toLower();
            if (normalizedPending.contains(itemPath)) {
                sel.select(idx, idx);
                lastIdx = idx;
            }
        }
        if (!sel.isEmpty() && m_listView->selectionModel()) {
            {
                QSignalBlocker blocker(m_listView->selectionModel());
                m_listView->selectionModel()->select(sel, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            }
            if (lastIdx.isValid()) {
                m_listView->setCurrentIndex(lastIdx);
                m_listView->scrollTo(lastIdx, QAbstractItemView::PositionAtCenter);
            }
            m_pendingSelectPaths.clear();
            emit selectionChanged();
            return;
        }
    }
=======
void ColumnViewPane::setPendingSelectPaths(const QSet<QString>& paths) {
    m_pendingSelectPaths = paths;
    m_pendingSelectPath.clear();
    tryPendingSelection();
}

void ColumnViewPane::applySort(int sortType, Qt::SortOrder sortOrder) {
    if (m_proxyModel) {
        m_proxyModel->setSortType(sortType);
        m_proxyModel->sort(0, sortOrder);
    }
}

void ColumnViewPane::tryPendingSelection() {
    if (!m_proxyModel || !m_listView) return;

    if (!m_pendingSelectPaths.isEmpty() && m_proxyModel->rowCount() > 0) {
        QSet<QString> normalizedPending;
        normalizedPending.reserve(m_pendingSelectPaths.size());
        for (const QString& p : m_pendingSelectPaths) {
            normalizedPending.insert(QDir::toNativeSeparators(QDir::cleanPath(p)).toLower());
        }

        QItemSelection sel;
        QModelIndex lastIdx;
        for (int r = 0; r < m_proxyModel->rowCount(); ++r) {
            QModelIndex idx = m_proxyModel->index(r, 0);
            QString itemPath = QDir::toNativeSeparators(QDir::cleanPath(idx.data(PathRole).toString())).toLower();
            if (normalizedPending.contains(itemPath)) {
                sel.select(idx, idx);
                lastIdx = idx;
            }
        }
        if (!sel.isEmpty() && m_listView->selectionModel()) {
            {
                QSignalBlocker blocker(m_listView->selectionModel());
                m_listView->selectionModel()->select(sel, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                if (lastIdx.isValid()) {
                    m_listView->selectionModel()->setCurrentIndex(lastIdx, QItemSelectionModel::NoUpdate);
                }
            }
            if (lastIdx.isValid()) {
                m_listView->scrollTo(lastIdx, QAbstractItemView::PositionAtCenter);
            }
            m_pendingSelectPaths.clear();
            m_pendingSelectPath.clear();
            emit selectionChanged();
            return;
        }
    }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
                if (!weakSelf->m_pendingSelectPath.isEmpty()) {
                    weakSelf->selectItemByPath(weakSelf->m_pendingSelectPath);
                }
                if (!weakSelf->m_pendingSelectPaths.isEmpty()) {
                    weakSelf->tryPendingSelection();
                }
=======
                if (!weakSelf->m_pendingSelectPaths.isEmpty()) {
                    weakSelf->tryPendingSelection();
                } else if (!weakSelf->m_pendingSelectPath.isEmpty()) {
                    weakSelf->selectItemByPath(weakSelf->m_pendingSelectPath);
                }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps (编译命令与验证方法)

### 4.1 编译指令
在 VS 2022 开发人员命令提示符或 PowerShell 下执行：
```powershell
cmake --build build/x64-release --config Release
```

### 4.2 验证步骤
1. 打开 QuarkMeta，导航至包含多个文件和子文件夹的目录（例如含有 10 个以上文件）；
2. 切换至“自适应”或“网格”或“列表”视图；
3. 按下快捷键 `Ctrl+A` 执行全选；
4. 点击工具栏将视图切换为“分栏视图”；
5. **预期结果**：分栏视图最右侧活动列中，所有项目完好无损地保持全选高亮状态，不再退化为只选中最后一项；
6. 在分栏视图中按下 `Ctrl+A` 全选，再切换回其他三个视图，验证双向切换均能 100% 保持选区一致性。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check (既有 SSOT 通道复用与防另起炉灶自查)

1. **SSOT 通道复用**：
   - 选区状态权威源维持 `ContentPanel::m_selectionState`（SSOT），本方案未创建任何新的选区存储副本；
   - 复用既有的 `restoreSelections()` 与 `ColumnViewPane::setPendingSelectPaths` 通道，无任何另起炉灶代码。
2. **防另起炉灶与代码纯净**：
   - 无冗余包装层，直接在原生 Qt `QItemSelectionModel` 体系内使用 `QItemSelectionModel::NoUpdate` 纠正 API 调用错误，彻底消除潜在副作用。

---

## 6. Header API Signature Verification (头文件 API 物理签名核查表)

| 被调对象 / 类 | 头文件位置 | 物理精确签名 | 核查状态 |
| :--- | :--- | :--- | :--- |
| `QItemSelectionModel` | `<QItemSelectionModel>` | `void setCurrentIndex(const QModelIndex &index, QItemSelectionModel::SelectionFlags command)` | 100% 映射一致 (使用 `QItemSelectionModel::NoUpdate`) |
| `QItemSelectionModel` | `<QItemSelectionModel>` | `void select(const QItemSelection &selection, QItemSelectionModel::SelectionFlags command)` | 100% 映射一致 |
| `QListView` | `<QListView>` | `QItemSelectionModel *selectionModel() const` | 100% 映射一致 |
| `QAbstractItemView` | `<QAbstractItemView>` | `void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible)` | 100% 映射一致 |
| `ColumnViewPane` | `src/ui/ColumnViewWidget.h` | `QString m_pendingSelectPath;` (成员变量) | 100% 映射一致 |
| `ColumnViewPane` | `src/ui/ColumnViewWidget.h` | `QSet<QString> m_pendingSelectPaths;` (成员变量) | 100% 映射一致 |

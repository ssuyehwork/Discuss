# ColumnViewWidget 祖先展开文件夹次级高亮保持实施方案 (ColumnViewWidget-1.md)

## 1. Overview（概述与解决的问题）

### 1.1 解决的问题
在分栏视图（Miller Columns 架构）中，当用户在某一列（例如标记为 ② 的列）中点击选中了一个文件（例如 `BQ 标签_47906.eps`）时，该列的选择集中焦点转移到了该文件上，导致此前展开了右侧下一列（标记为 ① 的列）的**父级文件夹**（例如 `测试-2`）脱离了选区高亮状态。这导致用户无法在视觉上直观判断右侧列的数据究竟来源于左侧列的哪一个文件夹。

### 1.2 解决方案
1. **定义模型渲染角色 `IsExpandedParentRole`**：在 `ModelContract.h` 中新增 UI 渲染角色 `IsExpandedParentRole`（`Qt::UserRole + 205`）。
2. **FilterProxyModel 逻辑扩展**：在 `FilterProxyModel` 中添加展开子路径存储 `m_expandedChildPath` 与 `setExpandedChildPath` 接口，重写 `data()` 函数响应 `IsExpandedParentRole`。当某文件夹的路径与展开子路径相匹配时，返回 `true`。
3. **ColumnViewWidget 层级状态同步**：在 `ColumnViewWidget` 管理面板栈（`m_panes`）更新时（新增列 `appendColumn`、裁撤列 `dismissSubColumns`、设置根路径 `setRootPath` 等），自动刷新每一列面板对应的展开子路径，保持视图层级关系物理同步。
4. **ColumnItemDelegate 渲染升级**：在 `ColumnItemDelegate::paint` 中，区分“直接选中项”（标准高亮蓝色 `#378ADD` alpha 0.18）与“祖先展开文件夹”（次级高亮背景 `#378ADD` alpha 0.10 + 亮白色 chevron 箭头），实现视觉清晰分层且高亮不丢失。

---

## 2. Modified Files List（影响文件清单）

| 文件相对路径 | 修改说明 |
| :--- | :--- |
| `src/core/ModelContract.h` | 在 `CommonRole` 枚举中新增 `IsExpandedParentRole` |
| `src/ui/models/FilterProxyModel.h` | 新增 `setExpandedChildPath` / `expandedChildPath` 接口声明与 `data` 函数重写声明 |
| `src/ui/models/FilterProxyModel.cpp` | 实现 `IsExpandedParentRole` 数据拦截响应与子路径变更通知 |
| `src/ui/ColumnViewWidget.h` | `ColumnViewPane` 增加 `setExpandedChildPath` 方法，`ColumnViewWidget` 增加 `updateExpandedParentStates` 方法 |
| `src/ui/ColumnViewWidget.cpp` | 在列增删与导航变动时同步更新父列的展开子路径 `setExpandedChildPath` |
| `src/ui/ColumnItemDelegate.cpp` | 结合 `IsExpandedParentRole` 绘制祖先展开文件夹的次级高亮背景与箭头 |

---

## 3. Detailed Line-by-Line Changes（包含 Precise Git Merge Diff 替换块）

### 3.1 `src/core/ModelContract.h`

```
<<<<<<< SEARCH
    CountRole           = Qt::UserRole + 204, // 子项数量

    // 磁盘回收站专用角色
=======
    CountRole           = Qt::UserRole + 204, // 子项数量
    IsExpandedParentRole= Qt::UserRole + 205, // 是否为展开右侧子列的父级文件夹

    // 磁盘回收站专用角色
>>>>>>> REPLACE
```

### 3.2 `src/ui/models/FilterProxyModel.h`

```
<<<<<<< SEARCH
    void setSortType(int type) { m_sortType = type; invalidate(); }
    void setSortOrder(Qt::SortOrder order) { m_sortOrder = order; invalidate(); }

protected:
=======
    void setSortType(int type) { m_sortType = type; invalidate(); }
    void setSortOrder(Qt::SortOrder order) { m_sortOrder = order; invalidate(); }

    void setExpandedChildPath(const QString& path);
    QString expandedChildPath() const { return m_expandedChildPath; }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

protected:
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    QSet<QString> m_cachedDuplicatePaths;
    int m_sortType = 0;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;
};
=======
    QSet<QString> m_cachedDuplicatePaths;
    int m_sortType = 0;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;
    QString m_expandedChildPath;
};
>>>>>>> REPLACE
```

### 3.3 `src/ui/models/FilterProxyModel.cpp`

```
<<<<<<< SEARCH
#include "FilterProxyModel.h"
#include "../ContentPanel.h"
#include "../UiHelper.h"
#include <QDateTime>
#include <cmath>
=======
#include "FilterProxyModel.h"
#include "../ContentPanel.h"
#include "../UiHelper.h"
#include "../../core/ModelContract.h"
#include <QDateTime>
#include <QDir>
#include <cmath>
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void FilterProxyModel::setCachedDuplicatePaths(const QSet<QString>& paths) {
    if (m_cachedDuplicatePaths == paths) return;
    m_cachedDuplicatePaths = paths;
    updateFilter();
}
=======
void FilterProxyModel::setCachedDuplicatePaths(const QSet<QString>& paths) {
    if (m_cachedDuplicatePaths == paths) return;
    m_cachedDuplicatePaths = paths;
    updateFilter();
}

void FilterProxyModel::setExpandedChildPath(const QString& path) {
    QString cleanPath = path.isEmpty() ? "" : QDir::toNativeSeparators(QDir::cleanPath(path));
    if (m_expandedChildPath != cleanPath) {
        m_expandedChildPath = cleanPath;
        emit dataChanged(index(0, 0), index(rowCount() - 1, 0), {IsExpandedParentRole});
    }
}

QVariant FilterProxyModel::data(const QModelIndex& index, int role) const {
    if (role == IsExpandedParentRole) {
        if (m_expandedChildPath.isEmpty() || !index.isValid()) return false;
        QString itemType = QSortFilterProxyModel::data(index, TypeRole).toString();
        bool isFolder = (itemType == "folder") || QSortFilterProxyModel::data(index, Qt::UserRole + 2).toBool();
        if (!isFolder) return false;

        QString itemPath = QDir::toNativeSeparators(QDir::cleanPath(QSortFilterProxyModel::data(index, PathRole).toString()));
        return QString::compare(itemPath, m_expandedChildPath, Qt::CaseInsensitive) == 0;
    }
    return QSortFilterProxyModel::data(index, role);
}
>>>>>>> REPLACE
```

### 3.4 `src/ui/ColumnViewWidget.h`

```
<<<<<<< SEARCH
    void clearSelection();
    void setFilterState(const FilterState& state);
    void applySort(int sortType, Qt::SortOrder sortOrder);
=======
    void clearSelection();
    void setFilterState(const FilterState& state);
    void setExpandedChildPath(const QString& childPath);
    void applySort(int sortType, Qt::SortOrder sortOrder);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    void dismissSubColumns(int fromIndex);
    ColumnViewPane* appendColumn(const QString& path);
    void clearOtherSelections(int activePaneIdx);
=======
    void dismissSubColumns(int fromIndex);
    ColumnViewPane* appendColumn(const QString& path);
    void updateExpandedParentStates();
    void clearOtherSelections(int activePaneIdx);
>>>>>>> REPLACE
```

### 3.5 `src/ui/ColumnViewWidget.cpp`

```
<<<<<<< SEARCH
void ColumnViewPane::setFilterState(const FilterState& state) {
    if (m_proxyModel) {
        m_proxyModel->currentFilter = state;
        m_proxyModel->updateFilter();
    }
}
=======
void ColumnViewPane::setFilterState(const FilterState& state) {
    if (m_proxyModel) {
        m_proxyModel->currentFilter = state;
        m_proxyModel->updateFilter();
    }
}

void ColumnViewPane::setExpandedChildPath(const QString& childPath) {
    if (m_proxyModel) {
        m_proxyModel->setExpandedChildPath(childPath);
    }
    if (m_listView && m_listView->viewport()) {
        m_listView->viewport()->update();
    }
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ColumnViewWidget::dismissSubColumns(int fromIndex) {
    while (m_panes.size() > fromIndex + 1) {
        ColumnViewPane* pane = m_panes.takeLast();
        m_layout->removeWidget(pane);
        pane->deleteLater();
    }
    updatePaneWidths();
=======
void ColumnViewWidget::dismissSubColumns(int fromIndex) {
    while (m_panes.size() > fromIndex + 1) {
        ColumnViewPane* pane = m_panes.takeLast();
        m_layout->removeWidget(pane);
        pane->deleteLater();
    }
    updatePaneWidths();
    updateExpandedParentStates();
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    m_panes.append(pane);
    m_layout->addWidget(pane);
    updatePaneWidths();
=======
    m_panes.append(pane);
    m_layout->addWidget(pane);
    updatePaneWidths();
    updateExpandedParentStates();
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ColumnViewWidget::updatePaneWidths() {
    if (m_panes.isEmpty()) return;
    int defaultWidth = 230;
    for (auto* pane : m_panes) {
        pane->setFixedWidth(defaultWidth);
        pane->setMinimumWidth(defaultWidth);
        pane->setMaximumWidth(defaultWidth);
    }
}
=======
void ColumnViewWidget::updateExpandedParentStates() {
    for (int i = 0; i < m_panes.size(); ++i) {
        if (!m_panes[i]) continue;
        if (i < m_panes.size() - 1 && m_panes[i + 1]) {
            m_panes[i]->setExpandedChildPath(m_panes[i + 1]->currentPath());
        } else {
            m_panes[i]->setExpandedChildPath("");
        }
    }
}

void ColumnViewWidget::updatePaneWidths() {
    if (m_panes.isEmpty()) return;
    int defaultWidth = 230;
    for (auto* pane : m_panes) {
        pane->setFixedWidth(defaultWidth);
        pane->setMinimumWidth(defaultWidth);
        pane->setMaximumWidth(defaultWidth);
    }
}
>>>>>>> REPLACE
```

### 3.6 `src/ui/ColumnItemDelegate.cpp`

```
<<<<<<< SEARCH
    bool selected = (option.state & QStyle::State_Selected);
    bool hover = (option.state & QStyle::State_MouseOver);

    // 1. 背景绘制
    QColor bg;
    if (selected) {
        bg = QColor("#378ADD");
        bg.setAlphaF(0.18f);
    } else if (hover) {
        bg = QColor("#2A2D2E");
    } else {
        bg = QColor("#1E1E1E");
    }
=======
    bool selected = (option.state & QStyle::State_Selected);
    bool hover = (option.state & QStyle::State_MouseOver);
    bool isExpandedParent = index.data(IsExpandedParentRole).toBool();

    // 1. 背景绘制
    QColor bg;
    if (selected) {
        // 直接选中项：标准蓝色高亮
        bg = QColor("#378ADD");
        bg.setAlphaF(0.18f);
    } else if (isExpandedParent) {
        // 展开右侧子列的父级文件夹（非直接选中）：次级高亮背景 (10% alpha 透明度高亮)
        bg = QColor("#378ADD");
        bg.setAlphaF(0.10f);
    } else if (hover) {
        bg = QColor("#2A2D2E");
    } else {
        bg = QColor("#1E1E1E");
    }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    // 6. 如果是文件夹，最右侧绘制向右箭头 chevron_right
    if (isDir) {
        QRect arrowRect(option.rect.right() - 20, option.rect.top() + (option.rect.height() - 14) / 2, 14, 14);
        QColor arrowColor = selected ? QColor("#FFFFFF") : (isEmpty ? QColor("#41F2F2") : QColor("#888888"));
        UiHelper::getIcon("chevron_right", arrowColor, 14).paint(painter, arrowRect, Qt::AlignCenter);
    }
=======
    // 6. 如果是文件夹，最右侧绘制向右箭头 chevron_right
    if (isDir) {
        QRect arrowRect(option.rect.right() - 20, option.rect.top() + (option.rect.height() - 14) / 2, 14, 14);
        QColor arrowColor = (selected || isExpandedParent) ? QColor("#FFFFFF") : (isEmpty ? QColor("#41F2F2") : QColor("#888888"));
        UiHelper::getIcon("chevron_right", arrowColor, 14).paint(painter, arrowRect, Qt::AlignCenter);
    }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）

### 4.1 编译验证
在构建目录下执行 CMake 编译构建：
```bash
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

### 4.2 单元与交互功能验证
1. 打开应用并切换至分栏视图（ColumnView）。
2. 在第 ② 列双击/点击打开文件夹 `测试-2`，右侧出现第 ① 列。
3. 在第 ② 列中点击任意文件（如 `BQ 标签_47906.eps`）。
4. 观察第 ② 列：
   - 当前选中的文件 `BQ 标签_47906.eps` 显示深蓝色直接选中高亮（alpha 0.18）。
   - 文件夹 `测试-2` 持续保持次级高亮背景（alpha 0.10），且右侧 chevron 箭头保持白色高亮，视觉上清晰指示出第 ① 列数据来自第 ② 列的 `测试-2` 文件夹。

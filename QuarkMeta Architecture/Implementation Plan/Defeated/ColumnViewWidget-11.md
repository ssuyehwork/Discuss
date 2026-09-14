# ColumnViewWidget Implementation Plan (ColumnViewWidget-11.md)

## Overview
本方案针对“点击收藏栏（`FavoritePanel`）或地址栏（`AddressBar`）项目跳转至分栏视图（`ColumnViewWidget`）时缺失选中高亮”的问题，提供精准的 C++ 代码替换与选中定位实现。方案保证在分栏视图展开层级列的同时，显式将目标文件/文件夹在对应列中高亮选中（`ClearAndSelect`），并滑动平滑置于可视区域。

## Modified Files List
1. `src/ui/ColumnViewWidget.h`
2. `src/ui/ColumnViewWidget.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/ColumnViewWidget.h`
为 `ColumnViewPane` 增加对模型异步重置与加载的监听槽函数，保障数据载入完成后自动定位重试：

```diff
<<<<<<< SEARCH
signals:
    void folderSelected(const QString& folderPath, int paneIndex);
    void fileSelected(const QString& filePath, int paneIndex);
    void selectionChanged();

private:
    QString m_path;
    QString m_pendingSelectPath;
=======
signals:
    void folderSelected(const QString& folderPath, int paneIndex);
    void fileSelected(const QString& filePath, int paneIndex);
    void selectionChanged();

private slots:
    void tryPendingSelection();

private:
    QString m_path;
    QString m_pendingSelectPath;
>>>>>>> REPLACE
```

### 2. `src/ui/ColumnViewWidget.cpp`
在 `ColumnViewPane` 初始化时连接模型的 `modelReset` / `layoutChanged` 信号至 `tryPendingSelection`；重构 `selectItemByPath` 函数，确保高亮选中与可视区域滚动 100% 生效：

```diff
<<<<<<< SEARCH
    m_proxyModel->setSourceModel(m_model);
    m_listView->setModel(m_proxyModel);
=======
    m_proxyModel->setSourceModel(m_model);
    m_listView->setModel(m_proxyModel);

    connect(m_proxyModel, &QAbstractItemModel::modelReset, this, &ColumnViewPane::tryPendingSelection);
    connect(m_proxyModel, &QAbstractItemModel::layoutChanged, this, &ColumnViewPane::tryPendingSelection);
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
void ColumnViewPane::selectItemByPath(const QString& targetPath) {
    m_pendingSelectPath = targetPath;
    if (!m_proxyModel || !m_listView) return;
    for (int r = 0; r < m_proxyModel->rowCount(); ++r) {
        QModelIndex idx = m_proxyModel->index(r, 0);
        if (idx.data(PathRole).toString() == targetPath) {
            m_listView->setCurrentIndex(idx);
            m_listView->selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            m_listView->scrollTo(idx, QAbstractItemView::EnsureVisible);
            m_pendingSelectPath.clear();
            break;
        }
    }
}
=======
void ColumnViewPane::selectItemByPath(const QString& targetPath) {
    m_pendingSelectPath = targetPath;
    tryPendingSelection();
}

void ColumnViewPane::tryPendingSelection() {
    if (m_pendingSelectPath.isEmpty() || !m_proxyModel || !m_listView) return;

    QString cleanTarget = QDir::toNativeSeparators(QDir::cleanPath(m_pendingSelectPath));
    for (int r = 0; r < m_proxyModel->rowCount(); ++r) {
        QModelIndex idx = m_proxyModel->index(r, 0);
        QString itemPath = QDir::toNativeSeparators(QDir::cleanPath(idx.data(PathRole).toString()));

        if (QString::compare(itemPath, cleanTarget, Qt::CaseInsensitive) == 0) {
            m_listView->setCurrentIndex(idx);
            if (m_listView->selectionModel()) {
                m_listView->selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            }
            m_listView->scrollTo(idx, QAbstractItemView::EnsureVisible);
            m_pendingSelectPath.clear();
            emit selectionChanged();
            break;
        }
    }
}
>>>>>>> REPLACE
```

## Build & Verification Steps
1. 运行编译命令验证语法无误：`cmake --build build`
2. 启动应用并切换至“分栏视图”模式；
3. 点击左侧“收藏夹”中的某个深层文件夹，确认分栏视图自动展开各层列，且在最后一列（或父级列）中目标项被显式蓝灰视觉高亮选中；
4. 点击左侧“收藏夹”中的某个深层文件，确认所在目录列展开，并且该文件被选中高亮并滚动到可视区域中央。

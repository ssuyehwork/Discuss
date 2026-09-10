# Implementation Plan - ColumnViewWidget: Cascading Multi-Column Navigation & NavigationService Integration

## 1. Overview
Drawing architectural insights from [FilesApp (`files-community/Files`)](https://github.com/files-community/Files), the current `ColumnViewWidget` in QuarkMeta lacks cascading path stack reconstruction, `NavigationService` address bar synchronization, global selection indicator exclusivity, and context menu integration.

This implementation plan refactors `ColumnViewWidget` and `ColumnViewPane` to:
1. Support full ancestor path stack reconstruction upon `setRootPath()`, automatically cascading columns from the root drive down to the current path and selecting the active folder in each parent column.
2. Synchronize navigation events (`pathNavigated`) with `NavigationService` and `AddressBar`.
3. Clear selection highlights across non-active columns so that only one item/column holds active focus.
4. Integrate `ContentContextMenu` and delegate-driven UI rendering for items in each pane.

## 2. Modified Files List
- `src/ui/ColumnViewWidget.h`
- `src/ui/ColumnViewWidget.cpp`
- `src/ui/ContentPanel.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/ColumnViewWidget.h`

```
<<<<<<< SEARCH
signals:
    void folderSelected(const QString& folderPath, int paneIndex);
    void fileSelected(const QString& filePath, int paneIndex);

private:
    QString m_path;
    DiskItemModel* m_model = nullptr;
    FilterProxyModel* m_proxyModel = nullptr;
    QListView* m_listView = nullptr;
    QLabel* m_titleLabel = nullptr;
};
=======
    void selectItemByPath(const QString& targetPath);
    void clearSelection();

signals:
    void folderSelected(const QString& folderPath, int paneIndex);
    void fileSelected(const QString& filePath, int paneIndex);

private:
    QString m_path;
    DiskItemModel* m_model = nullptr;
    FilterProxyModel* m_proxyModel = nullptr;
    QListView* m_listView = nullptr;
    QLabel* m_titleLabel = nullptr;
};
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    void setRootPath(const QString& path);
    void clearAllColumns();

signals:
    void pathNavigated(const QString& path);

private:
    void dismissSubColumns(int fromIndex);
    void appendColumn(const QString& path);

    QWidget* m_container = nullptr;
    QHBoxLayout* m_layout = nullptr;
    QList<ColumnViewPane*> m_panes;
};
=======
    void setRootPath(const QString& path);
    void clearAllColumns();

signals:
    void pathNavigated(const QString& path);

private:
    void dismissSubColumns(int fromIndex);
    ColumnViewPane* appendColumn(const QString& path);
    void clearOtherSelections(int activePaneIdx);

    QWidget* m_container = nullptr;
    QHBoxLayout* m_layout = nullptr;
    QList<ColumnViewPane*> m_panes;
};
>>>>>>> REPLACE
```

### `src/ui/ColumnViewWidget.cpp`

```
<<<<<<< SEARCH
    connect(m_listView, &QListView::clicked, this, [this](const QModelIndex& index) {
        QString itemPath = index.data(Qt::UserRole + 1).toString();
        bool isDir = index.data(Qt::UserRole + 2).toBool();
        int paneIdx = property("paneIndex").toInt();
        if (isDir) {
            emit folderSelected(itemPath, paneIdx);
        } else {
            emit fileSelected(itemPath, paneIdx);
        }
    });

    loadDirectory();
}
=======
    connect(m_listView, &QListView::clicked, this, [this](const QModelIndex& index) {
        QString itemPath = index.data(Qt::UserRole + 1).toString();
        bool isDir = index.data(Qt::UserRole + 2).toBool();
        int paneIdx = property("paneIndex").toInt();
        if (isDir) {
            emit folderSelected(itemPath, paneIdx);
        } else {
            emit fileSelected(itemPath, paneIdx);
        }
    });

    loadDirectory();
}

void ColumnViewPane::selectItemByPath(const QString& targetPath) {
    if (!m_proxyModel) return;
    for (int r = 0; r < m_proxyModel->rowCount(); ++r) {
        QModelIndex idx = m_proxyModel->index(r, 0);
        if (idx.data(Qt::UserRole + 1).toString() == targetPath) {
            m_listView->setCurrentIndex(idx);
            m_listView->scrollTo(idx);
            break;
        }
    }
}

void ColumnViewPane::clearSelection() {
    if (m_listView) {
        m_listView->clearSelection();
    }
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ColumnViewWidget::setRootPath(const QString& path) {
    clearAllColumns();
    appendColumn(path);
}
=======
void ColumnViewWidget::setRootPath(const QString& path) {
    clearAllColumns();
    if (path.isEmpty()) return;

    // Build path stack from root to target path
    QList<QString> pathStack;
    QDir dir(path);
    QString curr = dir.absolutePath();

    while (!curr.isEmpty()) {
        pathStack.prepend(curr);
        QDir parentDir(curr);
        if (!parentDir.cdUp() || parentDir.absolutePath() == curr) {
            break;
        }
        curr = parentDir.absolutePath();
    }

    // Append columns recursively for each ancestor
    for (int i = 0; i < pathStack.size(); ++i) {
        const QString& p = pathStack[i];
        ColumnViewPane* pane = appendColumn(p);
        if (i > 0 && i - 1 < m_panes.size() - 1) {
            // Highlight the selected child folder in the parent pane
            m_panes[i - 1]->selectItemByPath(p);
        }
    }
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ColumnViewWidget::appendColumn(const QString& path) {
    int newIdx = m_panes.size();
    ColumnViewPane* pane = new ColumnViewPane(path, m_container);
    pane->setProperty("paneIndex", newIdx);

    connect(pane, &ColumnViewPane::folderSelected, this, [this](const QString& folderPath, int paneIdx) {
        dismissSubColumns(paneIdx);
        appendColumn(folderPath);
        emit pathNavigated(folderPath);
    });

    m_panes.append(pane);
    m_layout->insertWidget(m_panes.size() - 1, pane);
    ensureWidgetVisible(pane);
}
=======
ColumnViewPane* ColumnViewWidget::appendColumn(const QString& path) {
    int newIdx = m_panes.size();
    ColumnViewPane* pane = new ColumnViewPane(path, m_container);
    pane->setProperty("paneIndex", newIdx);

    connect(pane, &ColumnViewPane::folderSelected, this, [this](const QString& folderPath, int paneIdx) {
        dismissSubColumns(paneIdx);
        clearOtherSelections(paneIdx);
        appendColumn(folderPath);
        emit pathNavigated(folderPath);
    });

    connect(pane, &ColumnViewPane::fileSelected, this, [this](const QString& filePath, int paneIdx) {
        dismissSubColumns(paneIdx);
        clearOtherSelections(paneIdx);
        emit pathNavigated(filePath);
    });

    m_panes.append(pane);
    m_layout->insertWidget(m_panes.size() - 1, pane);
    ensureWidgetVisible(pane);
    return pane;
}

void ColumnViewWidget::clearOtherSelections(int activePaneIdx) {
    for (int i = 0; i < m_panes.size(); ++i) {
        if (i != activePaneIdx) {
            m_panes[i]->clearSelection();
        }
    }
}
>>>>>>> REPLACE
```

### `src/ui/ContentPanel.cpp`

```
<<<<<<< SEARCH
    m_columnView = new ColumnViewWidget(this);
=======
    m_columnView = new ColumnViewWidget(this);
    connect(m_columnView, &ColumnViewWidget::pathNavigated, this, [this](const QString& path) {
        emit pathNavigated(path);
    });
>>>>>>> REPLACE
```

## 4. Build & Verification Steps

1. Configure build system:
   `cmake -B build -G "Ninja"`
2. Compile application:
   `cmake --build build`
3. Launch QuarkMeta and test Column View:
   - Switch to Column View via status bar or top menu while in a deep path `/A/B/C/D`.
   - Verify that ancestor columns (`/A`, `/B`, `/C`, `/D`) cascade horizontally.
   - Verify that clicking a folder in any column appends a sub-column and updates the top address bar (`AddressBar`) and `NavigationService`.
   - Verify smooth horizontal scrolling to the newest column.

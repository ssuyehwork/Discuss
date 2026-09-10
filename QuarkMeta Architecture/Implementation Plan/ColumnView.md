# Column View (Miller Columns) Implementation Plan (ColumnView.md)

## 1. Overview
This implementation plan introduces **Column View (Miller Columns)** as the 4th parallel layout mode inside `ContentPanel` (alongside Grid, List, and Justified modes).

Column View provides classic macOS Finder multi-column drill-down navigation:
- Clicking a folder in column $i$ automatically truncates sub-columns $k > i$ and appends a new column for the clicked folder path.
- Clicking a non-folder file in column $i$ appends a file preview column displaying thumbnail and metadata.
- Each column (`ColumnViewPane`) maintains its own lightweight `DiskItemModel` and `FilterProxyModel`, asynchronously populated via `DiskScanService::scanDirectory` without interfering with `ContentPanel`'s primary data model.

## 2. Modified & Created Files List
- `src/ui/ColumnViewWidget.h` (New File)
- `src/ui/ColumnViewWidget.cpp` (New File)
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`
- `CMakeLists.txt`

## 3. Detailed Line-by-Line Changes

### 3.1 Register New Files in `CMakeLists.txt`
Add `ColumnViewWidget.h` and `ColumnViewWidget.cpp` to `CMakeLists.txt`.

```git
<<<<<<< SEARCH
    src/ui/DropListView.h
    src/ui/DropListView.cpp
=======
    src/ui/DropListView.h
    src/ui/DropListView.cpp
    src/ui/ColumnViewWidget.h
    src/ui/ColumnViewWidget.cpp
>>>>>>> REPLACE
```

### 3.2 Add `ViewModeColumn` to `ContentPanel.h`
Add `ViewModeColumn` to `ContentPanel::ViewMode` enum and declare `m_columnView`.

```git
<<<<<<< SEARCH
    enum ViewMode {
        ViewModeGrid,
        ViewModeList,
        ViewModeJustified
    };
=======
    enum ViewMode {
        ViewModeGrid,
        ViewModeList,
        ViewModeJustified,
        ViewModeColumn
    };
>>>>>>> REPLACE
```

```git
<<<<<<< SEARCH
    DropJustifiedView* m_gridView = nullptr;
    DropTreeView*      m_treeView = nullptr;
=======
    DropJustifiedView* m_gridView = nullptr;
    DropTreeView*      m_treeView = nullptr;
    ColumnViewWidget*  m_columnView = nullptr;
>>>>>>> REPLACE
```

### 3.3 Create `src/ui/ColumnViewWidget.h`

```cpp
#pragma once

#include <QScrollArea>
#include <QHBoxLayout>
#include <QListView>
#include <QLabel>
#include <QPointer>
#include "models/DiskItemModel.h"
#include "models/FilterProxyModel.h"

namespace QuarkMeta {

class ColumnViewPane : QWidget {
    Q_OBJECT
public:
    explicit ColumnViewPane(const QString& path, QWidget* parent = nullptr);
    ~ColumnViewPane() override = default;

    QString currentPath() const { return m_path; }
    void loadDirectory();

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

class ColumnViewWidget : public QScrollArea {
    Q_OBJECT
public:
    explicit ColumnViewWidget(QWidget* parent = nullptr);
    ~ColumnViewWidget() override = default;

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

} // namespace QuarkMeta
```

### 3.4 Create `src/ui/ColumnViewWidget.cpp`

```cpp
#include "ColumnViewWidget.h"
#include "../core/DiskScanService.h"
#include "UiHelper.h"
#include <QFileInfo>
#include <QtConcurrent/QtConcurrent>
#include <QCoreApplication>

namespace QuarkMeta {

ColumnViewPane::ColumnViewPane(const QString& path, QWidget* parent)
    : QWidget(parent), m_path(path)
{
    setFixedWidth(230);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    m_titleLabel = new QLabel(QFileInfo(path).fileName().isEmpty() ? path : QFileInfo(path).fileName());
    m_titleLabel->setStyleSheet("font-weight: bold; padding: 4px; color: #EEEEEE; background: #252526;");
    layout->addWidget(m_titleLabel);

    m_model = new DiskItemModel(this);
    m_proxyModel = new FilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);

    m_listView = new QListView(this);
    m_listView->setModel(m_proxyModel);
    m_listView->setStyleSheet("QListView { background: #1E1E1E; border: 1px solid #333333; color: #CCCCCC; }"
                              "QListView::item:selected { background: #3E3E42; color: #FFFFFF; }");
    layout->addWidget(m_listView);

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

void ColumnViewPane::loadDirectory() {
    QString path = m_path;
    QPointer<ColumnViewPane> weakSelf(this);
    (void)QtConcurrent::run([weakSelf, path]() {
        if (!weakSelf) return;
        std::vector<ItemRecord> items = DiskScanService::scanDirectory(path, false, nullptr);
        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakSelf, items]() {
            if (weakSelf && weakSelf->m_model) {
                weakSelf->m_model->setRecords(items);
            }
        });
    });
}

ColumnViewWidget::ColumnViewWidget(QWidget* parent)
    : QScrollArea(parent)
{
    setWidgetResizable(true);
    setStyleSheet("QScrollArea { background: #181818; border: none; }");

    m_container = new QWidget(this);
    m_layout = new QHBoxLayout(m_container);
    m_layout->setContentsMargins(4, 4, 4, 4);
    m_layout->setSpacing(4);
    m_layout->addStretch();

    setWidget(m_container);
}

void ColumnViewWidget::setRootPath(const QString& path) {
    clearAllColumns();
    appendColumn(path);
}

void ColumnViewWidget::clearAllColumns() {
    dismissSubColumns(-1);
}

void ColumnViewWidget::dismissSubColumns(int fromIndex) {
    while (m_panes.size() > fromIndex + 1) {
        ColumnViewPane* pane = m_panes.takeLast();
        m_layout->removeWidget(pane);
        pane->deleteLater();
    }
}

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

} // namespace QuarkMeta
```

## 4. Build & Verification Steps
1. **Compilation Verification**:
   ```bash
   cmake --build build
   ```
2. **Functional Verification**:
   - Toggle layout mode to Column View in `ContentPanel`.
   - Click a folder in column 1 -> Column 2 smoothly appends on the right.
   - Click a different folder in column 1 -> Column 2 is replaced, deeper columns are truncated.

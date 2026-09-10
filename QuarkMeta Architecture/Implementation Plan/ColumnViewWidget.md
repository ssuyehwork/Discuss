# Implementation Plan - ColumnViewWidget: Clean UI Header Removal, Flexible Column Fill & TreeItemDelegate Integration

## 1. Overview
The user provided a screenshot demonstrating two severe UI defects in the column view:
1. **Redundant Header Label Box (`G:/`)**: `ColumnViewPane` instantiated an ugly `QLabel` header (`m_titleLabel`) with a dark background (`#252526`), creating visual clutter and duplicating the top `AddressBar`.
2. **Huge Blank Dark Space on the Right**: `ColumnViewPane` had a rigid `setFixedWidth(230)` constraint with an `addStretch()` layout in `ColumnViewWidget`. When only 1 or 2 columns were open, it created a massive black void on the right instead of gracefully filling the container.

This plan details the complete UI refactoring of `ColumnViewWidget` and `ColumnViewPane`:
- Completely strip out `m_titleLabel` from `ColumnViewPane`.
- Replace rigid `setFixedWidth(230)` with `setMinimumWidth(220)` and flexible layout management.
- Attach `TreeItemDelegate` to each column's `QListView` for unified SVG icons and hover/selection styling (`#3E3E42`).
- Support recursive ancestor path stack expansion in `setRootPath()`.
- Propagate `pathNavigated` to `ContentPanel` and `NavigationService`.

## 2. Modified Files List
- `src/ui/ColumnViewWidget.h`
- `src/ui/ColumnViewWidget.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/ColumnViewWidget.h`

```
<<<<<<< SEARCH
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
    void updatePaneWidths();

    QWidget* m_container = nullptr;
    QHBoxLayout* m_layout = nullptr;
    QList<ColumnViewPane*> m_panes;
};
>>>>>>> REPLACE
```

### `src/ui/ColumnViewWidget.cpp`

```
<<<<<<< SEARCH
#include "ColumnViewWidget.h"
#include "../core/DiskScanService.h"
#include "UiHelper.h"
#include <QFileInfo>
#include <QVBoxLayout>
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

    m_titleLabel = new QLabel(QFileInfo(path).fileName().isEmpty() ? path : QFileInfo(path).fileName(), this);
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
=======
#include "ColumnViewWidget.h"
#include "../core/DiskScanService.h"
#include "TreeItemDelegate.h"
#include "UiHelper.h"
#include <QFileInfo>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrent>
#include <QCoreApplication>
#include <QDir>

namespace QuarkMeta {

ColumnViewPane::ColumnViewPane(const QString& path, QWidget* parent)
    : QWidget(parent), m_path(path)
{
    setMinimumWidth(220);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_model = new DiskItemModel(this);
    m_proxyModel = new FilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);

    m_listView = new QListView(this);
    m_listView->setModel(m_proxyModel);
    m_listView->setItemDelegate(new TreeItemDelegate(this, false, false));
    m_listView->setStyleSheet("QListView { background: #1E1E1E; border: none; border-right: 1px solid #2D2D2D; color: #CCCCCC; }"
                              "QListView::item:selected { background: #3E3E42; color: #FFFFFF; }");
    layout->addWidget(m_listView);
>>>>>>> REPLACE
```

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

    for (int i = 0; i < pathStack.size(); ++i) {
        const QString& p = pathStack[i];
        ColumnViewPane* pane = appendColumn(p);
        if (i > 0 && i - 1 < m_panes.size() - 1) {
            m_panes[i - 1]->selectItemByPath(p);
        }
    }
    updatePaneWidths();
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
=======
void ColumnViewWidget::dismissSubColumns(int fromIndex) {
    while (m_panes.size() > fromIndex + 1) {
        ColumnViewPane* pane = m_panes.takeLast();
        m_layout->removeWidget(pane);
        pane->deleteLater();
    }
    updatePaneWidths();
}

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
    updatePaneWidths();
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

void ColumnViewWidget::updatePaneWidths() {
    if (m_panes.isEmpty()) return;
    int availableWidth = width();
    if (availableWidth <= 0) availableWidth = 800;

    int colCount = m_panes.size();
    int defaultWidth = 230;

    if (colCount * defaultWidth < availableWidth) {
        // Distribute remaining space to the last active column so there is no dark void
        for (int i = 0; i < colCount - 1; ++i) {
            m_panes[i]->setFixedWidth(defaultWidth);
        }
        m_panes.last()->setMinimumWidth(availableWidth - (colCount - 1) * defaultWidth - 10);
        m_panes.last()->setMaximumWidth(QWIDGETSIZE_MAX);
    } else {
        for (auto* pane : m_panes) {
            pane->setFixedWidth(defaultWidth);
        }
    }
}
>>>>>>> REPLACE
```

## 4. Build & Verification Steps

1. Configure build system:
   `cmake -B build -G "Ninja"`
2. Compile project:
   `cmake --build build`
3. Launch QuarkMeta and test Column View:
   - Verify that no ugly `G:/` label header appears above the list.
   - Verify that when opening Column View with 1 column, the column fills the width gracefully without leaving huge black gaps on the right.
   - Verify that clicking folders cascades columns horizontally with vector icons and smooth scrolling.

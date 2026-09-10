# Implementation Plan - ColumnViewWidget: Cascading Multi-Column Navigation, Ancestor Stack Reconstruction & Arrow Indicator

## 1. Overview
Based on the reference screenshot provided by the user, the ideal Column View experience requires:
1. **Ancestor Path Stack Reconstruction**: When navigating to a deep path (e.g. `C:\Users\xiaoinfx\Desktop\ProjectManagement\CustomerA\Project1\Contract`), `ColumnViewWidget::setRootPath()` must reconstruct the full chain of parent columns (`ProjectManagement` ➔ `CustomerA` ➔ `Project1` ➔ `Contract` ➔ `File List`), cascading horizontally.
2. **Parent Item Selection & Right Arrow (`>`) Indicator**: In each parent column, the selected folder must be visually highlighted with a right-arrow `>` indicator at the trailing edge, explicitly signaling that its contents are expanded in the next column.
3. **No Redundant Header Labels**: Remove the top ugly `QLabel` header (`m_titleLabel`) from `ColumnViewPane` entirely so that columns seamlessly blend into the content pane.
4. **Adaptive Width & Smooth Auto-Scrolling**: Expand columns horizontally without black empty gaps on the right, automatically scrolling to ensure the active/newest column is visible.

This plan details the code changes in `ColumnViewWidget.h`, `ColumnViewWidget.cpp`, and `TreeItemDelegate.h`.

## 2. Modified Files List
- `src/ui/ColumnViewWidget.h`
- `src/ui/ColumnViewWidget.cpp`
- `src/ui/TreeItemDelegate.h`
- `src/ui/ContentPanel.cpp`

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

    // 1. Deconstruct full ancestor path stack
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

    // 2. Cascade columns recursively and highlight child items in parent columns
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

### `src/ui/TreeItemDelegate.h`

```
<<<<<<< SEARCH
        // 5. 画文案
        QRect textRect = rect;
        textRect.setLeft(iconRect.right() + 6);
        painter->setPen(isSelected ? QColor("#FFFFFF") : QColor("#CCCCCC"));
        QString text = painter->fontMetrics().elidedText(index.data(Qt::DisplayRole).toString(), Qt::ElideRight, textRect.width());
        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, text);
=======
        // 5. 画文案
        QRect textRect = rect;
        textRect.setLeft(iconRect.right() + 6);
        if (isDir) {
            textRect.setRight(rect.right() - 18);
        }
        painter->setPen(isSelected ? QColor("#FFFFFF") : QColor("#CCCCCC"));
        QString text = painter->fontMetrics().elidedText(index.data(Qt::DisplayRole).toString(), Qt::ElideRight, textRect.width());
        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, text);

        // 6. 如果是文件夹，在最右侧画 trailing 箭头指示器 (>)
        if (isDir) {
            QRect arrowRect(rect.right() - 16, rect.top(), 12, rect.height());
            painter->setPen(isSelected ? QColor("#FFFFFF") : QColor("#888888"));
            painter->drawText(arrowRect, Qt::AlignCenter, ">");
        }
>>>>>>> REPLACE
```

## 4. Build & Verification Steps

1. Configure build system:
   `cmake -B build -G "Ninja"`
2. Compile project:
   `cmake --build build`
3. Launch QuarkMeta and test Column View:
   - Navigate to a deep directory like `C:\Users\xiaoinfx\Desktop\ProjectManagement\CustomerA\Project1\Contract`.
   - Verify that ancestor columns (`ProjectManagement` ➔ `CustomerA` ➔ `Project1` ➔ `Contract` ➔ `Files`) cascade horizontally like the reference screenshot.
   - Verify that every parent folder displays a trailing `>` arrow indicator at the right edge of its item row.
   - Verify that selected items in parent columns remain highlighted in `#3E3E42`.

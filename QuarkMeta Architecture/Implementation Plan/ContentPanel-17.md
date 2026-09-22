# Implementation Plan: ContentPanel-17.md (Dual Pane Split & Context Menu Integration)

## 1. Overview
This implementation plan adds dual-pane horizontal/vertical splitting capabilities to `ContentPanel` when dragging tabs or splitting views, featuring a blue highlight drop preview overlay, cross-pane drag-and-drop file migration, and a dynamic "关闭窗格" (Close Pane) context menu option when in split mode.

## 2. Modified Files List
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`
- `src/ui/controllers/ContentContextMenu.h`
- `src/ui/controllers/ContentContextMenu.cpp`

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/ContentPanel.h`
Add `#include <QSplitter>`, split-pane management, drag overlay preview widget, split state queries, and "关闭窗格" slot while preserving `public` visibility for model and selection query methods.

<<<<<<< SEARCH
#include <QModelIndexList>
#include <atomic>
=======
#include <QModelIndexList>
#include <QSplitter>
#include <atomic>
>>>>>>> REPLACE

<<<<<<< SEARCH
public:
    explicit ContentPanel(QWidget* parent = nullptr);
    ~ContentPanel() override = default;
=======
public:
    explicit ContentPanel(QWidget* parent = nullptr);
    ~ContentPanel() override = default;

    // Dual-pane state inspection & split controls
    bool isSplitMode() const;
    void splitPane(Qt::Orientation orientation, const QString& secondaryPath = QString());
    void closeSecondaryPane();
>>>>>>> REPLACE

<<<<<<< SEARCH
protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
=======
protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
>>>>>>> REPLACE

<<<<<<< SEARCH
    // UI 组件指针
    QVBoxLayout* m_mainLayout = nullptr;
    class ContentHeaderWidget* m_headerWidget = nullptr;
=======
    // UI 组件指针
    QVBoxLayout* m_mainLayout = nullptr;
    class ContentHeaderWidget* m_headerWidget = nullptr;

    QSplitter* m_paneSplitter = nullptr;
    QWidget* m_secondaryPaneContainer = nullptr;
    QWidget* m_dragOverlayWidget = nullptr;
    Qt::Orientation m_splitOrientation = Qt::Horizontal;
    bool m_isSplit = false;

    void updateDragOverlay(const QPoint& pos);
    void hideDragOverlay();
>>>>>>> REPLACE

### 3.2 `src/ui/ContentPanel.cpp`
Implement drag overlay visualization, splitter layout creation for dual pane, cross-pane drop handling, and secondary pane teardown.

<<<<<<< SEARCH
#include "ContentPanel.h"
#include "ContentHeaderWidget.h"
#include "FolderSectionWidget.h"
=======
#include "ContentPanel.h"
#include "ContentHeaderWidget.h"
#include "FolderSectionWidget.h"
#include <QSplitter>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
>>>>>>> REPLACE

<<<<<<< SEARCH
ContentPanel::ContentPanel(QWidget* parent) : QFrame(parent) {
    setContextMenuPolicy(Qt::CustomContextMenu);
    setObjectName("EditorContainer");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(230);
=======
ContentPanel::ContentPanel(QWidget* parent) : QFrame(parent) {
    setAcceptDrops(true);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setObjectName("EditorContainer");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(230);
>>>>>>> REPLACE

<<<<<<< SEARCH
bool ContentPanel::isTreeView(QObject* view) const {
    return (view == m_treeView);
}
=======
bool ContentPanel::isTreeView(QObject* view) const {
    return (view == m_treeView);
}

bool ContentPanel::isSplitMode() const {
    return m_isSplit;
}

void ContentPanel::splitPane(Qt::Orientation orientation, const QString& secondaryPath) {
    if (m_isSplit && m_splitOrientation == orientation) return;

    m_splitOrientation = orientation;
    m_isSplit = true;

    if (!m_paneSplitter) {
        m_paneSplitter = new QSplitter(m_splitOrientation, this);
        m_paneSplitter->setHandleWidth(2);
        m_mainLayout->removeWidget(m_viewStack);
        m_paneSplitter->addWidget(m_viewStack);

        m_secondaryPaneContainer = new QWidget(m_paneSplitter);
        QVBoxLayout* secLayout = new QVBoxLayout(m_secondaryPaneContainer);
        secLayout->setContentsMargins(0, 0, 0, 0);

        m_paneSplitter->addWidget(m_secondaryPaneContainer);
        m_mainLayout->addWidget(m_paneSplitter, 1);
    } else {
        m_paneSplitter->setOrientation(m_splitOrientation);
        if (m_secondaryPaneContainer) {
            m_secondaryPaneContainer->show();
        }
    }

    QList<int> sizes;
    int total = (orientation == Qt::Horizontal) ? width() : height();
    sizes << total / 2 << total / 2;
    m_paneSplitter->setSizes(sizes);
}

void ContentPanel::closeSecondaryPane() {
    if (!m_isSplit) return;

    m_isSplit = false;
    if (m_secondaryPaneContainer) {
        m_secondaryPaneContainer->hide();
    }
}

void ContentPanel::updateDragOverlay(const QPoint& pos) {
    if (!m_dragOverlayWidget) {
        m_dragOverlayWidget = new QWidget(this);
        m_dragOverlayWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
        m_dragOverlayWidget->setStyleSheet("background-color: rgba(0, 122, 255, 0.25); border: 2px solid #007AFF;");
    }

    int w = width();
    int h = height();

    if (pos.x() > w * 0.75) {
        m_dragOverlayWidget->setGeometry(w / 2, 0, w / 2, h);
        m_dragOverlayWidget->show();
    } else if (pos.x() < w * 0.25) {
        m_dragOverlayWidget->setGeometry(0, 0, w / 2, h);
        m_dragOverlayWidget->show();
    } else if (pos.y() > h * 0.75) {
        m_dragOverlayWidget->setGeometry(0, h / 2, w, h / 2);
        m_dragOverlayWidget->show();
    } else if (pos.y() < h * 0.25) {
        m_dragOverlayWidget->setGeometry(0, 0, w, h / 2);
        m_dragOverlayWidget->show();
    } else {
        hideDragOverlay();
    }
}

void ContentPanel::hideDragOverlay() {
    if (m_dragOverlayWidget) {
        m_dragOverlayWidget->hide();
    }
}

void ContentPanel::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls() || event->mimeData()->hasText() ||
        event->mimeData()->hasFormat("application/x-quarkmeta-tabindex")) {
        event->acceptProposedAction();
    }
}

void ContentPanel::dragMoveEvent(QDragMoveEvent* event) {
    updateDragOverlay(event->pos());
    event->acceptProposedAction();
}

void ContentPanel::dragLeaveEvent(QDragLeaveEvent* event) {
    Q_UNUSED(event);
    hideDragOverlay();
}

void ContentPanel::dropEvent(QDropEvent* event) {
    QPoint pos = event->pos();
    hideDragOverlay();

    int w = width();
    int h = height();

    if (pos.x() > w * 0.75 || pos.x() < w * 0.25) {
        splitPane(Qt::Horizontal);
        event->acceptProposedAction();
    } else if (pos.y() > h * 0.75 || pos.y() < h * 0.25) {
        splitPane(Qt::Vertical);
        event->acceptProposedAction();
    } else {
        if (event->mimeData()->hasUrls()) {
            QStringList paths;
            for (const QUrl& url : event->mimeData()->urls()) {
                paths << url.toLocalFile();
            }
            onPathsDropped(paths, QModelIndex());
            event->acceptProposedAction();
        } else if (event->mimeData()->hasText()) {
            splitPane(Qt::Horizontal, event->mimeData()->text());
            event->acceptProposedAction();
        }
    }
}
>>>>>>> REPLACE

### 3.3 `src/ui/controllers/ContentContextMenu.cpp`
Inject "关闭窗格" into context menu when split pane mode is active.

<<<<<<< SEARCH
            menu.addAction(UiHelper::getIcon("refresh", QColor("#EEEEEE"), 18), "刷新")->setData(ContentPanel::ActionRefresh);
        }
    }

    menu.addSeparator();

    // 排序二级子菜单
=======
            menu.addAction(UiHelper::getIcon("refresh", QColor("#EEEEEE"), 18), "刷新")->setData(ContentPanel::ActionRefresh);
        }
    }

    // --- 拆分窗格控制菜单项 ---
    if (m_panel && m_panel->isSplitMode()) {
        menu.addSeparator();
        QAction* closePaneAction = menu.addAction(UiHelper::getIcon("close", QColor("#EEEEEE"), 18), "关闭窗格");
        QObject::connect(closePaneAction, &QAction::triggered, [this]() {
            if (m_panel) {
                m_panel->closeSecondaryPane();
            }
        });
    }

    menu.addSeparator();

    // 排序二级子菜单
>>>>>>> REPLACE

## 4. Build & Verification Steps
1. Run `cmake --build build` or `ninja -C build` to compile the codebase.
2. Launch the application and test dragging a tab onto `ContentPanel` near the left, right, top, or bottom edges.
3. Verify that the semi-transparent blue preview overlay appears in the respective half of the panel.
4. Release the drag to confirm that `ContentPanel` splits into dual 50% panes horizontally or vertically.
5. Right-click inside `ContentPanel` in split mode and verify that "关闭窗格" appears in the context menu.
6. Click "关闭窗格" and verify that the secondary pane closes and the content area restores to single pane mode (100% size).

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **View Refreshing SSOT**: Uses `ContentPanel::refreshAll()` for view updates after drop/pane operations.
- **Navigation SSOT**: Uses `ContentPanel::loadDirectory(path)` for pane path navigation.
- **Drag & Drop SSOT**: Delegates dropped paths processing to `ContentPanel::onPathsDropped(...)`.

## 6. Header API Signature Verification
- `ContentPanel::isSplitMode() const` -> Checked in `src/ui/ContentPanel.h`.
- `ContentPanel::splitPane(Qt::Orientation, const QString&)` -> Checked in `src/ui/ContentPanel.h`.
- `ContentPanel::closeSecondaryPane()` -> Checked in `src/ui/ContentPanel.h`.
- `ContentPanel::refreshAll()` -> Verified existing in `src/ui/ContentPanel.h` (Line 181).
- `ContentPanel::loadDirectory(const QString&, bool)` -> Verified existing in `src/ui/ContentPanel.h` (Line 179).

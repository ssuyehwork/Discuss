# Implementation Plan - MetaPanel & FilterPanel Drag-and-Drop Marked Transfer Entry

## 1. Overview & Goal
This plan adds a 2nd drag-and-drop entry point for transferring items to marked destination folders by dropping selected files onto `MetaPanel` or `FilterPanel`.
Key requirements:
1. Panel-wide drag takeover: Sub-controls (line edits, buttons, scroll area viewports, checkboxes, etc.) must NOT swallow drag events.
2. Drag overlay mask (`#DragOverlayWidget` styling `rgba(0, 122, 255, 0.25)` background, `2px solid #007AFF` border):
   - Active marked count $N > 0$: Text "迁移到 N 个标记文件夹", cursor accepts drop.
   - $N = 0$: Text "尚未标记目的地", cursor shows forbidden (`Qt::IgnoreAction`).
   - Overlay immediately disappears on `DragLeave` or `Drop`.
3. Drop dispatch:
   - When marked folders exist, pass `sourcePaths` from `mimeData` directly into `MarkedFolderService::instance().executeTransferToMarked(sourcePaths)`.
   - When no marked folders exist, issue Toast `"尚未标记目的地，请先标记文件夹"`.

---

## 2. Affected Files
1. `src/ui/MetaPanel.h`
2. `src/ui/MetaPanel.cpp`
3. `src/ui/FilterPanel.h`
4. `src/ui/FilterPanel.cpp`

---

## 3. Detailed Changes & Search/Replace Blocks

### 3.1 `src/ui/MetaPanel.h`
Declare drag event handlers, overlay management helper, and recursive child event filter installation in `MetaPanel`.

```
<<<<<<< SEARCH
protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
=======
protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
>>>>>>> REPLACE
```

Declare overlay helper methods and members in `MetaPanel`:

```
<<<<<<< SEARCH
    QSet<QString> m_pendingRemovedTags;
    QStringList m_tagRemovalTargetSnapshot;
    QTimer* m_tagRemoveDebounceTimer = nullptr;
=======
    QSet<QString> m_pendingRemovedTags;
    QStringList m_tagRemovalTargetSnapshot;
    QTimer* m_tagRemoveDebounceTimer = nullptr;

    QWidget* m_dragOverlayWidget = nullptr;
    QLabel* m_dragOverlayLabel = nullptr;
    void showDragOverlay(int activeCount);
    void hideDragOverlay();
    void installRecursiveDragEventFilter(QWidget* widget);
>>>>>>> REPLACE
```

### 3.2 `src/ui/MetaPanel.cpp`
Include mandatory headers and enable `setAcceptDrops(true)` in `MetaPanel::initUi`.

```
<<<<<<< SEARCH
#include "MetaPanel.h"
#include "ColorPicker.h"
=======
#include "MetaPanel.h"
#include "ColorPicker.h"
#include "../meta/MarkedFolderService.h"
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void MetaPanel::initUi() {
    QWidget* header = new QWidget(this);
=======
void MetaPanel::initUi() {
    setAcceptDrops(true);
    QWidget* header = new QWidget(this);
>>>>>>> REPLACE
```

Implement `showDragOverlay`, `hideDragOverlay`, and `installRecursiveDragEventFilter` in `MetaPanel.cpp`:

```
<<<<<<< SEARCH
void MetaPanel::resizeEvent(QResizeEvent* event) {
    QFrame::resizeEvent(event);
=======
void MetaPanel::showDragOverlay(int activeCount) {
    if (!m_dragOverlayWidget) {
        m_dragOverlayWidget = new QWidget(this);
        m_dragOverlayWidget->setObjectName("DragOverlayWidget");
        m_dragOverlayWidget->setAttribute(Qt::WA_TransparentForMouseEvents);

        QVBoxLayout* layout = new QVBoxLayout(m_dragOverlayWidget);
        layout->setContentsMargins(0, 0, 0, 0);
        m_dragOverlayLabel = new QLabel(m_dragOverlayWidget);
        m_dragOverlayLabel->setAlignment(Qt::AlignCenter);
        m_dragOverlayLabel->setStyleSheet("color: #FFFFFF; font-size: 14px; font-weight: bold;");
        layout->addWidget(m_dragOverlayLabel);
    }

    if (activeCount > 0) {
        m_dragOverlayLabel->setText(QString("迁移到 %1 个标记文件夹").arg(activeCount));
    } else {
        m_dragOverlayLabel->setText("尚未标记目的地");
    }

    m_dragOverlayWidget->setGeometry(rect());
    m_dragOverlayWidget->show();
    m_dragOverlayWidget->raise();
}

void MetaPanel::hideDragOverlay() {
    if (m_dragOverlayWidget) {
        m_dragOverlayWidget->hide();
    }
}

void MetaPanel::installRecursiveDragEventFilter(QWidget* widget) {
    if (!widget) return;
    widget->setAcceptDrops(true);
    widget->installEventFilter(this);
    for (QObject* child : widget->children()) {
        if (QWidget* childWidget = qobject_cast<QWidget*>(child)) {
            installRecursiveDragEventFilter(childWidget);
        }
    }
}

void MetaPanel::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData() && event->mimeData()->hasUrls()) {
        int activeCount = MarkedFolderService::instance().getActiveMarked().size();
        showDragOverlay(activeCount);
        if (activeCount > 0) {
            event->acceptProposedAction();
        } else {
            event->ignore();
        }
        return;
    }
    QFrame::dragEnterEvent(event);
}

void MetaPanel::dragMoveEvent(QDragMoveEvent* event) {
    if (event->mimeData() && event->mimeData()->hasUrls()) {
        int activeCount = MarkedFolderService::instance().getActiveMarked().size();
        showDragOverlay(activeCount);
        if (activeCount > 0) {
            event->acceptProposedAction();
        } else {
            event->ignore();
        }
        return;
    }
    QFrame::dragMoveEvent(event);
}

void MetaPanel::dragLeaveEvent(QDragLeaveEvent* event) {
    hideDragOverlay();
    QFrame::dragLeaveEvent(event);
}

void MetaPanel::dropEvent(QDropEvent* event) {
    hideDragOverlay();
    if (event->mimeData() && event->mimeData()->hasUrls()) {
        QStringList paths;
        for (const QUrl& url : event->mimeData()->urls()) {
            QString localPath = url.toLocalFile();
            if (!localPath.isEmpty()) {
                paths.append(localPath);
            }
        }

        int activeCount = MarkedFolderService::instance().getActiveMarked().size();
        if (activeCount > 0) {
            MarkedFolderService::instance().executeTransferToMarked(paths);
            event->acceptProposedAction();
        } else {
            ToolTipOverlay::instance()->showToast("尚未标记目的地，请先标记文件夹", 2500, QColor("#e81123"));
            event->ignore();
        }
        return;
    }
    QFrame::dropEvent(event);
}

void MetaPanel::resizeEvent(QResizeEvent* event) {
    QFrame::resizeEvent(event);
    if (m_dragOverlayWidget && m_dragOverlayWidget->isVisible()) {
        m_dragOverlayWidget->setGeometry(rect());
    }
>>>>>>> REPLACE
```

Update `eventFilter` in `MetaPanel.cpp` to forward drag events from sub-controls:

```
<<<<<<< SEARCH
bool MetaPanel::eventFilter(QObject* watched, QEvent* event) {
=======
bool MetaPanel::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::DragEnter) {
        auto* de = static_cast<QDragEnterEvent*>(event);
        if (de->mimeData() && de->mimeData()->hasUrls()) {
            dragEnterEvent(de);
            return true;
        }
    } else if (event->type() == QEvent::DragMove) {
        auto* dm = static_cast<QDragMoveEvent*>(event);
        if (dm->mimeData() && dm->mimeData()->hasUrls()) {
            dragMoveEvent(dm);
            return true;
        }
    } else if (event->type() == QEvent::DragLeave) {
        dragLeaveEvent(static_cast<QDragLeaveEvent*>(event));
        return true;
    } else if (event->type() == QEvent::Drop) {
        auto* dropEv = static_cast<QDropEvent*>(event);
        if (dropEv->mimeData() && dropEv->mimeData()->hasUrls()) {
            dropEvent(dropEv);
            return true;
        }
    }
>>>>>>> REPLACE
```

---

## 4. Verification Plan
1. Check CMake build for compilation safety.
2. Drag items onto `MetaPanel` and `FilterPanel`: verify overlay mask displays with exact active marked count text.
3. Drop items onto `MetaPanel`: verify `executeTransferToMarked` is invoked and task progress bar displays.

# Implementation Plan - Tab Drag & Drop Reordering

This plan implements mouse drag-and-drop reordering for tabs in `TabBarWidget`, allowing users to drag tabs left or right to reorder them, with immediate UI updates and persistence to `AppConfig`.

## Overview
1. **Drag Initiation in `TabItemButton`**:
   - In `TabItemButton::mousePressEvent`, store `m_dragStartPos = event->pos()`.
   - In `TabItemButton::mouseMoveEvent`, if the left mouse button is pressed and moved beyond `QApplication::startDragDistance()`, create a `QDrag` containing `QMimeData` with custom format `application/x-quarkmeta-tabindex`.
2. **Drop & Reordering in `TabBarWidget`**:
   - Enable `dragMoveEvent` on `TabBarWidget`.
   - In `TabBarWidget::dragEnterEvent` & `dragMoveEvent`, accept drops with format `application/x-quarkmeta-tabindex` (or file URLs).
   - In `TabBarWidget::dropEvent`, if format is `application/x-quarkmeta-tabindex`:
     - Extract `fromIndex`.
     - Determine `toIndex` by finding which tab widget's geometry intersects the drop horizontal position `event->pos()`.
     - Move `m_tabs[fromIndex]` to `toIndex`.
     - Update `m_currentIndex` so the active tab remains active.
     - Call `rebuildTabsUi()` and `saveStateToConfig()`.

## Modified Files List
- `src/ui/TabBarWidget.h`
- `src/ui/TabBarWidget.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/TabBarWidget.h`
<<<<<<< SEARCH
protected:
    void mousePressEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    int m_index = -1;
=======
protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    int m_index = -1;
    QPoint m_dragStartPos;
>>>>>>> REPLACE

<<<<<<< SEARCH
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
=======
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
>>>>>>> REPLACE

### 2. `src/ui/TabBarWidget.cpp`
<<<<<<< SEARCH
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
=======
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QDrag>
#include <QMimeData>
#include <QUrl>
#include <QApplication>
>>>>>>> REPLACE

<<<<<<< SEARCH
void TabItemButton::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit tabClicked(m_index);
        event->accept();
        return;
    } else if (event->button() == Qt::MiddleButton) {
        emit middleClicked(m_index);
        event->accept();
        return;
    }
    QPushButton::mousePressEvent(event);
}
=======
void TabItemButton::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragStartPos = event->pos();
        emit tabClicked(m_index);
        event->accept();
        return;
    } else if (event->button() == Qt::MiddleButton) {
        emit middleClicked(m_index);
        event->accept();
        return;
    }
    QPushButton::mousePressEvent(event);
}

void TabItemButton::mouseMoveEvent(QMouseEvent* event) {
    if ((event->buttons() & Qt::LeftButton) && !m_dragStartPos.isNull()) {
        if ((event->pos() - m_dragStartPos).manhattanLength() >= QApplication::startDragDistance()) {
            QDrag* drag = new QDrag(this);
            QMimeData* mimeData = new QMimeData();
            mimeData->setData("application/x-quarkmeta-tabindex", QByteArray::number(m_index));
            drag->setMimeData(mimeData);

            QPixmap pixmap = grab();
            drag->setPixmap(pixmap);
            drag->setHotSpot(event->pos());

            drag->exec(Qt::MoveAction);
            m_dragStartPos = QPoint();
            return;
        }
    }
    QPushButton::mouseMoveEvent(event);
}
>>>>>>> REPLACE

<<<<<<< SEARCH
void TabBarWidget::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData() && event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        QWidget::dragEnterEvent(event);
    }
}

void TabBarWidget::dropEvent(QDropEvent* event) {
    if (event->mimeData() && event->mimeData()->hasUrls()) {
        for (const QUrl& url : event->mimeData()->urls()) {
            QString path = url.toLocalFile();
            if (!path.isEmpty() && QFileInfo(path).isDir()) {
                openOrFocusTab(path);
                event->acceptProposedAction();
                return;
            }
        }
    }
    QWidget::dropEvent(event);
}
=======
void TabBarWidget::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData() && (event->mimeData()->hasFormat("application/x-quarkmeta-tabindex") || event->mimeData()->hasUrls())) {
        event->acceptProposedAction();
    } else {
        QWidget::dragEnterEvent(event);
    }
}

void TabBarWidget::dragMoveEvent(QDragMoveEvent* event) {
    if (event->mimeData() && (event->mimeData()->hasFormat("application/x-quarkmeta-tabindex") || event->mimeData()->hasUrls())) {
        event->acceptProposedAction();
    } else {
        QWidget::dragMoveEvent(event);
    }
}

void TabBarWidget::dropEvent(QDropEvent* event) {
    if (event->mimeData()) {
        if (event->mimeData()->hasFormat("application/x-quarkmeta-tabindex")) {
            int fromIdx = event->mimeData()->data("application/x-quarkmeta-tabindex").toInt();
            if (fromIdx >= 0 && fromIdx < m_tabs.size()) {
                QPoint dropPos = event->pos();
                int toIdx = m_tabs.size() - 1;
                for (int i = 0; i < m_tabWidgets.size(); ++i) {
                    QRect rect = m_tabWidgets[i]->geometry();
                    if (dropPos.x() < rect.center().x()) {
                        toIdx = i;
                        break;
                    }
                }

                if (fromIdx != toIdx) {
                    TabInfo movedTab = m_tabs.takeAt(fromIdx);
                    m_tabs.insert(toIdx, movedTab);

                    if (m_currentIndex == fromIdx) {
                        m_currentIndex = toIdx;
                    } else if (m_currentIndex > fromIdx && m_currentIndex <= toIdx) {
                        m_currentIndex--;
                    } else if (m_currentIndex < fromIdx && m_currentIndex >= toIdx) {
                        m_currentIndex++;
                    }

                    for (int i = 0; i < m_tabs.size(); ++i) {
                        m_tabs[i].active = (i == m_currentIndex);
                    }

                    rebuildTabsUi();
                    saveStateToConfig();
                }
            }
            event->acceptProposedAction();
            return;
        } else if (event->mimeData()->hasUrls()) {
            for (const QUrl& url : event->mimeData()->urls()) {
                QString path = url.toLocalFile();
                if (!path.isEmpty() && QFileInfo(path).isDir()) {
                    openOrFocusTab(path);
                    event->acceptProposedAction();
                    return;
                }
            }
        }
    }
    QWidget::dropEvent(event);
}
>>>>>>> REPLACE

## Build & Verification Steps
1. Build application.
2. Drag tab items horizontally with the left mouse button.
3. Observe tab items changing order and updating layout.
4. Close and relaunch app to verify order is persisted in `AppConfig`.

## SSOT API Reuse & Anti-Redundancy Self-Check
- Reused `rebuildTabsUi()` and `saveStateToConfig()`.

## Header API Signature Verification
- `TabItemButton::mouseMoveEvent(QMouseEvent*)`
- `TabBarWidget::dragMoveEvent(QDragMoveEvent*)`

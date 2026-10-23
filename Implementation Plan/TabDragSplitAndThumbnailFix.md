# Implementation Plan - TabDragSplitAndThumbnailFix.md

## 1. Overview
This implementation plan addresses two core issues in tab dragging and split view interactions:
1. **Highlight and Drop Result Misalignment Fix**:
   - **Single SSOT Region Evaluation**: Introduces `ContentPaneSplitManager::evaluateSplitDrop(QPoint pos, QSize refSize)` as the sole truth source for drop region evaluation across the application. It returns a `SplitEvaluationResult` containing validity (`isValid`), orientation (`Qt::Orientation`), insertion position (`insertBefore`), and the precise highlight rectangle (`highlightRect`).
   - **Unified Evaluation Order**: Evaluates horizontal boundaries first (x < 25% for Left/Front, x > 75% for Right/Rear), then vertical boundaries (y < 25% for Top/Front, y > 75% for Bottom/Rear). Four corners default to horizontal left/right splits. The middle region is marked invalid (`isValid = false`).
   - **Overlay and Drop Synchronization**: Both `updateDragOverlay` and `ContentPanel::dropEvent` strictly invoke `evaluateSplitDrop(pos, size())`. In the middle region, overlay is hidden and `dropEvent` ignores the action without creating a split pane.
   - **Front vs Rear Insertion Support**: Adds `bool insertBefore = false` to `splitPane(...)`. When `insertBefore == true` (Left/Top split), `container`, `newPane`, and wrapper widgets are inserted at `index 0` (`insertWidget(0, container)`, `prepend(container)`, `prepend(newPane)`). When `insertBefore == false` (Right/Bottom split), widgets are appended to the end.
2. **Tab Thumbnail Hotspot Deviation Fix**:
   - **Clamped Initial Press Hotspot**: In `TabItemButton`, records `m_dragStartPos` in `mousePressEvent`. In `mouseMoveEvent`, clamps `m_dragStartPos` to `rect()` (`x` bounded in `[0, width() - 1]`, `y` bounded in `[0, height() - 1]`) and passes the clamped point to `drag->setHotSpot(...)`. This prevents hotspot deviation when dragging rapidly.
   - **Deferred `tabClicked` Emission**: Removes `emit tabClicked` from `mousePressEvent`. Adds `mouseReleaseEvent` to emit `tabClicked` only when left button is released and no drag occurred (`!m_isDragging`).

---

## 2. Modified Files List
- `src/ui/controllers/ContentPaneSplitManager.h` (Declare `SplitEvaluationResult`, `evaluateSplitDrop`, update `splitPane` declaration)
- `src/ui/controllers/ContentPaneSplitManager.cpp` (Implement `evaluateSplitDrop`, update `updateDragOverlay` and `splitPane` for front/rear insertion)
- `src/ui/ContentPanel.h` (Update `splitPane` declaration with `insertBefore = false` default parameter)
- `src/ui/ContentPanel.cpp` (Refactor `dropEvent` to call `evaluateSplitDrop` and check `eval.isValid`)
- `src/ui/TabBarWidget.h` (Add `m_isDragging` flag and declare `mouseReleaseEvent` override in `TabItemButton`)
- `src/ui/TabBarWidget.cpp` (Update `TabItemButton::mousePressEvent`, `mouseMoveEvent`, and implement `mouseReleaseEvent`)

---

## 3. Detailed Line-by-Line Changes

### Change 1: `src/ui/controllers/ContentPaneSplitManager.h`
<<<<<<< SEARCH
    void splitPane(Qt::Orientation orientation, const QString& secondaryPath = QString());
    void closePane(ContentPanel* pane);
=======
    struct SplitEvaluationResult {
        bool isValid = false;
        Qt::Orientation orientation = Qt::Horizontal;
        bool insertBefore = false;
        QRect highlightRect;
    };

    static SplitEvaluationResult evaluateSplitDrop(const QPoint& pos, const QSize& refSize);

    void splitPane(Qt::Orientation orientation, const QString& secondaryPath = QString(), bool insertBefore = false);
    void closePane(ContentPanel* pane);
>>>>>>> REPLACE

### Change 2: `src/ui/controllers/ContentPaneSplitManager.cpp`
<<<<<<< SEARCH
void ContentPaneSplitManager::splitPane(Qt::Orientation orientation, const QString& secondaryPath) {
    if (rootPane() != m_panel) {
        rootPane()->m_splitManager->splitPane(orientation, secondaryPath);
        return;
    }
=======
ContentPaneSplitManager::SplitEvaluationResult ContentPaneSplitManager::evaluateSplitDrop(const QPoint& pos, const QSize& refSize) {
    SplitEvaluationResult res;
    int w = refSize.width();
    int h = refSize.height();
    if (w <= 0 || h <= 0) return res;

    if (pos.x() < w * 0.25) {
        res.isValid = true;
        res.orientation = Qt::Horizontal;
        res.insertBefore = true;
        res.highlightRect = QRect(0, 0, w / 2, h);
    } else if (pos.x() > w * 0.75) {
        res.isValid = true;
        res.orientation = Qt::Horizontal;
        res.insertBefore = false;
        res.highlightRect = QRect(w / 2, 0, w / 2, h);
    } else if (pos.y() < h * 0.25) {
        res.isValid = true;
        res.orientation = Qt::Vertical;
        res.insertBefore = true;
        res.highlightRect = QRect(0, 0, w, h / 2);
    } else if (pos.y() > h * 0.75) {
        res.isValid = true;
        res.orientation = Qt::Vertical;
        res.insertBefore = false;
        res.highlightRect = QRect(0, h / 2, w, h / 2);
    }
    return res;
}

void ContentPaneSplitManager::splitPane(Qt::Orientation orientation, const QString& secondaryPath, bool insertBefore) {
    if (rootPane() != m_panel) {
        rootPane()->m_splitManager->splitPane(orientation, secondaryPath, insertBefore);
        return;
    }
>>>>>>> REPLACE

<<<<<<< SEARCH
    layout->addWidget(newPane);
    m_paneSplitter->addWidget(container);

    m_paneContainers.append(container);
    m_panes.append(newPane);
=======
    layout->addWidget(newPane);
    if (insertBefore) {
        m_paneSplitter->insertWidget(0, container);
        m_paneContainers.prepend(container);
        m_panes.prepend(newPane);
    } else {
        m_paneSplitter->addWidget(container);
        m_paneContainers.append(container);
        m_panes.append(newPane);
    }
>>>>>>> REPLACE

<<<<<<< SEARCH
void ContentPaneSplitManager::updateDragOverlay(const QPoint& pos) {
    if (!m_dragOverlayWidget) {
        m_dragOverlayWidget = new QWidget(m_panel);
        m_dragOverlayWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
        m_dragOverlayWidget->setStyleSheet("background-color: rgba(0, 122, 255, 0.25); border: 2px solid #007AFF;");
    }

    int w = m_panel->width();
    int h = m_panel->height();

    if (pos.x() > w * 0.75) {
        m_dragOverlayWidget->setGeometry(w / 2, 0, w / 2, h);
        m_dragOverlayWidget->show();
        m_dragOverlayWidget->raise();
    } else if (pos.x() < w * 0.25) {
        m_dragOverlayWidget->setGeometry(0, 0, w / 2, h);
        m_dragOverlayWidget->show();
        m_dragOverlayWidget->raise();
    } else if (pos.y() > h * 0.75) {
        m_dragOverlayWidget->setGeometry(0, h / 2, w, h / 2);
        m_dragOverlayWidget->show();
        m_dragOverlayWidget->raise();
    } else if (pos.y() < h * 0.25) {
        m_dragOverlayWidget->setGeometry(0, 0, w, h / 2);
        m_dragOverlayWidget->show();
        m_dragOverlayWidget->raise();
    } else {
        hideDragOverlay();
    }
}
=======
void ContentPaneSplitManager::updateDragOverlay(const QPoint& pos) {
    SplitEvaluationResult eval = evaluateSplitDrop(pos, m_panel->size());
    if (!eval.isValid) {
        hideDragOverlay();
        return;
    }

    if (!m_dragOverlayWidget) {
        m_dragOverlayWidget = new QWidget(m_panel);
        m_dragOverlayWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
        m_dragOverlayWidget->setStyleSheet("background-color: rgba(0, 122, 255, 0.25); border: 2px solid #007AFF;");
    }

    m_dragOverlayWidget->setGeometry(eval.highlightRect);
    m_dragOverlayWidget->show();
    m_dragOverlayWidget->raise();
}
>>>>>>> REPLACE

### Change 3: `src/ui/ContentPanel.h`
<<<<<<< SEARCH
    void splitPane(Qt::Orientation orientation, const QString& secondaryPath = QString());
=======
    void splitPane(Qt::Orientation orientation, const QString& secondaryPath = QString(), bool insertBefore = false);
>>>>>>> REPLACE

### Change 4: `src/ui/ContentPanel.cpp`
<<<<<<< SEARCH
void ContentPanel::splitPane(Qt::Orientation orientation, const QString& secondaryPath) {
    if (m_splitManager) m_splitManager->splitPane(orientation, secondaryPath);
}
=======
void ContentPanel::splitPane(Qt::Orientation orientation, const QString& secondaryPath, bool insertBefore) {
    if (m_splitManager) m_splitManager->splitPane(orientation, secondaryPath, insertBefore);
}
>>>>>>> REPLACE

<<<<<<< SEARCH
void ContentPanel::dropEvent(QDropEvent* event) {
    if (event->mimeData() && event->mimeData()->hasFormat("application/x-quarkmeta-taburl")) {
        if (paneCount() < kMaxPanes) {
            hideDragOverlay();
            QString tabUrl = QString::fromUtf8(event->mimeData()->data("application/x-quarkmeta-taburl"));
            if (tabUrl.isEmpty()) {
                tabUrl = event->mimeData()->text();
            }

            if (!tabUrl.isEmpty()) {
                QPoint pos = event->position().toPoint();
                int w = width();
                int h = height();
                Qt::Orientation orientation = Qt::Horizontal;

                if (pos.y() < h * 0.25 || pos.y() > h * 0.75) {
                    orientation = Qt::Vertical;
                } else if (pos.x() < w * 0.25 || pos.x() > w * 0.75) {
                    orientation = Qt::Horizontal;
                } else {
                    orientation = Qt::Horizontal;
                }

                splitPane(orientation, tabUrl);
                event->acceptProposedAction();
                return;
            }
        }
    }
    hideDragOverlay();
    QFrame::dropEvent(event);
}
=======
void ContentPanel::dropEvent(QDropEvent* event) {
    if (event->mimeData() && event->mimeData()->hasFormat("application/x-quarkmeta-taburl")) {
        if (paneCount() < kMaxPanes) {
            hideDragOverlay();
            QString tabUrl = QString::fromUtf8(event->mimeData()->data("application/x-quarkmeta-taburl"));
            if (tabUrl.isEmpty()) {
                tabUrl = event->mimeData()->text();
            }

            if (!tabUrl.isEmpty()) {
                QPoint pos = event->position().toPoint();
                ContentPaneSplitManager::SplitEvaluationResult eval = ContentPaneSplitManager::evaluateSplitDrop(pos, size());
                if (eval.isValid) {
                    splitPane(eval.orientation, tabUrl, eval.insertBefore);
                    event->acceptProposedAction();
                    return;
                }
            }
        }
    }
    hideDragOverlay();
    QFrame::dropEvent(event);
}
>>>>>>> REPLACE

### Change 5: `src/ui/TabBarWidget.h`
<<<<<<< SEARCH
protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    int m_index;
    QPoint m_dragStartPos;
=======
protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    int m_index;
    QPoint m_dragStartPos;
    bool m_isDragging = false;
>>>>>>> REPLACE

### Change 6: `src/ui/TabBarWidget.cpp`
<<<<<<< SEARCH
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

            TabBarWidget* tabBar = qobject_cast<TabBarWidget*>(parentWidget());
            if (!tabBar && parentWidget()) {
                tabBar = qobject_cast<TabBarWidget*>(parentWidget()->parentWidget());
            }
            if (tabBar) {
                QString url = tabBar->tabUrl(m_index);
                if (!url.isEmpty()) {
                    mimeData->setData("application/x-quarkmeta-taburl", url.toUtf8());
                    mimeData->setText(url);
                }
            }
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
=======
void TabItemButton::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragStartPos = event->pos();
        m_isDragging = false;
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
            m_isDragging = true;
            QDrag* drag = new QDrag(this);
            QMimeData* mimeData = new QMimeData();
            mimeData->setData("application/x-quarkmeta-tabindex", QByteArray::number(m_index));

            TabBarWidget* tabBar = qobject_cast<TabBarWidget*>(parentWidget());
            if (!tabBar && parentWidget()) {
                tabBar = qobject_cast<TabBarWidget*>(parentWidget()->parentWidget());
            }
            if (tabBar) {
                QString url = tabBar->tabUrl(m_index);
                if (!url.isEmpty()) {
                    mimeData->setData("application/x-quarkmeta-taburl", url.toUtf8());
                    mimeData->setText(url);
                }
            }
            drag->setMimeData(mimeData);

            QPixmap pixmap = grab();
            drag->setPixmap(pixmap);

            QPoint clampedHotSpot = m_dragStartPos;
            clampedHotSpot.setX(qBound(0, clampedHotSpot.x(), width() - 1));
            clampedHotSpot.setY(qBound(0, clampedHotSpot.y(), height() - 1));
            drag->setHotSpot(clampedHotSpot);

            drag->exec(Qt::MoveAction);
            m_dragStartPos = QPoint();
            return;
        }
    }
    QPushButton::mouseMoveEvent(event);
}

void TabItemButton::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        if (!m_isDragging) {
            emit tabClicked(m_index);
        }
        m_isDragging = false;
        event->accept();
        return;
    }
    QPushButton::mouseReleaseEvent(event);
}
>>>>>>> REPLACE

---

## 4. Build & Verification Steps
1. Clean build directory and compile using CMake & Ninja / MSVC:
   ```bash
   cmake -B build -G Ninja
   cmake --build build --config Release
   ```
2. Verify Tab Split Overlay and Direction:
   - Drag tab to Left 25% boundary: verify blue highlight overlay covers left half and new pane appears on the LEFT side.
   - Drag tab to Right 25% boundary: verify blue highlight overlay covers right half and new pane appears on the RIGHT side.
   - Drag tab to Top 25% boundary: verify blue highlight overlay covers top half and new pane appears on the TOP side.
   - Drag tab to Bottom 25% boundary: verify blue highlight overlay covers bottom half and new pane appears on the BOTTOM side.
   - Drag tab to middle area: verify overlay is hidden, and releasing does not split pane.
3. Verify Tab Thumbnail Hotspot Alignment:
   - Drag tab slowly vs fast downwards / sideways: verify drag thumbnail remains anchored exactly under cursor at initial press point.
4. Verify Tab Click & Reorder:
   - Click tab without dragging: verify tab switches instantly upon mouse release.
   - Drag tab without dropping onto content area: verify tab reorders cleanly without triggering tab click navigation on press.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Single SSOT Drop Evaluation Entry Point**: `ContentPaneSplitManager::evaluateSplitDrop` is the unique function evaluating split drop position across the application.
- **Clamped Logical Hotspot**: Reused `m_dragStartPos` with bounds clamping, avoiding unconstrained mouse move coordinates.

---

## 6. Header API Signature Verification
- `static SplitEvaluationResult ContentPaneSplitManager::evaluateSplitDrop(const QPoint& pos, const QSize& refSize)`
- `void ContentPaneSplitManager::splitPane(Qt::Orientation orientation, const QString& secondaryPath = QString(), bool insertBefore = false)`
- `void TabItemButton::mouseReleaseEvent(QMouseEvent* event)`

---

## 7. Header Inclusion Chain & Type Completeness Check
- `ContentPaneSplitManager.h` includes `QPoint`, `QSize`, `QRect`, `Qt`.
- `TabBarWidget.h` includes `QPushButton`, `QMouseEvent`.
- `ContentPanel.cpp` includes `controllers/ContentPaneSplitManager.h`.

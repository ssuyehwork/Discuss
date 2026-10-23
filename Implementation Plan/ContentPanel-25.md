# Implementation Plan - Fix Tab Drag Split URL, Secondary Pane Folder Navigation, and Bidirectional Close Pane Context Menu

## 1. Overview
This implementation plan fixes three specific code disconnects in dual-pane split view & tab drag-and-drop:
1. **Tab Drag URL Payload**: Exposes `tabUrl(int index)` on `TabBarWidget` and attaches `application/x-quarkmeta-taburl` MIME payload in `TabItemButton::mouseMoveEvent`.
2. **Secondary Pane Identification & Navigation**: Adds `m_isSecondaryPane` flag, `requestClosePane()` method, and `closePaneRequested` signal to `ContentPanel`. Connects secondary pane directory selection to load subdirectories into the secondary pane itself.
3. **Bidirectional Close Pane Context Menu**: Updates `ContentContextMenu.cpp` to show "关闭窗格" (Close Pane) for both primary (`isSplitMode()`) and secondary (`isSecondaryPane()`) panes, invoking `requestClosePane()`.

---

## 2. Modified Files List
- `src/ui/TabBarWidget.h`
- `src/ui/TabBarWidget.cpp`
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`
- `src/ui/controllers/ContentContextMenu.cpp`

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/ui/TabBarWidget.h`
<<<<<<< SEARCH
    int tabCount() const { return m_tabs.size(); }
=======
    int tabCount() const { return m_tabs.size(); }
    QString tabUrl(int index) const {
        if (index >= 0 && index < m_tabs.size()) return m_tabs[index].url;
        return QString();
    }
>>>>>>> REPLACE

---

### File 2: `src/ui/TabBarWidget.cpp`
<<<<<<< SEARCH
            QDrag* drag = new QDrag(this);
            QMimeData* mimeData = new QMimeData();
            mimeData->setData("application/x-quarkmeta-tabindex", QByteArray::number(m_index));
            drag->setMimeData(mimeData);
=======
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
>>>>>>> REPLACE

---

### File 3: `src/ui/ContentPanel.h`
<<<<<<< SEARCH
    // Dual-pane state inspection & split controls
    bool isSplitMode() const;
    void splitPane(Qt::Orientation orientation, const QString& secondaryPath = QString());
    void closeSecondaryPane();
=======
    // Dual-pane state inspection & split controls
    bool isSplitMode() const;
    bool isSecondaryPane() const { return m_isSecondaryPane; }
    void setIsSecondaryPane(bool secondary) { m_isSecondaryPane = secondary; }
    void splitPane(Qt::Orientation orientation, const QString& secondaryPath = QString());
    void closeSecondaryPane();
    void requestClosePane();
>>>>>>> REPLACE

<<<<<<< SEARCH
signals:
    void secondaryPaneCreated(ContentPanel* pane);
    void secondaryPaneClosed();
=======
signals:
    void secondaryPaneCreated(ContentPanel* pane);
    void secondaryPaneClosed();
    void closePaneRequested();
>>>>>>> REPLACE

<<<<<<< SEARCH
    Qt::Orientation m_splitOrientation = Qt::Horizontal;
    bool m_isSplit = false;
=======
    Qt::Orientation m_splitOrientation = Qt::Horizontal;
    bool m_isSplit = false;
    bool m_isSecondaryPane = false;
>>>>>>> REPLACE

---

### File 4: `src/ui/ContentPanel.cpp`

```cpp
<<<<<<< SEARCH
        m_secondaryContentPanel = new ContentPanel(m_secondaryPaneContainer);
        secLayout->addWidget(m_secondaryContentPanel);

        m_paneSplitter->addWidget(m_secondaryPaneContainer);
        m_mainLayout->addWidget(m_paneSplitter, 1);

        connect(m_secondaryContentPanel, &ContentPanel::directorySelected, this, [this](const QString&) {
            if (m_isSplit) {
                QString p1 = m_currentPath;
                QString p2 = m_secondaryContentPanel ? m_secondaryContentPanel->currentPath() : QString();
                emit dualPanePathsChanged(p1, p2);
            }
        });
=======
        m_secondaryContentPanel = new ContentPanel(m_secondaryPaneContainer);
        m_secondaryContentPanel->setIsSecondaryPane(true);
        connect(m_secondaryContentPanel, &ContentPanel::closePaneRequested, this, &ContentPanel::closeSecondaryPane);

        secLayout->addWidget(m_secondaryContentPanel);

        m_paneSplitter->addWidget(m_secondaryPaneContainer);
        m_mainLayout->addWidget(m_paneSplitter, 1);

        connect(m_secondaryContentPanel, &ContentPanel::directorySelected, this, [this](const QString& path) {
            if (m_isSplit && m_secondaryContentPanel) {
                // 1. 让副窗格自身加载被双击的下级目录
                m_secondaryContentPanel->loadDirectory(path);
                // 2. 向上派发双窗格路径更新
                QString p1 = m_currentPath;
                QString p2 = path;
                emit dualPanePathsChanged(p1, p2);
            }
        });
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
void ContentPanel::closeSecondaryPane() {
=======
void ContentPanel::requestClosePane() {
    if (m_isSecondaryPane) {
        emit closePaneRequested();
    } else if (m_isSplit) {
        closeSecondaryPane();
    }
}

void ContentPanel::closeSecondaryPane() {
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    if (pos.x() > w * 0.75 || pos.x() < w * 0.25) {
        splitPane(Qt::Horizontal);
        event->acceptProposedAction();
    } else if (pos.y() > h * 0.75 || pos.y() < h * 0.25) {
        splitPane(Qt::Vertical);
        event->acceptProposedAction();
    }
=======
    QString targetUrl;
    if (event->mimeData()->hasFormat("application/x-quarkmeta-taburl")) {
        targetUrl = QString::fromUtf8(event->mimeData()->data("application/x-quarkmeta-taburl"));
    } else if (event->mimeData()->hasText()) {
        targetUrl = event->mimeData()->text();
    }

    if (pos.x() > w * 0.75 || pos.x() < w * 0.25) {
        splitPane(Qt::Horizontal, targetUrl);
        event->acceptProposedAction();
    } else if (pos.y() > h * 0.75 || pos.y() < h * 0.25) {
        splitPane(Qt::Vertical, targetUrl);
        event->acceptProposedAction();
    }
>>>>>>> REPLACE
```

---

### File 5: `src/ui/controllers/ContentContextMenu.cpp`
<<<<<<< SEARCH
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
=======
    // --- 拆分窗格控制菜单项 ---
    if (m_panel && (m_panel->isSplitMode() || m_panel->isSecondaryPane())) {
        menu.addSeparator();
        QAction* closePaneAction = menu.addAction(UiHelper::getIcon("close", QColor("#EEEEEE"), 18), "关闭窗格");
        QObject::connect(closePaneAction, &QAction::triggered, [this]() {
            if (m_panel) {
                m_panel->requestClosePane();
            }
        });
    }
>>>>>>> REPLACE

---

## 4. Build & Verification Steps
1. Verify code edits against `.h` and `.cpp` files.
2. Ensure signatures match perfectly across header and implementation.
3. Test tab drag payload extraction, secondary pane subfolder navigation, and right-click "Close Pane" action.

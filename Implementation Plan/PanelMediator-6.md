# PanelMediator Selection Performance Optimization Implementation Plan

## Overview
Optimize selection change handler in `PanelMediator.cpp` to prevent heavy synchronous UI blocking during file selection in large directories.

## Changes

```path
src/ui/PanelMediator.cpp
```

<<<<<<< SEARCH
        // 4. 元数据面板选中项 & 模型数据变动同步
        if (metaPanel) {
            connect(panel, &ContentPanel::selectionChanged, metaPanel, [this, panel, updateMetaPanelFromPanel](const QStringList&) {
                if (m_activeContentPanel == panel || (!m_activeContentPanel && panel == m_contentPanel.data())) {
                    updateMetaPanelFromPanel(panel);
                }
            });

            if (panel->model()) {
=======
        // 4. 元数据面板选中项 & 模型数据变动同步 (添加轻量级 QTimer 防抖保护，避免海量数据下频繁选中卡顿)
        if (metaPanel) {
            QTimer* selectionTimer = new QTimer(panel);
            selectionTimer->setSingleShot(true);
            selectionTimer->setInterval(35); // 35ms 微秒级防抖，极速连续按键/点击时零卡顿

            connect(selectionTimer, &QTimer::timeout, metaPanel, [this, panel, updateMetaPanelFromPanel]() {
                if (m_activeContentPanel == panel || (!m_activeContentPanel && panel == m_contentPanel.data())) {
                    updateMetaPanelFromPanel(panel);
                }
            });

            connect(panel, &ContentPanel::selectionChanged, metaPanel, [selectionTimer](const QStringList&) {
                selectionTimer->start();
            });

            if (panel->model()) {
>>>>>>> REPLACE

# Implementation Plan - Unconditional Color Strip & Icon Switcher in Tab Menu

This plan removes the `isFolder` conditional check in `TabBarWidget::showTabContextMenu` so that the color strip picker (`ColorStripPicker`) and "切换图标" sub-menu appear on ALL tabs unconditionally (including "此电脑").

## Overview
1. **Unconditional Tab Menu Options**:
   - In `TabBarWidget::showTabContextMenu`, remove `if (isFolder)` requirement.
   - Always display `ColorStripPicker` and "切换图标" sub-menu at the top of the context menu for any tab.
   - For all tabs (including `computer://`), selecting a color or icon updates `m_tabs[index].color` / `m_tabs[index].iconKey` and calls `updateTabsUiState()` for immediate 0ms UI update.
   - For physical folder paths, also dispatch `AppCommandType::SetColor` to persist the color tag across the application.

## Modified Files List
- `src/ui/TabBarWidget.cpp`

## Detailed Line-by-Line Changes

### `src/ui/TabBarWidget.cpp`
<<<<<<< SEARCH
void TabBarWidget::showTabContextMenu(int index, const QPoint& globalPos) {
    if (index < 0 || index >= m_tabs.size()) return;

    const auto& tab = m_tabs[index];
    bool isFolder = !tab.url.startsWith("computer://") && !tab.url.isEmpty();

    QMenu menu(this);
    menu.setObjectName("TabContextMenu");
    UiHelper::applyMenuStyle(&menu);

    if (isFolder) {
        // 1. 颜色条组件
        QString curColorHex = tab.color.isEmpty() ? "#888888" : tab.color;
=======
void TabBarWidget::showTabContextMenu(int index, const QPoint& globalPos) {
    if (index < 0 || index >= m_tabs.size()) return;

    const auto& tab = m_tabs[index];

    QMenu menu(this);
    menu.setObjectName("TabContextMenu");
    UiHelper::applyMenuStyle(&menu);

    // 1. 颜色条组件（所有标签页无条件展示）
    QString curColorHex = tab.color.isEmpty() ? "#888888" : tab.color;
>>>>>>> REPLACE

<<<<<<< SEARCH
        connect(colorPickerWidget, &ColorStripPicker::colorSelected, this, [this, index, iconMenu, iconButtons](const QString& hexColor) {
            if (index < 0 || index >= m_tabs.size()) return;

            QString finalColor = hexColor.isEmpty() ? "#888888" : hexColor.toUpper();
            m_tabs[index].color = finalColor;

            QString iconKey = m_tabs[index].iconKey.isEmpty() ? "folder_filled" : m_tabs[index].iconKey;
            QString targetPath = m_tabs[index].url;

            iconMenu->setIcon(UiHelper::getIcon(iconKey, QColor(finalColor)));
            for (const auto& btnPair : iconButtons) {
                btnPair.first->setIcon(UiHelper::getIcon(btnPair.second, QColor(finalColor), 18));
            }

            if (!targetPath.isEmpty()) {
                AppCommand cmd;
                cmd.type = AppCommandType::SetColor;
                cmd.targetPaths = {targetPath};
                cmd.params["color"] = finalColor;
                CoreEngine::instance().executeCommand(cmd);
            }

            updateTabsUiState();
        });

        menu.addSeparator();
    }
=======
        connect(colorPickerWidget, &ColorStripPicker::colorSelected, this, [this, index, iconMenu, iconButtons](const QString& hexColor) {
            if (index < 0 || index >= m_tabs.size()) return;

            QString finalColor = hexColor.isEmpty() ? "#888888" : hexColor.toUpper();
            m_tabs[index].color = finalColor;

            QString iconKey = m_tabs[index].iconKey.isEmpty() ? "folder_filled" : m_tabs[index].iconKey;
            QString targetPath = m_tabs[index].url;

            iconMenu->setIcon(UiHelper::getIcon(iconKey, QColor(finalColor)));
            for (const auto& btnPair : iconButtons) {
                btnPair.first->setIcon(UiHelper::getIcon(btnPair.second, QColor(finalColor), 18));
            }

            if (!targetPath.isEmpty() && !targetPath.startsWith("computer://")) {
                AppCommand cmd;
                cmd.type = AppCommandType::SetColor;
                cmd.targetPaths = {targetPath};
                cmd.params["color"] = finalColor;
                CoreEngine::instance().executeCommand(cmd);
            }

            updateTabsUiState();
        });

        menu.addSeparator();
>>>>>>> REPLACE

## Build & Verification Steps
1. Build application.
2. Right-click on ANY tab (including "此电脑").
3. Verify that the color strip picker and "切换图标" sub-menu appear at the top of the context menu.

## SSOT API Reuse & Anti-Redundancy Self-Check
- Reused `ColorStripPicker` and `UiHelper::getIcon`.

## Header API Signature Verification
- `TabBarWidget::showTabContextMenu(int index, const QPoint& globalPos)`

# Implementation Plan - 快速预览 QuickLook Toast 提示定位与右键菜单补全方案

## 1. 方案概述 (Overview)
本方案旨在为快速预览窗口（`QuickLookWindow`）补充星级与颜色标记变更时的 `ToolTipOverlay` Toast 气泡提示，并对齐主界面补全 QuickLook 右键上下文菜单缺失的三大核心功能项：
1. **Toast 提示集成与坐标对齐**：当用户在 QuickLook 界面设置星级（`1~5`）或颜色（`Alt+1~9` / 颜色选择栏）时，通过 `ToolTipOverlay::showToast` 弹出 Toast 提示，且坐标严格锁定在 `QuickLookWindow` 水平居中、顶部下方 10px 处；
2. **右键菜单功能补齐**：在 `QuickLookWindow` 右键菜单中植入 `ColorStripPicker` 颜色标记选择栏、“置顶/取消置顶”动作（`buildPinToggleAction`）、以及“添加至‘库’/从‘库’中移除”动作（`buildLibraryAction`），并同步将“打开方式”归一化为 `buildOpenWithMenu`。

---

## 2. 问题分析与根因溯源 (Analysis & Root Cause)
1. **Toast 反馈缺失**：在 `PanelMediator.cpp` 处理 `QuickLookWindow::ratingRequested` 与 `colorRequested` 信号时，仅提交命令至 `CoreEngine` 并同步更新 `MetaPanel`，未触发 `ToolTipOverlay::showToast` 给予用户直观反馈。`ToolTipOverlay::showToast` 接口支持传入 `targetWidget` 目标控件参数（传入 `QuickLookWindow` 实例），可自动计算水平居中及顶部下方 10px 坐标。
2. **右键菜单功能不完整**：`QuickLookWindow::showContextMenu` 仅包含旋转、翻转、缩放、复制、剪切、删除等基本操作，缺乏主视图菜单具备的 `ColorStripPicker` 颜色选择栏、`ContextMenuFactory::buildPinToggleAction` 窗口置顶切换、以及 `LibraryService::buildLibraryAction` 库管理能力。

---

## 3. 修改文件范围 (Scope of Changes)

### 核心代码文件
- `src/ui/PanelMediator.cpp`：在处理 `ratingRequested` 与 `colorRequested` 的 Lambda 回调中，增加 `ToolTipOverlay::showToast` 调用，目标指针为 `&QuickLookWindow::instance()`。
- `src/ui/QuickLookWindow.cpp`：在 `showContextMenu` 中引入 `ColorPicker.h`、`FramelessWindowHelper.h` 与 `LibraryService.h`，依次构建 `ColorStripPicker` 颜色选择栏、`buildPinToggleAction` 置顶切换与 `buildLibraryAction` 库管理菜单项。

---

## 4. 架构契约与规范核查 (Architecture Checks)
- **【契约锁】SSOT 唯一真理源**：置顶控制委托 `FramelessWindowHelper`，库管理委托 `LibraryService`， Toast 提示统一走 `ToolTipOverlay` 通道。
- **【依赖锁】单向依赖**：`QuickLookWindow` / `PanelMediator` 调度 Service 与 Factory，绝不跨层包含无关头文件。
- **【门禁锁】中性色契约**：新增菜单项图标统一采用 `#EEEEEE` 中性单色。

---

## 5. 详细实现方案 (Implementation Details)

### 5.1 Toast 提示计算与派发
在 `PanelMediator.cpp` 中：
```cpp
// 评级 Toast 派发
ToolTipOverlay::instance()->showToast(
    QString("已设为 %1 星").arg(rating),
    1500, QColor("#3498db"), &QuickLookWindow::instance()
);

// 颜色标记 Toast 派发
QString toastMsg = color.isEmpty() ? "已清除颜色标记" : "已应用颜色标记";
ToolTipOverlay::instance()->showToast(
    toastMsg, 1500, QColor("#2ecc71"), &QuickLookWindow::instance()
);
```
`ToolTipOverlay::showToast` 内部接收到 `QuickLookWindow` 控件指针后，自动计算：
- 水平 X：`globalTopLeft.x() + (quickLookWidth - tipWidth) / 2`（水平居中）；
- 垂直 Y：`globalTopLeft.y() + 10`（顶部下方 10px）。

### 5.2 右键菜单项扩展
在 `QuickLookWindow::showContextMenu` 中：
1. **打开方式归一化**：通过 `ContextMenuFactory::buildOpenWithMenu` 生成完整级联菜单；
2. **颜色选择栏**：读取当前文件 `manualColor` 元数据，实例化 `ColorStripPicker` 并通过 `QWidgetAction` 添加至菜单；
3. **窗口置顶**：通过 `ContextMenuFactory::buildPinToggleAction` 结合 `FramelessWindowHelper::isAlwaysOnTop` 与 `FramelessWindowHelper::setAlwaysOnTop` 切换；
4. **添加至“库”**：通过 `LibraryService::instance().buildLibraryAction` 动态构建。

---

## 6. 代码修改 (Search & Replace Blocks)

### Block 1: `src/ui/PanelMediator.cpp`
<<<<<<< SEARCH
    // QuickLook 改星级 -> 同步更新内容面板卡片
    connect(&QuickLookWindow::instance(), &QuickLookWindow::ratingRequested, this, [this, metaPanel](int rating) {
        QString curPath = QuickLookWindow::instance().currentPath();
        if (curPath.isEmpty()) return;

        AppCommand cmd;
        cmd.type = AppCommandType::SetRating;
        cmd.targetPaths << curPath;
        cmd.params["rating"] = rating;
        CoreEngine::instance().executeCommand(cmd);

        if (metaPanel) metaPanel->setRating(rating, false);
    });

    // QuickLook 改颜色 -> 同步更新内容面板卡片
    connect(&QuickLookWindow::instance(), &QuickLookWindow::colorRequested, this, [this, metaPanel](const QString& color) {
        QString curPath = QuickLookWindow::instance().currentPath();
        if (curPath.isEmpty()) return;

        AppCommand cmd;
        cmd.type = AppCommandType::SetColor;
        cmd.targetPaths << curPath;
        cmd.params["color"] = color;
        CoreEngine::instance().executeCommand(cmd);

        if (metaPanel) metaPanel->setColor(color, false);
    });
=======
    // QuickLook 改星级 -> 同步更新内容面板卡片并弹出 Toast 提示
    connect(&QuickLookWindow::instance(), &QuickLookWindow::ratingRequested, this, [this, metaPanel](int rating) {
        QString curPath = QuickLookWindow::instance().currentPath();
        if (curPath.isEmpty()) return;

        AppCommand cmd;
        cmd.type = AppCommandType::SetRating;
        cmd.targetPaths << curPath;
        cmd.params["rating"] = rating;
        CoreEngine::instance().executeCommand(cmd);

        if (metaPanel) metaPanel->setRating(rating, false);

        ToolTipOverlay::instance()->showToast(
            QString("已设为 %1 星").arg(rating),
            1500, QColor("#3498db"), &QuickLookWindow::instance()
        );
    });

    // QuickLook 改颜色 -> 同步更新内容面板卡片并弹出 Toast 提示
    connect(&QuickLookWindow::instance(), &QuickLookWindow::colorRequested, this, [this, metaPanel](const QString& color) {
        QString curPath = QuickLookWindow::instance().currentPath();
        if (curPath.isEmpty()) return;

        AppCommand cmd;
        cmd.type = AppCommandType::SetColor;
        cmd.targetPaths << curPath;
        cmd.params["color"] = color;
        CoreEngine::instance().executeCommand(cmd);

        if (metaPanel) metaPanel->setColor(color, false);

        QString toastMsg = color.isEmpty() ? "已清除颜色标记" : "已应用颜色标记";
        ToolTipOverlay::instance()->showToast(
            toastMsg, 1500, QColor("#2ecc71"), &QuickLookWindow::instance()
        );
    });
>>>>>>> REPLACE

---

### Block 2: `src/ui/QuickLookWindow.cpp` (头文件包含)
<<<<<<< SEARCH
#include "controllers/ContextMenuFactory.h"
#include "dialogs/TextExtensionDialog.h"
#include "FormatDecoders.h"
=======
#include "controllers/ContextMenuFactory.h"
#include "dialogs/TextExtensionDialog.h"
#include "ColorPicker.h"
#include "FramelessWindowHelper.h"
#include "../meta/MetadataManager.h"
#include "../meta/LibraryService.h"
#include "FormatDecoders.h"
#include <QWidgetAction>
>>>>>>> REPLACE

---

### Block 3: `src/ui/QuickLookWindow.cpp` (右键菜单构造)
<<<<<<< SEARCH
    QAction* actOpenDefault = menu.addAction(UiHelper::getIcon("launch", QColor("#EEEEEE"), 18), "打开方式");
    ContextMenuFactory::buildShowInExplorerAction(&menu, m_currentPath, this);
    menu.addSeparator();

    QAction* actCopy = menu.addAction(UiHelper::getIcon("copy", QColor("#EEEEEE"), 18), "复制");
=======
    ContextMenuFactory::buildOpenWithMenu(&menu, m_currentPath, this);
    ContextMenuFactory::buildShowInExplorerAction(&menu, m_currentPath, this);
    menu.addSeparator();

    // 1. 颜色标记选择栏
    QString currentColorStr;
    auto meta = MetadataManager::instance().getMeta(m_currentPath.toStdWString());
    currentColorStr = QString::fromStdWString(meta.manualColor);

    QWidgetAction* pickerAction = new QWidgetAction(&menu);
    ColorStripPicker* pickerWidget = new ColorStripPicker(currentColorStr, &menu);
    pickerAction->setDefaultWidget(pickerWidget);
    menu.addAction(pickerAction);

    connect(pickerWidget, &ColorStripPicker::colorSelected, this, [this, &menu](const QString& hexColor) {
        emit colorRequested(hexColor);
        menu.close();
    });

    // 2. 置顶功能项
    ContextMenuFactory::buildPinToggleAction(&menu, FramelessWindowHelper::isAlwaysOnTop(this), [this](bool pin) {
        FramelessWindowHelper::setAlwaysOnTop(this, pin);
    }, this);

    // 3. 添加至“库”功能项
    LibraryService::instance().buildLibraryAction(&menu, QStringList{m_currentPath}, this);
    menu.addSeparator();

    QAction* actCopy = menu.addAction(UiHelper::getIcon("copy", QColor("#EEEEEE"), 18), "复制");
>>>>>>> REPLACE

---

### Block 4: `src/ui/QuickLookWindow.cpp` (清理 actOpenDefault 触发响应)
<<<<<<< SEARCH
    } else if (selected == actFit) {
        m_graphicsView->fitImage();
    } else if (selected == actOpenDefault) {
        HistoryService::instance().recordFileAccess(m_currentPath);
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_currentPath));
    } else if (selected == actCopy) {
=======
    } else if (selected == actFit) {
        m_graphicsView->fitImage();
    } else if (selected == actCopy) {
>>>>>>> REPLACE

---

## 7. 三道关自检与验证步骤 (Verification)

### 1. 第一道关：静态编译核查
通过编译核查确保所有新增头文件包含与成员函数调用语法完全正确。

### 2. 第二道关：自检脚本验证
运行方案合规性校验工具：
```bash
python3 tools/plan_verify/verify_plan.py "Implementation Plan/QuickLook_Toast_And_ContextMenu_Plan.md"
```

---

## 8. 总结 (Summary)
本实施方案精准解决了 QuickLook 窗口设定星级/颜色时的视觉反馈定位需求，同时通过对齐右键菜单能力补齐了颜色标注栏、置顶切换及库管理功能，使 QuickLook 上下文菜单与主视图体验保持完全一致。

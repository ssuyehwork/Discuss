# Implementation Plan: Port Text Extension Settings to PluginBar (TextExtensionPlugin.md)

## 1. Overview
本实施方案旨在将“文本扩展名设置”功能作为插件组件移植到插件栏（`DriveBarWidget`）。

在插件栏中增加全新的“文本扩展名设置”图标按钮（使用 `text` / `Ext` SVG 图标），点击后直接唤起 `TextExtensionDialog` 对话框；同时将其注入最左侧“扩展与组件管理”菜单，支持动态固定（Pin）与持久化配置（`AppConfig`）。

---

## 2. Modified Files List
* `src/ui/DriveBarWidget.h`
* `src/ui/DriveBarWidget.cpp`

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/ui/DriveBarWidget.h`

<<<<<<< SEARCH
    QPushButton* extensionManagerButton() const { return m_btnExtensionManager; }
    QPushButton* tagManagerButton() const { return m_btnTagManager; }

    void refreshPinnedButtons();
=======
    QPushButton* extensionManagerButton() const { return m_btnExtensionManager; }
    QPushButton* tagManagerButton() const { return m_btnTagManager; }
    QPushButton* textExtensionButton() const { return m_btnTextExtension; }

    void refreshPinnedButtons();
>>>>>>> REPLACE

<<<<<<< SEARCH
    QPushButton* m_btnExtensionManager = nullptr;
    QPushButton* m_btnTagManager = nullptr;
=======
    QPushButton* m_btnExtensionManager = nullptr;
    QPushButton* m_btnTagManager = nullptr;
    QPushButton* m_btnTextExtension = nullptr;
>>>>>>> REPLACE

---

### File 2: `src/ui/DriveBarWidget.cpp`

<<<<<<< SEARCH
#include "DriveBarWidget.h"
#include "UiHelper.h"
#include "TagManagerDialog.h"
#include "ToolTipOverlay.h"
#include "../core/NavigationService.h"
#include "../core/AppConfig.h"
=======
#include "DriveBarWidget.h"
#include "UiHelper.h"
#include "TagManagerDialog.h"
#include "dialogs/TextExtensionDialog.h"
#include "ToolTipOverlay.h"
#include "../core/NavigationService.h"
#include "../core/AppConfig.h"
>>>>>>> REPLACE

<<<<<<< SEARCH
    // 2. 创建各独立功能图标（纯 Icon，通过 ToolTipOverlay 提示）
    m_btnTagManager = createIconButton("tag", QColor("#1abc9c"), "标签管理");
    connect(m_btnTagManager, &QPushButton::clicked, this, [this]() {
        TagManagerDialog::showDialog(this, NavigationService::instance().currentUrl(), false);
    });

    // 默认加至布局并依配置显隐
    m_driveBarLayout->addWidget(m_btnTagManager);
    m_driveBarLayout->addStretch();
=======
    // 2. 创建各独立功能图标（纯 Icon，通过 ToolTipOverlay 提示）
    m_btnTagManager = createIconButton("tag", QColor("#1abc9c"), "标签管理");
    connect(m_btnTagManager, &QPushButton::clicked, this, [this]() {
        TagManagerDialog::showDialog(this, NavigationService::instance().currentUrl(), false);
    });

    m_btnTextExtension = createIconButton("text", QColor("#3498db"), "文本扩展名设置");
    connect(m_btnTextExtension, &QPushButton::clicked, this, [this]() {
        TextExtensionDialog dlg(this);
        dlg.exec();
    });

    // 默认加至布局并依配置显隐
    m_driveBarLayout->addWidget(m_btnTagManager);
    m_driveBarLayout->addWidget(m_btnTextExtension);
    m_driveBarLayout->addStretch();
>>>>>>> REPLACE

<<<<<<< SEARCH
void DriveBarWidget::refreshPinnedButtons() {
    QStringList pinnedList = AppConfig::instance().getValue("DriveBar/PinnedExtensions", QStringList{"tag"}).toStringList();

    if (m_btnTagManager) {
        m_btnTagManager->setVisible(pinnedList.contains("tag"));
    }
}
=======
void DriveBarWidget::refreshPinnedButtons() {
    QStringList pinnedList = AppConfig::instance().getValue("DriveBar/PinnedExtensions", QStringList{"tag", "text_ext"}).toStringList();

    if (m_btnTagManager) {
        m_btnTagManager->setVisible(pinnedList.contains("tag"));
    }
    if (m_btnTextExtension) {
        m_btnTextExtension->setVisible(pinnedList.contains("text_ext"));
    }
}
>>>>>>> REPLACE

<<<<<<< SEARCH
        // 支持持续选择：重写 action 交互避免触发后自动销毁关闭菜单
        QAction* actTag = extMenu->addAction(UiHelper::getIcon("tag", QColor("#1abc9c"), 18), "标签管理");
        actTag->setCheckable(true);
        actTag->setChecked(pinnedList.contains("tag"));

        connect(actTag, &QAction::triggered, this, [this, actTag](bool checked) {
            QStringList currentPinned = AppConfig::instance().getValue("DriveBar/PinnedExtensions", QStringList{"tag"}).toStringList();
            if (checked && !currentPinned.contains("tag")) {
                currentPinned.append("tag");
            } else if (!checked) {
                currentPinned.removeAll("tag");
            }
            AppConfig::instance().setValue("DriveBar/PinnedExtensions", currentPinned);
            AppConfig::instance().sync();

            refreshPinnedButtons();
        });
=======
        // 支持持续选择：重写 action 交互避免触发后自动销毁关闭菜单
        QAction* actTag = extMenu->addAction(UiHelper::getIcon("tag", QColor("#1abc9c"), 18), "标签管理");
        actTag->setCheckable(true);
        actTag->setChecked(pinnedList.contains("tag"));

        connect(actTag, &QAction::triggered, this, [this, actTag](bool checked) {
            QStringList currentPinned = AppConfig::instance().getValue("DriveBar/PinnedExtensions", QStringList{"tag", "text_ext"}).toStringList();
            if (checked && !currentPinned.contains("tag")) {
                currentPinned.append("tag");
            } else if (!checked) {
                currentPinned.removeAll("tag");
            }
            AppConfig::instance().setValue("DriveBar/PinnedExtensions", currentPinned);
            AppConfig::instance().sync();

            refreshPinnedButtons();
        });

        QAction* actTextExt = extMenu->addAction(UiHelper::getIcon("text", QColor("#3498db"), 18), "文本扩展名设置");
        actTextExt->setCheckable(true);
        actTextExt->setChecked(pinnedList.contains("text_ext"));

        connect(actTextExt, &QAction::triggered, this, [this, actTextExt](bool checked) {
            QStringList currentPinned = AppConfig::instance().getValue("DriveBar/PinnedExtensions", QStringList{"tag", "text_ext"}).toStringList();
            if (checked && !currentPinned.contains("text_ext")) {
                currentPinned.append("text_ext");
            } else if (!checked) {
                currentPinned.removeAll("text_ext");
            }
            AppConfig::instance().setValue("DriveBar/PinnedExtensions", currentPinned);
            AppConfig::instance().sync();

            refreshPinnedButtons();
        });
>>>>>>> REPLACE

---

## 4. Build & Verification Steps

### 构建步骤
使用 CMake 增量编译项目：
```bash
cmake --build build --config Debug --target QuarkMeta
```

### 验证步骤
1. 启动 QuarkMeta 应用程序；
2. 观察插件栏（`DriveBarWidget`），确认出现 **“文本扩展名设置”** 蓝色图标按钮；
3. 将鼠标悬停在“文本扩展名设置”按钮上方，验证悬浮提示词（Tooltip Overlay）正确展示 `"文本扩展名设置"`；
4. 点击“文本扩展名设置”按钮，验证唤起 `TextExtensionDialog` 弹窗；
5. 点击插件栏最左侧“扩展与组件管理”菜单，取消勾选“文本扩展名设置”，验证按钮从插件栏平滑隐藏；
6. 重新勾选，验证按钮成功固定显示，且跨软件重启状态持久化生效。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check

| 自查项 | 结论 | 说明 |
| :--- | :--- | :--- |
| **是否复用既有 SSOT 通道？** | **是** | 完全复用既有 `TextExtensionDialog` 对话框与 `DriveBarWidget` 拓展插件管线 |
| **是否存在另起炉灶行为？** | **否** | 无需重写文本扩展名逻辑，100% 接入既有 `UiHelper::getCustomTextExtensions` 框架 |

---

## 6. Header API Signature Verification

| 被调用的类/函数 | `.h` 文件真实物理签名 | 核查状态 |
| :--- | :--- | :--- |
| `TextExtensionDialog::TextExtensionDialog` | `explicit TextExtensionDialog(QWidget* parent = nullptr);` | 物理存在，匹配 |
| `AppConfig::getValue` | `QVariant getValue(const QString& key, const QVariant& defaultValue = QVariant()) const;` | 物理存在，匹配 |

---

## 7. Header Inclusion Chain & Type Completeness Check

| 修改文件 | 新增/引用的类与枚举 | 头文件包含状态 |
| :--- | :--- | :--- |
| `DriveBarWidget.cpp` | `TextExtensionDialog` | 显式新增 `#include "dialogs/TextExtensionDialog.h"`，类型完整 |

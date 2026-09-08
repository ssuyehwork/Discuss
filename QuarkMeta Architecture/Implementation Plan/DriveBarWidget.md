# DriveBar Extension System & Icon-Only Redesign Implementation Plan

## 1. Overview
This implementation plan redesigns the DriveBar (`DriveBarWidget`) to eliminate text labels on tool buttons in favor of clean, icon-only representations with `ToolTipOverlay` hover feedback. 

Furthermore, it introduces a plugin-style extension manager at the leftmost position of the bar, represented by the `gridgapm` SVG icon. Clicking this icon pops up a persistent overlay panel allowing users to continuously pin or unpin extensions (such as "标签管理") directly to or from the DriveBar, with pinned states persisted via `AppConfig`.

## 2. Modified Files List
- `src/ui/DriveBarWidget.h`
- `src/ui/DriveBarWidget.cpp`

## 3. Detailed Line-by-Line Changes

### 3.1 Update `src/ui/DriveBarWidget.h`
Add members for the `gridgapm` extension manager button, dynamic extension pin/unpin tracking, and hover event filters.

```
<<<<<<< SEARCH
    QPushButton* tagManagerButton() const { return m_btnTagManager; }
    QHBoxLayout* driveBarLayout() const { return m_driveBarLayout; }

private:
    void initUi();

    QHBoxLayout* m_driveBarLayout = nullptr;
    QPushButton* m_btnTagManager = nullptr;
};
=======
    QPushButton* tagManagerButton() const { return m_btnTagManager; }
    QPushButton* extensionManagerButton() const { return m_btnExtensionManager; }
    QHBoxLayout* driveBarLayout() const { return m_driveBarLayout; }

    void refreshPinnedButtons();

private:
    void initUi();
    void setupExtensionMenu();
    QPushButton* createIconButton(const QString& iconKey, const QColor& color, const QString& tooltipText);

    QHBoxLayout* m_driveBarLayout = nullptr;
    QPushButton* m_btnExtensionManager = nullptr;
    QPushButton* m_btnTagManager = nullptr;
};
>>>>>>> REPLACE
```

### 3.2 Update `src/ui/DriveBarWidget.cpp`
Implement icon-only button creation, `gridgapm` extension button placement at position 0, `ToolTipOverlay` hover filters, and persistent pin/unpin logic.

```
<<<<<<< SEARCH
#include "DriveBarWidget.h"
#include "UiHelper.h"
#include "TagManagerDialog.h"
#include "../core/NavigationService.h"

namespace QuarkMeta {

DriveBarWidget::DriveBarWidget(QWidget* parent)
    : QWidget(parent) {
    setObjectName("DriveBar");
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedHeight(42);
    initUi();
}

void DriveBarWidget::initUi() {
    m_driveBarLayout = new QHBoxLayout(this);
    m_driveBarLayout->setContentsMargins(15, 5, 15, 5);
    m_driveBarLayout->setSpacing(8);

    m_btnTagManager = new QPushButton(UiHelper::getIcon("tag", QColor("#1abc9c"), 18), " 标签管理", this);
    m_btnTagManager->setFixedHeight(32);
    m_btnTagManager->setCursor(Qt::PointingHandCursor);
    m_btnTagManager->setObjectName("BtnTagManager");

    connect(m_btnTagManager, &QPushButton::clicked, this, [this]() {
        TagManagerDialog::showDialog(this, NavigationService::instance().currentUrl(), false);
    });

    m_driveBarLayout->addWidget(m_btnTagManager);
    m_driveBarLayout->addStretch();
}

} // namespace QuarkMeta
=======
#include "DriveBarWidget.h"
#include "UiHelper.h"
#include "TagManagerDialog.h"
#include "ToolTipOverlay.h"
#include "../core/NavigationService.h"
#include "../core/AppConfig.h"

#include <QMenu>
#include <QWidgetAction>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QLabel>

namespace QuarkMeta {

DriveBarWidget::DriveBarWidget(QWidget* parent)
    : QWidget(parent) {
    setObjectName("DriveBar");
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedHeight(42);
    initUi();
}

QPushButton* DriveBarWidget::createIconButton(const QString& iconKey, const QColor& color, const QString& tooltipText) {
    QPushButton* btn = new QPushButton(UiHelper::getIcon(iconKey, color, 18), "", this);
    btn->setFixedSize(32, 32);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setObjectName("DriveBarIconButton");
    btn->setProperty("tooltipText", tooltipText);
    btn->setAttribute(Qt::WA_Hover);

    // 绑定 ToolTipOverlay 悬浮提示，纯图标零文字暴露
    btn->installEventFilter(this);
    return btn;
}

void DriveBarWidget::initUi() {
    m_driveBarLayout = new QHBoxLayout(this);
    m_driveBarLayout->setContentsMargins(15, 5, 15, 5);
    m_driveBarLayout->setSpacing(8);

    // 1. 最左侧（第一个按钮）：固定放置 gridgapm 图标的扩展与组件管理器按钮
    m_btnExtensionManager = createIconButton("gridgapm", QColor("#EEEEEE"), "扩展与组件管理");
    setupExtensionMenu();
    m_driveBarLayout->addWidget(m_btnExtensionManager);

    // 2. 创建各独立功能图标（纯 Icon，通过 ToolTipOverlay 提示）
    m_btnTagManager = createIconButton("tag", QColor("#1abc9c"), "标签管理");
    connect(m_btnTagManager, &QPushButton::clicked, this, [this]() {
        TagManagerDialog::showDialog(this, NavigationService::instance().currentUrl(), false);
    });

    // 默认加至布局并依配置显隐
    m_driveBarLayout->addWidget(m_btnTagManager);
    m_driveBarLayout->addStretch();

    refreshPinnedButtons();
}

void DriveBarWidget::refreshPinnedButtons() {
    QStringList pinnedList = AppConfig::instance().getValue("DriveBar/PinnedExtensions", QStringList{"tag"}).toStringList();

    if (m_btnTagManager) {
        m_btnTagManager->setVisible(pinnedList.contains("tag"));
    }
}

void DriveBarWidget::setupExtensionMenu() {
    QMenu* extMenu = new QMenu(m_btnExtensionManager);
    extMenu->setObjectName("DriveBarExtensionMenu");
    UiHelper::applyMenuStyle(extMenu);

    connect(m_btnExtensionManager, &QPushButton::clicked, this, [this, extMenu]() {
        extMenu->clear();

        QStringList pinnedList = AppConfig::instance().getValue("DriveBar/PinnedExtensions", QStringList{"tag"}).toStringList();

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

        extMenu->popup(m_btnExtensionManager->mapToGlobal(QPoint(0, m_btnExtensionManager->height())));
    });
}

} // namespace QuarkMeta
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. **Compilation**: Compile using CMake / MSVC build environment.
2. **Visual Verification**:
   - Inspect DriveBar: confirm all tool buttons (e.g. `gridgapm`, `tag`) show **only icons** without inline text labels.
   - Hover over `gridgapm`: verify `ToolTipOverlay` displays "扩展与组件管理".
   - Hover over `tag`: verify `ToolTipOverlay` displays "标签管理".
3. **Extension Persistence Verification**:
   - Click `gridgapm` icon at position 0 to open the extension manager panel.
   - Toggle "标签管理" pin state: verify that the `tag` button dynamically appears or vanishes from DriveBar while allowing continuous selection in the menu.
   - Restart the app: confirm that the pinned button configuration persists.

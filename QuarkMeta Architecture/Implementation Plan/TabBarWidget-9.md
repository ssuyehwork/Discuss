# Implementation Plan - ToolTip Compliance & Warning Removal

This plan removes native `setToolTip(...)` calls in `TabBarWidget` to satisfy `Memories.md` Rule #7 ("全应用严禁使用 QWidget 原生 setToolTip 硬编码悬浮提示，悬浮提示只可使用统一的 ToolTipOverlay 控件结合 m_hoverFilter 事件过滤器"), passes `m_hoverFilter` into `TabBarWidget`, and fixes the MSVC C4996 warning in `dropEvent`.

## Overview
1. **Pass `HoverEventFilter* hoverFilter` to `TabBarWidget`**:
   - Update `TabBarWidget` constructor signature to `explicit TabBarWidget(QWidget* parent = nullptr, HoverEventFilter* hoverFilter = nullptr)`.
   - Store `m_hoverFilter`.
2. **Use `tooltipText` Property + `m_hoverFilter` in `TabItemButton` and `NewTabBtn`**:
   - In `TabItemButton`: set `setAttribute(Qt::WA_Hover, true);`, set `setProperty("tooltipText", title);`, remove `setToolTip(title);`, and install `m_hoverFilter`.
   - In `m_btnNewTab`: set `setAttribute(Qt::WA_Hover, true);` and install `m_hoverFilter`.
3. **Fix MSVC C4996 Warning**:
   - Replace `event->pos()` in `TabBarWidget::dropEvent` with `event->position().toPoint()`.

## Modified Files List
- `src/ui/TabBarWidget.h`
- `src/ui/TabBarWidget.cpp`
- `src/ui/TitleBarWidget.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/TabBarWidget.h`
<<<<<<< SEARCH
namespace QuarkMeta {

struct TabInfo {
=======
namespace QuarkMeta {

class HoverEventFilter;

struct TabInfo {
>>>>>>> REPLACE

<<<<<<< SEARCH
class TabItemButton : public QPushButton {
    Q_OBJECT
public:
    explicit TabItemButton(int index, QWidget* parent = nullptr);
=======
class TabItemButton : public QPushButton {
    Q_OBJECT
public:
    explicit TabItemButton(int index, QWidget* parent = nullptr, HoverEventFilter* hoverFilter = nullptr);
>>>>>>> REPLACE

<<<<<<< SEARCH
class TabBarWidget : public QWidget {
    Q_OBJECT

public:
    explicit TabBarWidget(QWidget* parent = nullptr);
=======
class TabBarWidget : public QWidget {
    Q_OBJECT

public:
    explicit TabBarWidget(QWidget* parent = nullptr, HoverEventFilter* hoverFilter = nullptr);
>>>>>>> REPLACE

<<<<<<< SEARCH
    QHBoxLayout* m_mainLayout = nullptr;
    QHBoxLayout* m_tabsLayout = nullptr;
    QPushButton* m_btnNewTab = nullptr;
=======
    QHBoxLayout* m_mainLayout = nullptr;
    QHBoxLayout* m_tabsLayout = nullptr;
    QPushButton* m_btnNewTab = nullptr;
    HoverEventFilter* m_hoverFilter = nullptr;
>>>>>>> REPLACE

### 2. `src/ui/TabBarWidget.cpp`
<<<<<<< SEARCH
#include "UiHelper.h"
#include "StyleLibrary.h"
#include "ColorPicker.h"
=======
#include "UiHelper.h"
#include "StyleLibrary.h"
#include "ColorPicker.h"
#include "HoverEventFilter.h"
>>>>>>> REPLACE

<<<<<<< SEARCH
TabItemButton::TabItemButton(int index, QWidget* parent)
    : QPushButton(parent), m_index(index) {
    setObjectName("TabItem");
    setFocusPolicy(Qt::NoFocus);
=======
TabItemButton::TabItemButton(int index, QWidget* parent, HoverEventFilter* hoverFilter)
    : QPushButton(parent), m_index(index) {
    setObjectName("TabItem");
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_Hover, true);
    if (hoverFilter) {
        installEventFilter(hoverFilter);
    }
>>>>>>> REPLACE

<<<<<<< SEARCH
void TabItemButton::setTabTitle(const QString& title) {
    if (m_titleLabel) {
        QFontMetrics fm(m_titleLabel->font());
        QString elided = fm.elidedText(title, Qt::ElideRight, 110);
        m_titleLabel->setText(elided);
        setToolTip(title);
    }
}
=======
void TabItemButton::setTabTitle(const QString& title) {
    if (m_titleLabel) {
        QFontMetrics fm(m_titleLabel->font());
        QString elided = fm.elidedText(title, Qt::ElideRight, 110);
        m_titleLabel->setText(elided);
        setProperty("tooltipText", title);
    }
}
>>>>>>> REPLACE

<<<<<<< SEARCH
TabBarWidget::TabBarWidget(QWidget* parent) : QWidget(parent) {
    setObjectName("TabBarWidget");
=======
TabBarWidget::TabBarWidget(QWidget* parent, HoverEventFilter* hoverFilter)
    : QWidget(parent), m_hoverFilter(hoverFilter) {
    setObjectName("TabBarWidget");
>>>>>>> REPLACE

<<<<<<< SEARCH
    m_btnNewTab = new QPushButton(this);
    m_btnNewTab->setFocusPolicy(Qt::NoFocus);
    m_btnNewTab->setFixedSize(22, 22);
    m_btnNewTab->setIcon(UiHelper::getIcon("add", QColor("#EEEEEE")));
    m_btnNewTab->setIconSize(QSize(14, 14));
    m_btnNewTab->setObjectName("NewTabBtn");
    m_btnNewTab->setProperty("tooltipText", "新建标签页 (Ctrl+T)");
=======
    m_btnNewTab = new QPushButton(this);
    m_btnNewTab->setFocusPolicy(Qt::NoFocus);
    m_btnNewTab->setAttribute(Qt::WA_Hover, true);
    m_btnNewTab->setFixedSize(22, 22);
    m_btnNewTab->setIcon(UiHelper::getIcon("add", QColor("#EEEEEE")));
    m_btnNewTab->setIconSize(QSize(14, 14));
    m_btnNewTab->setObjectName("NewTabBtn");
    m_btnNewTab->setProperty("tooltipText", "新建标签页 (Ctrl+T)");
    if (m_hoverFilter) {
        m_btnNewTab->installEventFilter(m_hoverFilter);
    }
>>>>>>> REPLACE

<<<<<<< SEARCH
                QPoint dropPos = event->pos();
=======
                QPoint dropPos = event->position().toPoint();
>>>>>>> REPLACE

<<<<<<< SEARCH
        TabItemButton* tabItem = new TabItemButton(i, this);
=======
        TabItemButton* tabItem = new TabItemButton(i, this, m_hoverFilter);
>>>>>>> REPLACE

### 3. `src/ui/TitleBarWidget.cpp`
<<<<<<< SEARCH
    m_tabBar = new TabBarWidget(this);
=======
    m_tabBar = new TabBarWidget(this, hoverFilter);
>>>>>>> REPLACE

## Build & Verification Steps
1. Recompile project.
2. Confirm native ToolTip windows are completely gone and replaced by custom `ToolTipOverlay` custom styled popups on tab item hover.
3. Verify MSVC warning C4996 is removed.

## SSOT API Reuse & Anti-Redundancy Self-Check
- Complies 100% with `Memories.md` Rule #7 (ToolTipOverlay + HoverEventFilter).

## Header API Signature Verification
- `TabBarWidget::TabBarWidget(QWidget*, HoverEventFilter*)`
- `TabItemButton::TabItemButton(int, QWidget*, HoverEventFilter*)`

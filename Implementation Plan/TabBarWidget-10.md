# Implementation Plan - Option A: Remove Tab Item Tooltips

This plan implements Option A according to user consensus: completely removing tooltip hover popups from tab items (`TabItemButton`) to prevent visual distraction, while retaining the tooltip on the New Tab (`+`) button.

## Overview
- In `TabItemButton`: remove `setProperty("tooltipText", title)` and remove `installEventFilter(hoverFilter)`. Tab buttons will no longer trigger `ToolTipOverlay` hover popups.
- In `m_btnNewTab`: keep `setProperty("tooltipText", "新建标签页 (Ctrl+T)")` and `m_hoverFilter` for clear action guidance.

## Modified Files List
- `src/ui/TabBarWidget.h`
- `src/ui/TabBarWidget.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/TabBarWidget.h`
<<<<<<< SEARCH
class TabItemButton : public QPushButton {
    Q_OBJECT
public:
    explicit TabItemButton(int index, QWidget* parent = nullptr, HoverEventFilter* hoverFilter = nullptr);
=======
class TabItemButton : public QPushButton {
    Q_OBJECT
public:
    explicit TabItemButton(int index, QWidget* parent = nullptr);
>>>>>>> REPLACE

### 2. `src/ui/TabBarWidget.cpp`
<<<<<<< SEARCH
TabItemButton::TabItemButton(int index, QWidget* parent, HoverEventFilter* hoverFilter)
    : QPushButton(parent), m_index(index) {
    setObjectName("TabItem");
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_Hover, true);
    if (hoverFilter) {
        installEventFilter(hoverFilter);
    }
=======
TabItemButton::TabItemButton(int index, QWidget* parent)
    : QPushButton(parent), m_index(index) {
    setObjectName("TabItem");
    setFocusPolicy(Qt::NoFocus);
>>>>>>> REPLACE

<<<<<<< SEARCH
void TabItemButton::setTabTitle(const QString& title) {
    if (m_titleLabel) {
        QFontMetrics fm(m_titleLabel->font());
        QString elided = fm.elidedText(title, Qt::ElideRight, 110);
        m_titleLabel->setText(elided);
        setProperty("tooltipText", title);
    }
}
=======
void TabItemButton::setTabTitle(const QString& title) {
    if (m_titleLabel) {
        QFontMetrics fm(m_titleLabel->font());
        QString elided = fm.elidedText(title, Qt::ElideRight, 110);
        m_titleLabel->setText(elided);
    }
}
>>>>>>> REPLACE

<<<<<<< SEARCH
        TabItemButton* tabItem = new TabItemButton(i, this, m_hoverFilter);
=======
        TabItemButton* tabItem = new TabItemButton(i, this);
>>>>>>> REPLACE

## Build & Verification Steps
1. Recompile project.
2. Hover mouse over tab items: verify no tooltips pop up.
3. Hover mouse over the "+" New Tab button: verify "新建标签页 (Ctrl+T)" still displays cleanly via `ToolTipOverlay`.

## SSOT API Reuse & Anti-Redundancy Self-Check
- Cleanly removes unused tooltip properties from `TabItemButton`.

## Header API Signature Verification
- `TabItemButton::TabItemButton(int, QWidget*)`

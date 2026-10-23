# Implementation Plan - Remove Redundant Favorite Header Widget

## Overview
This plan completely removes the obsolete header widget (`header`, `iconLabel`, `titleLabel` with `FavoritePanelTitleLabel`) from `FavoritePanel::initUi()`.
Since `SidebarContainerWidget` now hosts the top `QTabBar` with "收藏夹" and "库" tabs, the internal header in `FavoritePanel` is redundant and causes a duplicate "收藏夹" label row.

---

## Modified Files List
- `src/ui/FavoritePanel.cpp`
- `resources/style.qss`

---

## Detailed Line-by-Line Changes

### 1. Update `src/ui/FavoritePanel.cpp`

```diff
<<<<<<< SEARCH
void FavoritePanel::initUi() {
    QWidget* header = new QWidget(this);
    header->setObjectName("ContainerHeader");
    header->setFixedHeight(32);
// ContainerHeader in style.qss
    QHBoxLayout* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(15, 0, 5, 0);
    headerLayout->setSpacing(5);

    QLabel* iconLabel = new QLabel(header);
    iconLabel->setPixmap(UiHelper::getIcon("star_filled", QColor("#888888"), 18).pixmap(18, 18));
    headerLayout->addWidget(iconLabel);

    QLabel* titleLabel = new QLabel("收藏夹", header);
    titleLabel->setObjectName("FavoritePanelTitleLabel");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    m_mainLayout->addWidget(header);

    m_favoriteView = new DropTreeView(this);
=======
void FavoritePanel::initUi() {
    m_favoriteView = new DropTreeView(this);
>>>>>>> REPLACE
```

### 2. Update `resources/style.qss`

Remove obsolete `QLabel#FavoritePanelTitleLabel` rule:

```diff
<<<<<<< SEARCH
QLabel#FavoritePanelTitleLabel {
    color: #EEEEEE;
    font-size: 13px;
    font-weight: bold;
}
=======
>>>>>>> REPLACE
```

---

## Build & Verification Steps
1. Recompile standard C++ build target.
2. Confirm that `FavoritePanel` no longer renders the secondary "★ 收藏夹" header row beneath the top `QTabBar`.
3. Confirm `m_favoriteView` sits cleanly beneath `SidebarTabBar` without gaps.
4. Verify `#ContainerHeader` QSS style in `resources/style.qss` remains untouched for other widgets (`NavPanel`, `MetaPanel`, `FilterPanel`, `SidebarContainerWidget`).

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- Eliminates redundant UI header instantiation.
- `#ContainerHeader` style preserved for remaining 4 panel headers.

---

## Header API Signature Verification
- No C++ header signature changes.

---

## Header Inclusion Chain & Type Completeness Check
- Removed unused local `QLabel` instantiation inside `FavoritePanel::initUi()`.

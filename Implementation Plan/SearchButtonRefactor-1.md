# Implementation Plan - Search Button Left Positioning & White Icon Refactor

## Overview
This plan adjusts the search button `QPushButton#BtnSearchAddress` in `SearchController`:
1. Places the search button on the **left side** of `SearchContainer` (before `QLineEdit#SearchEdit`).
2. Renders the `seach-3` icon in **pure white (`#FFFFFF`)**, ensuring it remains crisp and visible at all times.
3. Configures QSS in `resources/style.qss` for `BtnSearchAddress` with a right border divider (`border-right: 1px solid #333333`), left rounded corners (`border-top-left-radius: 6px; border-bottom-left-radius: 6px`), and hover highlight feedback (`#3E3E42`).
4. Retains the TODO callback for future search extensions.

---

## Modified Files List
- `src/ui/SearchController.cpp`
- `resources/style.qss`

---

## Detailed Line-by-Line Changes

### 1. Update `src/ui/SearchController.cpp`

```diff
<<<<<<< SEARCH
    m_btnSearch = new QPushButton(m_searchContainer);
    m_btnSearch->setObjectName("BtnSearchAddress");
    m_btnSearch->setFixedSize(28, 30);
    m_btnSearch->setIcon(UiHelper::getIcon("seach-3", TextMuted, 16));
    m_btnSearch->setIconSize(QSize(16, 16));
    m_btnSearch->setCursor(Qt::PointingHandCursor);
    m_btnSearch->setProperty("tooltipText", "搜索");

    // TODO: 预留搜索按钮扩展功能（例如高级搜索菜单或触发搜索）
    connect(m_btnSearch, &QPushButton::clicked, this, [this]() {
        // TODO: Extended search functionality
        doSearch(m_searchEdit->text().trimmed());
    });

    searchLayout->addWidget(m_searchEdit, 1);
    searchLayout->addWidget(m_btnSearch, 0);
=======
    m_btnSearch = new QPushButton(m_searchContainer);
    m_btnSearch->setObjectName("BtnSearchAddress");
    m_btnSearch->setFixedSize(28, 30);
    m_btnSearch->setIcon(UiHelper::getIcon("seach-3", QColor("#FFFFFF"), 16));
    m_btnSearch->setIconSize(QSize(16, 16));
    m_btnSearch->setCursor(Qt::PointingHandCursor);
    m_btnSearch->setProperty("tooltipText", "搜索");

    // TODO: 预留搜索按钮扩展功能（例如高级搜索菜单或触发搜索）
    connect(m_btnSearch, &QPushButton::clicked, this, [this]() {
        // TODO: Extended search functionality
        doSearch(m_searchEdit->text().trimmed());
    });

    searchLayout->addWidget(m_btnSearch, 0);
    searchLayout->addWidget(m_searchEdit, 1);
>>>>>>> REPLACE
```

### 2. Update `resources/style.qss`

```diff
<<<<<<< SEARCH
QLineEdit#SearchEdit {
    background: transparent;
    border: none;
    color: #EEEEEE;
    padding-left: 8px;
    font-size: 12px;
}
QPushButton#BtnSearchAddress {
    background: transparent;
    border: none;
    border-left: 1px solid #333333;
    border-top-right-radius: 6px;
    border-bottom-right-radius: 6px;
}
QPushButton#BtnSearchAddress:hover {
    background-color: #3E3E42;
}
QPushButton#BtnSearchAddress:pressed {
    background-color: #4E4E52;
}
=======
QLineEdit#SearchEdit {
    background: transparent;
    border: none;
    color: #EEEEEE;
    padding-left: 6px;
    font-size: 12px;
}
QPushButton#BtnSearchAddress {
    background: transparent;
    border: none;
    border-right: 1px solid #333333;
    border-top-left-radius: 6px;
    border-bottom-left-radius: 6px;
}
QPushButton#BtnSearchAddress:hover {
    background-color: #3E3E42;
}
QPushButton#BtnSearchAddress:pressed {
    background-color: #4E4E52;
}
>>>>>>> REPLACE
```

---

## Build & Verification Steps

### Build Command
```bash
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
```

### Verification Steps
1. Launch QuarkMeta.
2. Inspect the search box in the top right header:
   - The `seach-3` icon button is on the **left side** of the search widget.
   - The icon is **white (`#FFFFFF`)** and always visible.
   - Hovering over the button triggers the `#3E3E42` highlight.
   - Right border divider `1px #333333` clearly separates the search button from the input text field.

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- [x] Uses `UiHelper::getIcon("seach-3", QColor("#FFFFFF"), 16)`.
- [x] Keeps QSS parameters clean and aligned with project standards.

---

## Header API Signature Verification
*(No header API changes in this iteration)*

---

## Header Inclusion Chain & Type Completeness Check
*(No header inclusions changed in this iteration)*

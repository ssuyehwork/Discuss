# Implementation Plan - Search Button Icon Color Consistency Fix

## Overview
This plan aligns the icon color and hover behavior of `m_btnSearch` (in `SearchController.cpp`) 100% with `m_btnRefresh` (in `AddressBar.cpp`).
Currently, `m_btnRefresh` in `AddressBar.cpp` uses `#CCCCCC` in normal state and `#FFFFFF` on hover via an event filter.
In contrast, `m_btnSearch` in `SearchController.cpp` was hardcoded to `#FFFFFF` in normal state.
This change updates `SearchController.cpp` to use `QColor("#CCCCCC")` for normal state and `Qt::white` on hover via an event filter on `m_btnSearch`, ensuring 100% visual consistency between `AddressBar` and `SearchController`.

---

## Modified Files List
- `src/ui/SearchController.cpp`

---

## Detailed Line-by-Line Changes

### 1. Update `src/ui/SearchController.cpp`

```diff
<<<<<<< SEARCH
    m_btnSearch->setIcon(UiHelper::getIcon("seach-3", QColor("#FFFFFF"), 16));
    m_btnSearch->setIconSize(QSize(16, 16));
    m_btnSearch->setCursor(Qt::PointingHandCursor);
    m_btnSearch->setProperty("tooltipText", "搜索");

    // TODO: 预留搜索按钮扩展功能（例如高级搜索菜单或触发搜索）
    connect(m_btnSearch, &QPushButton::clicked, this, [this]() {
        // TODO: Extended search functionality
        doSearch(m_searchEdit->text().trimmed());
    });
=======
    m_btnSearch->setIcon(UiHelper::getIcon("seach-3", QColor("#CCCCCC"), 16));
    m_btnSearch->setIconSize(QSize(16, 16));
    m_btnSearch->setCursor(Qt::PointingHandCursor);
    m_btnSearch->setProperty("tooltipText", "搜索");
    m_btnSearch->setAttribute(Qt::WA_Hover);
    m_btnSearch->installEventFilter(this);

    // TODO: 预留搜索按钮扩展功能（例如高级搜索菜单或触发搜索）
    connect(m_btnSearch, &QPushButton::clicked, this, [this]() {
        // TODO: Extended search functionality
        doSearch(m_searchEdit->text().trimmed());
    });
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
bool SearchController::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::MouseButtonDblClick && watched == m_searchEdit) {
        auto history = SearchHistoryService::instance().getHistory("global");
        if (!history.isEmpty()) {
            m_searchHistoryPanel->setHistory(history);
            m_searchHistoryPanel->showBelow(m_searchEdit);
        }
        return true;
    }
    return QObject::eventFilter(watched, event);
}
=======
bool SearchController::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::MouseButtonDblClick && watched == m_searchEdit) {
        auto history = SearchHistoryService::instance().getHistory("global");
        if (!history.isEmpty()) {
            m_searchHistoryPanel->setHistory(history);
            m_searchHistoryPanel->showBelow(m_searchEdit);
        }
        return true;
    }

    if (watched == m_btnSearch) {
        if (event->type() == QEvent::Enter) {
            m_btnSearch->setIcon(UiHelper::getIcon("seach-3", Qt::white, 16));
        } else if (event->type() == QEvent::Leave) {
            m_btnSearch->setIcon(UiHelper::getIcon("seach-3", QColor("#CCCCCC"), 16));
        }
    }

    return QObject::eventFilter(watched, event);
}
>>>>>>> REPLACE
```

---

## Build & Verification Steps
1. Recompile standard C++ build target.
2. Verify `m_btnSearch` displays `#CCCCCC` icon in normal state, matching `m_btnRefresh` in `AddressBar.cpp`.
3. Hover over `m_btnSearch` and confirm the icon turns white `#FFFFFF`, matching hover behavior of `m_btnRefresh`.

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- Reuses `AddressBar.cpp`'s established `QColor("#CCCCCC")` / `Qt::white` hover event filter pattern.
- Reuses `UiHelper::getIcon(...)` SSOT icon rendering API.

---

## Header API Signature Verification
- `UiHelper::getIcon(const QString &iconName, const QColor &color, int size)`: verified signature in `src/ui/UiHelper.h`.

---

## Header Inclusion Chain & Type Completeness Check
- `SearchController.cpp`: All event handling and widget includes exist.

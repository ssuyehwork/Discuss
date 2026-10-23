# SearchController Height & Alignment Fix Implementation Plan

## Overview
Fix the search controller height and vertical alignment issue in the `NavBarWidget` toolbar. The search box had an inconsistent fixed height (`32px`), causing layout clipping/misalignment against the navigation bar's 30px address bar controls. This plan restores the standard 30px height, ensures crisp vertical centering, and aligns the inner action icon positioning.

## Modified Files List
- `src/ui/SearchController.cpp`
- `resources/style.qss`

## Detailed Line-by-Line Changes

### 1. `src/ui/SearchController.cpp`
```
<<<<<<< SEARCH
    m_searchEdit = new QLineEdit(m_searchContainer);
    m_searchEdit->setPlaceholderText("搜索...");
    m_searchEdit->setFixedSize(230, 32);
    m_searchEdit->addAction(UiHelper::getIcon("search", TextMuted), QLineEdit::LeadingPosition);
=======
    m_searchEdit = new QLineEdit(m_searchContainer);
    m_searchEdit->setPlaceholderText("搜索...");
    m_searchEdit->setFixedSize(230, 30);
    m_searchEdit->addAction(UiHelper::getIcon("search", TextMuted), QLineEdit::LeadingPosition);
>>>>>>> REPLACE
```

### 2. `resources/style.qss`
```
<<<<<<< SEARCH
QLineEdit#SearchEdit {
    background-color: #1E1E1E;
    border: 1px solid #333333;
    border-radius: 6px;
    color: #EEEEEE;
    padding-left: 4px;
    padding-right: 24px;
    font-size: 12px;
}
=======
QLineEdit#SearchEdit {
    background-color: #1E1E1E;
    border: 1px solid #333333;
    border-radius: 6px;
    color: #EEEEEE;
    padding-left: 4px;
    padding-right: 24px;
    font-size: 12px;
    height: 30px;
}
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Run `cmake --build rebuild_repo.bat` or MSVC build.
2. Launch QuarkMeta executable and check `NavBarWidget` top right search bar.
3. Verify that search input box height matches the `30px` address bar height seamlessly.

## SSOT API Reuse & Anti-Redundancy Self-Check
- `UiHelper::getIcon` is re-used for leading and trailing actions in `QLineEdit`.
- No new custom implementations added.

## Header API Signature Verification
- `QLineEdit::setFixedSize(int w, int h)` (Qt standard QWidget API)
- `QLineEdit::setPlaceholderText(const QString &)` (Qt standard API)

## Header Inclusion Chain & Type Completeness Check
- `SearchController.cpp` includes `SearchController.h`, `UiHelper.h`, `StyleLibrary.h`. All header chains intact.

# Implementation Plan: TabBarWidget-11.md (Merged Dual Pane Tab Title Formatting)

## 1. Overview
Updates `TabBarWidget` tab title formatting when in dual-pane split mode. If a tab represents a dual pane split view with two active paths, `updateCurrentTabTitle` formats the tab title as `"FolderA | FolderB"` to visually indicate the paired dual-pane state.

## 2. Modified Files List
- `src/ui/TabBarWidget.h`
- `src/ui/TabBarWidget.cpp`

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/TabBarWidget.h`
Add `updateDualPaneTabTitle` declaration to `TabBarWidget`.

```
<<<<<<< SEARCH
    void updateCurrentTabTitle(const QString& title, const QString& url);
=======
    void updateCurrentTabTitle(const QString& title, const QString& url);
    void updateDualPaneTabTitle(const QString& title1, const QString& url1, const QString& title2, const QString& url2);
>>>>>>> REPLACE
```

### 3.2 `src/ui/TabBarWidget.cpp`
Implement `updateDualPaneTabTitle` to format title as `"FolderA | FolderB"`.

```
<<<<<<< SEARCH
void TabBarWidget::updateCurrentTabTitle(const QString& title, const QString& url) {
=======
void TabBarWidget::updateDualPaneTabTitle(const QString& title1, const QString& url1, const QString& title2, const QString& url2) {
    if (m_currentIndex < 0 || m_currentIndex >= m_tabs.size()) return;

    auto cleanName = [](const QString& t, const QString& u) -> QString {
        if (u == "computer://" || u.isEmpty()) return "此电脑";
        if (t.contains("/") || t.contains("\\")) {
            QString cleanPath = QDir::cleanPath(u);
            QFileInfo fi(cleanPath);
            QString fn = fi.fileName();
            return fn.isEmpty() ? cleanPath : fn;
        }
        return t.isEmpty() ? "此电脑" : t;
    };

    QString name1 = cleanName(title1, url1);
    QString name2 = cleanName(title2, url2);
    QString mergedTitle = name1 + " | " + name2;

    m_tabs[m_currentIndex].title = mergedTitle;
    m_tabs[m_currentIndex].url = url1;

    if (m_currentIndex < m_tabWidgets.size()) {
        auto tabBtn = m_tabWidgets[m_currentIndex];
        tabBtn->setTabTitle(mergedTitle);
    }
    saveStateToConfig();
}

void TabBarWidget::updateCurrentTabTitle(const QString& title, const QString& url) {
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Run `cmake --build build` or `ninja -C build`.
2. Split pane in `ContentPanel`.
3. Verify tab title displays as `FolderA | FolderB`.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Tab State SSOT**: Reuses `TabInfo` structure and `saveStateToConfig()`.

## 6. Header API Signature Verification
- `TabBarWidget::updateCurrentTabTitle(const QString&, const QString&)` -> Verified existing in `src/ui/TabBarWidget.h`.
- `TabBarWidget::updateDualPaneTabTitle(const QString&, const QString&, const QString&, const QString&)` -> Declared in `src/ui/TabBarWidget.h`.

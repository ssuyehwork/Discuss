# AppShortcutController Exclusive Star Rating Shortcut Binding Implementation Plan (AppShortcutController-1.md)

## 1. Overview
This implementation plan strictly enforces that `Ctrl+1` ~ `Ctrl+5` keyboard shortcuts are **100% exclusively dedicated to item star rating (1 to 5 stars)** and completely unbinds them from tab switching in `AppShortcutController`.

### Key Solved Issues:
1. **Absolute Unbinding of Ctrl+1 ~ Ctrl+5 from Tab Bar**: Completely removes `Ctrl+1` ~ `Ctrl+5` shortcut registrations from `AppShortcutController`, preventing top-level `QShortcut` from intercepting key events.
2. **Uncontested Star Rating Pipeline**: Restores `Ctrl+0` ~ `Ctrl+5` in `ContentKeyHandler` as the sole, direct handler for item star rating (0 = clear, 1 = 1 star, 2 = 2 stars, 3 = 3 stars, 4 = 4 stars, 5 = 5 stars).
3. **Strict Zero-Value-Alteration Contract**: Preserves 100% of existing UI visual properties, tab bar capabilities for other shortcuts, and star rating data roles.

---

## 2. Modified Files List
1. `src/ui/AppShortcutController.cpp` (Modifies tab shortcut registration loop to restrict number shortcuts to Ctrl+6 ~ Ctrl+9, leaving Ctrl+1 ~ Ctrl+5 exclusively for star rating)

---

## 3. Detailed Line-by-Line Changes

### 3.1 Update `src/ui/AppShortcutController.cpp`
```
<<<<<<< SEARCH
    // 6c. Ctrl+1 ~ Ctrl+9 数字快捷切换标签
    for (int i = 1; i <= 9; ++i) {
        QShortcut* scNumTab = new QShortcut(QKeySequence(Qt::CTRL | (Qt::Key_0 + i)), m_window);
        scNumTab->setContext(Qt::WindowShortcut);
        connect(scNumTab, &QShortcut::activated, this, [this, i]() {
            if (m_window) {
                if (auto titleBar = m_window->findChild<TitleBarWidget*>()) {
                    if (titleBar->tabBar()) {
                        int targetIdx = (i == 9) ? titleBar->tabBar()->tabCount() - 1 : i - 1;
                        titleBar->tabBar()->setCurrentIndex(targetIdx, true);
                    }
                }
            }
        });
    }
=======
    // 6c. Ctrl+6 ~ Ctrl+9 数字快捷切换标签 (Ctrl+1 ~ Ctrl+5 物理上 100% 独占归属于 ContentKeyHandler 星级评分)
    for (int i = 6; i <= 9; ++i) {
        QShortcut* scNumTab = new QShortcut(QKeySequence(Qt::CTRL | (Qt::Key_0 + i)), m_window);
        scNumTab->setContext(Qt::WindowShortcut);
        connect(scNumTab, &QShortcut::activated, this, [this, i]() {
            if (m_window) {
                if (auto titleBar = m_window->findChild<TitleBarWidget*>()) {
                    if (titleBar->tabBar()) {
                        int targetIdx = (i == 9) ? titleBar->tabBar()->tabCount() - 1 : i - 1;
                        titleBar->tabBar()->setCurrentIndex(targetIdx, true);
                    }
                }
            }
        });
    }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

### 4.1 CMake & Build Verification Commands
```bash
# Clean and re-configure CMake
cmake -B build -S .

# Build the QuarkMeta executable
cmake --build build --config Release
```

### 4.2 Verification Steps
1. Launch `QuarkMeta` application.
2. Select any file or folder in Grid, List, or Column view.
3. Press `Ctrl+1`, `Ctrl+2`, `Ctrl+3`, `Ctrl+4`, `Ctrl+5` and verify that star ratings 1, 2, 3, 4, and 5 are assigned immediately and reliably.
4. Press `Ctrl+0` and verify that star rating is reset to 0 stars.
5. Verify that `Ctrl+1` ~ `Ctrl+5` never trigger tab switching under any circumstances.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- [x] **`ContentKeyHandler::handleKeyPress()`**: Reused existing rating role setting pipeline (`RatingRole`).
- [x] **Zero Duplication**: Eliminated duplicate/conflicting shortcut definitions between AppShortcutController and ContentKeyHandler.

---

## 6. Header API Signature Verification

| Called Class / Function | Physical Signature in `.h` Header | Status |
| :--- | :--- | :--- |
| `ContentKeyHandler::handleKeyPress` | `bool handleKeyPress(QObject* obj, QEvent* event)` | Verified |
| `TitleBarWidget::tabBar` | `TabBarWidget* tabBar() const` | Verified |
| `TabBarWidget::setCurrentIndex` | `void setCurrentIndex(int index, bool animate = true)` | Verified |

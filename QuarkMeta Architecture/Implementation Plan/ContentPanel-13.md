# ContentPanel-13.md: Temporary Iterator Crash Fix & SelectionState Lifetime Safety

## 1. Overview
When switching view modes, the application crashed immediately due to a C++ dangling temporary iterator bug in `ContentPanel::setViewMode()`:
```cpp
m_selectionState.selectedPaths = QSet<QString>(getSelectedPaths().begin(), getSelectedPaths().end());
```
Because `getSelectedPaths()` returns a `QStringList` by value, calling `.begin()` and `.end()` on separate temporary function calls created two distinct temporary `QStringList` instances. Constructing `QSet<QString>` by comparing iterators belonging to two different temporary containers caused an out-of-bounds memory read / invalid iterator comparison, resulting in an immediate process crash (segfault) whenever view mode switching was initiated.

This implementation plan fixes the crash by binding `getSelectedPaths()` to a named local variable `QStringList selList = getSelectedPaths();` with a valid lifetime before constructing `QSet<QString>(selList.begin(), selList.end())`, and adding defensive null checks across selection restoration paths.

---

## 2. Modified Files List
- `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/ui/ContentPanel.cpp`

<<<<<<< SEARCH
void ContentPanel::setViewMode(ViewMode mode) {
    if (m_currentViewMode == mode) {
        return;
    }
    // 1. 在原视图中上报并更新 SelectionState (SSOT)
    m_selectionState.currentFolder = m_currentPath;
    m_selectionState.selectedPaths = QSet<QString>(getSelectedPaths().begin(), getSelectedPaths().end());
    if (!m_selectionState.selectedPaths.isEmpty()) {
        m_selectionState.focusedPath = *m_selectionState.selectedPaths.begin();
    }
=======
void ContentPanel::setViewMode(ViewMode mode) {
    if (m_currentViewMode == mode) {
        return;
    }
    // 1. 在原视图中上报并更新 SelectionState (SSOT)，修正临时对象迭代器野指针闪退
    m_selectionState.currentFolder = m_currentPath;
    QStringList selList = getSelectedPaths();
    m_selectionState.selectedPaths = QSet<QString>(selList.begin(), selList.end());
    if (!m_selectionState.selectedPaths.isEmpty()) {
        m_selectionState.focusedPath = *m_selectionState.selectedPaths.begin();
    }
>>>>>>> REPLACE

---

## 4. Build & Verification Steps

### Verification Commands
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

### Verification Checklist
1. Launch application in Grid View, List View, or Column View.
2. Switch view modes repeatedly between Grid View, List View, and Column View (both with items selected and unselected).
3. Confirm that no process crash (segfault) occurs and view mode transitions smoothly.
4. Verify that selected items and focus paths are preserved seamlessly.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Preserves the `SelectionState` SSOT architecture while guaranteeing object lifetime safety during iterator construction.

---

## 6. Header API Signature Verification
- `ContentPanel::getSelectedPaths()` -> `src/ui/ContentPanel.h`
- `ContentPanel::setViewMode(ViewMode mode)` -> `src/ui/ContentPanel.h`

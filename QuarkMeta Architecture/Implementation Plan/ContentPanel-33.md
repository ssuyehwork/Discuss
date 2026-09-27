# Implementation Plan - ContentPanel (Single Scroll Area Dynamic Height Reset Fix)

## 1. Overview
This implementation plan fixes the layout collapse bug (0px height black box issue) when opening folders with large item counts (e.g. `H:\测试` with 15 folders and 2039 files).

Previously, `ContentPanel` wrapped `m_folderGridView` and `m_gridView` inside a single outer `QScrollArea` (`m_gridScrollArea`) and dynamically executed `m_folderGridView->setFixedHeight(fjv->totalHeight())`. When loading folders with thousands of files, asynchronous layout calculation delays caused `fjv->totalHeight()` to temporarily return `0`, executing `setFixedHeight(0)` on `m_folderGridView` and collapsing the grid viewport to a 0px black region.

This change maintains the unified single vertical scroll area. It eliminates `setFixedHeight(0)` bugs by ensuring height limits are only applied when `height > 0`, and unconstraining `minimumHeight`/`maximumHeight` whenever `height <= 0` or during model reset/clear events.

---

## 2. Modified Files List
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### Change 1: `src/ui/ContentPanel.cpp`
Update `ContentPanel::initGridView()` height connection to safeguard against zero-height layout calculations and unconstrain layout on reset.

```
<<<<<<< SEARCH
    if (auto* fjv = qobject_cast<JustifiedView*>(m_folderGridView)) {
        connect(fjv, &JustifiedView::totalHeightChanged, this, [this](int height) {
            if (m_folderGridView && m_folderProxyModel && m_folderProxyModel->rowCount() > 0) {
                m_folderGridView->setFixedHeight(height);
            }
        });
    }
=======
    if (auto* fjv = qobject_cast<JustifiedView*>(m_folderGridView)) {
        connect(fjv, &JustifiedView::totalHeightChanged, this, [this](int height) {
            if (!m_folderGridView || !m_folderProxyModel) return;
            if (m_folderProxyModel->rowCount() > 0 && height > 0) {
                m_folderGridView->setMinimumHeight(height);
                m_folderGridView->setMaximumHeight(height);
            } else {
                m_folderGridView->setMinimumHeight(0);
                m_folderGridView->setMaximumHeight(QWIDGETSIZE_MAX);
            }
        });
    }
>>>>>>> REPLACE
```

---

### Change 2: `src/ui/ContentPanel.cpp`
Update `updateGridSectionCounts` in `ContentPanel::initGridView()` to prevent setting `setFixedHeight(0)` during initial model resets.

```
<<<<<<< SEARCH
        if (m_folderGridView) {
            if (folderCount == 0) {
                m_folderGridView->hide();
            } else {
                bool collapsed = m_gridFolderHeader ? m_gridFolderHeader->isCollapsed() : false;
                m_folderGridView->setVisible(!collapsed);
                if (auto* fjv = qobject_cast<JustifiedView*>(m_folderGridView)) {
                    m_folderGridView->setFixedHeight(fjv->totalHeight());
                }
            }
        }
=======
        if (m_folderGridView) {
            if (folderCount == 0) {
                m_folderGridView->hide();
                m_folderGridView->setMinimumHeight(0);
                m_folderGridView->setMaximumHeight(QWIDGETSIZE_MAX);
            } else {
                bool collapsed = m_gridFolderHeader ? m_gridFolderHeader->isCollapsed() : false;
                m_folderGridView->setVisible(!collapsed);
                if (auto* fjv = qobject_cast<JustifiedView*>(m_folderGridView)) {
                    int h = fjv->totalHeight();
                    if (h > 0) {
                        m_folderGridView->setMinimumHeight(h);
                        m_folderGridView->setMaximumHeight(h);
                    } else {
                        m_folderGridView->setMinimumHeight(0);
                        m_folderGridView->setMaximumHeight(QWIDGETSIZE_MAX);
                    }
                }
            }
        }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. **Build Verification**:
   ```bash
   cmake -B build -S .
   cmake --build build --config Release
   ```
2. **Behavior Verification**:
   - Open `QuarkMeta` in grid view for a directory with subfolders and 2000+ files (e.g. `H:\测试`).
   - Verify that `m_folderGridView` does not collapse to 0px height or black screen.
   - Verify smooth scrolling across both folder and file sections within the outer scroll container.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **API Reuse**: Retained `JustifiedView::totalHeightChanged` and `m_gridScrollArea`.
- **Anti-Redundancy**: Preserved unified single-scroll view structure without introducing extra splitters or duplicate controls.

---

## 6. Header API Signature Verification

| Header File | Class Name | Verified Signature |
| :--- | :--- | :--- |
| `src/ui/JustifiedView.h` | `JustifiedView` | `int totalHeight() const` |
| `src/ui/JustifiedView.h` | `JustifiedView` | `void totalHeightChanged(int height)` |
| `<QWidget>` | `QWidget` | `void setMinimumHeight(int minh)` |
| `<QWidget>` | `QWidget` | `void setMaximumHeight(int maxh)` |
